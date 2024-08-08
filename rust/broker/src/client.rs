use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::{TcpListener, TcpStream};

use crate::server::{ConnectionId, Msg, MsgSender, MsgWrapper, BUF_SIZE};

pub async fn start_addonmessage_client<
    SetConnectionIdCallback: FnOnce(ConnectionId) -> (),
    MsgHandler: Fn(MsgWrapper) -> () + Send + 'static,
>(
    rx: std::sync::mpsc::Receiver<MsgWrapper>,
    set_connection_id: SetConnectionIdCallback,
    msg_handler: MsgHandler,
) -> Result<(), std::io::Error> {
    let addr = "127.0.0.1:1337";
    let mut stream = TcpStream::connect(addr).await?;

    let hello = MsgWrapper {
        from: MsgSender::PeerWithoutConnectionId,
        message: Msg::Hello,
    };

    let mut serialization_buffer = bitcode::Buffer::new();

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
            if let (Msg::Welcome(id), MsgSender::Server) = (msg.message, msg.from) {
                set_connection_id(id);
                println!("set connection id to {id}");
            }
        }
        e => {
            panic!("{e:?}");
        }
    }
    tokio::spawn(async move {
        let mut serialization_buffer = bitcode::Buffer::new();
        loop {
            match read.read(&mut read_buf).await {
                Ok(bytes @ 1usize..) => {
                    eprintln!("read {bytes} bytes");
                    let msg = serialization_buffer
                        .decode::<MsgWrapper>(&read_buf[..bytes])
                        .unwrap();
                    println!("received {msg:?}");
                    msg_handler(msg);
                }
                _ => panic!("socket read error"),
            }
        }
    });
    loop {
        let msg = rx.recv().unwrap();
        println!("relaying {msg} to socket");
        write
            .write_all(serialization_buffer.encode(&msg))
            .await
            .unwrap();
    }
}

#[tokio::main]
async fn main() -> Result<(), std::io::Error> {
    let (tx, rx) = std::sync::mpsc::channel();
    start_addonmessage_client(rx, |_| {}, |_| {}).await
}
