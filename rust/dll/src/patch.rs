use std::collections::HashMap;
use std::ffi::c_void;
use std::pin::Pin;
use std::sync::LazyLock;

use windows::Win32::System::{
    Diagnostics::Debug::WriteProcessMemory,
    Memory::{PAGE_EXECUTE_READWRITE, PAGE_PROTECTION_FLAGS, VirtualProtect},
    Threading::GetCurrentProcess,
};

use crate::ctm::prepare_ctm_finished_patch;
use crate::lua::prepare_ClosePetStables_patch;
use crate::socket::prepare_dump_outbound_packet_patch;
use crate::spell_error::prepare_spell_err_msg_trampoline;
use crate::{Addr, EndScene_hook, LoleError, LoleResult, assembly};

pub static AVAILABLE_PATCHES: LazyLock<HashMap<&'static str, Patch>> = LazyLock::new(|| {
    HashMap::from_iter([
        ("EndScene", prepare_endscene_trampoline()),
        ("ClosePetStables__lop_exec", prepare_ClosePetStables_patch()),
        ("CTM_finished", prepare_ctm_finished_patch()),
        ("dump_outbound_packet", prepare_dump_outbound_packet_patch()),
        ("SpellErrMsg", prepare_spell_err_msg_trampoline()),
    ])
});

use crate::addrs::offsets;

const INSTRBUF_SIZE: usize = 64;

#[derive(Debug)]
pub struct InstructionBuffer {
    instructions: Pin<Box<[u8; INSTRBUF_SIZE]>>,
    current_offset: usize,
}

impl Default for InstructionBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl InstructionBuffer {
    pub fn new() -> Self {
        Self {
            instructions: Pin::new(Box::new([assembly::INT3; INSTRBUF_SIZE])),
            current_offset: 0,
        }
    }
    pub fn instr_slice(&self) -> &[u8] {
        &self.instructions[..self.len()]
    }

    pub fn len(&self) -> usize {
        self.current_offset
    }

    pub fn get_address(&self) -> *const u8 {
        self.instructions.as_ptr()
    }

    pub fn push_slice(&mut self, slice: &[u8]) {
        self.instructions[self.current_offset..self.current_offset + slice.len()]
            .copy_from_slice(slice);
        self.current_offset += slice.len();
    }

    pub fn push_call_to(&mut self, to: Addr) {
        self.push(assembly::CALL);
        let target: i32 =
            (to as i64 - (self.get_address() as Addr + self.current_offset) as i64 - 4)
                .try_into()
                .expect("valid jmp target address");
        self.push_slice(&target.to_le_bytes());
    }

    pub fn push(&mut self, op: u8) {
        self.instructions[self.current_offset] = op;
        self.current_offset += 1;
    }

    pub fn push_default_return(&mut self, patch_addr: Addr, patch_size: usize) {
        let target = patch_addr + patch_size;
        self.push(assembly::PUSH_IMM);
        self.push_slice(&target.to_le_bytes());
        self.push(assembly::RET);
    }
}

#[derive(Debug)]
pub enum PatchKind {
    JmpToTrampoline,
    OverWrite,
}

#[derive(Debug)]
pub struct Patch {
    pub name: &'static str,
    pub patch_addr: Addr,
    pub original_opcodes: Box<[u8]>,
    pub patch_opcodes: InstructionBuffer,
    pub kind: PatchKind,
}

impl Patch {
    pub fn new(
        name: &'static str,
        patch_addr: Addr,
        patch_opcodes: InstructionBuffer,
        kind: PatchKind,
    ) -> Self {
        Patch {
            name,
            patch_addr,
            original_opcodes: copy_original_opcodes(patch_addr, patch_opcodes.len()),
            patch_opcodes,
            kind,
        }
    }

    unsafe fn commit(&self, patch: &InstructionBuffer) -> LoleResult<()> {
        write_addr(self.patch_addr, patch.instr_slice())
    }

    pub fn enable(&self) -> LoleResult<()> {
        match self.kind {
            PatchKind::JmpToTrampoline => {
                const PAGE_EXECUTE_READWRITE: u32 = 0x40;
                let mut old_flags = PAGE_PROTECTION_FLAGS(0);
                unsafe {
                    VirtualProtect(
                        self.patch_opcodes.get_address() as *const c_void,
                        self.patch_opcodes.len(),
                        PAGE_PROTECTION_FLAGS(PAGE_EXECUTE_READWRITE),
                        &mut old_flags,
                    )
                    .map_err(|e| LoleError::PatchError(format!("{:?}", e)))?;
                }

                let mut tmp_patch = InstructionBuffer::new();
                tmp_patch.push(assembly::JMP);
                let jmp_target =
                    self.patch_opcodes.get_address() as i32 - self.patch_addr as i32 - 5;
                tmp_patch.push_slice(&jmp_target.to_le_bytes());
                tmp_patch.push_slice(&vec![
                    assembly::NOP;
                    self.original_opcodes.len() - tmp_patch.len()
                ]);
                unsafe {
                    self.commit(&tmp_patch)?;
                }
            }
            PatchKind::OverWrite => unsafe {
                self.commit(&self.patch_opcodes)?;
            },
        }
        Ok(())
    }
    pub fn disable(&self) -> LoleResult<()> {
        tracing::info!("disabling {}", self.name);
        write_addr(self.patch_addr, &self.original_opcodes)
    }
}

