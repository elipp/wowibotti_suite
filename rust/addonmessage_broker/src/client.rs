use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};
use tokio::net::{TcpListener, TcpStream};

use crate::server::{ConnectionId, Msg, MsgSender, MsgWrapper, BUF_SIZE};

pub async fn start_addonmessage_client<
    SetConnectionIdCallback: FnOnce(ConnectionId) -> (),
    MsgHandler: Fn(MsgWrapper) -> () + Send + 'static,
>(
    rx: std::sync::mpsc::Receiver<MsgWrapper>,
    name: String,
    set_connection_id: SetConnectionIdCallback,
    msg_handler: MsgHandler,
) -> Result<(), std::io::Error> {
    let addr = "127.0.0.1:1337";
    let stream = TcpStream::connect(addr).await?;

    let hello = MsgWrapper {
        from: MsgSender::PeerWithoutConnectionId,
        message: Msg::Hello(name),
    };

    let mut serialization_buffer = bitcode::Buffer::new();

    let (mut read, mut write) = stream.into_split();

    hello
        .send(&mut write, &mut serialization_buffer)
        .await
        .unwrap();

    let mut read_buffer = vec![0u8; BUF_SIZE];

    let msg = MsgWrapper::read(&mut read, &mut serialization_buffer, &mut read_buffer)
        .await
        .unwrap();

    if let (Msg::Welcome(id), MsgSender::Server) = (msg.message, msg.from) {
        set_connection_id(id);
        println!("addonmessage_broker: set connection id to {id}");
    }
    tokio::spawn(async move {
        let mut serialization_buffer = bitcode::Buffer::new();
        loop {
            let msg = MsgWrapper::read(&mut read, &mut serialization_buffer, &mut read_buffer)
                .await
                .unwrap();
            msg_handler(msg);
        }
    });

    loop {
        let msg = rx.recv().unwrap();
        println!("relaying {msg} to socket");
        msg.send(&mut write, &mut serialization_buffer)
            .await
            .unwrap();
    }
}
