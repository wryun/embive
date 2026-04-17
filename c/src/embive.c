#include "embive.h"

#include <stdbool.h>
#include <string.h>

enum {
    EMBIVE_REG_A0 = 10,
    EMBIVE_REG_A1 = 11,
    EMBIVE_REG_A7 = 17,
};

enum {
    EMBIVE_COMPRESSED_REGISTER_OFFSET = 8u,
    EMBIVE_OPCODE_MASK = 0x1Fu,
    EMBIVE_AUIPC_OPCODE = 23u,
    EMBIVE_BRANCH_OPCODE = 24u,
    EMBIVE_JAL_OPCODE = 25u,
    EMBIVE_JALR_OPCODE = 26u,
    EMBIVE_LOAD_STORE_OPCODE = 27u,
    EMBIVE_LUI_OPCODE = 28u,
    EMBIVE_OP_IMM_OPCODE = 29u,
    EMBIVE_OP_AMO_OPCODE = 30u,
    EMBIVE_SYSTEM_MISC_MEM_OPCODE = 31u,
    EMBIVE_LB_FUNC = 0u,
    EMBIVE_LH_FUNC = 1u,
    EMBIVE_LW_FUNC = 2u,
    EMBIVE_LBU_FUNC = 3u,
    EMBIVE_LHU_FUNC = 4u,
    EMBIVE_SB_FUNC = 5u,
    EMBIVE_SH_FUNC = 6u,
    EMBIVE_SW_FUNC = 7u,
    EMBIVE_ADDI_FUNC = 0u,
    EMBIVE_SLLI_FUNC = 1u,
    EMBIVE_SLTI_FUNC = 2u,
    EMBIVE_SLTIU_FUNC = 3u,
    EMBIVE_XORI_FUNC = 4u,
    EMBIVE_SRLI_SRAI_FUNC = 5u,
    EMBIVE_ORI_FUNC = 6u,
    EMBIVE_ANDI_FUNC = 7u,
    EMBIVE_ADD_FUNC = 0u,
    EMBIVE_SUB_FUNC = 1u,
    EMBIVE_SLL_FUNC = 2u,
    EMBIVE_SLT_FUNC = 3u,
    EMBIVE_SLTU_FUNC = 4u,
    EMBIVE_XOR_FUNC = 5u,
    EMBIVE_SRL_FUNC = 6u,
    EMBIVE_SRA_FUNC = 7u,
    EMBIVE_OR_FUNC = 8u,
    EMBIVE_AND_FUNC = 9u,
    EMBIVE_MUL_FUNC = 10u,
    EMBIVE_MULH_FUNC = 11u,
    EMBIVE_MULHSU_FUNC = 12u,
    EMBIVE_MULHU_FUNC = 13u,
    EMBIVE_DIV_FUNC = 14u,
    EMBIVE_DIVU_FUNC = 15u,
    EMBIVE_REM_FUNC = 16u,
    EMBIVE_REMU_FUNC = 17u,
    EMBIVE_BEQ_FUNC = 0u,
    EMBIVE_BNE_FUNC = 1u,
    EMBIVE_BLT_FUNC = 2u,
    EMBIVE_BGE_FUNC = 3u,
    EMBIVE_BLTU_FUNC = 4u,
    EMBIVE_BGEU_FUNC = 5u,
    EMBIVE_MISC_FUNC = 0u,
    EMBIVE_CSRRW_FUNC = 1u,
    EMBIVE_CSRRS_FUNC = 2u,
    EMBIVE_CSRRC_FUNC = 3u,
    EMBIVE_CSRRWI_FUNC = 4u,
    EMBIVE_CSRRSI_FUNC = 5u,
    EMBIVE_CSRRCI_FUNC = 6u,
    EMBIVE_ECALL_IMM = 0u,
    EMBIVE_EBREAK_IMM = 1u,
    EMBIVE_FENCEI_IMM = 2u,
    EMBIVE_WFI_IMM = 3u,
    EMBIVE_MRET_IMM = 4u,
    EMBIVE_INSTRUCTION_SIZE = 4u,
    EMBIVE_COMPRESSED_INSTRUCTION_SIZE = 2u,
    EMBIVE_MSTATUS_MIE = 1u << 3,
    EMBIVE_MSTATUS_MPIE = 1u << 7,
    EMBIVE_MSTATUS_MASK = EMBIVE_MSTATUS_MIE | EMBIVE_MSTATUS_MPIE,
    EMBIVE_MTVEC_MODE_MASK = 0x3u,
    EMBIVE_MEPC_BIT0 = 0x1u,
    EMBIVE_MCAUSE_INTERRUPT = 1u << 31,
    EMBIVE_MI_E_P_MASK = 1u << EMBIVE_INTERRUPT_CODE,
    EMBIVE_MXL_32 = 0x1u,
    EMBIVE_MISA_A = 1u << 0,
    EMBIVE_MISA_I = 1u << 8,
    EMBIVE_MISA_M = 1u << 12,
};

enum {
    EMBIVE_MSTATUS_ADDR = 0x300,
    EMBIVE_MISA_ADDR = 0x301,
    EMBIVE_MIE_ADDR = 0x304,
    EMBIVE_MTVEC_ADDR = 0x305,
    EMBIVE_MSTATUSH_ADDR = 0x310,
    EMBIVE_MCOUNTINHIBIT_ADDR = 0x320,
    EMBIVE_MHPMEVENT31H_ADDR = 0x33F,
    EMBIVE_MSCRATCH_ADDR = 0x340,
    EMBIVE_MEPC_ADDR = 0x341,
    EMBIVE_MCAUSE_ADDR = 0x342,
    EMBIVE_MTVAL_ADDR = 0x343,
    EMBIVE_MIP_ADDR = 0x344,
    EMBIVE_MCYCLE_ADDR = 0xB00,
    EMBIVE_MHPMCOUNTER31H_ADDR = 0xB9F,
    EMBIVE_MVENDORID_ADDR = 0xF11,
    EMBIVE_MCONFIGPTR_ADDR = 0xF15,
};

