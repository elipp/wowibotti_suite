use std::sync::{LazyLock, Mutex, atomic::AtomicBool};

use nalgebra::{Matrix4, Perspective3, Rotation3, RowVector4, Translation3, Vector3, Vector4};
use windows::Win32::{
    Foundation::{HWND, RECT},
    UI::WindowsAndMessaging::{GetClientRect, GetForegroundWindow},
};

use crate::{
    Addr,
    addrs::offsets::{WOW_CAMERA, WOW_CAMERA_L2_OFFSET, Z_FAR_STATIC, Z_NEAR_STATIC},
    assembly::NOP,
    dostring,
    input::{get_cursor_position, get_screen_size},
    linalg::WowVector3,
    objectmanager::{GUID, ObjectManager, WowObject, WowObjectType},
    patch::{deref_opt_ptr, write_addr},
};

pub static WC3MODE_ENABLED: AtomicBool = AtomicBool::new(false);
pub static CUSTOM_CAMERA: LazyLock<Mutex<CustomCamera>> = LazyLock::new(Default::default);

pub static CONTROL_GROUPS: LazyLock<Mutex<[Vec<(String, GUID)>; 10]>> =
    LazyLock::new(Default::default);

#[derive(Debug)]
pub struct ScreenRegion {
    pub left: f32,
    pub top: f32,
    pub width: f32,
    pub height: f32,
}

#[repr(C)]
#[derive(Debug)]
pub struct WowCamera {
    pub unk1: u32,
    pub unk2: u32,
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub rot: [[f32; 3]; 3],
    pub znear: f32,
    pub zfar: f32,
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
        let n = self.znear;
        let f = self.zfar;

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
    pub s: f32,
    pub maxdistance: f32,
    pub pos: WowVector3,
    pub znear: Option<f32>,
    pub zfar: Option<f32>,
    pub fov: Option<f32>,
    pub aspect: Option<f32>,
}

const S_MIN: f32 = 0.3;
const S_MAX: f32 = 0.8;
const S_INCREMENT: f32 = 0.01;

impl Default for CustomCamera {
    fn default() -> Self {
        let res = Self {
            s: 0.5,
            maxdistance: 80.0,
            pos: WowVector3::new(0.0, 80.0, 0.0),
            znear: None,
            zfar: None,
            fov: None,
            aspect: None,
        };
        res
    }
}

