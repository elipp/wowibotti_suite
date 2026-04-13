fn main() {
    println!(
        "cargo:rustc-link-arg={}/divxdecoder.def",
        std::env::var("CARGO_MANIFEST_DIR").unwrap()
    );
}
