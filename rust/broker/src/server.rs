use std::sync::{mpsc, Arc, Mutex};
use std::time::Duration;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};
use tokio::net::TcpListener;

type ConnectionId = u32;

pub const BUF_SIZE: usize = 4096;
pub const SERVER_CONNECTION_ID: ConnectionId = 0;
pub const UNINITIALIZED_CONNECTION_ID: ConnectionId = u32::MAX;

struct Connection {
    id: ConnectionId,
    tx: mpsc::Sender<MsgWrapper>,
}

enum MsgType {
    RemoveClient(ConnectionId),
    RelayMsg(MsgWrapper),
}

impl std::fmt::Display for Connection {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Connection {{ id: {} }}", self.id)
    }
}

#[derive(Debug, Clone, bitcode::Encode, bitcode::Decode)]
pub enum Msg {
    Hello,
    Welcome(ConnectionId),
    String(String),
    AddonMessage(String, String, String),
}

#[derive(Debug, Clone, bitcode::Encode, bitcode::Decode)]
pub struct MsgWrapper {
    pub from_connection_id: ConnectionId,
    pub message: Msg,
}

impl std::fmt::Display for MsgWrapper {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "Message {{ from: {}, message: {:?} }}",
            self.from_connection_id, self.message
        )
    }
}

#[derive(Clone)]
struct Clients {
    connections: Arc<Mutex<Vec<Connection>>>,
    client_id_counter: Arc<Mutex<ConnectionId>>,
    main_tx: std::sync::mpsc::Sender<MsgType>,
}

impl Clients {
    fn push(&mut self, connection: Connection) {
        self.connections.lock().unwrap().push(connection);
    }
    fn remove_by_id(&mut self, connection_id: ConnectionId) {
        self.connections
            .lock()
            .unwrap()
            .retain(|c| c.id != connection_id);
    }
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
        tokio::spawn(async move {
            let mut buffer = bitcode::Buffer::new();
            let mut buf = vec![0; BUF_SIZE];
            loop {
                match s_read.read(&mut buf).await {
                    Ok(bytes_read @ 1..) => {
                        let msg = buffer.decode::<MsgWrapper>(&buf[..bytes_read]).unwrap();
                        println!("got message {msg}");
                        if msg.from_connection_id == UNINITIALIZED_CONNECTION_ID
                            && matches!(msg.message, Msg::Hello)
                        {
                            client_tx
                                .send(MsgWrapper {
                                    from_connection_id: SERVER_CONNECTION_ID,
                                    message: Msg::Welcome(connection_id),
                                })
                                .unwrap();
                        } else {
                            if let Err(e) = main_tx.send(MsgType::RelayMsg(msg)) {
                                eprintln!("connection {connection_id}: write: {e:?}");
                                main_tx.send(MsgType::RemoveClient(connection_id)).unwrap();
                                break;
                            }
                        }
                    }
                    Ok(0) | Err(_) => {
                        eprintln!("connection {connection_id}: error or read of 0, removing");
                        main_tx.send(MsgType::RemoveClient(connection_id)).unwrap();
                        break;
                    }
                }
            }
        });
    }
    fn spawn_message_forwarder_task(
        &self,
        connection_id: ConnectionId,
        rx: std::sync::mpsc::Receiver<MsgWrapper>,
        mut socket_writer: OwnedWriteHalf,
    )
    // - listen to messages from this client's `rx`
    // - relay message to client's socket
    {
        let main_tx = self.main_tx.clone();
        tokio::spawn(async move {
            let mut buffer = bitcode::Buffer::new();
            loop {
                match rx.recv() {
                    Ok(msg) => socket_writer.write_all(buffer.encode(&msg)).await.unwrap(),
                    Err(_e) => {
                        main_tx.send(MsgType::RemoveClient(connection_id)).unwrap();
                        break;
                    }
                }
            }
        });
    }
}

pub async fn start_addonmessage_relay() {
    println!("Broker listening on port 1337");
    let listener = TcpListener::bind("127.0.0.1:1337").await.unwrap();

    let (main_tx, main_rx) = mpsc::channel::<MsgType>(); // Channel for broadcasting messages
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
                MsgType::RemoveClient(id) => {
                    eprintln!("Removing client {id}");
                    let mut clients = cloned_clients.connections.lock().unwrap();
                    clients.retain(|c| c.id != id);
                }
                MsgType::RelayMsg(message) => {
                    let mut clients = cloned_clients.connections.lock().unwrap();
                    for client in clients
                        .iter_mut()
                        .filter(|c| c.id != message.from_connection_id)
                    {
                        if let Err(_) = client.tx.send(message.clone()) {
                            cloned_main_tx
                                .send(MsgType::RemoveClient(client.id))
                                .unwrap();
                        }
                    }
                }
            }
        }
    });

    let cloned_main_tx = main_tx.clone();

    tokio::spawn(async move {
        loop {
            tokio::time::sleep(Duration::from_millis(5000)).await;
            cloned_main_tx
                .send(MsgType::RelayMsg(MsgWrapper {
                    from_connection_id: SERVER_CONNECTION_ID,
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

        let connection = Connection {
            id: connection_id,
            tx: client_tx.clone(),
        };

        println!("Client {connection} connected");
        clients.push(connection);

        clients.spawn_socket_reader_task(connection_id, client_tx.clone(), s_read);
        clients.spawn_message_forwarder_task(connection_id, client_rx, s_write);
    }
}

#[tokio::main]
async fn main() {
    start_addonmessage_relay().await
}
