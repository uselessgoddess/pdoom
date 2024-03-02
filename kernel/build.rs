fn main() {
    println!("cargo:rerun-if-changed=cc/main.cxx");
    println!("cargo:rerun-if-changed=cc/CMakeLists.txt");
    println!("cargo:rerun-if-changed=cc/build/libdoom.a");

    // println!(
    //     "cargo:rustc-link-search={}",
    //     cmake::build("cc/CMakeLists.txt").join("lib").display()
    // );
    println!(
        "cargo:rustc-link-search={}",
        format!("{}{}", env!("CARGO_MANIFEST_DIR"), "/cc/build/")
    );
}
