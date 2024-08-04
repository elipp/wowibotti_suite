use std::future::Future;
use std::time::Duration;

use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};

use crate::server::{Msg, MsgWrapper, BUF_SIZE, SERVER_CONNECTION_ID, UNINITIALIZED_CONNECTION_ID};

pub async fn start_addonmessage_client(
    rx: std::sync::mpsc::Receiver<MsgWrapper>,
) -> Result<(), std::io::Error> {
    let addr = "127.0.0.1:1337";
    let mut stream = TcpStream::connect(addr).await?;
    let mut serialization_buffer = bitcode::Buffer::new();

    let mut connection_id = UNINITIALIZED_CONNECTION_ID;
    let hello = MsgWrapper {
        from_connection_id: connection_id,
        message: Msg::Hello,
    };

    stream
        .write_all(serialization_buffer.encode(&hello))
        .await
        .unwrap();

    let (mut read, mut write) = stream.into_split();
    let mut read_buf = vec![0; BUF_SIZE];
    match read.read(&mut read_buf).await {
        Ok(bytes @ 1..) => {
            eprintln!("read {bytes} bytes");
            let msg = serialization_buffer
                .decode::<MsgWrapper>(&read_buf[..bytes])
                .unwrap();
            println!("received {msg:?}");
            if connection_id == UNINITIALIZED_CONNECTION_ID
                && msg.from_connection_id == SERVER_CONNECTION_ID
            {
                if let Msg::Welcome(id) = msg.message {
                    connection_id = id;
                    println!("set connection id to {connection_id}");
                } else {
                    panic!("expected to receive Msg::Welcome as first msg");
                }
            }
        }
        e => {
            panic!("{e:?}");
        }
    }
    tokio::spawn(async move {
        loop {
            match read.read(&mut read_buf).await {
                Ok(bytes @ 1usize..) => {
                    eprintln!("read {bytes} bytes");
                    let msg = serialization_buffer
                        .decode::<MsgWrapper>(&read_buf[..bytes])
                        .unwrap();
                    println!("received {msg:?}");
                }
                _ => panic!("socket read error"),
            }
        }
    });
    loop {
        let msg = rx.recv().unwrap();
        println!("got msg {msg}");
    }
}

#[tokio::main]
async fn main() -> Result<(), std::io::Error> {
    let (tx, rx) = std::sync::mpsc::channel();
    start_addonmessage_client(rx).await
}
