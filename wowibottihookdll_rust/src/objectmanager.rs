use crate::Addr;

pub struct WowObject {
    base: Addr,
}

impl WowObject {
    fn valid(&self) -> bool {
        self.base != 0 && (self.base & 0x3) == 0
    }
    fn get_next(&self) -> Option<WowObject> {
        None
    }
}

pub struct WowObjectIterator(WowObject);

impl Iterator for WowObjectIterator {
    type Item = WowObject;

    fn next(&mut self) -> Option<Self::Item> {
        if !self.0.valid() {
            return None;
        } else {
            self.0.get_next()
        }
    }
}

impl IntoIterator for ObjectManager {
    type Item = WowObject;

    type IntoIter = WowObjectIterator;

    fn into_iter(self) -> Self::IntoIter {
        WowObjectIterator(self.get_first_object().unwrap_or(WowObject { base: 0 }))
    }
}

pub struct ObjectManager {
    base: Addr,
}

impl ObjectManager {
    fn get_first_object(&self) -> Option<WowObject> {
        None
    }
}
