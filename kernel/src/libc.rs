extern crate alloc;

use {
    alloc::borrow::ToOwned,
    core::{
        ffi::{c_char, c_float, c_int, CStr},
        mem, ptr,
        slice::memchr::{memchr, memrchr},
    },
};

#[no_mangle]
unsafe extern "C" fn exit(_code: c_int) {
    loop {
        core::arch::asm!("hlt")
    }
}

#[no_mangle]
extern "C" fn abs(n: c_int) -> c_int {
    n.abs()
}

#[no_mangle]
extern "C" fn toupper(n: c_int) -> c_int {
    if let Ok(n) = char::try_from(n as u32) { n.to_ascii_lowercase() as c_int } else { n }
}

#[no_mangle]
unsafe extern "C" fn strchr(str: *const c_char, ch: c_int) -> *const c_char {
    memchr(ch as u8, CStr::from_ptr(str).to_bytes_with_nul())
        .map(|x| unsafe { str.add(x) })
        .unwrap_or(ptr::null())
}

#[no_mangle]
unsafe extern "C" fn strrchr(str: *const c_char, ch: c_int) -> *const c_char {
    memrchr(ch as u8, CStr::from_ptr(str).to_bytes_with_nul())
        .map(|x| unsafe { str.add(x) })
        .unwrap_or(ptr::null())
}

#[no_mangle]
unsafe extern "C" fn strdup(str: *const c_char) -> *const c_char {
    let str = CStr::from_ptr(str).to_owned();
    let ptr = str.as_ptr();
    mem::forget(str);
    ptr
}

#[no_mangle]
unsafe extern "C" fn strncasecmp(a: *const c_char, b: *const c_char, n: usize) -> c_int {
    let a = CStr::from_ptr(a).to_bytes()[..n].iter();
    let b = CStr::from_ptr(b).to_bytes()[..n].iter();
    a.cmp_by(b, |a, b| a.to_ascii_lowercase().cmp(&b.to_ascii_lowercase())) as c_int
}

#[no_mangle]
unsafe extern "C" fn strcasecmp(a: *const c_char, b: *const c_char) -> c_int {
    let a = CStr::from_ptr(a).to_bytes().iter();
    let b = CStr::from_ptr(b).to_bytes().iter();
    a.cmp(b) as c_int
}

#[no_mangle]
unsafe extern "C" fn strcmp(a: *const c_char, b: *const c_char) -> c_int {
    CStr::from_ptr(a).cmp(CStr::from_ptr(b)) as c_int
}

#[no_mangle]
unsafe extern "C" fn atoi(str: *const c_char) -> c_int {
    // TODO: no satisfy standard libc behaviour
    CStr::from_ptr(str).to_string_lossy().parse::<c_int>().unwrap_or(0)
}

#[no_mangle]
unsafe extern "C" fn atof(str: *const c_char) -> c_float {
    // TODO: no satisfy standard libc behaviour
    CStr::from_ptr(str).to_string_lossy().parse::<c_float>().unwrap_or(0.0)
}

#[no_mangle]
unsafe extern "C" fn printf(str: *const c_char, ...) {
    puts(str);
}

#[no_mangle]
unsafe extern "C" fn puts(str: *const c_char) {
    log::info!("{}", CStr::from_ptr(str).to_string_lossy());
}

#[no_mangle]
extern "C" fn putchar(c: c_int) {
    if let Ok(c) = char::try_from(c as u32) {
        log::info!("{c}")
    } else {
        log::info!("#");
    }
}

#[no_mangle]
unsafe extern "C" fn cpu_time_us() -> u64 {
    x86::time::rdtsc() / 50000
}

#[no_mangle]
unsafe extern "C" fn sqrtf(f: f32) -> f32 {
    core::intrinsics::sqrtf32(f)
}
