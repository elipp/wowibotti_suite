use std::sync::{LazyLock, Mutex, atomic::Ordering};

use crate::{
    Addr, LoleError,
    addrs::offsets,
    assembly, dostring,
    lua::WC3MODE_ENABLED,
    patch::{InstructionBuffer, Patch, PatchKind, copy_original_opcodes},
    wc3::{CUSTOM_CAMERA, get_foreground_window, get_window_dimensions},
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
pub enum KeyModifier {
    Ctrl = 0x2,
    Alt = 0x4,
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

#[repr(C)]
#[derive(Debug)]
struct WowInputEvent {
    event: i32,
    param: i32,
    x: i32,
    y: i32,
    unk1: i32,
}

const INPUT_EVENT_DONT_PASS_TO_NORMAL_HANDLER: i32 = 0;
const INPUT_EVENT_PASS_TO_NORMAL_HANDLER: i32 = 1;

#[derive(Debug)]
struct MouseState {
    left_press_start_location: Option<(i32, i32)>,
    right_press_start_location: Option<(i32, i32)>,
}

static MOUSE_STATE: LazyLock<Mutex<MouseState>> = LazyLock::new(|| {
    Mutex::new(MouseState {
        left_press_start_location: None,
        right_press_start_location: None,
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

#[allow(non_snake_case)]
unsafe extern "stdcall" fn AddInputEvent_hook(event: *const WowInputEvent) -> i32 {
    if !WC3MODE_ENABLED.load(Ordering::Relaxed) {
        return INPUT_EVENT_PASS_TO_NORMAL_HANDLER;
    }
    if let Some(event) = unsafe { event.as_ref() } {
        if let Ok(e) = InputEvent::try_from(event.event) {
            match e {
                InputEvent::KeyDown => {}
                InputEvent::KeyUp => {}
                InputEvent::MouseDown => {
                    if let Ok(p) = MouseButton::try_from(event.param) {
                        let mut state = MOUSE_STATE.lock().unwrap();
                        if let Ok((cx, cy)) = get_cursor_position()
                            && let Ok((w, h)) = get_window_dimensions()
                        {
                            match p {
                                MouseButton::Left => {
                                    state.left_press_start_location = Some((cx, cy));
                                    dostring!("lole_start_wc3mode_rect({w}, {h}, {cx}, {cy})",);
                                }
                                MouseButton::Right => {
                                    state.right_press_start_location = Some((cx, cy))
                                }
                            }
                            return INPUT_EVENT_DONT_PASS_TO_NORMAL_HANDLER;
                        } else {
                            tracing::warn!("wh failed");
                            return INPUT_EVENT_PASS_TO_NORMAL_HANDLER;
                        }
                    }
                }
                InputEvent::MouseUp => {
                    if let Ok(p) = MouseButton::try_from(event.param) {
                        let mut state = MOUSE_STATE.lock().unwrap();
                        let Ok((cx, cy)) = get_cursor_position() else {
                            tracing::warn!("get_cursor_position failed");
                            return INPUT_EVENT_PASS_TO_NORMAL_HANDLER;
                        };
                        match p {
                            MouseButton::Left => {
                                if let Some((x, y)) = state.left_press_start_location {
                                    dostring!("lole_end_wc3mode_rect({cx}, {cy}, {x}, {y})",);
                                }
                                state.left_press_start_location = None;
                            }
                            MouseButton::Right => state.right_press_start_location = None,
                        }
                        return INPUT_EVENT_DONT_PASS_TO_NORMAL_HANDLER;
                    }
                }

                InputEvent::MouseDrag => {
                    tracing::info!("Drag: {event:?}");
                    return INPUT_EVENT_DONT_PASS_TO_NORMAL_HANDLER;
                }

                InputEvent::MouseMove => {
                    let state = MOUSE_STATE.lock().unwrap();
                    let Ok((cx, cy)) = get_cursor_position() else {
                        tracing::warn!("get_cursor_position failed");
                        return INPUT_EVENT_PASS_TO_NORMAL_HANDLER;
                    };
                    if let Some((x, y)) = state.left_press_start_location {
                        dostring!("lole_update_wc3mode_rect({cx}, {cy}, {x}, {y})",);
                    }
                }
                InputEvent::MouseWheel => {
                    if let Ok(mut camera) = CUSTOM_CAMERA.lock() {
                        if event.param > 0 {
                            camera.decrement_s();
                        } else {
                            camera.increment_s();
                        }
                    }
                }
            }
        }
    }
    INPUT_EVENT_PASS_TO_NORMAL_HANDLER
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
