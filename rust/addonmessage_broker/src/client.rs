use tokio::net::TcpStream;

use crate::server::{BUF_SIZE, ConnectionId, Msg, MsgSender, MsgWrapper};

pub async fn start_addonmessage_client<
    SetConnectionIdCallback: FnOnce(ConnectionId),
    MsgHandler: Fn(MsgWrapper) + Send + 'static,
>(
    rx: std::sync::mpsc::Receiver<MsgWrapper>,
    name: String,
    set_connection_id: SetConnectionIdCallback,
    msg_handler: MsgHandler,
    addr: String,
) -> Result<(), std::io::Error> {
    let stream = TcpStream::connect(&addr).await?;

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
        tracing::info!("set connection id to {id}");
    }
    tokio::spawn(async move {
        let mut serialization_buffer = bitcode::Buffer::new();
        loop {
            match MsgWrapper::read(&mut read, &mut serialization_buffer, &mut read_buffer).await {
                Ok(msg) => {
                    msg_handler(msg);
                }
                Err(e) => {
                    tracing::error!("msg.read: {e}");
                }
            }
        }
    });

    loop {
        let msg = rx.recv().unwrap();
        tracing::debug!("relaying {msg} to socket");
        match msg.send(&mut write, &mut serialization_buffer).await {
            Ok(_) => {}
            Err(e) => {
                tracing::error!("msg.send: {e}")
            }
        }
    }
}
