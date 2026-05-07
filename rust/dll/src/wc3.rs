use std::sync::{LazyLock, Mutex};

use nalgebra::{Matrix4, Perspective3, Rotation3, RowVector4, Translation3, Vector3, Vector4};
use windows::Win32::{
    Foundation::{HWND, RECT},
    UI::WindowsAndMessaging::{GetClientRect, GetForegroundWindow},
};

use crate::{
    Addr, LoleError,
    addrs::offsets::{WOW_CAMERA, WOW_CAMERA_L2_OFFSET},
    assembly::NOP,
    input::{get_cursor_position, get_screen_size},
    linalg::WowVector3,
    objectmanager::{GUID, ObjectManager, WowObject, WowObjectType},
    patch::{deref_opt_ptr, write_addr},
};

pub static CUSTOM_CAMERA: LazyLock<Mutex<CustomCamera>> = LazyLock::new(Default::default);

pub static SELECTED_UNIT_GUIDS: LazyLock<Mutex<Vec<GUID>>> = LazyLock::new(Default::default);

#[repr(C)]
#[derive(Debug)]
pub struct WowCamera {
    pub unk1: u32,
    pub unk2: u32,
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub rot: [[f32; 3]; 3],
    pub z_near: f32,
    pub z_far: f32,
    pub fov: f32,
    pub aspect: f32,
}

impl WowCamera {
    /// Extracts the X-axis rotation angle from the camera's rotation matrix.
    /// Only valid because WoW's camera rotates exclusively around the X axis.
    pub fn get_rot_angle(&self) -> f32 {
        self.rot[0][0].acos()
    }
    pub fn get_wow_proj_matrix(&self) -> Matrix4<f32> {
        let ys = 1.0 / (self.fov / 2.0).tan();
        let xs = ys / self.aspect;
        let n = self.z_near;
        let f = self.z_far;

        Matrix4::from_rows(&[
            RowVector4::new(xs, 0.0, 0.0, 0.0),
            RowVector4::new(0.0, ys, 0.0, 0.0),
            RowVector4::new(0.0, 0.0, (f + n) / (n - f), 2.0 * f * n / (n - f)),
            RowVector4::new(0.0, 0.0, -1.0, 0.0),
        ])
    }
    pub fn get_corresponding_rh_rot(&self) -> Matrix4<f32> {
        let a = -self.get_rot_angle();
        Rotation3::from_axis_angle(&Vector3::x_axis(), a).to_homogeneous()
    }

    pub fn get_wow_rot(&self) -> Matrix4<f32> {
        let r = &self.rot;
        Matrix4::from_rows(&[
            RowVector4::new(r[0][0], r[0][1], r[0][2], 0.0),
            RowVector4::new(r[1][0], r[1][1], r[1][2], 0.0),
            RowVector4::new(r[2][0], r[2][1], r[2][2], 0.0),
            RowVector4::new(0.0, 0.0, 0.0, 1.0),
        ])
    }

    pub fn get_wow_view_matrix(&self) -> Matrix4<f32> {
        let pos = Matrix4::from_rows(&[
            RowVector4::new(1.0, 0.0, 0.0, -self.x),
            RowVector4::new(0.0, 1.0, 0.0, -self.y),
            RowVector4::new(0.0, 0.0, 1.0, -self.z),
            RowVector4::new(0.0, 0.0, 0.0, 1.0),
        ]);
        pos * self.get_wow_rot()
    }

    pub fn set_pos(&mut self, pos: &WowVector3) {
        self.x = pos.0.x;
        self.y = pos.0.y;
        self.z = pos.0.z;
    }

    pub fn get_pos(&self) -> WowVector3 {
        return WowVector3::new(self.x, self.y, self.z);
    }

    pub fn set_rot(&mut self, rot: Rotation3<f32>) {
        let rot3 = rot.to_homogeneous().fixed_view::<3, 3>(0, 0).clone_owned(); //.transpose();
        self.rot = rot3.into();
    }