typedef struct {
    uint8_t rd_rs2;
    uint8_t func;
    uint8_t rs1;
    int32_t imm;
} embive_type_i_t;

typedef struct { uint8_t rd; int32_t imm; } embive_type_ciw_t;
typedef struct { uint8_t rd_rs2; uint8_t rs1; int32_t imm; } embive_type_cl_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_type_ci1_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_type_ci2_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_type_ci3_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_type_ci4_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_type_ci5_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_type_cb1_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_type_cb2_t;
typedef struct { uint8_t rs1; int32_t imm; } embive_type_cb4_t;
typedef struct { uint8_t rd_rs1; uint8_t rs2; } embive_type_cr_t;
typedef struct { uint8_t rd_rs1; uint8_t rs2; } embive_type_cs_t;
typedef struct { uint8_t rs2; int32_t imm; } embive_type_css_t;
typedef struct { int32_t imm; } embive_type_cj_t;

typedef struct {
    uint8_t rs1;
    uint8_t rs2;
    int32_t imm;
    uint8_t func;
} embive_type_b_t;

typedef struct {
    uint8_t rd;
    int32_t imm;
} embive_type_u_t;

typedef struct {
    uint8_t rd;
    int32_t imm;
} embive_type_j_t;

typedef struct {
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint16_t func;
} embive_type_r_t;

typedef enum {
    EMBIVE_CS_OP_NONE = 0,
    EMBIVE_CS_OP_WRITE,
    EMBIVE_CS_OP_SET,
    EMBIVE_CS_OP_CLEAR,
} embive_cs_op_kind_t;

typedef struct {
    embive_cs_op_kind_t kind;
    uint32_t value;
} embive_cs_op_t;

static void embive_zero_registers(embive_registers_t *registers) {
    memset(registers, 0, sizeof(*registers));
}

static uint32_t embive_get_misa(void) {
    return (EMBIVE_MXL_32 << 30) | EMBIVE_MISA_I | EMBIVE_MISA_M | EMBIVE_MISA_A;
}

static uint32_t embive_execute_cs_op(embive_cs_op_t op, uint32_t current) {
    switch (op.kind) {
        case EMBIVE_CS_OP_NONE:
            return current;
        case EMBIVE_CS_OP_WRITE:
            return op.value;
        case EMBIVE_CS_OP_SET:
            return current | op.value;
        case EMBIVE_CS_OP_CLEAR:
            return current & ~op.value;
        default:
            return current;
    }
}

embive_error_t embive_memory_slice(
    const embive_interpreter_t *interpreter,
    uint32_t address,
    const uint8_t **slice,
    size_t *len
) {
    size_t start = 0;
    const uint8_t *base = NULL;
    size_t base_len = 0;

    if (address >= EMBIVE_RAM_OFFSET) {
        start = (size_t)(address - EMBIVE_RAM_OFFSET);
        base = interpreter->memory.ram;
        base_len = interpreter->memory.ram_len;
    } else {
        start = (size_t)address;
        base = interpreter->memory.code;
        base_len = interpreter->memory.code_len;
    }

    if (start >= base_len) {
        return EMBIVE_ERR_INVALID_MEMORY_ADDRESS;
    }

    *slice = base + start;
    *len = base_len - start;
    return EMBIVE_OK;
}

static embive_error_t embive_load_bytes(
    embive_interpreter_t *interpreter,
    uint32_t address,
    size_t len,
    const uint8_t **bytes
) {
    size_t available = 0;
    embive_error_t err = embive_memory_slice(interpreter, address, bytes, &available);
    if (err != EMBIVE_OK) {
        return address == interpreter->program_counter
            ? EMBIVE_ERR_INVALID_PROGRAM_COUNTER
            : err;
    }

    if (len > available) {
        return address == interpreter->program_counter
            ? EMBIVE_ERR_INVALID_PROGRAM_COUNTER
            : EMBIVE_ERR_INVALID_MEMORY_ADDRESS;
    }

    return EMBIVE_OK;
}

static embive_error_t embive_load_u32(
    embive_interpreter_t *interpreter,
    uint32_t address,
    uint32_t *value
) {
    const uint8_t *bytes = NULL;
    embive_error_t err = embive_load_bytes(interpreter, address, sizeof(*value), &bytes);
    if (err != EMBIVE_OK) {
        return err;
    }

    *value = ((uint32_t)bytes[0]) |
        ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) |
        ((uint32_t)bytes[3] << 24);
    return EMBIVE_OK;
}

static embive_error_t embive_store_bytes(
    embive_interpreter_t *interpreter,
    uint32_t address,
    const uint8_t *bytes,
    size_t len
) {
    size_t start = 0;
    size_t end = 0;

    if (address < EMBIVE_RAM_OFFSET) {
        return EMBIVE_ERR_INVALID_MEMORY_ADDRESS;
    }

    start = (size_t)(address - EMBIVE_RAM_OFFSET);
    if (start > SIZE_MAX - len) {
        return EMBIVE_ERR_INVALID_MEMORY_ACCESS_LENGTH;
    }

    end = start + len;
    if (end > interpreter->memory.ram_len) {
        return EMBIVE_ERR_INVALID_MEMORY_ADDRESS;
    }

    memcpy(interpreter->memory.ram + start, bytes, len);
    return EMBIVE_OK;
}

static embive_error_t embive_store_u8(
    embive_interpreter_t *interpreter,
    uint32_t address,
    uint8_t value
) {
    return embive_store_bytes(interpreter, address, &value, sizeof(value));
}

