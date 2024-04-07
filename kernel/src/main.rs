#![no_std]
#![no_main]
#![feature(
    const_mut_refs,
    slice_internals,
    iter_order_by,
    c_variadic,
    slice_ptr_len,
    core_intrinsics
)]
#![feature(vec_into_raw_parts)]

mod alloc;
mod libc;
mod model;

use {
    bootloader_api::{entry_point, BootInfo, BootloaderConfig},
    bootloader_boot_config::LevelFilter,
    core::{fmt::Write, panic::PanicInfo},
};

pub const CONFIG: BootloaderConfig = {
    let mut config = BootloaderConfig::new_default();
    config.kernel_stack_size *= 1024;
    config
};

mod pic {
    use {pic8259::ChainedPics, spin::Mutex};

    pub const PIC_1_OFFSET: u8 = 32;
    pub const PIC_2_OFFSET: u8 = PIC_1_OFFSET + 8;

    pub static PICS: Mutex<ChainedPics> =
        Mutex::new(unsafe { ChainedPics::new(PIC_1_OFFSET, PIC_2_OFFSET) });

    pub fn init() {}
}

#[link(name = "render")]
extern "C" {
    fn kernel_main(ptr: *mut u8, len: u32);
}

fn kernel_entry(info: &'static mut BootInfo) -> ! {
    pic::init();

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