    pub fn fetch() -> Option<&'static Self> {
        Self::fetch_mut().map(|c| &*c)
    }

    pub fn fetch_mut() -> Option<&'static mut Self> {
        let c1 = deref_opt_ptr::<1>(WOW_CAMERA as _)?;
        let c2 = deref_opt_ptr::<1>(c1.wrapping_byte_offset(WOW_CAMERA_L2_OFFSET) as _)?;
        unsafe { (c2 as *mut Self).as_mut() }
    }
}

pub struct CustomCamera {
    s: f32,
    maxdistance: f32,
    pos: WowVector3,
}

const S_MIN: f32 = 0.4;
const S_MAX: f32 = 0.7;
const S_INCREMENT: f32 = 0.01;

impl Default for CustomCamera {
    fn default() -> Self {
        let res = Self {
            s: 0.5,
            maxdistance: 80.0,
            pos: WowVector3::new(0.0, 80.0, 0.0),
        };
        res
    }
}

impl CustomCamera {
    pub fn get_cameraoffset(&self) -> WowVector3 {
        // self.maxdistance * WowVector3::new(0.0, 2.0 * self.s * self.s, self.s)
        self.maxdistance * WowVector3::new(-0.5 * self.s.powi(2), 0.0, 0.5 * self.s)
    }
    pub fn get_angle(&self) -> f32 {
        0.5 + self.s * 0.6
    }
    pub fn increment_s(&mut self) {
        self.s = (self.s + S_INCREMENT).clamp(S_MIN, S_MAX);
    }
    pub fn decrement_s(&mut self) {
        self.s = (self.s - S_INCREMENT).clamp(S_MIN, S_MAX);
    }
    pub fn get_s(&self) -> f32 {
        self.s
    }

    pub fn get_rot(&self) -> Rotation3<f32> {
        Rotation3::new(Vector3::y() * self.get_angle())
    }

    pub fn update(&mut self, wow_camera: &mut WowCamera) -> anyhow::Result<()> {
        wow_camera.set_rot(self.get_rot());
        wow_camera.set_pos(&self.pos.into());
        Ok(())
    }

    /// Reset camera to the "default" view over the player
    pub fn reset_camera(&mut self, wow_camera: &mut WowCamera) -> anyhow::Result<()> {
        let Ok(om) = ObjectManager::new() else {
            return Err(LoleError::ObjectManagerIsNull)?;
        };

        let p = om.get_player()?;
        self.pos = p.get_pos()? + self.get_cameraoffset();

        wow_camera.set_pos(&self.pos);
        wow_camera.set_rot(self.get_rot());

        Ok(())
    }
}

pub fn get_foreground_window() -> anyhow::Result<HWND> {
    let hwnd = unsafe { GetForegroundWindow() };
    if hwnd.is_invalid() {
        return Err(anyhow::anyhow!("invalid fg window"));
    }
    Ok(hwnd)
}

pub fn get_window_dimensions() -> anyhow::Result<(i32, i32)> {
    unsafe {
        let hwnd = get_foreground_window()?;
        let mut rect = RECT::default();
        GetClientRect(hwnd, &mut rect)?;
        let width = rect.right - rect.left;
        let height = rect.bottom - rect.top;
        Ok((width, height))
    }
}

const CAMERAPATCH_BASE: Addr = 0x6075AB;
const CAMERAPATCH_END: Addr = 0x6075EC;
const CAMERAPATCH_SIZE: usize = CAMERAPATCH_END - CAMERAPATCH_BASE;

pub struct CameraPatch {
    original: [u8; CAMERAPATCH_SIZE],
    patched: bool,
}

static CAMERA_PATCH: LazyLock<Mutex<CameraPatch>> = LazyLock::new(|| {
    let mut res = CameraPatch {
        original: unsafe { CameraPatch::snapshot() },
        patched: false,
    };
    unsafe { res.patch().expect("Camera patching to succeed") }
    unsafe { res.unpatch().expect("Camera unpatching to succeed") }
    Mutex::new(res)
});

