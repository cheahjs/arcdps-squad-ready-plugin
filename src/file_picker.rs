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
    /// Pre-formatted display name to avoid allocations during render
    display_name: String,
    is_dir: bool,
}

/// Cache for directory contents to avoid re-reading every frame
struct DirCache {
    path: PathBuf,
    entries: Vec<CachedEntry>,
    /// Filtered entries cache - indices into entries Vec
    filtered_indices: Vec<usize>,
    /// The filter text used to compute filtered_indices
    filter_text: String,
    show_hidden: bool,
    error: Option<String>,
}

/// Cached drive list to avoid system calls every frame
struct DriveCache {
    drives: Vec<PathBuf>,
    /// Timestamp of last refresh (we refresh every ~5 seconds)
    last_refresh: std::time::Instant,
}

impl DriveCache {
    fn new() -> Self {
        Self {
            drives: get_available_drives(),
            last_refresh: std::time::Instant::now(),
        }
    }

    fn get(&mut self) -> &[PathBuf] {
        // Refresh drive list every 5 seconds
        if self.last_refresh.elapsed().as_secs() > 5 {
            self.drives = get_available_drives();
            self.last_refresh = std::time::Instant::now();
        }
        &self.drives
    }
}

impl DirCache {
    fn new() -> Self {
        Self {
            path: PathBuf::new(),
            entries: Vec::new(),
            filtered_indices: Vec::new(),
            filter_text: String::new(),
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
        self.filtered_indices.clear();
        self.filter_text.clear();
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
                        let display_name = if is_dir {
                            format!("[dir]  {}", name)
                        } else {
                            format!("[file] {}", name)
                        };
                        CachedEntry {
                            path,
                            name,
                            display_name,
                            is_dir,
                        }
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
                // Initialize filtered indices to show all entries
                self.filtered_indices = (0..self.entries.len()).collect();
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

    /// Update filtered indices if filter text changed
    fn update_filter(&mut self, filter_text: &str) {
        if self.filter_text == filter_text {
            return; // Filter unchanged
        }

        self.filter_text = filter_text.to_string();
        let filter_lower = filter_text.to_lowercase();

        if filter_lower.is_empty() {
            // No filter - show all entries
            self.filtered_indices = (0..self.entries.len()).collect();
        } else {
            // Filter entries by name
            self.filtered_indices = self
                .entries
                .iter()
                .enumerate()
                .filter(|(_, e)| e.name.to_lowercase().contains(&filter_lower))
                .map(|(i, _)| i)
                .collect();
        }
    }

    /// Get an iterator over filtered entries
    fn filtered_entries(&self) -> impl Iterator<Item = &CachedEntry> {
        self.filtered_indices.iter().map(|&i| &self.entries[i])
    }
}

pub struct FilePicker {
    pub title: String,
    pub current_dir: PathBuf,
    pub is_open: bool,
    pub show_hidden_files: bool,
    filter_text: String,
    cache: DirCache,
    drive_cache: DriveCache,
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
            drive_cache: DriveCache::new(),
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

        // Update filter (only does work if filter text changed)
        self.cache.update_filter(&self.filter_text);

        let mut selected_path = None;
        let mut window_open = self.is_open;
        let mut should_close = false;
        let mut new_dir: Option<PathBuf> = None;
        let mut toggle_hidden = false;

        // Borrow data needed in closure - avoid cloning where possible
        let current_dir = &self.current_dir;
        let mut show_hidden_files = self.show_hidden_files;
        let mut filter_text = self.filter_text.clone();

        // Get cached drives (refreshes periodically, not every frame)
        let drives = self.drive_cache.get();

        // Borrow cache data for use in closure
        let cache_error = &self.cache.error;
        let cache = &self.cache;

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
                    let current_drive = current_dir
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
                        for drive in drives {
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

                    // Iterate over path components directly to avoid Vec allocation
                    let mut accumulated_path = PathBuf::new();
                    for (i, comp) in current_dir.iter().enumerate() {
                        let name = comp.to_string_lossy();
                        accumulated_path.push(comp);
                        if ui.button(format!("{}##{}", name, i)) {
                            // Clone the path for navigation
                            let mut nav_path = accumulated_path.clone();
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
                        // Use cached filtered entries and pre-computed display names
                        for entry in cache.filtered_entries() {
                            // Use pre-computed display_name to avoid format! every frame
                            if entry.is_dir {
                                if ui.button(&entry.display_name) {
                                    new_dir = Some(entry.path.clone());
                                }
                            } else if ui.button(&entry.display_name) {
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
                        if let Some(parent) = current_dir.parent() {
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
                        new_dir = Some(current_dir.to_path_buf()); // Force cache refresh
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
