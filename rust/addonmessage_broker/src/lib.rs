pub mod client;
pub mod server;

pub struct SendSyncWrapper<T>(pub T);
unsafe impl<T> Send for SendSyncWrapper<T> {}
unsafe impl<T> Sync for SendSyncWrapper<T> {}
