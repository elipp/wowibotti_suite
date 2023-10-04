use std::ffi::c_void;
use std::mem::size_of;
use std::pin::Pin;

use windows::Win32::System::{
    Diagnostics::Debug::WriteProcessMemory,
    Memory::{VirtualProtect, PAGE_PROTECTION_FLAGS},
    Threading::GetCurrentProcess,
};

use crate::{asm, dump_to_logfile, Addr, EndScene_hook, LoleError, LoleResult};

const INSTRBUF_SIZE: usize = 64;

#[derive(Debug)]
pub struct InstructionBuffer {
    instructions: Pin<Box<[u8; INSTRBUF_SIZE]>>,
    current_offset: usize,
}

impl InstructionBuffer {
    pub fn new() -> Self {
        Self {
            instructions: Pin::new(Box::new([asm::INT3; INSTRBUF_SIZE])),
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
            .copy_from_slice(&slice);
        self.current_offset += slice.len();
    }

    pub fn push_call_to(&mut self, to: Addr) {
        self.push(asm::CALL);
        let target = to - (self.get_address() as u32 + self.current_offset as u32) - 4;
        self.push_slice(&target.to_le_bytes());
    }

    pub fn push(&mut self, op: u8) {
        self.instructions[self.current_offset] = op;
        self.current_offset += 1;
    }

    pub fn push_default_return(&mut self, patch_addr: u32, patch_size: usize) {
        let target = patch_addr + patch_size as u32;
        self.push(asm::PUSH);
        self.push_slice(&target.to_le_bytes());
        self.push(asm::RET);
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

    pub unsafe fn enable(&self) -> LoleResult<()> {
        match self.kind {
            PatchKind::JmpToTrampoline => {
                const PAGE_EXECUTE_READWRITE: u32 = 0x40;
                let mut old_flags = PAGE_PROTECTION_FLAGS(0);
                VirtualProtect(
                    self.patch_opcodes.get_address() as *const c_void,
                    self.patch_opcodes.len(),
                    PAGE_PROTECTION_FLAGS(PAGE_EXECUTE_READWRITE),
                    &mut old_flags,
                )
                .map_err(|e| LoleError::PatchError(format!("{:?}", e)))?;

                let mut patch = InstructionBuffer::new();
                patch.push(asm::JMP);
                let jmp_target =
                    self.patch_opcodes.get_address() as i32 - self.patch_addr as i32 - 5;
                patch.push_slice(&jmp_target.to_le_bytes());
                self.commit(&patch)?;
            }
            PatchKind::OverWrite => unsafe {
                self.commit(&self.patch_opcodes)?;
            },
        }
        Ok(())
    }
    pub fn disable(self) -> LoleResult<()> {
        println!("disabling {}", self.name);
        write_addr(self.patch_addr, &self.original_opcodes)
    }
}

pub fn deref<T: Copy, const N: u8>(addr: Addr) -> T {
    let mut t = addr;
    for _ in 1..N {
        t = unsafe { *(t as *const Addr) };
    }
    unsafe { *(t as *const T) }
}

pub fn read_elems_from_addr<const N: usize, T: Default + Sized + Copy + std::fmt::Debug>(
    addr: Addr,
) -> [T; N] {
    let mut res = [T::default(); N];
    unsafe { std::ptr::copy(addr as *const _, res.as_mut_ptr(), N) };
    res
}

pub fn write_addr<T: Sized + Copy + std::fmt::Debug>(addr: Addr, data: &[T]) -> LoleResult<()> {
    unsafe {
        let mut bytes_written = 0;
        WriteProcessMemory(
            GetCurrentProcess(),
            addr as *const _,
            data.as_ptr() as *const _,
            data.len() * size_of::<T>(),
            Some(&mut bytes_written),
        )
        .map_err(|_| LoleError::MemoryWriteError)?;
        if bytes_written != data.len() * size_of::<T>() {
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
    let EndScene = find_EndScene();
    let original_opcodes = copy_original_opcodes(EndScene, 7);

    let mut instruction_buffer = InstructionBuffer::new();
    instruction_buffer.push(asm::PUSHAD);
    instruction_buffer.push_call_to(EndScene_hook as u32);
    instruction_buffer.push(asm::POPAD);
    instruction_buffer.push_slice(&original_opcodes);
    instruction_buffer.push_default_return(EndScene, 7);

    Patch {
        name: "EndScene",
        patch_addr: EndScene,
        original_opcodes,
        patch_opcodes: instruction_buffer,
        kind: PatchKind::JmpToTrampoline,
    }
}

fn find_EndScene() -> Addr {
    let wowd3d9 = deref::<Addr, 1>(crate::addresses::D3D9Device);
    let d3d9 = deref::<Addr, 2>(wowd3d9 + crate::addresses::D3D9DeviceOffset);
    deref::<Addr, 1>(d3d9 + 0x2A * 4)
}
