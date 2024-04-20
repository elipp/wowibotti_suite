use http::{Method, Request, Response, StatusCode};
use hyper::server::conn::http1;
use hyper::service::service_fn;
use inject::{find_wow_windows_and_inject, InjectorError, InjectorResult};
use serde::{de::DeserializeOwned, Deserialize, Serialize};
use serde_json::json;
use std::{net::SocketAddr, os::windows::ffi::OsStrExt, path::PathBuf};
use tokio::net::TcpListener;
use tokio_util::task::LocalPoolHandle;
use windows::{
    core::PCWSTR,
    core::PWSTR,
    Win32::System::Threading::{
        CreateProcessW, PROCESS_CREATION_FLAGS, PROCESS_INFORMATION, STARTUPINFOW,
    },
};

use http_body_util::BodyExt;

mod inject;
mod tokio_io;

use crate::tokio_io::TokioIo;

use wowibottihookdll::{windows_string, CharacterInfo, WowAccount};

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

#[derive(Debug, Deserialize)]
struct PottiConfig {
    wow_client_path: PathBuf,
    accounts: Vec<WowAccount>,
}

#[derive(Debug, Serialize, Deserialize)]
struct CharacterList {
    characters: Vec<CharacterInfo>,
}

impl From<PottiConfig> for CharacterList {
    fn from(p: PottiConfig) -> Self {
        Self {
            characters: p.accounts.into_iter().map(|a| a.character).collect(),
        }
    }
}

#[derive(Debug, Deserialize)]
pub struct InjectQuery {
    enabled_characters: Vec<String>,
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

impl LaunchQuery {
    fn launch(self, config: PottiConfig) -> InjectorResult<()> {
        unsafe {
            let as_u16: Vec<u16> = config.wow_client_path.as_os_str().encode_wide().collect();
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
                    &mut process_information as *mut PROCESS_INFORMATION,
                )
                .map_err(|_e| InjectorError::LaunchError(format!("{_e:?}")))?;
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
                    "result": serde_json::to_value(CharacterList::from(config)).map_err(|_e|InjectorError::SerializationError(format!("{_e:?}")))?
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

#[tokio::main]
pub async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let pool = LocalPoolHandle::new(2);

    let port = 7070;
    let addr: SocketAddr = ([127, 0, 0, 1], port).into();
    println!("Injector listening for HTTP at {:?}", addr);

    // Bind to the port and listen for incoming TCP connections
    let listener = TcpListener::bind(addr).await?;
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
}