impl CustomCamera {
    pub fn get_cameraoffset(&self) -> WowVector3 {
        self.maxdistance * WowVector3::new(-0.1 * self.s.powi(2), 0.0, 0.5 * self.s)
    }
    pub fn get_angle(&self) -> f32 {
        0.9 + self.s * 0.4
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

    pub fn get_absolute_pos(&self) -> WowVector3 {
        self.pos + self.get_cameraoffset()
    }

    pub fn get_rot(&self) -> Rotation3<f32> {
        Rotation3::new(Vector3::y() * self.get_angle())
    }

    pub fn commit_to_wowcamera_memory(&mut self, wow_camera: &mut WowCamera) -> anyhow::Result<()> {
        wow_camera.set_rot(self.get_rot());
        wow_camera.set_pos(&self.get_absolute_pos().into());

        if let Some(znear) = self.znear {
            write_addr(Z_NEAR_STATIC, &[znear])?;
        }
        if let Some(zfar) = self.zfar {
            write_addr(Z_FAR_STATIC, &[zfar])?;
        }
        if let Some(fov) = self.fov {
            wow_camera.fov = fov;
        }
        if let Some(aspect) = self.aspect {
            wow_camera.aspect = aspect;
        }
        Ok(())
    }

    pub fn tick(&mut self, wow_camera: &mut WowCamera) -> anyhow::Result<()> {
        let (cx, cy) = get_cursor_position()?;
        let (sx, sy) = get_screen_size()?;

        let margin_x: i32 = ((sx as f32) * 0.12).round() as i32;
        let margin_y: i32 = ((sy as f32) * 0.12).round() as i32;
        const INCREMENT: f32 = 0.5;

        if cx < margin_x {
            self.pos.0.y += INCREMENT * ((margin_x - cx) as f32) / (sx as f32);
        } else if cx > sx - margin_x {
            self.pos.0.y += -INCREMENT * ((cx - (sx - margin_x)) as f32) / (sx as f32);
        }
        if cy < margin_y {
            self.pos.0.x += INCREMENT * ((margin_y - cy) as f32) / (sy as f32);
        } else if cy > sy - margin_y {
            self.pos.0.x += -INCREMENT * ((cy - (sy - margin_y)) as f32) / (sy as f32);
        }

        self.commit_to_wowcamera_memory(wow_camera)?;

        Ok(())
    }

    /// Reset camera to the "default" view over the player
    pub fn reset_camera(&mut self, wow_camera: &mut WowCamera) -> anyhow::Result<()> {
        let p = ObjectManager::new()?.get_player()?;
        self.pos = p.get_pos()?;
        self.commit_to_wowcamera_memory(wow_camera)?;
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

    {
        let mut custom_camera = CUSTOM_CAMERA
            .lock()
            .map_err(|_| anyhow::anyhow!("custom_camera mutex"))?;
        let mut wow_camera =
            WowCamera::fetch_mut().ok_or_else(|| anyhow::anyhow!("No Wow camera"))?;

        custom_camera.tick(&mut wow_camera)?;
    }

    // draw_debug_markers()?;

    Ok(())
}

pub fn draw_debug_markers() -> anyhow::Result<()> {
    let mvp = get_wow_mvp_matrix_in_nalgebra_space()?;
    let (sw, sh) = get_screen_size()?;

    let mut units = vec![];
    for unit in ObjectManager::new()?
        .iter()
        .filter(|o| matches!(o.get_type(), WowObjectType::Unit))
    {
        if let Some((sx, sy)) = get_screen_coords(unit.get_pos()?, &mvp, sw, sh) {
            units.push(format!(
                "{{x = {sx}, y = {sy}, name = '{}'}}",
                unit.get_name().to_owned()
            ));
        }
    }

    let markers = units.join(",");

    dostring!("lole_wc3mode.debug({{{}}})", markers);
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
        lua::wc3::get_selected_units,
        objectmanager::ObjectManager,
        patch::{InstructionBuffer, Patch, PatchKind, copy_original_opcodes},
    };

    pub unsafe extern "stdcall" fn pylpyr_hook() {
        let Ok(selected_units) = get_selected_units() else {
            return tracing::error!("selected units failed");
        };
        let Ok(om) = ObjectManager::new() else {
            return tracing::error!("No object manager");
        };
        for (_, guid) in selected_units.iter() {
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
    mut pos: WowVector3,
    mvp: &Matrix4<f32>,
    screen_w: i32,
    screen_h: i32,
) -> Option<(f32, f32)> {
    // match wow2glm axis swap from original
    pos.0.z += 1.0; // makes the selection a bit more intuitive (more like mass center of this unit)
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

pub fn get_wow_mvp_matrix_in_nalgebra_space() -> anyhow::Result<Matrix4<f32>> {
    let camera = WowCamera::fetch().ok_or_else(|| anyhow::anyhow!("No camera"))?;
    let custom_camera = CUSTOM_CAMERA.lock().unwrap();

    let camera_pos: Vector3<f32> = custom_camera.get_absolute_pos().into();

    const FOVY_CORRECTION_FACTOR: f32 = 0.92; // no idea why this is needed but. Found empirically
    let fovy = FOVY_CORRECTION_FACTOR * 2.0 * ((camera.fov / 2.0).tan() / camera.aspect).atan();

    let proj = Perspective3::new(camera.aspect, fovy, camera.znear, camera.zfar).to_homogeneous();

    let translation = Translation3::from(-camera_pos).to_homogeneous();
    let rot = Rotation3::new(Vector3::x() * custom_camera.get_angle()).to_homogeneous();
    let mvp = proj * rot * translation;
    Ok(mvp)
}

impl ScreenRegion {
    pub fn find_units_within_projection(
        &self,
        mvp: &Matrix4<f32>,
    ) -> anyhow::Result<Vec<WowObject>> {
        let om = ObjectManager::new()?;

        let (sw, sh) = get_screen_size()?;

        let mut res = vec![];
        for unit in om
            .iter()
            .filter(|o| matches!(o.get_type(), WowObjectType::Unit))
        {
            if let Some((sx, sy)) = get_screen_coords(unit.get_pos()?, &mvp, sw, sh)
                && self.contains(sx, sy)
            {
                // tracing::info!("{}: {:?}", unit.get_name(), (sx, sy));
                res.push(unit);
            }
        }

        Ok(res)
    }

    pub fn contains(&self, x: f32, y: f32) -> bool {
        in_rect(x, y, self.left, self.top, self.width, self.height)
    }
}
