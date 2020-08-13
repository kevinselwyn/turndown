use regex::*;
use rodio::{self, Source};
use std::io::Cursor;
use std::process::Command;
use std::str;

static TURNDOWN_WAV: &[u8] = include_bytes!("../turndown.wav");

pub struct Turndown {}

impl Turndown {
    pub fn new() -> Turndown {
        Turndown {}
    }

    pub fn listen(&self) -> Result<&'static str, String> {
        let re = match Regex::new(r"output volume:(\d+)") {
            Ok(re) => re,
            Err(e) => return Err(e.to_string()),
        };
        let volume_settings = match Command::new("osascript")
            .arg("-e")
            .arg("get volume settings")
            .output()
        {
            Ok(volume_settings) => volume_settings,
            Err(e) => return Err(e.to_string()),
        };

        let volume_caps = match re.captures(str::from_utf8(&volume_settings.stdout).unwrap()) {
            Some(volume_caps) => volume_caps,
            None => return Err("Could not parse volume".to_string()),
        };

        let volume: u64 = match volume_caps.get(1) {
            Some(volume) => volume.as_str().parse().unwrap(),
            None => 0,
        };

        if volume != 0 {
            return Ok("");
        }

        match Command::new("osascript")
            .arg("-e")
            .arg("set volume 100")
            .spawn()
        {
            Err(e) => return Err(e.to_string()),
            _ => {}
        };

        let device = match rodio::default_output_device() {
            Some(device) => device,
            _ => return Err("Could not open audio device".to_string()),
        };

        let source = match rodio::Decoder::new(Cursor::new(TURNDOWN_WAV)) {
            Ok(source) => source,
            Err(e) => return Err(e.to_string()),
        };

        rodio::play_raw(&device, source.convert_samples());

        Ok("TURN DOWN FOR WHAT?!")
    }
}
