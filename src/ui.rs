use std::sync::{Arc, RwLock};

pub type SharedState = Arc<RwLock<State>>;

pub struct State {
    pub ready_check_message: String,
    pub squad_ready_message: String,
}

impl State {
    pub fn new() -> Self {
        Self { ready_check_message: "".to_owned(), squad_ready_message: "".to_owned() }
    }
}