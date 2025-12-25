// Adapted from https://github.com/tseli0s/imfile
// MIT License - Aggelos Tselios
use arcdps::imgui::{ChildWindow, Condition, Selectable, Ui, Window};
use log::error;
use std::cmp::Ordering;
use std::fs;
use std::path::PathBuf;

/// Get available drive letters on Windows (e.g., ["C:\\", "D:\\", ...])
#[cfg(windows)]
fn get_available_drives() -> Vec<PathBuf> {
    use windows::Win32::Storage::FileSystem::GetLogicalDrives;

    let mut drives = Vec::new();
    // SAFETY: GetLogicalDrives has no unsafe preconditions
    let bitmask = unsafe { GetLogicalDrives() };

    for i in 0..26u32 {
        if (bitmask & (1 << i)) != 0 {
            let letter = (b'A' + i as u8) as char;
            let path = PathBuf::from(format!("{}:\\", letter));
            // Only include drives that are accessible
            if path.is_dir() {
                drives.push(path);
            }
        }
    }
    drives
}

#[cfg(not(windows))]
fn get_available_drives() -> Vec<PathBuf> {
    // On non-Windows, just return root
    vec![PathBuf::from("/")]
}

/// Cached directory entry for display
#[derive(Clone)]
struct CachedEntry {
    path: PathBuf,
    name: String,
    is_dir: bool,
}

/// Cache for directory contents to avoid re-reading every frame
struct DirCache {
    path: PathBuf,
    entries: Vec<CachedEntry>,
    show_hidden: bool,
    error: Option<String>,
}

impl DirCache {
    fn new() -> Self {
        Self {
            path: PathBuf::new(),
            entries: Vec::new(),
            show_hidden: false,
            error: None,
        }
    }

    /// Refresh the cache if the path or hidden files setting changed
    fn refresh_if_needed(&mut self, current_dir: &PathBuf, show_hidden: bool) {
        if self.path == *current_dir && self.show_hidden == show_hidden {
            return; // Cache is still valid
        }

        self.path = current_dir.clone();
        self.show_hidden = show_hidden;
        self.entries.clear();
        self.error = None;

        match fs::read_dir(current_dir) {
            Ok(entries_iter) => {
                let mut entries: Vec<CachedEntry> = entries_iter
                    .filter_map(|e| e.ok())
                    .filter(|e| {
                        if show_hidden {
                            true
                        } else {
                            !e.file_name().to_string_lossy().starts_with('.')
                        }
                    })
                    .map(|e| {
                        let path = e.path();
                        let is_dir = path.is_dir();
                        let name = e.file_name().to_string_lossy().to_string();
                        CachedEntry { path, name, is_dir }
                    })
                    .collect();

                // Sort: directories first, then alphabetically
                entries.sort_by(|a, b| {
                    if a.is_dir && !b.is_dir {
                        Ordering::Less
                    } else if !a.is_dir && b.is_dir {
                        Ordering::Greater
                    } else {
                        a.name.cmp(&b.name)
                    }
                });

                self.entries = entries;
            }
            Err(e) => {
                error!("failed to read directory: {}", e);
                self.error = Some(format!("Error reading directory: {}", e));
            }
        }
    }

    /// Force refresh on next access
    fn invalidate(&mut self) {
        self.path = PathBuf::new();
    }
}

pub struct FilePicker {
    pub title: String,
    pub current_dir: PathBuf,
    pub is_open: bool,
    pub show_hidden_files: bool,
    filter_text: String,
    cache: DirCache,
}

