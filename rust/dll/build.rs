fn main() {
    println!(
        "cargo:rustc-link-arg={}/version.def",
        std::env::var("CARGO_MANIFEST_DIR").unwrap()
    );
}
