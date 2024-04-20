use std::collections::HashSet;
use std::sync::{mpsc, Arc, Mutex};
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpListener;

type ConnectionId = u32;

pub const BUF_SIZE: usize = 4096;
pub const SERVER_CONNECTION_ID: ConnectionId = 0;
pub const UNINITIALIZED_CONNECTION_ID: ConnectionId = u32::MAX;

struct Connection {
    id: ConnectionId,
    sender: mpsc::Sender<MsgWrapper>,
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

#[tokio::main]
async fn main() {
    // Create a TCP listener bound to localhost on port 8080
    let listener = TcpListener::bind("127.0.0.1:8080").await.unwrap();
    println!("Server listening on port 8080");

    let (tx, rx) = mpsc::channel::<MsgWrapper>(); // Channel for broadcasting messages
    let clients: Arc<Mutex<Vec<Connection>>> = Arc::new(Mutex::new(Vec::new()));
    let client_id_counter: Arc<Mutex<ConnectionId>> = Arc::new(Mutex::new(1));

    let cloned_clients = clients.clone();
    // Spawn a task to handle broadcasting messages to all connected clients
    tokio::spawn(async move {
        while let Ok(message) = rx.recv() {
            let mut clients_to_remove = HashSet::new();
            let mut clients = cloned_clients.lock().unwrap();
            for client_tx in clients
                .iter_mut()
                .filter(|c| c.id != message.from_connection_id)
            {
                if let Err(_) = client_tx.sender.send(message.clone()) {
                    // Failed to send message to client, remove it from the set
                    println!("send failed: removing {}", message.from_connection_id);
                    clients_to_remove.insert(client_tx.id);
                }
            }
            clients.retain(|c| !clients_to_remove.contains(&c.id));
        }
    });

    while let Ok((stream, _)) = listener.accept().await {
        let (client_tx, client_rx) = mpsc::channel::<MsgWrapper>(); // Channel for receiving messages from this client
        let (mut s_read, mut s_write) = stream.into_split();
        let clients_clone = clients.clone();
        let client_id_clone = client_id_counter.clone();
        let tx_clone = tx.clone();
        let connection_id = {
            let mut client_id = client_id_clone.lock().unwrap();
            *client_id += 1;
            *client_id
        };

        let connection = Connection {
            id: connection_id,
            sender: client_tx,
        };

        tokio::spawn(async move {
            let mut buffer = bitcode::Buffer::new();
            loop {
                match client_rx.recv() {
                    Ok(msg) => {
                        println!("connection {}: received {msg}", connection_id);
                        match s_write.write_all(buffer.encode(&msg)).await {
                            Ok(_) => {}
                            Err(e) => panic!("{e:?}"),
                        }
                    }
                    Err(_) => panic!("e:?"),
                }
            }
        });

        tokio::spawn(async move {
            println!("Client {connection} connected");
            let connection_id = connection.id;
            clients_clone.lock().unwrap().push(connection);

            let mut buffer = bitcode::Buffer::new();
            let mut buf = vec![0; BUF_SIZE];
            loop {
                match s_read.read(&mut buf).await {
                    Ok(0) => {
                        println!("Client {connection_id} disconnected");
                        break;
                    }
                    Ok(bytes_read) => {
                        let msg = buffer
                            .decode::<MsgWrapper>(&buf[..bytes_read])
                            .expect("deserialization");
                        println!("got message {msg}");
                        if msg.from_connection_id == UNINITIALIZED_CONNECTION_ID {
                            if let Msg::Hello = msg.message {
                                tx_clone
                                    .send(MsgWrapper {
                                        from_connection_id: SERVER_CONNECTION_ID,
                                        message: Msg::Welcome(connection_id),
                                    })
                                    .expect("hello");
                            }
                        }
                        if let Err(_) = tx_clone.send(msg) {
                            // Failed to broadcast message, remove client's sender from the set
                            // clients_clone.lock().unwrap().retain(|c| c != &client_tx);
                            break;
                        }
                    }
                    Err(_) => {
                        // Error reading from client, remove client's sender from the set
                        // clients_clone.lock().unwrap().retain(|c| c != &client_tx);
                        break;
                    }
                }
            }
        });
    }
}