pub fn deref_t<T: Copy, const N: u8>(addr: Addr) -> T {
    deref_opt_t::<T, N>(addr as *const c_void).unwrap()
}

pub fn deref_opt_t<T: Copy, const N: u8>(ptr: *const c_void) -> Option<T> {
    if N == 0 {
        return None;
    }

    unsafe {
        let mut t = ptr;
        for _ in 1..N {
            if t.is_null() {
                return None;
            }
            t = std::ptr::read_unaligned(t as *const *const c_void);
        }
        if t.is_null() {
            return None;
        }
        Some(std::ptr::read_unaligned(t as *const T))
    }
}

pub fn deref_res_t<T: Copy, const N: u8>(ptr: *const c_void) -> LoleResult<T> {
    deref_opt_t::<T, N>(ptr).ok_or_else(|| LoleError::NullPtrError)
}

pub fn deref_ptr<const N: u8>(ptr: *const c_void) -> *const c_void {
    match deref_opt_t::<*const c_void, N>(ptr) {
        Some(ptr) => ptr,
        None => std::ptr::null(),
    }
}

pub fn deref_opt_ptr<const N: u8>(ptr: *const c_void) -> Option<*const c_void> {
    match deref_opt_t::<*const c_void, N>(ptr) {
        Some(ptr) if !ptr.is_null() => Some(ptr),
        _ => None,
    }
}

pub fn deref_res_ptr<const N: u8>(ptr: *const c_void) -> LoleResult<*const c_void> {
    match deref_opt_ptr::<N>(ptr) {
        Some(ptr) => Ok(ptr),
        _ => Err(LoleError::NullPtrError),
    }
}

pub fn read_elems_from_addr<const N: usize, T: Default + Sized + Copy + std::fmt::Debug>(
    addr: *const T,
) -> [T; N] {
    // 2025: can't use MaybeUninit here because [T; N] apparently is not a "fixed-size type"
    let mut res = [T::default(); N];
    unsafe { std::ptr::copy(addr, res.as_mut_ptr(), N) };
    res
}

pub fn write_addr<T: Sized + Copy + std::fmt::Debug>(addr: Addr, data: &[T]) -> LoleResult<()> {
    unsafe {
        let size = std::mem::size_of_val(data);
        let mut old_flags = PAGE_PROTECTION_FLAGS(0);
        VirtualProtect(
            addr as *const c_void,
            size,
            PAGE_EXECUTE_READWRITE,
            &mut old_flags,
        )
        .map_err(|e| LoleError::MemoryWriteError(format!("{e:?}")))?;

        // could use just std::ptr::copy_nonoverlapping
        let mut bytes_written = 0;
        let result = WriteProcessMemory(
            GetCurrentProcess(),
            addr as _,
            data.as_ptr() as _,
            std::mem::size_of_val(data),
            Some(&mut bytes_written),
        );
        VirtualProtect(addr as *const c_void, size, old_flags, &mut old_flags)
            .map_err(|e| LoleError::MemoryWriteError(format!("{e:?}")))?;

        if let Err(e) = result {
            return Err(LoleError::MemoryWriteError(format!("{e:?}")));
        }

        if bytes_written != std::mem::size_of_val(data) {
            return Err(LoleError::PartialMemoryWriteError);
        }
        Ok(())
    }
}

pub fn copy_original_opcodes(addr: Addr, n: usize) -> Box<[u8]> {
    let mut destination = vec![0u8; n];
    unsafe {
        std::ptr::copy_nonoverlapping(
            addr as *const u8,
            destination.as_mut_ptr(),
            destination.len(),
        )
    };
    destination.into()
}

pub fn prepare_endscene_trampoline() -> Patch {
    let endscene_addr = loop {
        if let Some(addr) = find_EndScene() {
            break addr;
        }
    };

    tracing::info!("EndScene: {:p}", endscene_addr as *const ());
    // has a different signature :D
    const LEN_TRAMP: usize = 7;

    let original_opcodes = copy_original_opcodes(endscene_addr, LEN_TRAMP);

    let mut patch_opcodes = InstructionBuffer::new();
    patch_opcodes.push(assembly::PUSHAD);
    patch_opcodes.push_call_to(EndScene_hook as *const () as Addr);
    patch_opcodes.push(assembly::POPAD);
    patch_opcodes.push_slice(&original_opcodes);
    patch_opcodes.push_default_return(endscene_addr, LEN_TRAMP);

    Patch {
        name: "EndScene",
        patch_addr: endscene_addr,
        original_opcodes,
        patch_opcodes,
        kind: PatchKind::JmpToTrampoline,
    }
}

#[allow(non_snake_case)]
fn find_EndScene() -> Option<Addr> {
    unsafe {
        let wowd3d9 = deref_opt_t::<*const c_void, 1>(offsets::D3D9_DEVICE as *const c_void)?;
        let d3d9 = deref_opt_t::<*const c_void, 2>(wowd3d9.offset(offsets::D3D9_DEVICE_OFFSET))?;
        deref_opt_t::<Addr, 1>(d3d9.offset(0x2A * 4))
    }
}
