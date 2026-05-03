#[cfg(feature = "addonmessage_broker")]
use std::{
    collections::VecDeque,
    sync::{Arc, Mutex, OnceLock},
};

#[cfg(feature = "addonmessage_broker")]
use addonmessage_broker::{
    client::start_addonmessage_client, server::AddonMessage, server::ConnectionId, server::Msg,
    server::MsgWrapper,
};

#[cfg(feature = "addonmessage_broker")]
use crate::dostring;

#[cfg(feature = "addonmessage_broker")]
pub fn unpack_broker_message_queue() {
    if let Some(state) = BROKER_STATE.get() {
        let mut queue = state.message_queue.lock().unwrap();
        for message in queue.drain(..) {
            tracing::debug!("{message:?}");
            let script = format!(
                "addonmessage_received([[{}]], [[{}]], {}, [[{}]])",
                message.prefix,
                message.text,
                message
                    .r#type
                    .map(|s| format!("[[{}]]", s))
                    .unwrap_or_else(|| "nil".to_string()),
                message.from,
            );
            dostring!(script);
        }
    }
}

#[cfg(feature = "addonmessage_broker")]
pub struct BrokerState {
    pub connection_id: ConnectionId,
    pub character_name: String,
    pub tx: std::sync::mpsc::Sender<MsgWrapper>,
    pub message_queue: Arc<Mutex<VecDeque<AddonMessage>>>,
}

#[cfg(feature = "addonmessage_broker")]
pub static BROKER_STATE: OnceLock<BrokerState> = OnceLock::new();

#[cfg(feature = "addonmessage_broker")]
pub fn start_client(character_name: String, addr: String) -> std::thread::JoinHandle<()> {
    let (tx, rx) = std::sync::mpsc::channel::<MsgWrapper>();
    let handle = std::thread::spawn(move || {
        let rt = tokio::runtime::Runtime::new().unwrap();
        rt.block_on(async {
            match start_addonmessage_client(
                rx,
                character_name.clone(),
                |connection_id| {
                    BROKER_STATE.get_or_init(|| BrokerState {
                        connection_id,
                        character_name,
                        tx,
                        message_queue: Default::default(),
                    });
                },
                |msg| {
                    if let (Msg::AddonMessage(msg), Some(state)) =
                        (msg.message, BROKER_STATE.get())
                    {
                        let mut queue = state.message_queue.lock().unwrap();
                        queue.push_back(msg.clone());
                    }
                },
                addr,
            )
            .await
            {
                Ok(_) => tracing::info!("Addonmessage client started"),
                Err(e) => tracing::error!("Couldn't connect to addonmessage server: {e:?}. Using regular SendAddonMessage API."),
            }
        });
    });
    handle
}
