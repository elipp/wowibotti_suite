use http::{Method, Request, Response, StatusCode};
use hyper::server::conn::http1;
use hyper::service::service_fn;
use inject::{
    find_wow_windows_and_inject, InjectorError, InjectorResult, INJ_MESSAGE_REGISTER_HOTKEY,
    INJ_MESSAGE_UNREGISTER_HOTKEY,
};
use lazy_static::lazy_static;
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use serde_json::json;
use std::sync::{Arc, Mutex};
use std::{ffi::OsString, net::SocketAddr, os::windows::ffi::OsStrExt, path::PathBuf};
use tokio::net::TcpListener;
use tokio_util::task::LocalPoolHandle;
use windows::core::w;
use windows::Win32::Foundation::{LPARAM, LRESULT, WPARAM};
use windows::Win32::UI::Input::KeyboardAndMouse::{RegisterHotKey, UnregisterHotKey, MOD_ALT};
use windows::Win32::UI::WindowsAndMessaging::{
    CreateWindowExW, DefWindowProcW, RegisterClassW, SetForegroundWindow, CS_HREDRAW, CS_VREDRAW,
    CW_USEDEFAULT, WINDOW_EX_STYLE, WM_USER, WNDCLASSW, WS_OVERLAPPEDWINDOW, WS_VISIBLE,
};
use windows::{
    core::PCWSTR,
    core::PWSTR,
    Win32::{
        Foundation::{FALSE, HWND},
        System::{
            LibraryLoader::GetModuleHandleW,
            Threading::{
                CreateProcessW, PROCESS_CREATION_FLAGS, PROCESS_INFORMATION, STARTUPINFOW,
            },
        },
        UI::WindowsAndMessaging::{
            DispatchMessageW, GetMessageW, TranslateMessage, MSG, WM_HOTKEY,
        },
    },
};

use http_body_util::BodyExt;

mod inject;
mod tokio_io;

use crate::inject::CLIENTS;
use crate::tokio_io::TokioIo;

use wowibottihookdll::{CharacterInfo, WowAccount};

lazy_static! {
    static ref DUMMY_WINDOW_HWND: Arc<Mutex<HWND>> = Arc::new(Mutex::new(HWND(0)));
}

fn json_response_builder() -> http::response::Builder {
    http::Response::builder()
        .header("Accept", "application/json")
        .header("Content-Type", "application/json")
        .header("Access-Control-Allow-Origin", "*")
}

pub type IHttpResult = http::Result<Response<String>>;

fn not_found() -> IHttpResult {
    Response::builder()
        .status(StatusCode::NOT_FOUND)
        .body(String::from("{}"))
}

fn bad_request<T: Serialize>(msg: T) -> IHttpResult {
    json_response_builder()
        .status(StatusCode::BAD_REQUEST)
        .body(
            json!({
                "error": "bad_request",
                "details": msg
            })
            .to_string(),
        )
}

fn internal_server_error<T: Serialize>(msg: T) -> IHttpResult {
    json_response_builder()
        .status(StatusCode::INTERNAL_SERVER_ERROR)
        .body(
            json!({
                "error": "internal_server_error",
                "details": msg
            })
            .to_string(),
        )
}

