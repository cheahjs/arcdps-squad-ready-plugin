use arcdps::imgui::{Condition, StyleColor, Ui, Window};
use log::*;
use serde::Deserialize;
use std::fs;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};
use windows::Win32::System::LibraryLoader::GetModuleFileNameW;

const GITHUB_REPO: &str = "cheahjs/arcdps-squad-ready-plugin";
const RELEASES_URL: &str = "https://github.com/cheahjs/arcdps-squad-ready-plugin/releases/latest";

/// Version represented as [major, minor, patch, build]
pub type Version = [u16; 4];

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum UpdateStatus {
    /// Initial state, checking for updates
    Unknown,
    /// Update is available
    UpdateAvailable,
    /// User dismissed the update window
    Dismissed,
    /// Update download in progress
    UpdateInProgress,
    /// Update completed successfully
    UpdateSuccessful,
    /// Update failed
    UpdateError,
}

#[derive(Debug)]
pub struct UpdateState {
    /// Current installed version
    pub current_version: Option<Version>,
    /// Path to the DLL file
    pub install_path: PathBuf,
    /// Current status
    status: Arc<Mutex<UpdateStatus>>,
    /// New version available (set when status becomes UpdateAvailable)
    pub new_version: Arc<Mutex<Option<Version>>>,
    /// Download URL for the new version
    pub download_url: Arc<Mutex<Option<String>>>,
    /// Error message if update failed
    pub error_message: Arc<Mutex<Option<String>>>,
    /// Background tasks
    tasks: Vec<JoinHandle<()>>,
}

impl UpdateState {
    pub fn new(current_version: Option<Version>, install_path: PathBuf) -> Self {
        Self {
            current_version,
            install_path,
            status: Arc::new(Mutex::new(UpdateStatus::Unknown)),
            new_version: Arc::new(Mutex::new(None)),
            download_url: Arc::new(Mutex::new(None)),
            error_message: Arc::new(Mutex::new(None)),
            tasks: Vec::new(),
        }
    }

    pub fn status(&self) -> UpdateStatus {
        *self.status.lock().unwrap()
    }

    pub fn set_status(&self, status: UpdateStatus) {
        *self.status.lock().unwrap() = status;
    }

    /// Wait for all pending tasks to complete
    pub fn finish_pending_tasks(&mut self) {
        for task in self.tasks.drain(..) {
            if let Err(e) = task.join() {
                error!("Error joining update task: {:?}", e);
            }
        }
    }
}

/// Get the current version from Cargo.toml (set at compile time)
pub fn get_current_version() -> Version {
    parse_version(env!("CARGO_PKG_VERSION")).unwrap_or([0, 0, 0, 0])
}

/// Get the path to the current DLL
pub fn get_dll_path() -> Option<PathBuf> {
    use windows::Win32::Foundation::HMODULE;
    use windows::Win32::System::LibraryLoader::{
        GetModuleHandleExW, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    };

    unsafe {
        // Get the module handle for the DLL by using the address of a function in this module
        let mut module: HMODULE = HMODULE::default();
        let self_addr = get_dll_path as *const ();

        if GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            windows::core::PCWSTR::from_raw(self_addr as *const u16),
            &mut module,
        )
        .is_err()
        {
            error!("GetModuleHandleExW failed");
            return None;
        }

        let mut path_buffer = [0u16; 260];
        let path_len = GetModuleFileNameW(Some(module), &mut path_buffer);
        if path_len == 0 {
            error!("GetModuleFileNameW failed");
            return None;
        }

        let path_str = String::from_utf16_lossy(&path_buffer[..path_len as usize]);
        Some(PathBuf::from(path_str))
    }
}

