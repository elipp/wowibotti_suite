cargo build -p wowibottihookdll --profile=dev --target i686-pc-windows-msvc --features host-windows

set RUST_LOG=injector=debug,addonmessage_broker::server=debug,debug
cargo run -p injector --profile=dev --target i686-pc-windows-msvc --features host-windows
