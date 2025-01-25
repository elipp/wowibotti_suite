use std::{sync::OnceLock, time::Duration};

use addonmessage_broker::{
    client::start_addonmessage_client,
    server::{AddonMessage, ConnectionId, Msg, MsgSender, MsgWrapper},
};
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

static CONNECTION_ID: OnceLock<ConnectionId> = OnceLock::new();

const LIPSUM: &str = 
"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

#[tokio::main]
async fn main() -> Result<(), std::io::Error> {
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| tracing_subscriber::EnvFilter::new("info")),
        )
        .with(tracing_subscriber::fmt::layer().with_ansi(true))
        .init();


    let character_name = String::from("Pylly");
    let (tx, rx) = std::sync::mpsc::channel();
    start_addonmessage_client(
        rx,
        String::default(),
        |id| {
            CONNECTION_ID.get_or_init(|| id);

            tokio::spawn(async move {
                loop {
                    tx.send(MsgWrapper {
                        from: MsgSender::Peer(
                            *CONNECTION_ID.get().unwrap_or(&0),
                            character_name.clone(),
                        ),
                        message: Msg::AddonMessage(AddonMessage {
                            prefix: String::from("moi"),
                            text: String::from(LIPSUM),
                            r#type: Some(String::from("RAID")),
                            target: None,
                            from: character_name.clone(),
                        }),
                    })
                    .unwrap();
                    tokio::time::sleep(Duration::from_millis(5000)).await;
                }
            });
        },
        |m| tracing::info!("message {m:?}"),
    )
    .await
    .unwrap();
    Ok(())
}