/// Clear old update files (.tmp and .old)
pub fn clear_old_files(dll_path: &Path) {
    let tmp_path = dll_path.with_extension("dll.tmp");
    let old_path = dll_path.with_extension("dll.old");

    if let Err(e) = fs::remove_file(&tmp_path) {
        if e.kind() != std::io::ErrorKind::NotFound {
            warn!("Failed to remove {}: {}", tmp_path.display(), e);
        }
    }

    if let Err(e) = fs::remove_file(&old_path) {
        if e.kind() != std::io::ErrorKind::NotFound {
            warn!("Failed to remove {}: {}", old_path.display(), e);
        }
    }
}

/// Compare versions, returns true if repo_version is newer than current_version
pub fn is_newer(repo_version: &Version, current_version: &Version) -> bool {
    // Compare major.minor.patch (ignore build)
    (current_version[0], current_version[1], current_version[2])
        < (repo_version[0], repo_version[1], repo_version[2])
}

/// Format version as string
pub fn version_to_string(version: &Version) -> String {
    format!("{}.{}.{}", version[0], version[1], version[2])
}

/// Parse version from a tag name (e.g., "v1.2.3" or "1.2.3")
pub fn parse_version(tag: &str) -> Option<Version> {
    // Remove leading 'v' if present
    let tag = tag.strip_prefix('v').unwrap_or(tag);

    let parts: Vec<&str> = tag.split('.').collect();
    if parts.len() < 3 {
        return None;
    }

    // Parse each part, stripping non-digit suffixes
    let parse_part = |s: &str| -> Option<u16> {
        let digits: String = s.chars().take_while(|c| c.is_ascii_digit()).collect();
        digits.parse().ok()
    };

    Some([
        parse_part(parts[0])?,
        parse_part(parts[1])?,
        parse_part(parts[2])?,
        0,
    ])
}

/// GitHub release asset
#[derive(Debug, Deserialize)]
struct GitHubAsset {
    name: String,
    browser_download_url: String,
}

/// GitHub release response
#[derive(Debug, Deserialize)]
struct GitHubRelease {
    tag_name: String,
    assets: Vec<GitHubAsset>,
    #[serde(default)]
    #[allow(dead_code)]
    prerelease: bool,
}

/// Check for updates from GitHub releases
pub fn check_for_update(state: &mut UpdateState, include_prereleases: bool) {
    let status = Arc::clone(&state.status);
    let new_version = Arc::clone(&state.new_version);
    let download_url = Arc::clone(&state.download_url);
    let current_version = state.current_version;

    let handle = thread::spawn(move || {
        // Use different endpoint based on whether we want prereleases
        let url = if include_prereleases {
            format!("https://api.github.com/repos/{}/releases", GITHUB_REPO)
        } else {
            format!(
                "https://api.github.com/repos/{}/releases/latest",
                GITHUB_REPO
            )
        };

        info!("Checking for updates at {}", url);

        let response = match minreq::get(&url)
            .with_header("User-Agent", "arcdps-squad-ready-plugin")
            .with_header("Accept", "application/vnd.github.v3+json")
            .send()
        {
            Ok(resp) => resp,
            Err(e) => {
                error!("Failed to fetch releases: {}", e);
                return;
            }
        };

        // Parse the release(s) - /releases returns array, /releases/latest returns single object
        let release: GitHubRelease = if include_prereleases {
            let releases: Vec<GitHubRelease> =
                match serde_json::from_str(response.as_str().unwrap_or("")) {
                    Ok(r) => r,
                    Err(e) => {
                        error!("Failed to parse releases JSON: {}", e);
                        return;
                    }
                };

            // Get the first (most recent) release that has a .dll asset
            match releases
                .into_iter()
                .find(|r| r.assets.iter().any(|a| a.name.ends_with(".dll")))
            {
                Some(r) => r,
                None => {
                    error!("No releases with .dll assets found");
                    return;
                }
            }
        } else {
            match serde_json::from_str(response.as_str().unwrap_or("")) {
                Ok(r) => r,
                Err(e) => {
                    error!("Failed to parse release JSON: {}", e);
                    return;
                }
            }
        };

        let release_version = match parse_version(&release.tag_name) {
            Some(v) => v,
            None => {
                error!("Failed to parse version from tag: {}", release.tag_name);
                return;
            }
        };

        // Check if update is needed
        if let Some(current) = current_version {
            if !is_newer(&release_version, &current) {
                info!(
                    "Current version {} is up to date (latest: {})",
                    version_to_string(&current),
                    version_to_string(&release_version)
                );
                return;
            }
        }

        // Find .dll asset
        let dll_url = release
            .assets
            .iter()
            .find(|a| a.name.ends_with(".dll"))
            .map(|a| a.browser_download_url.clone());

        let dll_url = match dll_url {
            Some(url) => url,
            None => {
                error!("No .dll asset found in release");
                return;
            }
        };

        info!(
            "Update available: {} -> {} ({})",
            current_version.map_or("unknown".to_string(), |v| version_to_string(&v)),
            version_to_string(&release_version),
            dll_url
        );

        // Update state
        *new_version.lock().unwrap() = Some(release_version);
        *download_url.lock().unwrap() = Some(dll_url);
        *status.lock().unwrap() = UpdateStatus::UpdateAvailable;
    });

    state.tasks.push(handle);
}