impl CameraPatch {
    /// Reads the current (patched) bytes at CAMERAPATCH_BASE
    unsafe fn snapshot() -> [u8; CAMERAPATCH_SIZE] {
        let mut res = [0x0; _];
        unsafe {
            std::ptr::copy_nonoverlapping(
                CAMERAPATCH_BASE as _,
                res.as_mut_ptr(),
                CAMERAPATCH_SIZE,
            );
        }
        res
    }

    unsafe fn write(&self, address: Addr, bytes: &[u8]) -> anyhow::Result<()> {
        write_addr(address, bytes)
    }

    /// Applies the camera patch (NOP out the write-back instructions).
    pub unsafe fn patch(&mut self) -> anyhow::Result<()> {
        if self.patched {
            return Ok(());
        }
        let nop4 = [NOP; 4];
        unsafe {
            // 2-byte NOP at CAMERAPATCH_BASE (0x6075AB)
            self.write(CAMERAPATCH_BASE, &nop4[..2])?;

            // 3-byte NOP at 0x6075B2
            self.write(0x6075B2, &nop4[..3])?;

            // 3-byte NOP at 0x6075C5
            self.write(0x6075C5, &nop4[..3])?;

            // 2-byte NOP at 0x6075D2
            self.write(0x6075D2, &nop4[..2])?;

            // 3-byte NOP at 0x6075E3
            self.write(0x6075E3, &nop4[..3])?;

            // 3-byte NOP at 0x6075E9
            self.write(0x6075E9, &nop4[..3])?;

            // 2-byte NOP at 0x4C5810 (rep movsd -> NOP, kills game camera rotation write-back)
            self.write(0x4C5810, &nop4[..2])?;
        }
        tracing::info!("Patched Wow camera writeback locations");
        self.patched = true;
        Ok(())
    }

    /// Restores the original bytes (undo the patch).
    pub unsafe fn unpatch(&mut self) -> anyhow::Result<()> {
        if !self.patched {
            return Ok(());
        }
        unsafe {
            self.write(CAMERAPATCH_BASE, &self.original)?;

            // Restore the two-byte rep movsd at 0x4C5810
            let orig_rot: [u8; _] = [0xF3, 0xA5];
            self.write(0x4C5810, &orig_rot)?;
        }
        tracing::info!("Restored Wow camera writeback locations");
        self.patched = false;
        Ok(())
    }
}

pub fn do_wc3mode_stuff() -> anyhow::Result<()> {
    let mut patch = CAMERA_PATCH
        .lock()
        .map_err(|_| anyhow::anyhow!("CAMERA_PATCH lock"))?;

    unsafe {
        patch.patch()?;
    }

    let mut custom_camera = CUSTOM_CAMERA
        .lock()
        .map_err(|_| anyhow::anyhow!("custom_camera mutex"))?;

    let mut wow_camera = WowCamera::fetch_mut().ok_or_else(|| anyhow::anyhow!("No Wow camera"))?;
    custom_camera.reset_camera(&mut wow_camera)?;

    Ok(())
}

pub fn undo_wc3mode_patches() -> anyhow::Result<()> {
    let mut patch = CAMERA_PATCH
        .lock()
        .map_err(|_| anyhow::anyhow!("CAMERA_PATCH lock"))?;

    unsafe {
        patch.unpatch()?;
    }
    // reset camera?
    Ok(())
}

pub mod pylpyr {
    use crate::{
        Addr,
        addrs::offsets,
        assembly,
        objectmanager::ObjectManager,
        patch::{InstructionBuffer, Patch, PatchKind, copy_original_opcodes},
        wc3::SELECTED_UNIT_GUIDS,
    };

    pub unsafe extern "stdcall" fn pylpyr_hook() {
        let Ok(selected_unit_guids) = SELECTED_UNIT_GUIDS.lock() else {
            return tracing::error!("SELECTED_UNIT_GUIDS lock error");
        };
        let Ok(om) = ObjectManager::new() else {
            return tracing::error!("No object manager");
        };
        for guid in selected_unit_guids.iter() {
            if let Ok(Some(obj)) = om.get_object_by_guid(*guid) {
                let _ = unsafe { obj.draw_pylpyr() };
            }
        }
    }

