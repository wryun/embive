#![allow(unsafe_code)]

use core::ffi::{c_int, c_uint};

pub const EMBIVE_CPU_REGISTER_COUNT: usize = 32;
pub const EMBIVE_RAM_OFFSET: u32 = 0x8000_0000;
pub const EMBIVE_INTERRUPT_CODE: u32 = 16;
pub const EMBIVE_SYSCALL_ARGS: usize = 7;

#[repr(C)]
#[allow(non_camel_case_types)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum embive_state_t {
    EMBIVE_STATE_RUNNING = 0,
    EMBIVE_STATE_CALLED = 1,
    EMBIVE_STATE_WAITING = 2,
    EMBIVE_STATE_HALTED = 3,
}

#[repr(C)]
#[allow(non_camel_case_types)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum embive_error_t {
    EMBIVE_OK = 0,
    EMBIVE_ERR_INVALID_MEMORY_ADDRESS,
    EMBIVE_ERR_INVALID_MEMORY_ACCESS_LENGTH,
    EMBIVE_ERR_INVALID_PROGRAM_COUNTER,
    EMBIVE_ERR_INVALID_INSTRUCTION,
    EMBIVE_ERR_INVALID_CS_REGISTER,
    EMBIVE_ERR_INVALID_CPU_REGISTER,
    EMBIVE_ERR_ILLEGAL_INSTRUCTION,
    EMBIVE_ERR_INTERRUPT_NOT_ENABLED,
    EMBIVE_ERR_NO_SYSCALL_FUNCTION,
    EMBIVE_ERR_UNIMPLEMENTED,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct embive_registers_t {
    pub x: [i32; EMBIVE_CPU_REGISTER_COUNT],
    pub mtvec: u32,
    pub mscratch: u32,
    pub mepc: u32,
    pub mcause: u32,
    pub mtval: i32,
    pub mie_embive: u8,
    pub mip_embive: u8,
    pub mstatus: u8,
}

#[repr(C)]
#[derive(Debug)]
pub struct embive_memory_t {
    pub code: *const u8,
    pub code_len: usize,
    pub ram: *mut u8,
    pub ram_len: usize,
}

#[repr(C)]
#[derive(Debug)]
pub struct embive_interpreter_t {
    pub program_counter: u32,
    pub registers: embive_registers_t,
    pub memory: embive_memory_t,
    pub instruction_limit: u32,
    pub has_reservation: u8,
    pub reservation_addr: u32,
    pub reservation_value: i32,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct embive_syscall_t {
    pub nr: i32,
    pub args: [i32; EMBIVE_SYSCALL_ARGS],
}

unsafe extern "C" {
    pub fn embive_init(
        interpreter: *mut embive_interpreter_t,
        code: *const u8,
        code_len: usize,
        ram: *mut u8,
        ram_len: usize,
        instruction_limit: c_uint,
    );

    pub fn embive_reset(interpreter: *mut embive_interpreter_t);

    pub fn embive_run(
        interpreter: *mut embive_interpreter_t,
        state: *mut embive_state_t,
    ) -> embive_error_t;

    pub fn embive_step(
        interpreter: *mut embive_interpreter_t,
        state: *mut embive_state_t,
    ) -> embive_error_t;

    pub fn embive_interrupt(
        interpreter: *mut embive_interpreter_t,
        value: c_int,
    ) -> embive_error_t;

    pub fn embive_get_syscall(
        interpreter: *const embive_interpreter_t,
        syscall: *mut embive_syscall_t,
    ) -> embive_error_t;

    pub fn embive_set_syscall_result(
        interpreter: *mut embive_interpreter_t,
        error_code: c_int,
        value: c_int,
    );
}
