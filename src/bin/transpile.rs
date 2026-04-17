use std::{env, error::Error, fs, io};

use embive::transpiler::transpile_elf;

const OUTPUT_CAPACITY: usize = 256 * 1024;

fn main() -> Result<(), Box<dyn Error>> {
    let mut args = env::args();
    let program = args.next().unwrap_or_else(|| String::from("transpile"));
    let input = args.next().ok_or_else(|| usage_error(&program))?;
    let output = args.next().ok_or_else(|| usage_error(&program))?;

    if args.next().is_some() {
        return Err(usage_error(&program).into());
    }

    let elf = fs::read(&input)?;
    let mut transpiled = vec![0; OUTPUT_CAPACITY];
    let transpiled_len = transpile_elf(&elf, &mut transpiled)?;

    fs::write(output, &transpiled[..transpiled_len])?;
    Ok(())
}

fn usage_error(program: &str) -> io::Error {
    io::Error::new(
        io::ErrorKind::InvalidInput,
        format!("Usage: {program} <input.elf> <output.bin>"),
    )
}
