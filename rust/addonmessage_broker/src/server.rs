use std::sync::{mpsc, Arc, Mutex, OnceLock};
use std::time::Duration;
use tokio::io::{AsyncReadExt, AsyncWrite, AsyncWriteExt};
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};
use tokio::net::TcpListener;
use tracing_subscriber::layer::SubscriberExt;
use tracing_subscriber::util::SubscriberInitExt;

pub type ConnectionId = u32;

#[derive(Clone, Debug, bitcode::Encode, bitcode::Decode, PartialEq)]
pub enum MsgSender {
    Server,
    PeerWithoutConnectionId,
    Peer(ConnectionId, String),
}

pub const BUF_SIZE: usize = 1024 * 32;

#[derive(Debug)]
struct BrokerClient {
    id: ConnectionId,
    name: String,
    tx: mpsc::Sender<MsgWrapper>,
}

enum ServerMsg {
    RemoveClient(ConnectionId, String),
    RelayMsg(MsgWrapper),
}

impl std::fmt::Display for BrokerClient {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Connection {{ id: {} }}", self.id)
    }
}

#[derive(Debug, Clone, bitcode::Encode, bitcode::Decode)]
pub struct AddonMessage {
    pub prefix: String,
    pub text: String,
    pub r#type: Option<String>,
    pub target: Option<String>,
    pub from: String,
}

#[derive(Debug, Clone, bitcode::Encode, bitcode::Decode)]
pub enum Msg {
    Hello(String),
    Welcome(ConnectionId),
    String(String),
    AddonMessage(AddonMessage),
}

#[derive(Debug, Clone, bitcode::Encode, bitcode::Decode)]
pub struct MsgWrapper {
    pub from: MsgSender,
    pub message: Msg,
}

impl MsgWrapper {
    pub async fn read(
        read: &mut OwnedReadHalf,
        serialization_buffer: &mut bitcode::Buffer,
        read_buffer: &mut [u8],
    ) -> Result<Self, std::io::Error> {
        let mut packet_size_buf = [0u8; std::mem::size_of::<usize>()];
        read.read_exact(&mut packet_size_buf).await?;
        let packet_size = usize::from_le_bytes(packet_size_buf.try_into().unwrap());

        read.read_exact(&mut read_buffer[..packet_size]).await?;
        let msg = serialization_buffer
            .decode::<MsgWrapper>(&read_buffer[..packet_size])
            .map_err(|_e| {
                std::io::Error::new(
                    std::io::ErrorKind::InvalidData,
                    format!("bitcode::Decode failed: {_e:?}"),
                )
            })?;

        Ok(msg)
    }

    pub async fn send(
        self,
        write: &mut OwnedWriteHalf,
        serialization_buffer: &mut bitcode::Buffer,
    ) -> Result<(), std::io::Error> {
        let encoded = serialization_buffer.encode(&self);
        write.write_all(&encoded.len().to_le_bytes()).await?;
        write.write_all(encoded).await?;
        Ok(())
    }
}

impl std::fmt::Display for MsgWrapper {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Message {{ from: {:?}, message: {:?} }}",
            self.from, self.message
        )
    }
}

#[derive(Clone)]
struct Clients {
    connections: Arc<Mutex<Vec<BrokerClient>>>,
    client_id_counter: Arc<Mutex<ConnectionId>>,
    main_tx: std::sync::mpsc::Sender<ServerMsg>,
}

