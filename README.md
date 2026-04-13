## Windows 

Install [Rust](https://www.rust-lang.org/tools/install), with the toolchain for 32-bit Windows (MSVC): `rustup target add i686-pc-windows-msvc`
- create `injector\potti.conf.json` (see example config)
- Run `rust\injector.bat` with Administrator privileges

## Linux
### Build
```
rustup target add i686-pc-windows-gnu
just all
just broker
```

### Run
```
cd <wow client dir>
cp DivxDecoder.dll DivxDecoder.real.dll
ln -sf /home/somebody/.../wowibotti_suite/rust/target/release/wowibottihookdll.dll DivxDecoder.dll

wine Wow.exe
```
