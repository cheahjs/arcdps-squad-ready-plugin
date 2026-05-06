use std::panic;

use log::error;

/// Installs a custom panic handler that logs panics instead of crashing.
/// Chains with any previously installed panic hook so other plugins' hooks
/// are not silently discarded.
pub fn install_panic_handler() {
    let previous_hook = panic::take_hook();
    panic::set_hook(Box::new(move |panic_info| {
        let payload = if let Some(s) = panic_info.payload().downcast_ref::<&str>() {
            s.to_string()
        } else if let Some(s) = panic_info.payload().downcast_ref::<String>() {
            s.clone()
        } else {
            "Unknown panic payload".to_string()
        };

        let location = if let Some(loc) = panic_info.location() {
            format!("{}:{}:{}", loc.file(), loc.line(), loc.column())
        } else {
            "unknown location".to_string()
        };

        error!("PANIC at {}: {}", location, payload);

        previous_hook(panic_info);
    }));
}
