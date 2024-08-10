## Backend ##
- Install [Rust](https://www.rust-lang.org/tools/install), with the toolchain for 32-bit Windows (MSVC): `rustup target add i686-pc-windows-msvc`
- create `injector\potti.conf.json` (see example config)
- Run `rust\injector.bat` with Administrator privileges

## ~~ Front ~~ (obsolete, just use `injector`) ##
- If you don't want to install Node.js etc: `python -m http.server -d front\dist 5173`
- Else: `cd front && npm run dev`
