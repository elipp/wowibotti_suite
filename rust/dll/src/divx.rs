use std::sync::OnceLock;

use windows::{
    Win32::System::LibraryLoader::{GetProcAddress, LoadLibraryA},
    core::PCSTR,
};

struct DivxReal {
    divx_decode: unsafe extern "C" fn(i32, i32, i32) -> i32,
    initialize: unsafe extern "C" fn(i32) -> i32,
    set_output_format: unsafe extern "C" fn(i32, i32, i32, i32) -> i32,
    uninitialize: unsafe extern "C" fn(i32) -> i32,
}

static REAL: OnceLock<DivxReal> = OnceLock::new();

fn real() -> &'static DivxReal {
    REAL.get_or_init(|| unsafe {
        let lib = LoadLibraryA(PCSTR(b"DivxDecoder.dll.real\0".as_ptr())).unwrap();
        macro_rules! proc {
            ($name:literal, $type:ty) => {
                std::mem::transmute::<_, $type>(GetProcAddress(lib, PCSTR($name.as_ptr())).unwrap())
            };
        }
        DivxReal {
            divx_decode: proc!(b"DivxDecode\0", unsafe extern "C" fn(i32, i32, i32) -> i32),
            initialize: proc!(b"InitializeDivxDecoder\0", unsafe extern "C" fn(i32) -> i32),
            set_output_format: proc!(
                b"SetOutputFormat\0",
                unsafe extern "C" fn(i32, i32, i32, i32) -> i32
            ),
            uninitialize: proc!(
                b"UnInitializeDivxDecoder\0",
                unsafe extern "C" fn(i32) -> i32
            ),
        }
    })
}

macro_rules! int3h {
    () => {
        asm! {
            "int 3h"
        }
    };
}

#[unsafe(no_mangle)]
extern "C" fn DivxDecode(a: i32, b: i32, c: i32) -> i32 {
    // int3h!();
    unsafe { (real().divx_decode)(a, b, c) }
}
#[unsafe(no_mangle)]
extern "C" fn InitializeDivxDecoder(a: i32) -> i32 {
    // int3h!();
    unsafe { (real().initialize)(a) }
}
#[unsafe(no_mangle)]
extern "C" fn SetOutputFormat(a: i32, b: i32, c: i32, d: i32) -> i32 {
    // int3h!();
    unsafe { (real().set_output_format)(a, b, c, d) }
}
#[unsafe(no_mangle)]
extern "C" fn UnInitializeDivxDecoder(a: i32) -> i32 {
    // int3h!();
    unsafe { (real().uninitialize)(a) }
}
