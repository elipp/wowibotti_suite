use std::collections::HashSet;
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
    fn spawn_message_forwarder_task(
        &self,
        connection_id: ConnectionId,
        rx: std::sync::mpsc::Receiver<MsgWrapper>,
        mut write: OwnedWriteHalf,
    )
    // - listen to messages from this client's `rx`
    // - relay message to client's socket
    {
        tokio::spawn(async move {
            let mut buffer = bitcode::Buffer::new();
            loop {
                match rx.recv() {
                    Ok(msg) => {
                        if msg.from_connection_id != SERVER_CONNECTION_ID
                            && msg.from_connection_id != connection_id
                        {
                            println!("connection {}: received peer {msg}", connection_id);
                        }
                        match write.write_all(buffer.encode(&msg)).await {
                            Ok(_) => {}
                            Err(e) => panic!("{e:?}"),
                        }
                    }
                    Err(e) => panic!("{e:?}"),
                }
            }
        });
    }

    fn spawn_socket_reader_task(&self, connection: Connection, mut s_read: OwnedReadHalf) {
        // - listen to messages from socket
        // - send message to this client's `rx` for relaying
        let mut self_cloned = self.clone();
        tokio::spawn(async move {
            println!("Client {connection} connected");
            let connection_id = connection.id;
            let tx = connection.tx.clone();
            self_cloned.push(connection);

            let mut buffer = bitcode::Buffer::new();
            let mut buf = vec![0; BUF_SIZE];
            loop {
                match s_read.read(&mut buf).await {
                    Ok(bytes_read @ 1..) => {
                        let msg = buffer
                            .decode::<MsgWrapper>(&buf[..bytes_read])
                            .expect("successful deserialization");
                        println!("got message {msg}");
                        if msg.from_connection_id == UNINITIALIZED_CONNECTION_ID {
                            if let Msg::Hello = msg.message {
                                tx.send(MsgWrapper {
                                    from_connection_id: SERVER_CONNECTION_ID,
                                    message: Msg::Welcome(connection_id),
                                })
                                .expect("hello");
                            }
                        }
                        if let Err(e) = tx.send(msg) {
                            eprintln!("connection {connection_id}: write: {e:?}");
                            self_cloned.remove_by_id(connection_id);
                            break;
                        }
                    }
                    Ok(0) | Err(_) => {
                        eprintln!("connection {connection_id}: read of 0 (or error), removing");
                        self_cloned.remove_by_id(connection_id);
                        break;
                    }
                }
            }
        });
    }
}

#[tokio::main]
async fn main() {
    // Create a TCP listener bound to localhost on port 8080
    let listener = TcpListener::bind("127.0.0.1:8080").await.unwrap();
    println!("Server listening on port 8080");

    let (tx, rx) = mpsc::channel::<MsgWrapper>(); // Channel for broadcasting messages
    let mut clients = Clients {
        connections: Arc::new(Mutex::new(Vec::new())),
        client_id_counter: Arc::new(Mutex::new(1)),
    };

    let cloned_clients = clients.clone();
    // Spawn a task to handle broadcasting messages to all connected clients
    tokio::spawn(async move {
        while let Ok(message) = rx.recv() {
            let mut clients_to_remove = HashSet::new();
            let mut clients = cloned_clients.connections.lock().unwrap();
            for client in clients
                .iter_mut()
                .filter(|c| c.id != message.from_connection_id)
            {
                if let Err(_) = client.tx.send(message.clone()) {
                    // Failed to send message to client, remove it from the set
                    println!("send failed: removing {}", message.from_connection_id);
                    clients_to_remove.insert(client.id);
                }
            }
            clients.retain(|c| !clients_to_remove.contains(&c.id));
        }
    });

    let tx_cloned = tx.clone();

    tokio::spawn(async move {
        loop {
            tokio::time::sleep(Duration::from_millis(5000)).await;
            tx_cloned
                .send(MsgWrapper {
                    from_connection_id: SERVER_CONNECTION_ID,
                    message: Msg::String(format!(
                        "The time is now {:?}",
                        std::time::Instant::now()
                    )),
                })
                .unwrap();
        }
    });

    while let Ok((stream, _)) = listener.accept().await {
        let (s_read, s_write) = stream.into_split();
        let (client_tx, client_rx) = mpsc::channel::<MsgWrapper>(); // Channel for receiving messages from this client
        let connection_id = clients.get_next_client_id();

        let connection = Connection {
            id: connection_id,
            tx: client_tx,
        };

        clients.spawn_socket_reader_task(connection, s_read);
        clients.spawn_message_forwarder_task(connection_id, client_rx, s_write);
    }
}
