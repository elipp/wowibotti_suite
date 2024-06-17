use std::{io::Read, io::Write, net::TcpStream};

mod server;
use server::{Msg, MsgWrapper, BUF_SIZE, UNINITIALIZED_CONNECTION_ID};

use crate::server::SERVER_CONNECTION_ID;

fn main() -> Result<(), std::io::Error> {
    let addr = "127.0.0.1:8080";
    let mut stream = TcpStream::connect(addr)?;
    let mut serialization_buffer = bitcode::Buffer::new();

    let mut connection_id = UNINITIALIZED_CONNECTION_ID;
    let hello = MsgWrapper {
        from_connection_id: connection_id,
        message: Msg::Hello,
    };

    stream
        .write_all(serialization_buffer.encode(&hello))
        .expect("hello");

    let mut read_buf = vec![0; BUF_SIZE];
    loop {
        if let Ok(bytes) = stream.read(&mut read_buf) {
            if bytes == 0 {
                println!("server disconnected lulz");
                break;
            } else {
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
                    }
                }
            }
        } else {
            break;
        }
    }

    Ok(())
}