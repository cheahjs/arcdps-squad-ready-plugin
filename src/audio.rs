use anyhow::{Context, Result};
use log::*;
use rodio::*;
use std::{
    fs::File,
    io::{BufReader, Cursor, Read},
    path,
    sync::{mpsc, Arc, RwLock},
    time::Duration,
};
use ticker::Ticker;

const READY_CHECK_SOUND: &'static [u8] = include_bytes!("../sounds/ready_check.wav");
const SQUAD_READY_SOUND: &'static [u8] = include_bytes!("../sounds/squad_ready.wav");

pub enum AudioAction {
    PlayReadyCheck,
    PlaySquadReady,
    SetReadyCheckVolume(u32),
    SetSquadReadyVolume(u32),
    SetReadyCheckPath(String),
    SetSquadReadyPath(String),
}

/// Static sound data stored in memory.
/// It is `Arc`'ed, so cheap to clone.
#[derive(Clone, Debug)]
pub struct SoundData(Arc<[u8]>);

impl SoundData {
    /// Load the file at the given path and create a new `SoundData` from it.
    pub fn new<P: AsRef<path::Path>>(path: P) -> anyhow::Result<Self> {
        let path = path.as_ref();
        let file = &mut File::open(path)?;
        SoundData::from_read(file)
    }

    /// Copies the data in the given slice into a new `SoundData` object.
    pub fn from_bytes(data: &[u8]) -> Self {
        SoundData(Arc::from(data))
    }

    /// Creates a `SoundData` from any `Read` object; this involves
    /// copying it into a buffer.
    pub fn from_read<R>(reader: &mut R) -> anyhow::Result<Self>
    where
        R: Read,
    {
        let mut buffer = Vec::new();
        let _ = reader.read_to_end(&mut buffer)?;

        Ok(SoundData::from(buffer))
    }
}

impl From<Arc<[u8]>> for SoundData {
    #[inline]
    fn from(arc: Arc<[u8]>) -> Self {
        SoundData(arc)
    }
}

impl From<Vec<u8>> for SoundData {
    fn from(v: Vec<u8>) -> Self {
        SoundData(Arc::from(v))
    }
}

impl From<Box<[u8]>> for SoundData {
    fn from(b: Box<[u8]>) -> Self {
        SoundData(Arc::from(b))
    }
}

impl AsRef<[u8]> for SoundData {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.0.as_ref()
    }
}

struct AudioTrack {
    data: Option<Cursor<SoundData>>,
    volume: u32,
    sink: rodio::Sink,
    status_message: String,
}

impl AudioTrack {
    fn new(sink: rodio::Sink) -> Self {
        Self {
            data: None,
            volume: 0,
            sink,
            status_message: "".to_string(),
        }
    }

    fn load_from_path(&mut self, path: &str, default: &'static [u8]) -> anyhow::Result<()> {
        if path == "" {
            self.data = Some(Cursor::new(SoundData::from_bytes(default)));
        } else {
            let sound_data = SoundData::new(path);
            match sound_data {
                Ok(sound_data) => self.data = Some(Cursor::new(sound_data)),
                Err(error) => {
                    self.data = None;
                    self.status_message =
                        format!("Failed to read '{}': {}", path, error.to_string());
                    return Err(error).context("failed to read");
                }
            }
        }
        // Test if sound data can be decoded
        match Decoder::new(self.data.as_ref().unwrap().clone()) {
            Ok(_decoder) => self.status_message = format!("Loaded '{}' successfully", path),
            Err(error) => {
                self.data = None;
                self.status_message = format!("Failed to decode '{}': {}", path, error.to_string());
                return Err(error).context("failed to decode");
            }
        }

        Ok(())
    }

    fn set_volume(&mut self, volume: u32) {
        self.volume = volume;
        // exponential volume control approximate using x^4
        self.sink.set_volume(f32::powi(volume as f32 / 100f32, 4))
    }

    fn play(&self) {}
}

pub struct AudioPlayer {
    event_chan: mpsc::Receiver<AudioAction>,

    _stream: rodio::OutputStream,
    stream_handle: rodio::OutputStreamHandle,

    ready_check_track: AudioTrack,
    squad_ready_track: AudioTrack,

    ui_state: crate::ui::SharedState,
}

impl AudioPlayer {
    pub fn try_new(
        event_chan: mpsc::Receiver<AudioAction>,
        ui_state: crate::ui::SharedState,
    ) -> anyhow::Result<Self> {
        let (_stream, stream_handle) =
            OutputStream::try_default().context("failed to get audio output stream")?;
        let ready_check_sink =
            Sink::try_new(&stream_handle).context("failed to create ready check sink")?;
        let squad_ready_sink =
            Sink::try_new(&stream_handle).context("failed to create squad ready sink")?;

        Ok(Self {
            _stream,
            stream_handle,
            ready_check_track: AudioTrack::new(ready_check_sink),
            squad_ready_track: AudioTrack::new(squad_ready_sink),
            event_chan,
            ui_state,
        })
    }

    pub fn start(&mut self) {
        self.start_event_loop();
    }

    fn start_event_loop(&mut self) {
        loop {
            let event = self.event_chan.recv();
            if event.is_err() {
                error!(
                    "audio: failed to receive event, stopping audio loop: {:?}",
                    event.err()
                );
                break;
            }
            match event.unwrap() {
                AudioAction::PlayReadyCheck => self.ready_check_track.play(),
                AudioAction::PlaySquadReady => self.squad_ready_track.play(),
                AudioAction::SetReadyCheckVolume(volume) => {
                    self.ready_check_track.set_volume(volume)
                }
                AudioAction::SetSquadReadyVolume(volume) => {
                    self.squad_ready_track.set_volume(volume)
                }
                AudioAction::SetReadyCheckPath(path) => {
                    let _ = self
                        .ready_check_track
                        .load_from_path(&path, READY_CHECK_SOUND);
                    let mut ui_state = self.ui_state.write().unwrap();
                    ui_state.ready_check_message = self.ready_check_track.status_message.clone();
                }
                AudioAction::SetSquadReadyPath(path) => {
                    let _ = self
                        .squad_ready_track
                        .load_from_path(&path, SQUAD_READY_SOUND);
                    let mut ui_state = self.ui_state.write().unwrap();
                    ui_state.squad_ready_message = self.squad_ready_track.status_message.clone();
                }
            }
        }
    }
}
