use std::time::Duration;

use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};
use tokio::net::{TcpListener, TcpStream};

mod server;
use server::{Msg, MsgWrapper, BUF_SIZE, UNINITIALIZED_CONNECTION_ID};

use crate::server::SERVER_CONNECTION_ID;

#[tokio::main]
async fn main() -> Result<(), std::io::Error> {
    let addr = "127.0.0.1:8080";
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
        .expect("hello");

    let (mut read, mut write) = stream.into_split();
    let mut read_buf = vec![0; BUF_SIZE];
    match read.read(&mut read_buf).await {
        Ok(bytes @ 1usize..) => {
            let msg = serialization_buffer
                .decode::<MsgWrapper>(&read_buf[..bytes])
                .expect("valid msg");
            println!("received {msg:?}");
            if connection_id == UNINITIALIZED_CONNECTION_ID
                && msg.from_connection_id == SERVER_CONNECTION_ID
            {
                if let Msg::Welcome(id) = msg.message {
                    connection_id = id;
                    println!("set connection id to {connection_id}");

                    tokio::spawn(async move {
                        let mut buffer = bitcode::Buffer::new();
                        loop {
                            let msg = buffer.encode(&MsgWrapper {
                                from_connection_id: id,
                                message: Msg::String(format!("pieruxd")),
                            });

                            write.write_all(msg).await.unwrap();
                            tokio::time::sleep(Duration::from_millis(5000)).await;
                        }
                    });
                } else {
                    panic!("expected to receive Msg::Welcome as first msg");
                }
            }
        }
        e => {
            panic!("{e:?}");
        }
    }
    loop {
        match read.read(&mut read_buf).await {
            Ok(bytes @ 1usize..) => {
                let msg = serialization_buffer
                    .decode::<MsgWrapper>(&read_buf[..bytes])
                    .expect("valid msg");
                println!("received {msg:?}");
            }
            _ => panic!("lolz"),
        }
    }

    Ok(())
}