    #[allow(non_snake_case)]
    pub fn prepare_pylpyr_patch() -> Patch {
        let original_opcodes = copy_original_opcodes(offsets::wow_cfuncs::Pylpyr, 9);
        let mut patch_opcodes = InstructionBuffer::new();
        // pushad
        patch_opcodes.push(assembly::PUSHAD);
        patch_opcodes.push_call_to(pylpyr_hook as *const () as Addr);
        // popad
        patch_opcodes.push(assembly::POPAD);
        patch_opcodes.push_slice(&original_opcodes);
        // push ret addr
        patch_opcodes.push(assembly::PUSH_IMM);
        patch_opcodes
            .push_slice(&(offsets::wow_cfuncs::Pylpyr + original_opcodes.len()).to_le_bytes());
        patch_opcodes.push(assembly::RET);
        Patch {
            name: "Pylpyr",
            patch_addr: offsets::wow_cfuncs::Pylpyr,
            original_opcodes,
            patch_opcodes,
            kind: PatchKind::JmpToTrampoline,
        }
    }
}

fn get_screen_coords(
    pos: WowVector3,
    mvp: &Matrix4<f32>,
    screen_w: i32,
    screen_h: i32,
) -> Option<(f32, f32)> {
    // match wow2glm axis swap from original
    let pos_gl: Vector3<f32> = pos.into();
    let pos_gl = Vector4::<f32>::new(pos_gl.x, pos_gl.y, pos_gl.z, 1.0);

    let clip = mvp * pos_gl;

    if clip.w == 0.0 {
        return None;
    }
    let ndc = clip / clip.w;

    // behind camera
    if ndc.z < -1.0 || ndc.z > 1.0 {
        return None;
    }

    // NDC to screen: NDC is [-1,1], map to [0, screen_w/h]
    let sx = (ndc.x + 1.0) / 2.0 * (screen_w as f32);
    let sy = (1.0 - ndc.y) / 2.0 * (screen_h as f32); // flip Y

    Some((sx, sy))
}

fn in_rect(sx: f32, sy: f32, left: f32, top: f32, w: f32, h: f32) -> bool {
    sx >= left && sx <= left + w && sy >= top && sy <= top + h
}

pub fn find_units_within_screen_region(
    left: f32,
    top: f32,
    width: f32,
    height: f32,
) -> anyhow::Result<Vec<WowObject>> {
    let camera = WowCamera::fetch().ok_or_else(|| anyhow::anyhow!("No camera"))?;
    let custom_camera = CUSTOM_CAMERA.lock().unwrap();

    let camera_pos: Vector3<f32> = custom_camera.pos.into();

    let proj =
        Perspective3::new(camera.aspect, camera.fov, camera.z_near, camera.z_far).to_homogeneous();

    let translation = Translation3::from(-camera_pos).to_homogeneous();
    let rot = Rotation3::new(Vector3::x() * custom_camera.get_angle()).to_homogeneous();
    let mvp = proj * rot * translation;

    // let view = Translation3::from(-camera_pos).to_homogeneous();
    // let rot = Rotation3::new(Vector3::z() * custom_camera.get_angle()).to_homogeneous();
    // let rot = custom_camera.get_rot().to_homogeneous();
    // let mvp = proj * rot * view;

    let om = ObjectManager::new()?;

    let (cx, cy) = get_cursor_position()?;
    tracing::info!("{:?}", (cx, cy));
    let (sw, sh) = get_screen_size()?;

    let mut res = vec![];
    for unit in om
        .iter()
        .filter(|o| matches!(o.get_type(), WowObjectType::Unit))
    {
        if let Some((sx, sy)) = get_screen_coords(unit.get_pos()?, &mvp, sw, sh)
            && in_rect(sx, sy, left, top, width, height)
        {
            tracing::info!("{}: {:?}", unit.get_name(), (sx, sy));
            res.push(unit);
        }
    }

    Ok(res)
}
