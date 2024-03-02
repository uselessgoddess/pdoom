fn main() {
    let uefi = env!("UEFI_PATH");
    let bios = env!("BIOS_PATH");

    println!("UEFI: `{uefi}`");
    println!("BIOS: `{bios}`");

    let is_uefi = false;

    let mut cmd = std::process::Command::new("qemu-system-x86_64");
    if is_uefi {
        todo!()
    } else {
        cmd.arg("-drive").arg(format!("format=raw,file={bios}"));
    }
    let mut child = cmd.spawn().unwrap();
    child.wait().unwrap();
}