/// Perform the update download and file replacement
pub fn perform_update(state: &mut UpdateState) {
    if state.status() != UpdateStatus::UpdateAvailable {
        warn!("Cannot perform update: status is {:?}", state.status());
        return;
    }

    state.set_status(UpdateStatus::UpdateInProgress);

    let install_path = state.install_path.clone();
    let download_url = state.download_url.lock().unwrap().clone();
    let status = Arc::clone(&state.status);
    let error_message = Arc::clone(&state.error_message);

    let Some(url) = download_url else {
        error!("No download URL available");
        *status.lock().unwrap() = UpdateStatus::UpdateError;
        *error_message.lock().unwrap() = Some("No download URL available".to_string());
        return;
    };

    let handle = thread::spawn(move || {
        let tmp_path = install_path.with_extension("dll.tmp");
        let old_path = install_path.with_extension("dll.old");

        info!("Downloading update from {}", url);

        // Download to temp file
        let response = match minreq::get(&url)
            .with_header("User-Agent", "arcdps-squad-ready-plugin")
            .send()
        {
            Ok(resp) => resp,
            Err(e) => {
                error!("Failed to download update: {}", e);
                *status.lock().unwrap() = UpdateStatus::UpdateError;
                *error_message.lock().unwrap() = Some(format!("Download failed: {}", e));
                return;
            }
        };

        // Write to temp file
        let mut file = match fs::File::create(&tmp_path) {
            Ok(f) => f,
            Err(e) => {
                error!("Failed to create temp file: {}", e);
                *status.lock().unwrap() = UpdateStatus::UpdateError;
                *error_message.lock().unwrap() = Some(format!("Failed to create temp file: {}", e));
                return;
            }
        };

        if let Err(e) = file.write_all(response.as_bytes()) {
            error!("Failed to write to temp file: {}", e);
            *status.lock().unwrap() = UpdateStatus::UpdateError;
            *error_message.lock().unwrap() = Some(format!("Failed to write temp file: {}", e));
            return;
        }

        drop(file);

        // Rename current to .old
        if let Err(e) = fs::rename(&install_path, &old_path) {
            error!(
                "Failed to rename {} to {}: {}",
                install_path.display(),
                old_path.display(),
                e
            );
            *status.lock().unwrap() = UpdateStatus::UpdateError;
            *error_message.lock().unwrap() = Some(format!("Failed to backup current file: {}", e));
            return;
        }

        // Rename .tmp to current
        if let Err(e) = fs::rename(&tmp_path, &install_path) {
            error!(
                "Failed to rename {} to {}: {}",
                tmp_path.display(),
                install_path.display(),
                e
            );
            // Try to restore the old file
            let _ = fs::rename(&old_path, &install_path);
            *status.lock().unwrap() = UpdateStatus::UpdateError;
            *error_message.lock().unwrap() = Some(format!("Failed to install new file: {}", e));
            return;
        }

        info!("Update completed successfully");
        *status.lock().unwrap() = UpdateStatus::UpdateSuccessful;
    });

    state.tasks.push(handle);
}

