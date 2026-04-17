#![no_std]

use core::marker::PhantomData;
use core::mem::MaybeUninit;

use embive_c_sys::{
    embive_error_t, embive_get_syscall, embive_init, embive_interpreter_t, embive_interrupt,
    embive_reset, embive_run, embive_set_syscall_result, embive_state_t, embive_step,
    embive_syscall_t, EMBIVE_CPU_REGISTER_COUNT, EMBIVE_SYSCALL_ARGS,
};

/// Safe wrapper around the C interpreter backend.
pub struct Interpreter<'a> {
    raw: embive_interpreter_t,
    _code: PhantomData<&'a [u8]>,
    _ram: PhantomData<&'a mut [u8]>,
}

/// Interpreter state returned by the C backend.
#[derive(Debug, Default, Clone, Copy, PartialEq, Eq)]
pub enum State {
    /// The guest is still running.
    #[default]
    Running,
    /// The guest executed a syscall instruction.
    Called,
    /// The guest is waiting for an interrupt.
    Waiting,
    /// The guest halted.
    Halted,
}

/// Error returned by the safe C backend wrapper.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Error {
    /// Memory address is out of bounds.
    InvalidMemoryAddress,
    /// Memory access length overflowed or was invalid.
    InvalidMemoryAccessLength,
    /// Program counter is out of bounds.
    InvalidProgramCounter,
    /// Instruction is invalid.
    InvalidInstruction,
    /// Control/status register is invalid.
    InvalidCSRegister,
    /// CPU register index is invalid.
    InvalidCPURegister,
    /// Instruction is illegal.
    IllegalInstruction,
    /// Interrupt has not been enabled by the guest.
    InterruptNotEnabled,
    /// No syscall function is configured.
    NoSyscallFunction,
    /// C backend entry point exists but is not implemented yet.
    Unimplemented,
}

/// Snapshot of syscall arguments read from the interpreter registers.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Syscall {
    /// Syscall number from `a7`.
    pub nr: i32,
    /// Syscall arguments from `a0..=a6`.
    pub args: [i32; EMBIVE_SYSCALL_ARGS],
}

impl<'a> Interpreter<'a> {
    /// Create a new C-backed interpreter around the provided code and RAM slices.
    pub fn new(code: &'a [u8], ram: &'a mut [u8], instruction_limit: u32) -> Self {
        let mut raw = MaybeUninit::<embive_interpreter_t>::uninit();

        // SAFETY:
        // - `raw` points to valid, writable storage for `embive_interpreter_t`.
        // - `code` and `ram` remain borrowed by `Self` for `'a`.
        // - the C function fully initializes the interpreter struct.
        unsafe {
            embive_init(
                raw.as_mut_ptr(),
                code.as_ptr(),
                code.len(),
                ram.as_mut_ptr(),
                ram.len(),
                instruction_limit,
            );

            Self {
                raw: raw.assume_init(),
                _code: PhantomData,
                _ram: PhantomData,
            }
        }
    }

    /// Reset interpreter state.
    pub fn reset(&mut self) {
        // SAFETY: `self.raw` is a valid interpreter initialized by `embive_init`.
        unsafe { embive_reset(&mut self.raw) }
    }

    /// Run until the instruction limit is reached or the guest changes state.
    pub fn run(&mut self) -> Result<State, Error> {
        self.call_stateful(embive_run)
    }

    /// Execute one instruction.
    pub fn step(&mut self) -> Result<State, Error> {
        self.call_stateful(embive_step)
    }

    /// Deliver an interrupt value to the guest.
    pub fn interrupt(&mut self, value: i32) -> Result<(), Error> {
        // SAFETY: `self.raw` is a valid interpreter initialized by `embive_init`.
        let err = unsafe { embive_interrupt(&mut self.raw, value) };
        map_error(err)
    }