async fn handle_request_wrapper(req: Request<hyper::body::Incoming>) -> IHttpResult {
    match handle_request(req).await {
        Ok(r) => Ok(r),
        Err(InjectorError::NotFound) => not_found(),
        Err(InjectorError::DebugPrivilegesFailed(e)) => internal_server_error(format!(
            "{e:?} (are you running injector as an Administrator?)"
        )),
        Err(e) => internal_server_error(format!("{e:?}")),
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct PatchConfig {
    name: String,
    enabled_by_default: bool,
}

#[derive(Debug, Deserialize)]
struct PottiConfig {
    wow_client_path: PathBuf,
    accounts: Vec<WowAccount>,
    available_patches: Vec<PatchConfig>,
}

#[derive(Debug, Serialize, Deserialize)]
struct ConfigResult {
    characters: Vec<CharacterInfo>,
    available_patches: Vec<PatchConfig>,
}

impl From<PottiConfig> for ConfigResult {
    fn from(p: PottiConfig) -> Self {
        Self {
            characters: p.accounts.into_iter().map(|a| a.character).collect(),
            available_patches: p.available_patches.clone(),
        }
    }
}

#[derive(Debug, Deserialize)]
pub struct InjectQuery {
    enabled_characters: Vec<String>,
    enabled_patches: Vec<String>,
}

fn read_potti_conf() -> Result<PottiConfig, InjectorError> {
    // let config_str = std::fs::read_to_string("potti.conf")
    //     .map_err(|_e| InjectorError::OtherError(format!("couldn't read potti.conf")))?;
    let config_str = include_str!("..\\potti.conf.json");
    let config: PottiConfig = serde_json::from_str(&config_str)
        .map_err(|_e| InjectorError::DeserializationError(format! {"{_e:?}"}))?;
    Ok(config)
}

async fn parse_json_body_into<O: DeserializeOwned>(
    req: Request<hyper::body::Incoming>,
) -> Result<O, InjectorError> {
    match req.collect().await {
        Ok(bytes) => match serde_json::from_slice(&bytes.to_bytes()) {
            Ok(o) => Ok(o),
            Err(e) => Err(InjectorError::DeserializationError(format!("{e:?}"))),
        },
        Err(e) => Err(InjectorError::DeserializationError(format!("{e:?}"))),
    }
}

#[derive(Debug, Deserialize)]
struct LaunchQuery {
    num_clients: i32,
}

pub fn str_into_vec_u16<S: Into<OsString>>(s: S) -> Vec<u16> {
    let oss = s.into();
    oss.as_os_str().encode_wide().chain([0u16]).collect()
}

impl LaunchQuery {
    fn launch(self, config: PottiConfig) -> InjectorResult<()> {
        unsafe {
            let path = config.wow_client_path.into_os_string();
            let as_u16: Vec<u16> = str_into_vec_u16(path.clone());
            for _ in 0..self.num_clients {
                let startup_info = STARTUPINFOW::default();
                let mut process_information = PROCESS_INFORMATION::default();
                CreateProcessW(
                    PCWSTR::from_raw(as_u16.as_ptr()),
                    PWSTR::null(),
                    None,
                    None,
                    false,
                    PROCESS_CREATION_FLAGS(0),
                    None,
                    None,
                    &startup_info as *const STARTUPINFOW,
                    &mut process_information,
                )
                .map_err(|_e| InjectorError::LaunchError(format!("{_e:?} ({:?})", path)))?;
                std::thread::sleep(std::time::Duration::from_millis(200));
            }
        }
        Ok(())
    }
}
async fn handle_request(
    req: Request<hyper::body::Incoming>,
) -> Result<Response<String>, InjectorError> {
    // if content-type is sent, CORS gets complicated
    // match req
    //     .headers()
    //     .get("content-type")
    //     .as_ref()
    //     .map(|h| h.as_bytes())
    // {
    //     Some(b"application/json") => {}
    //     _ => {
    //         return bad_request(json!({"description": "expected content-type: application/json"}));
    //     }
    // }

    match (req.method(), req.uri().path()) {
        (&Method::GET, "/config") => {
            let config = read_potti_conf()?;
            Ok(json_response_builder()
                .status(StatusCode::OK)
                .body(json!({
                    "status": "ok",
                    "result": serde_json::to_value(ConfigResult::from(config)).map_err(|_e|InjectorError::SerializationError(format!("{_e:?}")))?
                }).to_string())?)
        }

        (&Method::POST, "/inject") => {
            let config = read_potti_conf()?;
            let query: InjectQuery = parse_json_body_into(req).await?;
            let client_info = find_wow_windows_and_inject(config, query)?;
            let serialized = serde_json::to_value(&client_info)
                .map_err(|_e| InjectorError::SerializationError(format!("{_e:?}")))?;
            Ok(json_response_builder()
                .status(StatusCode::OK)
                .body(json!({"status": "ok", "clients": serialized}).to_string())?)
        }

        (&Method::POST, "/launch") => {
            let query: LaunchQuery = parse_json_body_into(req).await?;
            query.launch(read_potti_conf()?)?;
            Ok(json_response_builder()
                .status(StatusCode::OK)
                .body(json!({"status": "ok"}).to_string())?)
        }
        _ => Err(InjectorError::NotFound),
    }
}

unsafe extern "system" fn dummy_wndproc(
    hwnd: HWND,
    msg: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    match msg {
        WM_HOTKEY => {
            let clients = CLIENTS.lock().unwrap();
            if let Some(client) = clients.get(wparam.0) {
                if SetForegroundWindow(client.window_handle) == FALSE {
                    println!("warning: `SetForegroundWindow` failed");
                }
            }
        }
        INJ_MESSAGE_REGISTER_HOTKEY => {
            if let Err(e) = RegisterHotKey(hwnd, wparam.0 as i32, MOD_ALT, lparam.0 as u32) {
                println!("Warning: RegisterHotKey failed: {e:?}");
            }
        }

        INJ_MESSAGE_UNREGISTER_HOTKEY => {
            if let Err(e) = UnregisterHotKey(hwnd, wparam.0 as i32) {
                println!("Warning: UnregisterHotKey failed: {e:?}");
            }
        }
        _ => {}
    }
    DefWindowProcW(hwnd, msg, wparam, lparam)
}

fn start_dummy_window() -> std::thread::JoinHandle<Result<(), InjectorError>> {
    std::thread::spawn(|| unsafe {
        let instance = GetModuleHandleW(None).unwrap();

        let class_name = PCWSTR(
            "InjectorHiddenWindow"
                .encode_utf16()
                .collect::<Vec<u16>>()
                .as_ptr(),
        );
        let window_class = WNDCLASSW {
            style: CS_HREDRAW | CS_VREDRAW,
            lpfnWndProc: Some(dummy_wndproc),
            hInstance: instance.into(),
            lpszClassName: class_name,
            ..Default::default()
        };

        RegisterClassW(&window_class);

        let hwnd = CreateWindowExW(
            WINDOW_EX_STYLE(0),
            class_name,
            w!("injector"),
            WS_OVERLAPPEDWINDOW, //| WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            None,
            None,
            instance,
            None,
        );

        if hwnd == HWND(0) {
            return Err(InjectorError::OtherError(format!("CreateWindowExW failed")));
        }

        *DUMMY_WINDOW_HWND.lock().unwrap() = hwnd;

        // Normally you would show the window with ShowWindow, but we'll keep it hidden.
        // ShowWindow(hwnd, SW_SHOW);

        let mut msg = MSG::default();
        while GetMessageW(&mut msg, None, 0, 0) != FALSE {
            let _ = TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        println!("exiting");
        Ok(())
    })
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    start_dummy_window();
    let pool = LocalPoolHandle::new(2);
    let port = 7070;
    let addr: SocketAddr = ([127, 0, 0, 1], port).into();
    // Bind to the port and listen for incoming TCP connections
    let listener = TcpListener::bind(addr).await?;
    println!("Injector listening for HTTP at {:?}", addr);
    loop {
        let (stream, _) = listener.accept().await?;
        let io = TokioIo::new(stream);
        pool.spawn_pinned(|| {
            tokio::task::spawn_local(async move {
                match http1::Builder::new()
                    .serve_connection(io, service_fn(handle_request_wrapper))
                    .await
                {
                    Ok(_) => {}
                    Err(err) => println!("Error serving connection: {:?}", err),
                }
            })
        });
    }

    Ok(())
}
