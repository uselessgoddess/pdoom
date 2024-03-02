use {
    bootloader::{BiosBoot, BootConfig, UefiBoot},
    std::{env, fmt::Debug, path::PathBuf},
};

fn main() {
    let out_dir = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    let kernel = PathBuf::from(env::var_os("CARGO_BIN_FILE_KERNEL_kernel").unwrap());

    let mut config = BootConfig::default();
    config.frame_buffer.minimum_framebuffer_height = Some(25);
    config.frame_buffer.minimum_framebuffer_width = Some(80);

    let uefi_path = out_dir.join("uefi.img");
    UefiBoot::new(&kernel).create_disk_image(&uefi_path).unwrap();

    let bios_path = out_dir.join("bios.img");
    BiosBoot::new(&kernel).create_disk_image(&bios_path).unwrap();

    println!("cargo:rustc-env=UEFI_PATH={}", uefi_path.display());
    println!("cargo:rustc-env=BIOS_PATH={}", bios_path.display());
}
