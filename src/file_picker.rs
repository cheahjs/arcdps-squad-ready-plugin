// Adapted from https://github.com/tseli0s/imfile
// MIT License - Aggelos Tselios
use arcdps::imgui::{Condition, Ui, Window, ChildWindow};
use std::fs;
use std::path::{PathBuf};
use log::{error};
use std::cmp::Ordering;

pub struct FilePicker {
    pub title: String,
    pub current_dir: PathBuf,
    pub is_open: bool,
    pub show_hidden_files: bool,
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
        }
    }

    pub fn open(&mut self) {
        self.is_open = true;
    }

    pub fn render(&mut self, ui: &Ui) -> Option<PathBuf> {
        if !self.is_open {
            return None;
        }

        let mut selected_path = None;
        let mut window_open = self.is_open;
        let mut should_close = false;

        // Split borrows to satisfy borrow checker in closure
        let current_dir = &mut self.current_dir;
        let show_hidden_files = &mut self.show_hidden_files;

        Window::new(&self.title)
            .opened(&mut window_open)
            .size([600.0, 400.0], Condition::FirstUseEver)
            .build(ui, || {
                // Path bar
                if let Some(_child) = ChildWindow::new("Path Selection")
                    .horizontal_scrollbar(false)
                    .border(true)
                    .size([0.0, 40.0])
                    .begin(ui)
                {
                    ui.text("Path: ");
                    ui.same_line();
                    
                    let components: Vec<PathBuf> = current_dir.iter().map(PathBuf::from).collect();
                    
                    for (i, comp) in components.iter().enumerate() {
                        let name = comp.to_string_lossy();
                        if ui.button(&format!("{}##{}", name, i)) {
                            // Rebuild path up to this component
                            let mut new_path = PathBuf::new();
                            for j in 0..=i {
                                new_path.push(&components[j]);
                            }
                            *current_dir = new_path;
                        }
                        ui.same_line();
                    }
                }

                // File list
                if let Some(_child) = ChildWindow::new("Select file")
                    .border(true)
                    .size([0.0, -40.0])
                    .begin(ui)
                {
                    match fs::read_dir(&*current_dir) {
                        Ok(entries_iter) => {
                            let mut entries: Vec<_> = entries_iter
                                .filter_map(|e| e.ok())
                                .filter(|e| {
                                    if *show_hidden_files {
                                        true
                                    } else {
                                        !e.file_name().to_string_lossy().starts_with('.')
                                    }
                                })
                                .collect();

                            entries.sort_by(|a, b| {
                                let a_is_dir = a.path().is_dir();
                                let b_is_dir = b.path().is_dir();
                                if a_is_dir && !b_is_dir {
                                    Ordering::Less
                                } else if !a_is_dir && b_is_dir {
                                    Ordering::Greater
                                } else {
                                    a.file_name().cmp(&b.file_name())
                                }
                            });

                            for entry in entries {
                                let path = entry.path();
                                let os_name = entry.file_name();
                                let name = os_name.to_string_lossy();
                                if path.is_dir() {
                                    if ui.button(&format!("[dir]  {}", name)) {
                                        *current_dir = path;
                                    }
                                } else {
                                    if ui.button(&format!("[file] {}", name)) {
                                        selected_path = Some(path);
                                        should_close = true;
                                    }
                                }
                            }
                        }
                        Err(e) => {
                            error!("failed to read directory: {}", e);
                            ui.text_colored([1.0, 0.0, 0.0, 1.0], format!("Error reading directory: {}", e));
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
                            *current_dir = parent.to_path_buf();
                        }
                    }
                    ui.same_line();
                    ui.checkbox("Hidden Files", show_hidden_files);
                    ui.same_line();
                    if ui.button("Cancel") {
                        should_close = true;
                    }
                }
            });

        self.is_open = window_open && !should_close;
        selected_path
    }
}