static embive_error_t embive_store_u16(
    embive_interpreter_t *interpreter,
    uint32_t address,
    uint16_t value
) {
    uint8_t bytes[2];
    bytes[0] = (uint8_t)(value & 0xFFu);
    bytes[1] = (uint8_t)((value >> 8) & 0xFFu);
    return embive_store_bytes(interpreter, address, bytes, sizeof(bytes));
}

static embive_error_t embive_store_u32(
    embive_interpreter_t *interpreter,
    uint32_t address,
    uint32_t value
) {
    uint8_t bytes[4];
    bytes[0] = (uint8_t)(value & 0xFFu);
    bytes[1] = (uint8_t)((value >> 8) & 0xFFu);
    bytes[2] = (uint8_t)((value >> 16) & 0xFFu);
    bytes[3] = (uint8_t)((value >> 24) & 0xFFu);
    return embive_store_bytes(interpreter, address, bytes, sizeof(bytes));
}

static embive_error_t embive_load_u16(
    embive_interpreter_t *interpreter,
    uint32_t address,
    uint16_t *value
) {
    const uint8_t *bytes = NULL;
    embive_error_t err = embive_load_bytes(interpreter, address, sizeof(*value), &bytes);
    if (err != EMBIVE_OK) {
        return err;
    }

    *value = (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
    return EMBIVE_OK;
}

static embive_error_t embive_load_u8(
    embive_interpreter_t *interpreter,
    uint32_t address,
    uint8_t *value
) {
    const uint8_t *bytes = NULL;
    embive_error_t err = embive_load_bytes(interpreter, address, sizeof(*value), &bytes);
    if (err != EMBIVE_OK) {
        return err;
    }

    *value = bytes[0];
    return EMBIVE_OK;
}

static embive_type_i_t embive_decode_type_i(uint32_t instruction) {
    embive_type_i_t decoded;

    decoded.rd_rs2 = (uint8_t)((instruction >> 10) & 0x1Fu);
    decoded.func = (uint8_t)((instruction >> 7) & 0x07u);
    decoded.rs1 = (uint8_t)((instruction >> 15) & 0x1Fu);
    decoded.imm = ((int32_t)instruction) >> 20;

    return decoded;
}

static embive_type_ciw_t embive_decode_type_ciw(uint32_t instruction) {
    embive_type_ciw_t decoded;
    decoded.rd = (uint8_t)(((instruction >> 5) & 0x07u) + EMBIVE_COMPRESSED_REGISTER_OFFSET);
    decoded.imm = (int32_t)((instruction & (0xFFu << 8)) >> 6);
    return decoded;
}

static embive_type_cl_t embive_decode_type_cl(uint32_t instruction) {
    embive_type_cl_t decoded;
    decoded.rd_rs2 = (uint8_t)(((instruction >> 5) & 0x07u) + EMBIVE_COMPRESSED_REGISTER_OFFSET);
    decoded.rs1 = (uint8_t)(((instruction >> 8) & 0x07u) + EMBIVE_COMPRESSED_REGISTER_OFFSET);
    decoded.imm = (int32_t)((instruction & (0x1Fu << 11)) >> 9);
    return decoded;
}

static embive_type_ci1_t embive_decode_type_ci1(uint32_t instruction) {
    embive_type_ci1_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int16_t)(instruction & (0x3Fu << 10))) >> 10;
    return decoded;
}

static embive_type_ci2_t embive_decode_type_ci2(uint32_t instruction) {
    embive_type_ci2_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int16_t)(instruction & (0x3Fu << 10))) >> 6;
    return decoded;
}

static embive_type_ci3_t embive_decode_type_ci3(uint32_t instruction) {
    embive_type_ci3_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int32_t)(int16_t)(instruction & (0x3Fu << 10))) << 2;
    return decoded;
}

static embive_type_ci4_t embive_decode_type_ci4(uint32_t instruction) {
    embive_type_ci4_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 10);
    return decoded;
}

static embive_type_ci5_t embive_decode_type_ci5(uint32_t instruction) {
    embive_type_ci5_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 8);
    return decoded;
}

static embive_type_cb1_t embive_decode_type_cb1(uint32_t instruction) {
    embive_type_cb1_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 10);
    return decoded;
}

static embive_type_cb2_t embive_decode_type_cb2(uint32_t instruction) {
    embive_type_cb2_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int16_t)(instruction & (0x3Fu << 10))) >> 10;
    return decoded;
}

static embive_type_cb4_t embive_decode_type_cb4(uint32_t instruction) {
    embive_type_cb4_t decoded;
    decoded.rs1 = (uint8_t)(((instruction >> 5) & 0x07u) + EMBIVE_COMPRESSED_REGISTER_OFFSET);
    decoded.imm = ((int16_t)(instruction & (0xFFu << 8))) >> 7;
    return decoded;
}

static embive_type_cr_t embive_decode_type_cr(uint32_t instruction) {
    embive_type_cr_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.rs2 = (uint8_t)((instruction >> 10) & 0x1Fu);
    return decoded;
}

static embive_type_cs_t embive_decode_type_cs(uint32_t instruction) {
    embive_type_cs_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.rs2 = (uint8_t)((instruction >> 10) & 0x1Fu);
    return decoded;
}

static embive_type_css_t embive_decode_type_css(uint32_t instruction) {
    embive_type_css_t decoded;
    decoded.rs2 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 8);
    return decoded;
}

static embive_type_cj_t embive_decode_type_cj(uint32_t instruction) {
    embive_type_cj_t decoded;
    decoded.imm = ((int16_t)(instruction & (0x7FFu << 5))) >> 4;
    return decoded;
}

static embive_type_b_t embive_decode_type_b(uint32_t instruction) {
    embive_type_b_t decoded;

    decoded.imm = ((int32_t)(instruction & (0xFFFu << 20))) >> 19;
    decoded.func = (uint8_t)((instruction >> 7) & 0x07u);
    decoded.rs1 = (uint8_t)((instruction >> 10) & 0x1Fu);
    decoded.rs2 = (uint8_t)((instruction >> 15) & 0x1Fu);

    return decoded;
}