impl FilePicker {
    pub fn new(title: String, initial_dir: Option<PathBuf>) -> Self {
        let current_dir = initial_dir
            .filter(|p| p.is_dir())
            .unwrap_or_else(|| std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")));

        Self {
            title,
            current_dir,
            is_open: false,
            show_hidden_files: false,
            filter_text: String::new(),
            cache: DirCache::new(),
        }
    }

    pub fn open(&mut self) {
        self.is_open = true;
        // Invalidate cache and clear filter when opening
        self.cache.invalidate();
        self.filter_text.clear();
    }

    pub fn render(&mut self, ui: &Ui) -> Option<PathBuf> {
        if !self.is_open {
            return None;
        }

        // Refresh cache if needed (only does work if path/settings changed)
        self.cache
            .refresh_if_needed(&self.current_dir, self.show_hidden_files);

        let mut selected_path = None;
        let mut window_open = self.is_open;
        let mut should_close = false;
        let mut new_dir: Option<PathBuf> = None;
        let mut toggle_hidden = false;

        // Clone data needed in closure
        let current_dir_clone = self.current_dir.clone();
        let mut show_hidden_files = self.show_hidden_files;
        let mut filter_text = self.filter_text.clone();
        let filter_lower = filter_text.to_lowercase();

        // Filter cached entries based on filter text
        let cache_entries: Vec<CachedEntry> = self
            .cache
            .entries
            .iter()
            .filter(|e| {
                if filter_lower.is_empty() {
                    true
                } else {
                    e.name.to_lowercase().contains(&filter_lower)
                }
            })
            .cloned()
            .collect();
        let cache_error = self.cache.error.clone();

        Window::new(&self.title)
            .opened(&mut window_open)
            .size([600.0, 400.0], Condition::FirstUseEver)
            .build(ui, || {
                // Drive selector and path bar
                if let Some(_child) = ChildWindow::new("Path Selection")
                    .horizontal_scrollbar(false)
                    .border(true)
                    .size([0.0, 40.0])
                    .begin(ui)
                {
                    // Drive selector dropdown
                    let drives = get_available_drives();
                    let current_drive = current_dir_clone
                        .components()
                        .next()
                        .map(|c| {
                            let mut p = PathBuf::from(c.as_os_str());
                            // Ensure it ends with separator for display
                            if !p.to_string_lossy().ends_with(std::path::MAIN_SEPARATOR) {
                                p.push(std::path::MAIN_SEPARATOR.to_string());
                            }
                            p.to_string_lossy().to_string()
                        })
                        .unwrap_or_default();

                    ui.set_next_item_width(60.0);
                    if let Some(_combo) = ui.begin_combo("##drive", &current_drive) {
                        for drive in &drives {
                            let drive_str = drive.to_string_lossy();
                            let is_selected = drive_str == current_drive;
                            if Selectable::new(&drive_str).selected(is_selected).build(ui)
                                && drive.is_dir()
                            {
                                new_dir = Some(drive.clone());
                            }
                        }
                    }

                    ui.same_line();
                    ui.text("Path:");
                    ui.same_line();

                    let components: Vec<PathBuf> =
                        current_dir_clone.iter().map(PathBuf::from).collect();

                    for (i, comp) in components.iter().enumerate() {
                        let name = comp.to_string_lossy();
                        if ui.button(format!("{}##{}", name, i)) {
                            // Rebuild path up to this component
                            let mut nav_path = PathBuf::new();
                            for component in components.iter().take(i + 1) {
                                nav_path.push(component);
                            }
                            // On Windows, "C:" alone refers to CWD on that drive, not the root.
                            // We need "C:\" to refer to the root, so add a separator if needed.
                            if i == 0 && nav_path.to_string_lossy().ends_with(':') {
                                nav_path.push(std::path::MAIN_SEPARATOR.to_string());
                            }
                            // Only navigate if the path is a valid directory
                            if nav_path.is_dir() {
                                new_dir = Some(nav_path);
                            }
                        }
                        ui.same_line();
                    }
                }

                // Filter input
                ui.set_next_item_width(-1.0);
                ui.input_text("##filter", &mut filter_text)
                    .hint("Filter files...")
                    .build();

                // File list
                if let Some(_child) = ChildWindow::new("Select file")
                    .border(true)
                    .size([0.0, -40.0])
                    .begin(ui)
                {
                    if let Some(ref err) = cache_error {
                        ui.text_colored([1.0, 0.0, 0.0, 1.0], err);
                    } else {
                        for entry in &cache_entries {
                            if entry.is_dir {
                                if ui.button(format!("[dir]  {}", entry.name)) {
                                    new_dir = Some(entry.path.clone());
                                }
                            } else if ui.button(format!("[file] {}", entry.name)) {
                                selected_path = Some(entry.path.clone());
                                should_close = true;
                            }
                        }
                    }
                }

                // Bottom bar
                if let Some(_child) = ChildWindow::new("controls")
                    .border(false)
                    .size([0.0, 0.0])
                    .begin(ui)
                {
                    if ui.button("Back") {
                        if let Some(parent) = current_dir_clone.parent() {
                            let parent_path = parent.to_path_buf();
                            // Only navigate if parent is a valid directory
                            if parent_path.is_dir() {
                                new_dir = Some(parent_path);
                            }
                        }
                    }
                    ui.same_line();
                    if ui.checkbox("Hidden Files", &mut show_hidden_files) {
                        toggle_hidden = true;
                    }
                    ui.same_line();
                    if ui.button("Refresh") {
                        new_dir = Some(current_dir_clone.clone()); // Force cache refresh
                    }
                    ui.same_line();
                    if ui.button("Cancel") {
                        should_close = true;
                    }
                }
            });

        // Apply directory navigation outside of closure
        if let Some(dir) = new_dir {
            self.current_dir = dir;
            self.cache.invalidate();
            self.filter_text.clear(); // Clear filter when navigating
        } else {
            // Update filter text if it changed
            self.filter_text = filter_text;
        }

        // Apply hidden files toggle
        if toggle_hidden {
            self.show_hidden_files = show_hidden_files;
        }

        self.is_open = window_open && !should_close;
        selected_path
    }
}
