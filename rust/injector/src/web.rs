use http::{Method, Request, Response, StatusCode};
use hyper::server::conn::http1;
use hyper::service::service_fn;

use tokio::net::TcpListener;
use tokio_util::task::LocalPoolHandle;

use http_body_util::BodyExt;
use std::{ffi::OsString, net::SocketAddr, os::windows::ffi::OsStrExt, path::PathBuf};

use crate::start_dummy_window;
use crate::tokio_io::TokioIo;

use crate::{read_potti_conf, ConfigResult, InjectQuery, InjectorError, LaunchQuery};
use serde_json::json;

use serde::{de::DeserializeOwned, Serialize};

use crate::inject::find_wow_windows_and_inject;

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

pub async fn handle_request_wrapper(req: Request<hyper::body::Incoming>) -> IHttpResult {
    match handle_request(req).await {
        Ok(r) => Ok(r),
        Err(InjectorError::NotFound) => not_found(),
        Err(InjectorError::DebugPrivilegesFailed(e)) => internal_server_error(format!(
            "{e:?} (are you running injector as an Administrator?)"
        )),
        Err(e) => internal_server_error(format!("{e:?}")),
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

pub async fn main() -> Result<(), Box<dyn std::error::Error>> {
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
