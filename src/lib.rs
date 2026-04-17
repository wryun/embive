#![cfg_attr(not(test), no_std)]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(all(feature = "interpreter", feature = "transpiler"), doc = include_str!(concat!(env!("CARGO_MANIFEST_DIR"), "/README.md")))]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/embive/embive/6da108bce7d0d01ac15ccb78786b68310c83289e/assets/embive_logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/embive/embive/6da108bce7d0d01ac15ccb78786b68310c83289e/assets/embive_logo.svg"
)]
//!
#![warn(missing_docs, rust_2018_idioms, future_incompatible, keyword_idents)]
#![deny(unsafe_code)]

#[cfg(all(feature = "alloc", feature = "transpiler"))]
extern crate alloc;

mod format;
pub mod instruction;
#[cfg(feature = "interpreter")]
pub mod interpreter;
#[cfg(feature = "transpiler")]
pub mod transpiler;

#[cfg(all(test, feature = "interpreter", feature = "transpiler"))]
mod tests {
    use core::num::NonZeroI32;
    use std::{
        fs::read_dir,
        path::{Path, PathBuf},
        boxed::Box,
    };

    use embive_c as c_backend;

    use crate::{
        interpreter::{
            memory::{SliceMemory, RAM_OFFSET},
            Error, Interpreter, State, SYSCALL_ARGS,
        },
        transpiler::transpile_elf,
    };

    const RAM_SIZE: usize = 32 * 1024;
    const RV32UI_TESTS: usize = 39;
    const RV32UM_TESTS: usize = 8;
    const RV32UA_TESTS: usize = 10;
    const RV32UC_TESTS: usize = 1;

    thread_local! {
        static SYSCALL_COUNTER: std::cell::RefCell<i32> = const { std::cell::RefCell::new(0) };
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    enum BackendState {
        Running,
        Called,
        Waiting,
        Halted,
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    enum ExecutionOutcome {
        Passed,
        Unimplemented,
    }

    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    enum HarnessError {
        BackendUnimplemented,
    }

    fn rust_syscall(
        nr: i32,
        args: &[i32; SYSCALL_ARGS],
        _memory: &mut SliceMemory<'_>,
    ) -> Result<Result<i32, NonZeroI32>, Error> {
        if nr == 93 {
            if args[0] == 0 {
                println!("Test was successful");
            } else {
                panic!("Failed test number: {}", args[0] >> 1);
            }
        } else {
            panic!("Unknown syscall: {nr}");
        }

        SYSCALL_COUNTER.with(|c| *c.borrow_mut() += 1);
        Ok(Ok(0))
    }

    fn handle_host_syscall(nr: i32, args: &[i32; SYSCALL_ARGS]) {
        if nr == 93 {
            if args[0] == 0 {
                println!("Test was successful");
            } else {
                panic!("Failed test number: {}", args[0] >> 1);
            }
        } else {
            panic!("Unknown syscall: {nr}");
        }

        SYSCALL_COUNTER.with(|c| *c.borrow_mut() += 1);
    }

    trait Backend {
        fn set_program_counter(&mut self, value: u32);
        fn run(&mut self) -> Result<BackendState, HarnessError>;
        fn handle_syscall(&mut self) -> Result<(), HarnessError>;
    }

    trait BackendFactory {
        type Backend<'a>: Backend
        where
            Self: 'a;

        fn create<'a>(code: &'a [u8], ram: &'a mut [u8]) -> Self::Backend<'a>;
    }

    struct RustBackendFactory;

    struct RustBackend<'a> {
        interpreter: Interpreter<'a, SliceMemory<'a>>,
    }

    impl BackendFactory for RustBackendFactory {
        type Backend<'a> = RustBackend<'a>;

        fn create<'a>(code: &'a [u8], ram: &'a mut [u8]) -> Self::Backend<'a> {
            let memory = SliceMemory::new(code, ram);
            let memory = Box::new(memory);
            let memory = Box::leak(memory);

            RustBackend {
                interpreter: Interpreter::new(memory, 0),
            }
        }
    }

    impl Backend for RustBackend<'_> {
        fn set_program_counter(&mut self, value: u32) {
            self.interpreter.program_counter = value;
        }

        fn run(&mut self) -> Result<BackendState, HarnessError> {
            let state = self.interpreter.run().unwrap();
            Ok(match state {
                State::Running => BackendState::Running,
                State::Called => BackendState::Called,
                State::Waiting => BackendState::Waiting,
                State::Halted => BackendState::Halted,
            })
        }

        fn handle_syscall(&mut self) -> Result<(), HarnessError> {
            self.interpreter.syscall(&mut rust_syscall).unwrap();
            Ok(())
        }
    }

    struct CBackendFactory;