/// Draw the update window UI
pub fn draw_update_window(ui: &Ui, state: &mut UpdateState) {
    let status = state.status();

    // Only show window for certain statuses
    if status == UpdateStatus::Unknown || status == UpdateStatus::Dismissed {
        return;
    }

    let mut open = true;

    Window::new("Squad Ready Update")
        .opened(&mut open)
        .collapsible(false)
        .always_auto_resize(true)
        .size([350.0, 0.0], Condition::FirstUseEver)
        .build(ui, || {
            // Show version info
            if let Some(current) = state.current_version {
                let _red = ui.push_style_color(StyleColor::Text, [1.0, 0.3, 0.3, 1.0]);
                ui.text("A new version of Squad Ready is available!");
                ui.text(format!("Current version: {}", version_to_string(&current)));
            }

            if let Some(new_ver) = *state.new_version.lock().unwrap() {
                let _green = ui.push_style_color(StyleColor::Text, [0.3, 1.0, 0.3, 1.0]);
                ui.text(format!("New version: {}", version_to_string(&new_ver)));
            }

            ui.separator();

            // Open releases page button
            if ui.button("Open Releases Page") {
                let _ = std::process::Command::new("cmd")
                    .args(["/C", "start", RELEASES_URL])
                    .spawn();
            }

            // Status-specific UI
            match status {
                UpdateStatus::UpdateAvailable => {
                    ui.same_line();
                    if ui.button("Auto Update") {
                        perform_update(state);
                    }
                }
                UpdateStatus::UpdateInProgress => {
                    ui.text("Downloading update...");
                }
                UpdateStatus::UpdateSuccessful => {
                    let _green = ui.push_style_color(StyleColor::Text, [0.3, 1.0, 0.3, 1.0]);
                    ui.text("Update successful! Restart the game to apply.");
                }
                UpdateStatus::UpdateError => {
                    let _red = ui.push_style_color(StyleColor::Text, [1.0, 0.3, 0.3, 1.0]);
                    if let Some(ref msg) = *state.error_message.lock().unwrap() {
                        ui.text(format!("Update failed: {}", msg));
                    } else {
                        ui.text("Update failed!");
                    }
                }
                _ => {}
            }
        });

    // Handle window close
    if !open {
        state.set_status(UpdateStatus::Dismissed);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_version() {
        assert_eq!(parse_version("1.2.3"), Some([1, 2, 3, 0]));
        assert_eq!(parse_version("v1.2.3"), Some([1, 2, 3, 0]));
        assert_eq!(parse_version("v10.20.30"), Some([10, 20, 30, 0]));
        assert_eq!(parse_version("1.2"), None);
        assert_eq!(parse_version("1.2.3-beta"), Some([1, 2, 3, 0]));
    }

    #[test]
    fn test_is_newer() {
        assert!(is_newer(&[2, 0, 0, 0], &[1, 0, 0, 0]));
        assert!(is_newer(&[1, 1, 0, 0], &[1, 0, 0, 0]));
        assert!(is_newer(&[1, 0, 1, 0], &[1, 0, 0, 0]));
        assert!(!is_newer(&[1, 0, 0, 0], &[1, 0, 0, 0]));
        assert!(!is_newer(&[1, 0, 0, 0], &[2, 0, 0, 0]));
    }

    #[test]
    fn test_version_to_string() {
        assert_eq!(version_to_string(&[1, 2, 3, 0]), "1.2.3");
        assert_eq!(version_to_string(&[10, 20, 30, 0]), "10.20.30");
    }
}
