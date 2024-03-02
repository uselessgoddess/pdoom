use core::ffi::{c_char, c_int};

// #[link(name = "doom")]
// extern "C" {
//     pub fn doomgeneric_Create(ptr: *const u8, len: u32);
//     pub fn doomgeneric_Tick();
// }

pub static DOOM1: &[u8] = include_bytes!("../cc/doom1.wad");

#[no_mangle]
extern "C" fn wad_bytes() -> *const u8 {
    DOOM1.as_ptr()
}

#[no_mangle]
extern "C" fn wad_len() -> usize {
    DOOM1.len()
}