    struct CBackend<'a> {
        interpreter: c_backend::Interpreter<'a>,
    }

    impl BackendFactory for CBackendFactory {
        type Backend<'a> = CBackend<'a>;

        fn create<'a>(code: &'a [u8], ram: &'a mut [u8]) -> Self::Backend<'a> {
            CBackend {
                interpreter: c_backend::Interpreter::new(code, ram, 0),
            }
        }
    }

    impl Backend for CBackend<'_> {
        fn set_program_counter(&mut self, value: u32) {
            self.interpreter.set_program_counter(value);
        }

        fn run(&mut self) -> Result<BackendState, HarnessError> {
            let state = self.interpreter.run().map_err(map_c_error)?;
            Ok(match state {
                c_backend::State::Running => BackendState::Running,
                c_backend::State::Called => BackendState::Called,
                c_backend::State::Waiting => BackendState::Waiting,
                c_backend::State::Halted => BackendState::Halted,
            })
        }

        fn handle_syscall(&mut self) -> Result<(), HarnessError> {
            let syscall = self.interpreter.syscall().map_err(map_c_error)?;
            handle_host_syscall(syscall.nr, &syscall.args);
            self.interpreter.set_syscall_result(0, 0);
            Ok(())
        }
    }

    fn map_c_error(error: c_backend::Error) -> HarnessError {
        match error {
            c_backend::Error::Unimplemented => HarnessError::BackendUnimplemented,
            other => panic!("Unexpected C backend error: {other:?}"),
        }
    }

    fn execute_test_path<B: BackendFactory>(path: &Path) -> ExecutionOutcome {
        let code = &[];

        println!(
            "\nRunning: {}",
            path.file_name().expect("Test path should have a file name").to_string_lossy()
        );

        // Load binary into RAM
        let mut ram = [0; RAM_SIZE];
        let test_elf = std::fs::read(path).expect("Failed to read test file");
        transpile_elf(&test_elf, &mut ram).expect("Failed to transpile");

        let mut backend = B::create(code, &mut ram);
        backend.set_program_counter(RAM_OFFSET);

        // Get syscall counter prior to running
        let prev_syscall_counter = SYSCALL_COUNTER.with(|c| *c.borrow());

        // Run it
        loop {
            match backend.run() {
                Ok(BackendState::Running) => {}
                Ok(BackendState::Called) => {
                    backend.handle_syscall().unwrap();
                }
                Ok(BackendState::Waiting) => {}
                Ok(BackendState::Halted) => break,
                Err(HarnessError::BackendUnimplemented) => return ExecutionOutcome::Unimplemented,
            }
        }

        // Get syscall counter after running
        let new_syscall_counter = SYSCALL_COUNTER.with(|c| *c.borrow());

        // Check if syscall was incremented
        if new_syscall_counter <= prev_syscall_counter {
            panic!("No syscall was made");
        }

        ExecutionOutcome::Passed
    }

    fn execute_test_dir<B: BackendFactory>(subdir: &str) -> usize {
        let mut dir = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
        dir.push("tests/riscv");
        dir.push(subdir);

        let tests = read_dir(dir).expect("Failed to read directory");

        let mut tested_files = 0;
        for test in tests {
            let test = test.expect("Failed to get test");
            let outcome = execute_test_path::<B>(&test.path());
            assert_eq!(outcome, ExecutionOutcome::Passed);
            tested_files += 1;
        }

        tested_files
    }

    fn execute_single_test<B: BackendFactory>(path: &Path) -> ExecutionOutcome {
        let base = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
        execute_test_path::<B>(&base.join(path))
    }

    #[test]
    fn rv32ui_bin_tests() {
        assert_eq!(execute_test_dir::<RustBackendFactory>("rv32ui"), RV32UI_TESTS);
    }

    #[test]
    fn rv32um_bin_tests() {
        assert_eq!(execute_test_dir::<RustBackendFactory>("rv32um"), RV32UM_TESTS);
    }

    #[test]
    fn rv32ua_bin_tests() {
        assert_eq!(execute_test_dir::<RustBackendFactory>("rv32ua"), RV32UA_TESTS);
    }

    #[test]
    fn rv32uc_bin_tests() {
        assert_eq!(execute_test_dir::<RustBackendFactory>("rv32uc"), RV32UC_TESTS);
    }

    #[test]
    fn c_backend_scaffold_uses_shared_harness() {
        let test = Path::new("tests/riscv/rv32ui/simple.elf");
        let outcome = execute_single_test::<CBackendFactory>(test);
        assert_eq!(outcome, ExecutionOutcome::Passed);
    }

    #[test]
    fn c_backend_rv32ui_bin_tests() {
        assert_eq!(execute_test_dir::<CBackendFactory>("rv32ui"), RV32UI_TESTS);
    }

    #[test]
    fn c_backend_rv32um_bin_tests() {
        assert_eq!(execute_test_dir::<CBackendFactory>("rv32um"), RV32UM_TESTS);
    }

    #[test]
    fn c_backend_rv32ua_bin_tests() {
        assert_eq!(execute_test_dir::<CBackendFactory>("rv32ua"), RV32UA_TESTS);
    }

    #[test]
    fn c_backend_rv32uc_bin_tests() {
        assert_eq!(execute_test_dir::<CBackendFactory>("rv32uc"), RV32UC_TESTS);
    }
}
