use std::sync::{LazyLock, Mutex, atomic::Ordering};

use crate::{
    Addr, LoleError,
    addrs::offsets,
    assembly, dostring,
    lua::{
        LUA_FALSE, cursor_is_on_WorldFrame,
        input::get_kbd_modifiers,
        wc3::{get_selected_units, update_selected_units},
    },
    patch::{InstructionBuffer, Patch, PatchKind, copy_original_opcodes},
    wc3::{
        CONTROL_GROUPS, CUSTOM_CAMERA, WC3MODE_ENABLED, get_foreground_window,
        get_window_dimensions,
    },
};
use lole_macros::auto_enum_try_from;
use windows::{
    Win32::{
        Foundation::{POINT, RECT},
        Graphics::Gdi::ScreenToClient,
        UI::WindowsAndMessaging::{GetClientRect, GetCursorPos},
    },
    core::BOOL,
};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[auto_enum_try_from(i32)]
pub enum MouseButton {
    Left = 0x1,
    Right = 0x4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[auto_enum_try_from(i32)]
pub enum InputEvent {
    // the first two are some kind of keyboard events
    UnknownKeyEvent1 = 0x1,
    UnknownKeyEvent3 = 0x3,
    KeyDown = 0x7,
    KeyUp = 0x8,
    MouseDown = 0x9,
    MouseMove = 0xA,
    MouseWheel = 0xB,
    MouseDrag = 0xC,
    MouseUp = 0xD,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[auto_enum_try_from(i32)]
pub enum Key {
    Num0 = 0x30,
    Num1 = 0x31,
    Num2 = 0x32,
    Num3 = 0x33,
    Num4 = 0x34,
    Num5 = 0x35,
    Num6 = 0x36,
    Num7 = 0x37,
    Num8 = 0x38,
    Num9 = 0x39,
    H = 0x48,
    R = 0x52,
}

bitflags::bitflags! {
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub struct KbdModifiers: i32 {
        const CTRL = 1 << 0;
        const ALT = 1 << 1;
        const SHIFT = 1 << 2;
    }
}

#[repr(C)]
#[derive(Debug)]
struct WowInputEvent {
    event: i32,
    param: i32,
    x: i32,
    y: i32,
    unk1: i32,
}

const INPUT_EVENT_PREVENT_DEFAULT: i32 = 0;
const INPUT_EVENT_PASS_TO_NORMAL_HANDLER: i32 = 1;

#[derive(Debug)]
struct MouseState {
    left_press_start_location: Option<(i32, i32)>,
}

static MOUSE_STATE: LazyLock<Mutex<MouseState>> = LazyLock::new(|| {
    Mutex::new(MouseState {
        left_press_start_location: None,
    })
});

pub fn get_cursor_position() -> anyhow::Result<(i32, i32)> {
    unsafe {
        let hwnd = get_foreground_window()?;
        let mut point: POINT = std::mem::zeroed();
        GetCursorPos(&mut point)?;
        if ScreenToClient(hwnd, &mut point) == BOOL(0) {
            return Err(anyhow::anyhow!("ScreenToClient failed"));
        }
        return Ok((point.x, point.y));
    }
}

pub fn get_screen_size() -> anyhow::Result<(i32, i32)> {
    unsafe {
        let hwnd = get_foreground_window()?;
        let mut rect = RECT::default();
        GetClientRect(hwnd, &mut rect)?;

        let pixel_w = rect.right - rect.left;
        let pixel_h = rect.bottom - rect.top;
        Ok((pixel_w, pixel_h))
    }
}

fn add_input_event(event: *const WowInputEvent) -> anyhow::Result<i32> {
    if !WC3MODE_ENABLED.load(Ordering::Relaxed) {
        return Ok(INPUT_EVENT_PASS_TO_NORMAL_HANDLER);
    }
    let event = unsafe { event.as_ref() }.ok_or_else(|| anyhow::anyhow!("event.as_ref"))?;
    match event.event.try_into()? {
        InputEvent::UnknownKeyEvent1 => {}
        InputEvent::UnknownKeyEvent3 => {}
        InputEvent::KeyUp => match Key::try_from(event.param) {
            Ok(Key::R) => {}
            Ok(
                num @ (Key::Num1
                | Key::Num2
                | Key::Num3
                | Key::Num4
                | Key::Num5
                | Key::Num6
                | Key::Num7
                | Key::Num8
                | Key::Num9
                | Key::Num0),
            ) => {
                let cgroup_index = (if let Key::Num0 = num {
                    9
                } else {
                    event.param - 0x30 - 1
                })
                .clamp(0, 9) as usize;

                let kbd_modifiers = get_kbd_modifiers()?;

                let mut cgroups = CONTROL_GROUPS.lock().unwrap();
                if kbd_modifiers.contains(KbdModifiers::CTRL) {
                    cgroups[cgroup_index] = get_selected_units()?;
                    tracing::info!(
                        "Created control group {} {:?}",
                        cgroup_index + 1,
                        cgroups[cgroup_index]
                    );
                } else {
                    update_selected_units(cgroups[cgroup_index].clone())?;
                }
                return Ok(INPUT_EVENT_PREVENT_DEFAULT);
            }
            _ => {}
        },
        InputEvent::KeyDown => {}
        InputEvent::MouseDown => {
            let mut state = MOUSE_STATE.lock().unwrap();
            let (cx, cy) = get_cursor_position()?;
            let (w, h) = get_window_dimensions()?;
            match MouseButton::try_from(event.param)? {
                MouseButton::Left => {
                    if let Ok(LUA_FALSE) = cursor_is_on_WorldFrame() {
                        // we clicked on a visible frame of some kind
                        return Ok(INPUT_EVENT_PASS_TO_NORMAL_HANDLER);
                    }

                    state.left_press_start_location = Some((cx, cy));
                    dostring!("lole_wc3mode:set_window_dims_pixels({w}, {h})",);
                    dostring!("lole_wc3mode.selection:start({cx}, {cy})",);

                    return Ok(INPUT_EVENT_PREVENT_DEFAULT);
                }
                MouseButton::Right => {}
            }
        }
        InputEvent::MouseUp => {
            let mut state = MOUSE_STATE.lock().unwrap();
            let (cx, cy) = get_cursor_position()?;

            match MouseButton::try_from(event.param)? {
                MouseButton::Left => {
                    if let Some(_) = state.left_press_start_location {
                        dostring!("lole_wc3mode.selection:finish({cx}, {cy})",);
                        state.left_press_start_location = None;
                        return Ok(INPUT_EVENT_PREVENT_DEFAULT);
                    }
                }
                MouseButton::Right => {}
            }
        }

        InputEvent::MouseDrag => {}

        InputEvent::MouseMove => {
            // let state = MOUSE_STATE.lock().unwrap();
            // let Ok((cx, cy)) = get_cursor_position() else {
            //     tracing::warn!("get_cursor_position failed");
            //     return INPUT_EVENT_PASS_TO_NORMAL_HANDLER;
            // };
            // if let Some((x, y)) = state.left_press_start_location {
            //     dostring!("lole_wc3mode.selection:update({cx}, {cy})",);
            // }
        }
        InputEvent::MouseWheel => {
            let mut camera = CUSTOM_CAMERA.lock().unwrap();
            if event.param > 0 {
                camera.decrement_s();
            } else {
                camera.increment_s();
            }
        }
    }
    Ok(INPUT_EVENT_PASS_TO_NORMAL_HANDLER)
}

#[allow(non_snake_case)]
unsafe extern "stdcall" fn AddInputEvent_hook(event: *const WowInputEvent) -> i32 {
    match add_input_event(event) {
        Ok(res) => res,
        Err(e) => {
            tracing::error!("{e}");
            INPUT_EVENT_PASS_TO_NORMAL_HANDLER
        }
    }
}

#[allow(non_snake_case)]
pub fn prepare_AddInputEvent_patch() -> Patch {
    let original_opcodes = copy_original_opcodes(offsets::wow_cfuncs::AddInputEvent, 8);
    let mut patch_opcodes = InstructionBuffer::new();

    // pushad; lea ebx, [esp + 0x24]; push ebx
    patch_opcodes.push_slice(&[0x60, 0x8D, 0x5C, 0x24, 0x24, 0x53]);
    patch_opcodes.push_call_to(AddInputEvent_hook as *const () as Addr);

    // cmp eax, 0
    patch_opcodes.push_slice(&[0x83, 0xF8, 0x00]);

    // jz branch2 (branch2 is after: popad + original_opcodes + push_imm + dword + ret = 1 + size + 1 + 4 + 1 = 7 + size bytes)
    let branch2_offset: u32 = (1 + original_opcodes.len() + 1 + 4 + 1) as u32;
    patch_opcodes.push_slice(&[0x0F, 0x84]);
    patch_opcodes.push_slice(&branch2_offset.to_le_bytes());

    // branch 1 - execute original and continue
    patch_opcodes.push(assembly::POPAD);
    patch_opcodes.push_slice(&original_opcodes);
    patch_opcodes.push(assembly::PUSH_IMM);
    patch_opcodes
        .push_slice(&(offsets::wow_cfuncs::AddInputEvent + original_opcodes.len()).to_le_bytes());
    patch_opcodes.push(assembly::RET);

    // branch 2 - filter/skip
    patch_opcodes.push(assembly::POPAD);
    patch_opcodes.push(assembly::RET);

    Patch {
        name: "AddInputEvent",
        patch_addr: offsets::wow_cfuncs::AddInputEvent,
        original_opcodes,
        patch_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}
