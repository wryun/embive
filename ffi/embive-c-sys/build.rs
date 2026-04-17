fn main() {
    let root = std::path::PathBuf::from(
        std::env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR not set"),
    );
    let repo_root = root
        .parent()
        .and_then(|p| p.parent())
        .expect("ffi/embive-c-sys should live under the repository root");

    println!("cargo:rerun-if-changed={}", repo_root.join("c/include/embive.h").display());
    println!("cargo:rerun-if-changed={}", repo_root.join("c/src/embive.c").display());

    cc::Build::new()
        .include(repo_root.join("c/include"))
        .file(repo_root.join("c/src/embive.c"))
        .flag_if_supported("-std=c99")
        .compile("embive_c");
}
