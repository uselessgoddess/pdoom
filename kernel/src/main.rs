#![no_std]
#![no_main]
#![feature(const_mut_refs, slice_internals, iter_order_by, c_variadic, slice_ptr_len)]

mod alloc;
mod doom;
mod libc;

use {
    bootloader_api::{entry_point, BootInfo, BootloaderConfig},
    bootloader_boot_config::LevelFilter,
    core::{
        ffi::{c_char, c_int, CStr},
        fmt::Write,
        panic::PanicInfo,
    },
    log::log,
};

pub const CONFIG: BootloaderConfig = {
    let mut config = BootloaderConfig::new_default();
    config.kernel_stack_size *= 1024;
    config
};

#[link(name = "doom")]
extern "C" {
    fn kernel_main(ptr: *mut u8, len: u32);
}

fn kernel_entry(info: &'static mut BootInfo) -> ! {
    unsafe {
        alloc::ALLOC.init();

        let framebuffer = info.framebuffer.as_mut().unwrap();

        let info = framebuffer.info();
        let buf = framebuffer.buffer_mut() as *mut [u8];

        bootloader_x86_64_common::init_logger(&mut *buf, info, LevelFilter::Info, true, true);

        kernel_main(buf as *mut u8, buf.len() as u32);
    }

    loop {}
}

entry_point!(kernel_entry, config = &CONFIG);

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    if let Some(logger) = bootloader_x86_64_common::logger::LOGGER.get() {
        unsafe { logger.force_unlock() }
    }
    log::error!("{info}");
    loop {
        unsafe { core::arch::asm!("cli; hlt") };
    }
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