impl Clients {
    fn get_next_client_id(&mut self) -> ConnectionId {
        let mut client_id = self.client_id_counter.lock().unwrap();
        *client_id += 1;
        *client_id
    }
    fn spawn_socket_reader_task(
        &self,
        connection_id: ConnectionId,
        client_tx: std::sync::mpsc::Sender<MsgWrapper>,
        mut s_read: OwnedReadHalf,
    ) {
        // - listen to messages from socket
        // - send message to this client's `rx` for relaying
        let main_tx = self.main_tx.clone();
        let connections = self.connections.clone();
        tokio::spawn(async move {
            let mut serialization_buffer = bitcode::Buffer::new();
            let mut read_buffer = vec![0u8; BUF_SIZE];
            loop {
                let msg =
                    MsgWrapper::read(&mut s_read, &mut serialization_buffer, &mut read_buffer)
                        .await;
                if let Ok(msg) = msg {
                    if let (MsgSender::PeerWithoutConnectionId, Msg::Hello(name)) =
                        (&msg.from, &msg.message)
                    {
                        client_tx
                            .send(MsgWrapper {
                                from: MsgSender::Server,
                                message: Msg::Welcome(connection_id),
                            })
                            .unwrap();
                        let mut connections = connections.lock().unwrap();
                        connections.push(BrokerClient {
                            id: connection_id,
                            name: name.to_owned(),
                            tx: client_tx.clone(),
                        });
                    } else {
                        if let Err(e) = main_tx.send(ServerMsg::RelayMsg(msg.clone())) {
                            tracing::error!("connection {connection_id}: write: {e:?}");
                            main_tx
                                .send(ServerMsg::RemoveClient(
                                    connection_id,
                                    format!("mpsc::Sender::send failed: {e:?}"),
                                ))
                                .unwrap();
                            break;
                        }
                    }
                } else {
                    main_tx
                        .send(ServerMsg::RemoveClient(
                            connection_id,
                            format!("read_one_message failed: {msg:?}"),
                        ))
                        .unwrap();
                    break;
                }
            }
        });
    }
    fn spawn_message_forwarder_task(
        &self,
        connection_id: ConnectionId,
        rx: std::sync::mpsc::Receiver<MsgWrapper>,
        mut write: OwnedWriteHalf,
    )
    // - listen to messages from this client's `rx`
    // - relay message to client's socket
    {
        let main_tx = self.main_tx.clone();
        tokio::spawn(async move {
            let mut serialization_buffer = bitcode::Buffer::new();
            loop {
                match rx.recv() {
                    Ok(msg) => msg
                        .send(&mut write, &mut serialization_buffer)
                        .await
                        .unwrap(),
                    Err(e) => {
                        main_tx
                            .send(ServerMsg::RemoveClient(
                                connection_id,
                                format!("mpsc::Receiver::recv() failed: {e:?}"),
                            ))
                            .unwrap();
                        break;
                    }
                }
            }
        });
    }
}

pub async fn start_addonmessage_relay() {
    tracing::info!("listening on port 1337");
    let listener = TcpListener::bind("127.0.0.1:1337").await.unwrap();

    let (main_tx, main_rx) = mpsc::channel::<ServerMsg>(); // Channel for broadcasting messages
    let mut clients = Clients {
        connections: Arc::new(Mutex::new(Vec::new())),
        client_id_counter: Arc::new(Mutex::new(1)),
        main_tx: main_tx.clone(),
    };

    let cloned_clients = clients.clone();
    // Spawn a task to handle broadcasting messages to all connected clients
    let cloned_main_tx = main_tx.clone();
    tokio::spawn(async move {
        while let Ok(message) = main_rx.recv() {
            match message {
                ServerMsg::RelayMsg(message) => {
                    if let Msg::AddonMessage(ref msg) = message.message {
                        let mut clients = cloned_clients.connections.lock().unwrap();
                        match (msg.r#type.as_deref(), msg.target.as_deref()) {
                            (Some("WHISPER"), Some(name)) => {
                                if let Some(client) = clients.iter().find(|c| c.name == name) {
                                    client.tx.send(message.clone()).unwrap();
                                } else {
                                    tracing::warn!("warning: not forwarding message {message:?}: client '{name}' not found");
                                }
                            }
                            (Some("RAID"), _) | (Some("PARTY"), _) | (None, _) => {
                                for client in clients.iter_mut()
                                // .filter(|c| MsgSender::Peer(c.id) != message.from)
                                {
                                    if let Err(e) = client.tx.send(message.clone()) {
                                        cloned_main_tx
                                            .send(ServerMsg::RemoveClient(
                                                client.id,
                                                format!("mpsc::Sender::send failed: {e:?}"),
                                            ))
                                            .unwrap();
                                    }
                                }
                            }
                            _ => tracing::warn!("(warning: not forwarding message {message:?})"),
                        }
                    }
                }
                ServerMsg::RemoveClient(id, reason) => {
                    tracing::warn!("Removing client {id} for reason: {reason}");
                    let mut clients = cloned_clients.connections.lock().unwrap();
                    clients.retain(|c| c.id != id);
                }
            }
        }
    });

    let cloned_main_tx = main_tx.clone();

    tokio::spawn(async move {
        loop {
            tokio::time::sleep(Duration::from_millis(5000)).await;
            cloned_main_tx
                .send(ServerMsg::RelayMsg(MsgWrapper {
                    from: MsgSender::Server,
                    message: Msg::String(format!(
                        "The time is now {:?}",
                        std::time::Instant::now()
                    )),
                }))
                .unwrap();
        }
    });

    while let Ok((stream, _)) = listener.accept().await {
        let (s_read, s_write) = stream.into_split();
        let (client_tx, client_rx) = mpsc::channel::<MsgWrapper>(); // Channel for receiving messages from this client
        let connection_id = clients.get_next_client_id();

        clients.spawn_socket_reader_task(connection_id, client_tx.clone(), s_read);
        clients.spawn_message_forwarder_task(connection_id, client_rx, s_write);
    }
}
