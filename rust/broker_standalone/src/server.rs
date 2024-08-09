use broker::server::start_addonmessage_relay;

#[tokio::main]
async fn main() {
    start_addonmessage_relay().await
}
