cargo build --profile=dev --features= && COPY /Y target\i686-pc-windows-msvc\debug\wowibottihookdll.dll lolerust.dll && cargo run --profile=dev --bin=injector --features=