static embive_type_u_t embive_decode_type_u(uint32_t instruction) {
    embive_type_u_t decoded;

    decoded.rd = (uint8_t)((instruction >> 7) & 0x1Fu);
    decoded.imm = (int32_t)(instruction & (0xFFFFFu << 12));

    return decoded;
}

static embive_type_j_t embive_decode_type_j(uint32_t instruction) {
    embive_type_j_t decoded;

    decoded.rd = (uint8_t)((instruction >> 7) & 0x1Fu);
    decoded.imm = ((int32_t)(instruction & (0xFFFFFu << 12))) >> 11;

    return decoded;
}

static embive_type_r_t embive_decode_type_r(uint32_t instruction) {
    embive_type_r_t decoded;

    decoded.rd = (uint8_t)((instruction >> 17) & 0x1Fu);
    decoded.rs1 = (uint8_t)((instruction >> 22) & 0x1Fu);
    decoded.rs2 = (uint8_t)((instruction >> 27) & 0x1Fu);
    decoded.func = (uint16_t)((instruction >> 7) & 0x03FFu);

    return decoded;
}

static embive_error_t embive_cs_operation(
    embive_registers_t *registers,
    embive_cs_op_t op,
    uint16_t addr,
    uint32_t *ret
) {
    if ((addr >= EMBIVE_MCOUNTINHIBIT_ADDR && addr <= EMBIVE_MHPMEVENT31H_ADDR) ||
        (addr >= EMBIVE_MCYCLE_ADDR && addr <= EMBIVE_MHPMCOUNTER31H_ADDR) ||
        (addr >= EMBIVE_MVENDORID_ADDR && addr <= EMBIVE_MCONFIGPTR_ADDR)) {
        *ret = 0;
        return EMBIVE_OK;
    }

    switch (addr) {
        case EMBIVE_MSTATUS_ADDR:
            *ret = registers->mstatus;
            registers->mstatus = (uint8_t)(embive_execute_cs_op(op, *ret) & EMBIVE_MSTATUS_MASK);
            return EMBIVE_OK;
        case EMBIVE_MISA_ADDR:
            *ret = embive_get_misa();
            return EMBIVE_OK;
        case EMBIVE_MIE_ADDR:
            *ret = ((uint32_t)registers->mie_embive) << EMBIVE_INTERRUPT_CODE;
            registers->mie_embive =
                (uint8_t)((embive_execute_cs_op(op, *ret) & EMBIVE_MI_E_P_MASK) != 0u);
            return EMBIVE_OK;
        case EMBIVE_MTVEC_ADDR:
            *ret = registers->mtvec;
            registers->mtvec = embive_execute_cs_op(op, *ret) & ~EMBIVE_MTVEC_MODE_MASK;
            return EMBIVE_OK;
        case EMBIVE_MSTATUSH_ADDR:
            *ret = 0;
            return EMBIVE_OK;
        case EMBIVE_MSCRATCH_ADDR:
            *ret = registers->mscratch;
            registers->mscratch = embive_execute_cs_op(op, *ret);
            return EMBIVE_OK;
        case EMBIVE_MEPC_ADDR:
            *ret = registers->mepc;
            registers->mepc = embive_execute_cs_op(op, *ret) & ~EMBIVE_MEPC_BIT0;
            return EMBIVE_OK;
        case EMBIVE_MCAUSE_ADDR:
            *ret = registers->mcause;
            registers->mcause = embive_execute_cs_op(op, *ret);
            return EMBIVE_OK;
        case EMBIVE_MTVAL_ADDR:
            *ret = (uint32_t)registers->mtval;
            registers->mtval = (int32_t)embive_execute_cs_op(op, *ret);
            return EMBIVE_OK;
        case EMBIVE_MIP_ADDR:
            *ret = ((uint32_t)registers->mip_embive) << EMBIVE_INTERRUPT_CODE;
            registers->mip_embive =
                (uint8_t)((embive_execute_cs_op(op, *ret) & EMBIVE_MI_E_P_MASK) != 0u);
            return EMBIVE_OK;
        default:
            return EMBIVE_ERR_INVALID_CS_REGISTER;
    }
}

static bool embive_interrupt_enabled(const embive_registers_t *registers) {
    return registers->mie_embive != 0u && (registers->mstatus & EMBIVE_MSTATUS_MIE) != 0u;
}

static void embive_trap_entry(
    embive_interpreter_t *interpreter,
    int32_t value
) {
    if ((interpreter->registers.mstatus & EMBIVE_MSTATUS_MIE) != 0u) {
        interpreter->registers.mstatus |= EMBIVE_MSTATUS_MPIE;
    } else {
        interpreter->registers.mstatus &= (uint8_t)~EMBIVE_MSTATUS_MPIE;
    }

    interpreter->registers.mstatus &= (uint8_t)~EMBIVE_MSTATUS_MIE;
    interpreter->registers.mcause = EMBIVE_MCAUSE_INTERRUPT | EMBIVE_INTERRUPT_CODE;
    interpreter->registers.mepc = interpreter->program_counter;
    interpreter->registers.mtval = value;
    interpreter->registers.mip_embive = 1u;
    interpreter->program_counter = interpreter->registers.mtvec & ~EMBIVE_MTVEC_MODE_MASK;
}

static uint32_t embive_trap_return(embive_interpreter_t *interpreter) {
    if ((interpreter->registers.mstatus & EMBIVE_MSTATUS_MPIE) != 0u) {
        interpreter->registers.mstatus |= EMBIVE_MSTATUS_MIE;
    } else {
        interpreter->registers.mstatus &= (uint8_t)~EMBIVE_MSTATUS_MIE;
    }

    return interpreter->registers.mepc;
}