    /// Read syscall number and arguments from the guest registers.
    pub fn syscall(&self) -> Result<Syscall, Error> {
        let mut syscall = MaybeUninit::<embive_syscall_t>::uninit();

        // SAFETY:
        // - `self.raw` is initialized.
        // - `syscall` points to valid writable storage for the C output.
        let err = unsafe { embive_get_syscall(&self.raw, syscall.as_mut_ptr()) };
        map_error(err)?;

        // SAFETY: `embive_get_syscall` succeeded and initialized `syscall`.
        let syscall = unsafe { syscall.assume_init() };

        Ok(Syscall {
            nr: syscall.nr,
            args: syscall.args,
        })
    }

    /// Write syscall result registers back to the guest.
    pub fn set_syscall_result(&mut self, error_code: i32, value: i32) {
        // SAFETY: `self.raw` is a valid interpreter initialized by `embive_init`.
        unsafe { embive_set_syscall_result(&mut self.raw, error_code, value) }
    }

    /// Get the current program counter.
    pub fn program_counter(&self) -> u32 {
        self.raw.program_counter
    }

    /// Set the current program counter.
    pub fn set_program_counter(&mut self, value: u32) {
        self.raw.program_counter = value;
    }

    /// Read a general-purpose register by index.
    pub fn register(&self, index: usize) -> Option<i32> {
        self.raw.registers.x.get(index).copied()
    }

    /// Write a general-purpose register by index.
    pub fn set_register(&mut self, index: usize, value: i32) -> Result<(), Error> {
        let register = self
            .raw
            .registers
            .x
            .get_mut(index)
            .ok_or(Error::InvalidCPURegister)?;
        *register = value;
        Ok(())
    }

    /// Borrow the raw register array.
    pub fn registers(&self) -> &[i32; EMBIVE_CPU_REGISTER_COUNT] {
        &self.raw.registers.x
    }

    fn call_stateful(
        &mut self,
        function: unsafe extern "C" fn(
            *mut embive_interpreter_t,
            *mut embive_state_t,
        ) -> embive_error_t,
    ) -> Result<State, Error> {
        let mut state = embive_state_t::EMBIVE_STATE_RUNNING;

        // SAFETY:
        // - `self.raw` is initialized.
        // - `state` is a valid out-pointer for the C function to write into.
        let err = unsafe { function(&mut self.raw, &mut state) };
        map_error(err)?;
        Ok(map_state(state))
    }
}

fn map_state(state: embive_state_t) -> State {
    match state {
        embive_state_t::EMBIVE_STATE_RUNNING => State::Running,
        embive_state_t::EMBIVE_STATE_CALLED => State::Called,
        embive_state_t::EMBIVE_STATE_WAITING => State::Waiting,
        embive_state_t::EMBIVE_STATE_HALTED => State::Halted,
    }
}

