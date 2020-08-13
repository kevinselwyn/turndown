use std::{thread, time};
use turndown::*;

fn main() {
    let turndown = Turndown::new();
    let sleep_amt = time::Duration::from_secs(4);

    loop {
        match turndown.listen() {
            Ok(msg) => {
                if msg.len() == 0 {
                    continue;
                }

                println!("{}", msg);

                thread::sleep(sleep_amt);
            }
            Err(e) => {
                println!("{}", e);

                std::process::exit(1);
            }
        }
    }
}