static embive_error_t embive_execute_system_misc_mem(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_i_t decoded = embive_decode_type_i(instruction);
    uint32_t csr_value = 0;
    embive_cs_op_t op = {EMBIVE_CS_OP_NONE, 0};

    if (decoded.func == EMBIVE_MISC_FUNC) {
        switch ((uint32_t)decoded.imm) {
            case EMBIVE_ECALL_IMM:
                interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_CALLED;
                return EMBIVE_OK;
            case EMBIVE_EBREAK_IMM:
                interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_HALTED;
                return EMBIVE_OK;
            case EMBIVE_FENCEI_IMM:
                interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_RUNNING;
                return EMBIVE_OK;
            case EMBIVE_WFI_IMM:
                interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_WAITING;
                return EMBIVE_OK;
            case EMBIVE_MRET_IMM:
                interpreter->program_counter = embive_trap_return(interpreter);
                *state = EMBIVE_STATE_RUNNING;
                return EMBIVE_OK;
            default:
                return EMBIVE_ERR_INVALID_INSTRUCTION;
        }
    }

    switch (decoded.func) {
        case EMBIVE_CSRRW_FUNC:
            op.kind = EMBIVE_CS_OP_WRITE;
            op.value = (uint32_t)interpreter->registers.x[decoded.rs1];
            break;
        case EMBIVE_CSRRS_FUNC:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_SET;
                op.value = (uint32_t)interpreter->registers.x[decoded.rs1];
            }
            break;
        case EMBIVE_CSRRC_FUNC:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_CLEAR;
                op.value = (uint32_t)interpreter->registers.x[decoded.rs1];
            }
            break;
        case EMBIVE_CSRRWI_FUNC:
            op.kind = EMBIVE_CS_OP_WRITE;
            op.value = decoded.rs1;
            break;
        case EMBIVE_CSRRSI_FUNC:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_SET;
                op.value = decoded.rs1;
            }
            break;
        case EMBIVE_CSRRCI_FUNC:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_CLEAR;
                op.value = decoded.rs1;
            }
            break;
        default:
            return EMBIVE_ERR_INVALID_INSTRUCTION;
    }

    {
        embive_error_t err = embive_cs_operation(
            &interpreter->registers,
            op,
            (uint16_t)(decoded.imm & 0x0FFF),
            &csr_value
        );
        if (err != EMBIVE_OK) {
            return err;
        }
    }

    if (decoded.rd_rs2 != 0u) {
        interpreter->registers.x[decoded.rd_rs2] = (int32_t)csr_value;
    }

    interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_op_imm(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_i_t decoded = embive_decode_type_i(instruction);
    int32_t rs1 = interpreter->registers.x[decoded.rs1];
    int32_t result = 0;
    uint32_t shamt = ((uint32_t)decoded.imm) & 0x1Fu;

    if (decoded.rd_rs2 != 0u) {
        switch (decoded.func) {
            case EMBIVE_ADDI_FUNC:
                result = rs1 + decoded.imm;
                break;
            case EMBIVE_SLLI_FUNC:
                result = (int32_t)((uint32_t)rs1 << shamt);
                break;
            case EMBIVE_SLTI_FUNC:
                result = rs1 < decoded.imm ? 1 : 0;
                break;
            case EMBIVE_SLTIU_FUNC:
                result = ((uint32_t)rs1 < (uint32_t)decoded.imm) ? 1 : 0;
                break;
            case EMBIVE_XORI_FUNC:
                result = rs1 ^ decoded.imm;
                break;
            case EMBIVE_SRLI_SRAI_FUNC:
                if ((decoded.imm & (1 << 10)) != 0) {
                    result = rs1 >> shamt;
                } else {
                    result = (int32_t)(((uint32_t)rs1) >> shamt);
                }
                break;
            case EMBIVE_ORI_FUNC:
                result = rs1 | decoded.imm;
                break;
            case EMBIVE_ANDI_FUNC:
                result = rs1 & decoded.imm;
                break;
            default:
                return EMBIVE_ERR_INVALID_INSTRUCTION;
        }

        interpreter->registers.x[decoded.rd_rs2] = result;
    }

    interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_load_store(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_i_t decoded = embive_decode_type_i(instruction);
    uint32_t address = (uint32_t)interpreter->registers.x[decoded.rs1] + (uint32_t)decoded.imm;
    uint8_t u8_value = 0;
    uint16_t u16_value = 0;
    uint32_t u32_value = 0;
    int32_t rs2 = 0;
    embive_error_t err = EMBIVE_OK;

    switch (decoded.func) {
        case EMBIVE_LB_FUNC:
            err = embive_load_u8(interpreter, address, &u8_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)(int8_t)u8_value;
            break;
        case EMBIVE_LH_FUNC:
            err = embive_load_u16(interpreter, address, &u16_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)(int16_t)u16_value;
            break;
        case EMBIVE_LW_FUNC:
            err = embive_load_u32(interpreter, address, &u32_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)u32_value;
            break;
        case EMBIVE_LBU_FUNC:
            err = embive_load_u8(interpreter, address, &u8_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)u8_value;
            break;
        case EMBIVE_LHU_FUNC:
            err = embive_load_u16(interpreter, address, &u16_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)u16_value;
            break;
        case EMBIVE_SB_FUNC:
            rs2 = interpreter->registers.x[decoded.rd_rs2];
            err = embive_store_u8(interpreter, address, (uint8_t)rs2);
            if (err != EMBIVE_OK) {
                return err;
            }
            break;
        case EMBIVE_SH_FUNC:
            rs2 = interpreter->registers.x[decoded.rd_rs2];
            err = embive_store_u16(interpreter, address, (uint16_t)rs2);
            if (err != EMBIVE_OK) {
                return err;
            }
            break;
        case EMBIVE_SW_FUNC:
            rs2 = interpreter->registers.x[decoded.rd_rs2];
            err = embive_store_u32(interpreter, address, (uint32_t)rs2);
            if (err != EMBIVE_OK) {
                return err;
            }
            break;
        default:
            return EMBIVE_ERR_INVALID_INSTRUCTION;
    }

    interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static int32_t embive_div_signed(int32_t lhs, int32_t rhs) {
    if (rhs == 0) {
        return -1;
    }
    if (lhs == INT32_MIN && rhs == -1) {
        return INT32_MIN;
    }
    return lhs / rhs;
}

static int32_t embive_rem_signed(int32_t lhs, int32_t rhs) {
    if (rhs == 0) {
        return lhs;
    }
    if (lhs == INT32_MIN && rhs == -1) {
        return 0;
    }
    return lhs % rhs;
}

static embive_error_t embive_execute_op_amo(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_r_t decoded = embive_decode_type_r(instruction);
    int32_t rs1 = interpreter->registers.x[decoded.rs1];
    int32_t rs2 = interpreter->registers.x[decoded.rs2];
    int32_t result = 0;

    switch (decoded.func) {
        case EMBIVE_ADD_FUNC:
            result = rs1 + rs2;
            break;
        case EMBIVE_SUB_FUNC:
            result = rs1 - rs2;
            break;
        case EMBIVE_SLL_FUNC:
            result = (int32_t)(((uint32_t)rs1) << ((uint32_t)rs2 & 31u));
            break;
        case EMBIVE_SLT_FUNC:
            result = rs1 < rs2 ? 1 : 0;
            break;
        case EMBIVE_SLTU_FUNC:
            result = ((uint32_t)rs1 < (uint32_t)rs2) ? 1 : 0;
            break;
        case EMBIVE_XOR_FUNC:
            result = rs1 ^ rs2;
            break;
        case EMBIVE_SRL_FUNC:
            result = (int32_t)(((uint32_t)rs1) >> ((uint32_t)rs2 & 31u));
            break;
        case EMBIVE_SRA_FUNC:
            result = rs1 >> ((uint32_t)rs2 & 31u);
            break;
        case EMBIVE_OR_FUNC:
            result = rs1 | rs2;
            break;
        case EMBIVE_AND_FUNC:
            result = rs1 & rs2;
            break;
        case EMBIVE_MUL_FUNC:
            result = (int32_t)((uint32_t)rs1 * (uint32_t)rs2);
            break;
        case EMBIVE_MULH_FUNC:
            result = (int32_t)(((int64_t)rs1 * (int64_t)rs2) >> 32);
            break;
        case EMBIVE_MULHSU_FUNC:
            result = (int32_t)(((int64_t)rs1 * (int64_t)(uint32_t)rs2) >> 32);
            break;
        case EMBIVE_MULHU_FUNC:
            result = (int32_t)(((uint64_t)(uint32_t)rs1 * (uint64_t)(uint32_t)rs2) >> 32);
            break;
        case EMBIVE_DIV_FUNC:
            result = embive_div_signed(rs1, rs2);
            break;
        case EMBIVE_DIVU_FUNC:
            result = rs2 == 0 ? -1 : (int32_t)((uint32_t)rs1 / (uint32_t)rs2);
            break;
        case EMBIVE_REM_FUNC:
            result = embive_rem_signed(rs1, rs2);
            break;
        case EMBIVE_REMU_FUNC:
            result = rs2 == 0 ? rs1 : (int32_t)((uint32_t)rs1 % (uint32_t)rs2);
            break;
        default:
        {
            uint32_t address = (uint32_t)rs1;
            uint32_t loaded = 0;
            embive_error_t err = embive_load_u32(interpreter, address, &loaded);
            if (err != EMBIVE_OK) return err;
            result = (int32_t)loaded;

            switch (decoded.func) {
                case 18:
                    interpreter->has_reservation = 1u;
                    interpreter->reservation_addr = address;
                    interpreter->reservation_value = result;
                    break;
                case 19:
                    if (interpreter->has_reservation &&
                        interpreter->reservation_addr == address &&
                        interpreter->reservation_value == result) {
                        err = embive_store_u32(interpreter, address, (uint32_t)rs2);
                        if (err != EMBIVE_OK) return err;
                        result = 0;
                    } else {
                        result = 1;
                    }
                    interpreter->has_reservation = 0u;
                    break;
                case 20:
                    err = embive_store_u32(interpreter, address, (uint32_t)rs2);
                    if (err != EMBIVE_OK) return err;
                    break;
                case 21:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result + rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case 22:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result ^ rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case 23:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result & rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case 24:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result | rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case 25:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result < rs2 ? result : rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case 26:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result > rs2 ? result : rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case 27:
                    err = embive_store_u32(interpreter, address, ((uint32_t)result < (uint32_t)rs2) ? (uint32_t)result : (uint32_t)rs2);
                    if (err != EMBIVE_OK) return err;
                    break;
                case 28:
                    err = embive_store_u32(interpreter, address, ((uint32_t)result > (uint32_t)rs2) ? (uint32_t)result : (uint32_t)rs2);
                    if (err != EMBIVE_OK) return err;
                    break;
                default:
                    return EMBIVE_ERR_UNIMPLEMENTED;
            }
            break;
        }
    }

    if (decoded.rd != 0u) {
        interpreter->registers.x[decoded.rd] = result;
    }

    interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_auipc(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_u_t decoded = embive_decode_type_u(instruction);

    if (decoded.rd != 0u) {
        interpreter->registers.x[decoded.rd] =
            (int32_t)(interpreter->program_counter + (uint32_t)decoded.imm);
    }

    interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_branch(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_b_t decoded = embive_decode_type_b(instruction);
    int32_t rs1 = interpreter->registers.x[decoded.rs1];
    int32_t rs2 = interpreter->registers.x[decoded.rs2];
    bool branch = false;

    switch (decoded.func) {
        case EMBIVE_BEQ_FUNC:
            branch = rs1 == rs2;
            break;
        case EMBIVE_BNE_FUNC:
            branch = rs1 != rs2;
            break;
        case EMBIVE_BLT_FUNC:
            branch = rs1 < rs2;
            break;
        case EMBIVE_BGE_FUNC:
            branch = rs1 >= rs2;
            break;
        case EMBIVE_BLTU_FUNC:
            branch = (uint32_t)rs1 < (uint32_t)rs2;
            break;
        case EMBIVE_BGEU_FUNC:
            branch = (uint32_t)rs1 >= (uint32_t)rs2;
            break;
        default:
            return EMBIVE_ERR_INVALID_INSTRUCTION;
    }

    if (branch) {
        interpreter->program_counter += (uint32_t)decoded.imm;
    } else {
        interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
    }

    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_jal(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_j_t decoded = embive_decode_type_j(instruction);

    if (decoded.rd != 0u) {
        interpreter->registers.x[decoded.rd] =
            (int32_t)(interpreter->program_counter + EMBIVE_INSTRUCTION_SIZE);
    }

    interpreter->program_counter += (uint32_t)decoded.imm;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_jalr(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_i_t decoded = embive_decode_type_i(instruction);
    int32_t rs1 = interpreter->registers.x[decoded.rs1];

    if (decoded.rd_rs2 != 0u) {
        interpreter->registers.x[decoded.rd_rs2] =
            (int32_t)(interpreter->program_counter + EMBIVE_INSTRUCTION_SIZE);
    }

    interpreter->program_counter = (uint32_t)rs1 + (uint32_t)decoded.imm;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_lui(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_type_u_t decoded = embive_decode_type_u(instruction);

    if (decoded.rd != 0u) {
        interpreter->registers.x[decoded.rd] = decoded.imm;
    }

    interpreter->program_counter += EMBIVE_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_compressed(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    switch (instruction & EMBIVE_OPCODE_MASK) {
        case 0: {
            embive_type_ciw_t decoded = embive_decode_type_ciw(instruction);
            int32_t sp = interpreter->registers.x[2];
            if (decoded.imm == 0) {
                return EMBIVE_ERR_ILLEGAL_INSTRUCTION;
            }
            interpreter->registers.x[decoded.rd] = sp + decoded.imm;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 1: {
            embive_type_cl_t decoded = embive_decode_type_cl(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[decoded.rs1] + (uint32_t)decoded.imm;
            uint32_t value = 0;
            embive_error_t err = embive_load_u32(interpreter, address, &value);
            if (err != EMBIVE_OK) return err;
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)value;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 2: {
            embive_type_cl_t decoded = embive_decode_type_cl(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[decoded.rs1] + (uint32_t)decoded.imm;
            embive_error_t err = embive_store_u32(interpreter, address, (uint32_t)interpreter->registers.x[decoded.rd_rs2]);
            if (err != EMBIVE_OK) return err;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 3: {
            embive_type_ci1_t decoded = embive_decode_type_ci1(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] += decoded.imm;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 4: {
            embive_type_cj_t decoded = embive_decode_type_cj(instruction);
            interpreter->registers.x[1] = (int32_t)(interpreter->program_counter + EMBIVE_COMPRESSED_INSTRUCTION_SIZE);
            interpreter->program_counter += (uint32_t)decoded.imm;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 5: {
            embive_type_ci1_t decoded = embive_decode_type_ci1(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] = decoded.imm;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 6: {
            embive_type_ci2_t decoded = embive_decode_type_ci2(instruction);
            interpreter->registers.x[2] += decoded.imm;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 7: {
            embive_type_ci3_t decoded = embive_decode_type_ci3(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] = decoded.imm;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 8: {
            embive_type_cb1_t decoded = embive_decode_type_cb1(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] = (int32_t)(((uint32_t)interpreter->registers.x[decoded.rd_rs1]) >> (uint32_t)decoded.imm);
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 9: {
            embive_type_cb1_t decoded = embive_decode_type_cb1(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] >>= (uint32_t)decoded.imm;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 10: {
            embive_type_cb2_t decoded = embive_decode_type_cb2(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] &= decoded.imm;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 11:
        case 12:
        case 13:
        case 14: {
            embive_type_cs_t decoded = embive_decode_type_cs(instruction);
            int32_t rs2 = interpreter->registers.x[decoded.rs2];
            if (decoded.rd_rs1 != 0u) {
                if ((instruction & EMBIVE_OPCODE_MASK) == 11) interpreter->registers.x[decoded.rd_rs1] -= rs2;
                else if ((instruction & EMBIVE_OPCODE_MASK) == 12) interpreter->registers.x[decoded.rd_rs1] ^= rs2;
                else if ((instruction & EMBIVE_OPCODE_MASK) == 13) interpreter->registers.x[decoded.rd_rs1] |= rs2;
                else interpreter->registers.x[decoded.rd_rs1] &= rs2;
            }
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 15: {
            embive_type_cj_t decoded = embive_decode_type_cj(instruction);
            interpreter->program_counter += (uint32_t)decoded.imm;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 16:
        case 17: {
            embive_type_cb4_t decoded = embive_decode_type_cb4(instruction);
            bool cond = interpreter->registers.x[decoded.rs1] == 0;
            if ((instruction & EMBIVE_OPCODE_MASK) == 17) cond = !cond;
            if (cond) interpreter->program_counter += (uint32_t)decoded.imm;
            else interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 18: {
            embive_type_ci4_t decoded = embive_decode_type_ci4(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] = (int32_t)(((uint32_t)interpreter->registers.x[decoded.rd_rs1]) << (uint32_t)decoded.imm);
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 19: {
            embive_type_ci5_t decoded = embive_decode_type_ci5(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[2] + (uint32_t)decoded.imm;
            uint32_t value = 0;
            embive_error_t err = embive_load_u32(interpreter, address, &value);
            if (err != EMBIVE_OK) return err;
            interpreter->registers.x[decoded.rd_rs1] = (int32_t)value;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 20: {
            embive_type_cr_t decoded = embive_decode_type_cr(instruction);
            if (decoded.rs2 == 0u) {
                interpreter->program_counter = (uint32_t)interpreter->registers.x[decoded.rd_rs1];
            } else {
                interpreter->registers.x[decoded.rd_rs1] = interpreter->registers.x[decoded.rs2];
                interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            }
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 21: {
            embive_type_cr_t decoded = embive_decode_type_cr(instruction);
            if (decoded.rs2 == 0u) {
                if (decoded.rd_rs1 == 0u) {
                    interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
                    *state = EMBIVE_STATE_HALTED;
                    return EMBIVE_OK;
                }
                interpreter->registers.x[1] = (int32_t)(interpreter->program_counter + EMBIVE_COMPRESSED_INSTRUCTION_SIZE);
                interpreter->program_counter = (uint32_t)interpreter->registers.x[decoded.rd_rs1];
            } else {
                interpreter->registers.x[decoded.rd_rs1] += interpreter->registers.x[decoded.rs2];
                interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            }
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case 22: {
            embive_type_css_t decoded = embive_decode_type_css(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[2] + (uint32_t)decoded.imm;
            embive_error_t err = embive_store_u32(interpreter, address, (uint32_t)interpreter->registers.x[decoded.rs2]);
            if (err != EMBIVE_OK) return err;
            interpreter->program_counter += EMBIVE_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        default:
            return EMBIVE_ERR_UNIMPLEMENTED;
    }
}

void embive_init(
    embive_interpreter_t *interpreter,
    const uint8_t *code,
    size_t code_len,
    uint8_t *ram,
    size_t ram_len,
    uint32_t instruction_limit
) {
    interpreter->memory.code = code;
    interpreter->memory.code_len = code_len;
    interpreter->memory.ram = ram;
    interpreter->memory.ram_len = ram_len;
    interpreter->instruction_limit = instruction_limit;

    embive_reset(interpreter);
}

void embive_reset(embive_interpreter_t *interpreter) {
    interpreter->program_counter = 0;
    interpreter->has_reservation = 0;
    interpreter->reservation_addr = 0;
    interpreter->reservation_value = 0;

    embive_zero_registers(&interpreter->registers);
}

embive_error_t embive_run(
    embive_interpreter_t *interpreter,
    embive_state_t *state
) {
    uint32_t limit = interpreter->instruction_limit;

    if (limit == 0) {
        for (;;) {
            embive_error_t err = embive_step(interpreter, state);
            if (err != EMBIVE_OK) {
                return err;
            }
            if (*state != EMBIVE_STATE_RUNNING) {
                return EMBIVE_OK;
            }
        }
    }

    for (uint32_t i = 0; i < limit; ++i) {
        embive_error_t err = embive_step(interpreter, state);
        if (err != EMBIVE_OK) {
            return err;
        }
        if (*state != EMBIVE_STATE_RUNNING) {
            return EMBIVE_OK;
        }
    }

    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

embive_error_t embive_step(
    embive_interpreter_t *interpreter,
    embive_state_t *state
) {
    uint32_t instruction = 0;
    embive_error_t err = embive_load_u32(
        interpreter,
        interpreter->program_counter,
        &instruction
    );
    if (err != EMBIVE_OK) {
        return err;
    }

    switch (instruction & EMBIVE_OPCODE_MASK) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
            return embive_execute_compressed(interpreter, instruction, state);
        case EMBIVE_AUIPC_OPCODE:
            return embive_execute_auipc(interpreter, instruction, state);
        case EMBIVE_BRANCH_OPCODE:
            return embive_execute_branch(interpreter, instruction, state);
        case EMBIVE_JAL_OPCODE:
            return embive_execute_jal(interpreter, instruction, state);
        case EMBIVE_JALR_OPCODE:
            return embive_execute_jalr(interpreter, instruction, state);
        case EMBIVE_LOAD_STORE_OPCODE:
            return embive_execute_load_store(interpreter, instruction, state);
        case EMBIVE_LUI_OPCODE:
            return embive_execute_lui(interpreter, instruction, state);
        case EMBIVE_OP_IMM_OPCODE:
            return embive_execute_op_imm(interpreter, instruction, state);
        case EMBIVE_OP_AMO_OPCODE:
            return embive_execute_op_amo(interpreter, instruction, state);
        case EMBIVE_SYSTEM_MISC_MEM_OPCODE:
            return embive_execute_system_misc_mem(interpreter, instruction, state);
        default:
            *state = EMBIVE_STATE_HALTED;
            return EMBIVE_ERR_UNIMPLEMENTED;
    }
}

embive_error_t embive_interrupt(
    embive_interpreter_t *interpreter,
    int32_t value
) {
    if (!embive_interrupt_enabled(&interpreter->registers)) {
        return EMBIVE_ERR_INTERRUPT_NOT_ENABLED;
    }

    embive_trap_entry(interpreter, value);
    return EMBIVE_OK;
}

embive_error_t embive_get_syscall(
    const embive_interpreter_t *interpreter,
    embive_syscall_t *syscall
) {
    syscall->nr = interpreter->registers.x[EMBIVE_REG_A7];
    memcpy(
        syscall->args,
        &interpreter->registers.x[EMBIVE_REG_A0],
        sizeof(syscall->args)
    );
    return EMBIVE_OK;
}

void embive_set_syscall_result(
    embive_interpreter_t *interpreter,
    int32_t error_code,
    int32_t value
) {
    interpreter->registers.x[EMBIVE_REG_A0] = error_code;
    interpreter->registers.x[EMBIVE_REG_A1] = value;
}