fn map_error(error: embive_error_t) -> Result<(), Error> {
    match error {
        embive_error_t::EMBIVE_OK => Ok(()),
        embive_error_t::EMBIVE_ERR_INVALID_MEMORY_ADDRESS => Err(Error::InvalidMemoryAddress),
        embive_error_t::EMBIVE_ERR_INVALID_MEMORY_ACCESS_LENGTH => {
            Err(Error::InvalidMemoryAccessLength)
        }
        embive_error_t::EMBIVE_ERR_INVALID_PROGRAM_COUNTER => {
            Err(Error::InvalidProgramCounter)
        }
        embive_error_t::EMBIVE_ERR_INVALID_INSTRUCTION => Err(Error::InvalidInstruction),
        embive_error_t::EMBIVE_ERR_INVALID_CS_REGISTER => Err(Error::InvalidCSRegister),
        embive_error_t::EMBIVE_ERR_INVALID_CPU_REGISTER => Err(Error::InvalidCPURegister),
        embive_error_t::EMBIVE_ERR_ILLEGAL_INSTRUCTION => Err(Error::IllegalInstruction),
        embive_error_t::EMBIVE_ERR_INTERRUPT_NOT_ENABLED => Err(Error::InterruptNotEnabled),
        embive_error_t::EMBIVE_ERR_NO_SYSCALL_FUNCTION => Err(Error::NoSyscallFunction),
        embive_error_t::EMBIVE_ERR_UNIMPLEMENTED => Err(Error::Unimplemented),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const AUIPC_OPCODE: u32 = 23;
    const BRANCH_OPCODE: u32 = 24;
    const JAL_OPCODE: u32 = 25;
    const JALR_OPCODE: u32 = 26;
    const SYSTEM_MISC_MEM_OPCODE: u32 = 31;
    const LOAD_STORE_OPCODE: u32 = 27;
    const LUI_OPCODE: u32 = 28;
    const OP_IMM_OPCODE: u32 = 29;
    const OP_AMO_OPCODE: u32 = 30;
    const LW_FUNC: u8 = 2;
    const SW_FUNC: u8 = 7;
    const MISC_FUNC: u8 = 0;
    const CSRRW_FUNC: u8 = 1;
    const CSRRWI_FUNC: u8 = 4;
    const ADDI_FUNC: u8 = 0;
    const BEQ_FUNC: u8 = 0;
    const BNE_FUNC: u8 = 1;
    const ADD_FUNC: u16 = 0;
    const MULH_FUNC: u16 = 11;
    const LR_FUNC: u16 = 18;
    const SC_FUNC: u16 = 19;
    const ECALL_IMM: i32 = 0;
    const EBREAK_IMM: i32 = 1;
    const WFI_IMM: i32 = 3;
    const MRET_IMM: i32 = 4;
    const MSTATUS_ADDR: i32 = 0x300;
    const MIE_ADDR: i32 = 0x304;
    const MTVEC_ADDR: i32 = 0x305;

    fn encode_opcode_type_i(opcode: u32, rd_rs2: u8, func: u8, rs1: u8, imm: i32) -> [u8; 4] {
        let instruction = ((rd_rs2 as u32) << 10)
            | ((func as u32) << 7)
            | ((rs1 as u32) << 15)
            | (((imm as u32) & 0x0fff) << 20)
            | opcode;
        instruction.to_le_bytes()
    }

    fn encode_type_b(func: u8, rs1: u8, rs2: u8, imm: i32) -> [u8; 4] {
        let instruction = ((func as u32) << 7)
            | ((rs1 as u32) << 10)
            | ((rs2 as u32) << 15)
            | (((imm as u32) << 19) & (0x0fff << 20))
            | BRANCH_OPCODE;
        instruction.to_le_bytes()
    }

    fn encode_type_u(opcode: u32, rd: u8, imm: i32) -> [u8; 4] {
        let instruction = ((rd as u32) << 7) | ((imm as u32) & (0x000f_ffff << 12)) | opcode;
        instruction.to_le_bytes()
    }

    fn encode_type_j(rd: u8, imm: i32) -> [u8; 4] {
        let instruction = ((rd as u32) << 7) | ((imm as u32) << 11) | JAL_OPCODE;
        instruction.to_le_bytes()
    }

    fn encode_ci1(opcode: u32, rd_rs1: u8, imm: i32) -> [u8; 4] {
        let instruction = ((rd_rs1 as u32) << 5) | (((imm as u32) << 10) & (0x3f << 10)) | opcode;
        instruction.to_le_bytes()
    }

    fn encode_ci2(opcode: u32, rd_rs1: u8, imm: i32) -> [u8; 4] {
        let instruction = ((rd_rs1 as u32) << 5) | (((imm as u32) << 6) & (0x3f << 10)) | opcode;
        instruction.to_le_bytes()
    }

    fn encode_cl(opcode: u32, rd_rs2: u8, rs1: u8, imm: i32) -> [u8; 4] {
        let instruction = (((rd_rs2 - 8) as u32) << 5)
            | (((rs1 - 8) as u32) << 8)
            | ((imm as u32) << 9)
            | opcode;
        instruction.to_le_bytes()
    }

    fn encode_cr(opcode: u32, rd_rs1: u8, rs2: u8) -> [u8; 4] {
        let instruction = ((rd_rs1 as u32) << 5) | ((rs2 as u32) << 10) | opcode;
        instruction.to_le_bytes()
    }

    fn encode_type_r(rd: u8, rs1: u8, rs2: u8, func: u16) -> [u8; 4] {
        let instruction = ((rd as u32) << 17)
            | ((rs1 as u32) << 22)
            | ((rs2 as u32) << 27)
            | ((func as u32) << 7)
            | OP_AMO_OPCODE;
        instruction.to_le_bytes()
    }

    #[test]
    fn initializes_and_exposes_register_state() {
        let code = [];
        let mut ram = [0_u8; 16];
        let interpreter = Interpreter::new(&code, &mut ram, 0);

        assert_eq!(interpreter.program_counter(), 0);
        assert_eq!(interpreter.registers(), &[0; EMBIVE_CPU_REGISTER_COUNT]);
    }

    #[test]
    fn syscall_helpers_round_trip_registers() {
        let code = [];
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        interpreter.set_register(10, 1).unwrap();
        interpreter.set_register(11, 2).unwrap();
        interpreter.set_register(12, 3).unwrap();
        interpreter.set_register(16, 7).unwrap();
        interpreter.set_register(17, 99).unwrap();

        let syscall = interpreter.syscall().unwrap();
        assert_eq!(syscall.nr, 99);
        assert_eq!(syscall.args, [1, 2, 3, 0, 0, 0, 7]);

        interpreter.set_syscall_result(-5, 123);
        assert_eq!(interpreter.register(10), Some(-5));
        assert_eq!(interpreter.register(11), Some(123));
    }

    #[test]
    fn executes_ecall() {
        let code = encode_opcode_type_i(SYSTEM_MISC_MEM_OPCODE, 0, MISC_FUNC, 0, ECALL_IMM);
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        assert_eq!(interpreter.step(), Ok(State::Called));
        assert_eq!(interpreter.program_counter(), 4);
    }

    #[test]
    fn executes_ebreak() {
        let code = encode_opcode_type_i(SYSTEM_MISC_MEM_OPCODE, 0, MISC_FUNC, 0, EBREAK_IMM);
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        assert_eq!(interpreter.step(), Ok(State::Halted));
        assert_eq!(interpreter.program_counter(), 4);
    }

    #[test]
    fn executes_wfi() {
        let code = encode_opcode_type_i(SYSTEM_MISC_MEM_OPCODE, 0, MISC_FUNC, 0, WFI_IMM);
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        assert_eq!(interpreter.step(), Ok(State::Waiting));
        assert_eq!(interpreter.program_counter(), 4);
    }

    #[test]
    fn executes_csrrw_interrupt_and_mret() {
        let mut code = [0_u8; 16];
        code[0..4].copy_from_slice(&encode_opcode_type_i(
            SYSTEM_MISC_MEM_OPCODE,
            0,
            CSRRWI_FUNC,
            8,
            MSTATUS_ADDR,
        ));
        code[4..8].copy_from_slice(&encode_opcode_type_i(
            SYSTEM_MISC_MEM_OPCODE,
            0,
            CSRRW_FUNC,
            1,
            MIE_ADDR,
        ));
        code[8..12].copy_from_slice(&encode_opcode_type_i(
            SYSTEM_MISC_MEM_OPCODE,
            0,
            CSRRWI_FUNC,
            12,
            MTVEC_ADDR,
        ));
        code[12..16].copy_from_slice(&encode_opcode_type_i(
            SYSTEM_MISC_MEM_OPCODE,
            0,
            MISC_FUNC,
            0,
            MRET_IMM,
        ));
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        interpreter.set_register(1, 1 << 16).unwrap();
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.interrupt(55), Ok(()));
        assert_eq!(interpreter.program_counter(), 12);
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.program_counter(), 12);
    }

    #[test]
    fn executes_addi() {
        let code = encode_opcode_type_i(OP_IMM_OPCODE, 1, ADDI_FUNC, 2, -7);
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        interpreter.set_register(2, 100).unwrap();
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(1), Some(93));
        assert_eq!(interpreter.program_counter(), 4);
    }

    #[test]
    fn executes_sw_and_lw() {
        let mut code = [0_u8; 8];
        code[0..4].copy_from_slice(&encode_opcode_type_i(OP_IMM_OPCODE, 1, ADDI_FUNC, 0, 4));
        code[4..8].copy_from_slice(&encode_opcode_type_i(LOAD_STORE_OPCODE, 3, SW_FUNC, 2, 4));
        let mut ram = [0_u8; 32];
        {
            let mut interpreter = Interpreter::new(&code, &mut ram, 0);

            interpreter
                .set_register(2, embive_c_sys::EMBIVE_RAM_OFFSET as i32)
                .unwrap();
            interpreter.set_register(3, 0x1234_5678).unwrap();
            assert_eq!(interpreter.step(), Ok(State::Running));
            assert_eq!(interpreter.step(), Ok(State::Running));
            assert_eq!(interpreter.register(1), Some(4));
        }

        let load_code = encode_opcode_type_i(LOAD_STORE_OPCODE, 5, LW_FUNC, 2, 4);
        let mut load_interpreter = Interpreter::new(&load_code, &mut ram, 0);
        load_interpreter
            .set_register(2, embive_c_sys::EMBIVE_RAM_OFFSET as i32)
            .unwrap();
        assert_eq!(load_interpreter.step(), Ok(State::Running));
        assert_eq!(load_interpreter.register(5), Some(0x1234_5678));
    }

    #[test]
    fn executes_op_amo_arithmetic() {
        let mut code = [0_u8; 8];
        code[0..4].copy_from_slice(&encode_type_r(1, 2, 3, ADD_FUNC));
        code[4..8].copy_from_slice(&encode_type_r(4, 5, 6, MULH_FUNC));
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        interpreter.set_register(2, 20).unwrap();
        interpreter.set_register(3, 22).unwrap();
        interpreter.set_register(5, i32::MAX).unwrap();
        interpreter.set_register(6, 2).unwrap();

        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(1), Some(42));
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(4), Some(0));
    }

    #[test]
    fn executes_lui_and_auipc() {
        let mut code = [0_u8; 8];
        code[0..4].copy_from_slice(&encode_type_u(LUI_OPCODE, 1, 0x1234_5000));
        code[4..8].copy_from_slice(&encode_type_u(AUIPC_OPCODE, 2, -0x1000));
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(1), Some(0x1234_5000));
        assert_eq!(interpreter.program_counter(), 4);
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(2), Some(-0xffc));
        assert_eq!(interpreter.program_counter(), 8);
    }

    #[test]
    fn executes_jal_and_jalr() {
        let mut code = [0_u8; 8];
        code[0..4].copy_from_slice(&encode_type_j(1, 8));
        code[4..8].copy_from_slice(&encode_opcode_type_i(JALR_OPCODE, 3, 0, 2, -4));
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(1), Some(4));
        assert_eq!(interpreter.program_counter(), 8);

        interpreter.set_program_counter(4);
        interpreter.set_register(2, 0x200).unwrap();
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(3), Some(8));
        assert_eq!(interpreter.program_counter(), 0x1fc);
    }

    #[test]
    fn executes_branches() {
        let mut code = [0_u8; 8];
        code[0..4].copy_from_slice(&encode_type_b(BEQ_FUNC, 1, 2, 8));
        code[4..8].copy_from_slice(&encode_type_b(BNE_FUNC, 3, 4, -8));
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);
        interpreter.set_register(1, 7).unwrap();
        interpreter.set_register(2, 7).unwrap();

        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.program_counter(), 8);

        interpreter.set_program_counter(4);
        interpreter.set_register(3, 1).unwrap();
        interpreter.set_register(4, 2).unwrap();
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.program_counter(), 0xffff_fffc);
    }

    #[test]
    fn executes_compressed_add_and_load_store() {
        let mut ram = [0_u8; 32];
        {
            let mut code = [0_u8; 6];
            code[0..4].copy_from_slice(&encode_ci1(3, 9, 5));
            code[2..6].copy_from_slice(&encode_cl(2, 8, 9, 4));
            let mut interpreter = Interpreter::new(&code, &mut ram, 0);
            interpreter
                .set_register(8, 0x7856_3412u32 as i32)
                .unwrap();
            interpreter
                .set_register(9, embive_c_sys::EMBIVE_RAM_OFFSET as i32)
                .unwrap();
            assert_eq!(interpreter.step(), Ok(State::Running));
            assert_eq!(interpreter.register(9), Some(embive_c_sys::EMBIVE_RAM_OFFSET as i32 + 5));
        }

        let mut store_code = [0_u8; 4];
        store_code[0..4].copy_from_slice(&encode_cl(2, 8, 9, 4));
        let mut store_interpreter = Interpreter::new(&store_code, &mut ram, 0);
        store_interpreter
            .set_register(8, 0x7856_3412u32 as i32)
            .unwrap();
        store_interpreter
            .set_register(9, embive_c_sys::EMBIVE_RAM_OFFSET as i32)
            .unwrap();
        assert_eq!(store_interpreter.step(), Ok(State::Running));

        let load_code = encode_cl(1, 8, 9, 4);
        let mut load_interpreter = Interpreter::new(&load_code, &mut ram, 0);
        load_interpreter
            .set_register(9, embive_c_sys::EMBIVE_RAM_OFFSET as i32)
            .unwrap();
        assert_eq!(load_interpreter.step(), Ok(State::Running));
        assert_eq!(load_interpreter.register(8), Some(0x7856_3412u32 as i32));
    }

    #[test]
    fn executes_compressed_addi16sp_negative() {
        let code = encode_ci2(6, 2, -0x200);
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);
        interpreter.set_register(2, 0x424).unwrap();

        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(2), Some(0x224));
        assert_eq!(interpreter.program_counter(), 2);
    }

    #[test]
    fn executes_compressed_jr_mv_and_ebreak() {
        let mut code = [0_u8; 6];
        code[0..4].copy_from_slice(&encode_cr(20, 1, 2));
        code[2..6].copy_from_slice(&encode_cr(21, 0, 0));
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);
        interpreter.set_register(2, 44).unwrap();
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(1), Some(44));
        interpreter.set_program_counter(2);
        assert_eq!(interpreter.step(), Ok(State::Halted));
    }

    #[test]
    fn executes_atomic_lr_sc() {
        let mut code = [0_u8; 8];
        code[0..4].copy_from_slice(&encode_type_r(1, 2, 0, LR_FUNC));
        code[4..8].copy_from_slice(&encode_type_r(3, 2, 4, SC_FUNC));
        let mut ram = [0_u8; 16];
        ram[0..4].copy_from_slice(&123i32.to_le_bytes());
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);
        interpreter
            .set_register(2, embive_c_sys::EMBIVE_RAM_OFFSET as i32)
            .unwrap();
        interpreter.set_register(4, 456).unwrap();
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(1), Some(123));
        assert_eq!(interpreter.step(), Ok(State::Running));
        assert_eq!(interpreter.register(3), Some(0));
        assert_eq!(&ram[0..4], &456i32.to_le_bytes());
    }

    #[test]
    fn unsupported_opcode_is_still_unimplemented() {
        let code = encode_type_r(1, 2, 3, 29);
        let mut ram = [0_u8; 16];
        let mut interpreter = Interpreter::new(&code, &mut ram, 0);

        assert_eq!(interpreter.step(), Err(Error::Unimplemented));
    }
}
