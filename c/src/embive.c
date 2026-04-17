#include "embive.h"

#include <stdbool.h>
#include <string.h>

typedef enum {
    EMBIVE_REGISTER_INDEX_A0 = 10,
    EMBIVE_REGISTER_INDEX_A1 = 11,
    EMBIVE_REGISTER_INDEX_A7 = 17,
} embive_register_index_t;

typedef enum {
    EMBIVE_LAYOUT_COMPRESSED_REGISTER_OFFSET = 8u,
    EMBIVE_LAYOUT_OPCODE_MASK = 0x1Fu,
    EMBIVE_LAYOUT_INSTRUCTION_SIZE = 4u,
    EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE = 2u,
} embive_layout_t;

typedef enum {
    EMBIVE_LOAD_STORE_FUNC_LB = 0u,
    EMBIVE_LOAD_STORE_FUNC_LH = 1u,
    EMBIVE_LOAD_STORE_FUNC_LW = 2u,
    EMBIVE_LOAD_STORE_FUNC_LBU = 3u,
    EMBIVE_LOAD_STORE_FUNC_LHU = 4u,
    EMBIVE_LOAD_STORE_FUNC_SB = 5u,
    EMBIVE_LOAD_STORE_FUNC_SH = 6u,
    EMBIVE_LOAD_STORE_FUNC_SW = 7u,
} embive_load_store_func_t;

typedef enum {
    EMBIVE_OP_IMM_FUNC_ADDI = 0u,
    EMBIVE_OP_IMM_FUNC_SLLI = 1u,
    EMBIVE_OP_IMM_FUNC_SLTI = 2u,
    EMBIVE_OP_IMM_FUNC_SLTIU = 3u,
    EMBIVE_OP_IMM_FUNC_XORI = 4u,
    EMBIVE_OP_IMM_FUNC_SRLI_SRAI = 5u,
    EMBIVE_OP_IMM_FUNC_ORI = 6u,
    EMBIVE_OP_IMM_FUNC_ANDI = 7u,
} embive_op_imm_func_t;

typedef enum {
    EMBIVE_OP_AMO_FUNC_ADD = 0u,
    EMBIVE_OP_AMO_FUNC_SUB = 1u,
    EMBIVE_OP_AMO_FUNC_SLL = 2u,
    EMBIVE_OP_AMO_FUNC_SLT = 3u,
    EMBIVE_OP_AMO_FUNC_SLTU = 4u,
    EMBIVE_OP_AMO_FUNC_XOR = 5u,
    EMBIVE_OP_AMO_FUNC_SRL = 6u,
    EMBIVE_OP_AMO_FUNC_SRA = 7u,
    EMBIVE_OP_AMO_FUNC_OR = 8u,
    EMBIVE_OP_AMO_FUNC_AND = 9u,
    EMBIVE_OP_AMO_FUNC_MUL = 10u,
    EMBIVE_OP_AMO_FUNC_MULH = 11u,
    EMBIVE_OP_AMO_FUNC_MULHSU = 12u,
    EMBIVE_OP_AMO_FUNC_MULHU = 13u,
    EMBIVE_OP_AMO_FUNC_DIV = 14u,
    EMBIVE_OP_AMO_FUNC_DIVU = 15u,
    EMBIVE_OP_AMO_FUNC_REM = 16u,
    EMBIVE_OP_AMO_FUNC_REMU = 17u,
    EMBIVE_OP_AMO_FUNC_LR = 18u,
    EMBIVE_OP_AMO_FUNC_SC = 19u,
    EMBIVE_OP_AMO_FUNC_SWAP = 20u,
    EMBIVE_OP_AMO_FUNC_AMOADD = 21u,
    EMBIVE_OP_AMO_FUNC_AMOXOR = 22u,
    EMBIVE_OP_AMO_FUNC_AMOAND = 23u,
    EMBIVE_OP_AMO_FUNC_AMOOR = 24u,
    EMBIVE_OP_AMO_FUNC_AMOMIN = 25u,
    EMBIVE_OP_AMO_FUNC_AMOMAX = 26u,
    EMBIVE_OP_AMO_FUNC_AMOMINU = 27u,
    EMBIVE_OP_AMO_FUNC_AMOMAXU = 28u,
} embive_op_amo_func_t;

typedef enum {
    EMBIVE_BRANCH_FUNC_BEQ = 0u,
    EMBIVE_BRANCH_FUNC_BNE = 1u,
    EMBIVE_BRANCH_FUNC_BLT = 2u,
    EMBIVE_BRANCH_FUNC_BGE = 3u,
    EMBIVE_BRANCH_FUNC_BLTU = 4u,
    EMBIVE_BRANCH_FUNC_BGEU = 5u,
} embive_branch_func_t;

typedef enum {
    EMBIVE_SYSTEM_FUNC_MISC = 0u,
    EMBIVE_SYSTEM_FUNC_CSRRW = 1u,
    EMBIVE_SYSTEM_FUNC_CSRRS = 2u,
    EMBIVE_SYSTEM_FUNC_CSRRC = 3u,
    EMBIVE_SYSTEM_FUNC_CSRRWI = 4u,
    EMBIVE_SYSTEM_FUNC_CSRRSI = 5u,
    EMBIVE_SYSTEM_FUNC_CSRRCI = 6u,
} embive_system_func_t;

typedef enum {
    EMBIVE_SYSTEM_IMM_ECALL = 0u,
    EMBIVE_SYSTEM_IMM_EBREAK = 1u,
    EMBIVE_SYSTEM_IMM_FENCEI = 2u,
    EMBIVE_SYSTEM_IMM_WFI = 3u,
    EMBIVE_SYSTEM_IMM_MRET = 4u,
} embive_system_imm_t;

typedef enum {
    EMBIVE_MACHINE_CONSTANT_MSTATUS_MIE = 1u << 3,
    EMBIVE_MACHINE_CONSTANT_MSTATUS_MPIE = 1u << 7,
    EMBIVE_MACHINE_CONSTANT_MSTATUS_MASK =
        EMBIVE_MACHINE_CONSTANT_MSTATUS_MIE | EMBIVE_MACHINE_CONSTANT_MSTATUS_MPIE,
    EMBIVE_MACHINE_CONSTANT_MTVEC_MODE_MASK = 0x3u,
    EMBIVE_MACHINE_CONSTANT_MEPC_BIT0_MASK = 0x1u,
    EMBIVE_MACHINE_CONSTANT_MCAUSE_INTERRUPT = 1u << 31,
    EMBIVE_MACHINE_CONSTANT_MIE_MIP_EMBIVE_MASK = 1u << EMBIVE_INTERRUPT_CODE,
    EMBIVE_MACHINE_CONSTANT_MXL_32 = 0x1u,
    EMBIVE_MACHINE_CONSTANT_MISA_A = 1u << 0,
    EMBIVE_MACHINE_CONSTANT_MISA_I = 1u << 8,
    EMBIVE_MACHINE_CONSTANT_MISA_M = 1u << 12,
} embive_machine_constant_t;

typedef enum {
    EMBIVE_CS_ADDR_MSTATUS = 0x300,
    EMBIVE_CS_ADDR_MISA = 0x301,
    EMBIVE_CS_ADDR_MIE = 0x304,
    EMBIVE_CS_ADDR_MTVEC = 0x305,
    EMBIVE_CS_ADDR_MSTATUSH = 0x310,
    EMBIVE_CS_ADDR_MCOUNTINHIBIT = 0x320,
    EMBIVE_CS_ADDR_MHPMEVENT31H = 0x33F,
    EMBIVE_CS_ADDR_MSCRATCH = 0x340,
    EMBIVE_CS_ADDR_MEPC = 0x341,
    EMBIVE_CS_ADDR_MCAUSE = 0x342,
    EMBIVE_CS_ADDR_MTVAL = 0x343,
    EMBIVE_CS_ADDR_MIP = 0x344,
    EMBIVE_CS_ADDR_MCYCLE = 0xB00,
    EMBIVE_CS_ADDR_MHPMCOUNTER31H = 0xB9F,
    EMBIVE_CS_ADDR_MVENDORID = 0xF11,
    EMBIVE_CS_ADDR_MCONFIGPTR = 0xF15,
} embive_cs_addr_t;

typedef struct {
    uint8_t rd_rs2;
    uint8_t func;
    uint8_t rs1;
    int32_t imm;
} embive_i_args_t;

typedef struct { uint8_t rd; int32_t imm; } embive_c_addi4spn_args_t;
typedef struct { uint8_t rd_rs2; uint8_t rs1; int32_t imm; } embive_c_lw_sw_args_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_c_addi_li_args_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_c_addi16sp_args_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_c_lui_args_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_c_lwsp_args_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_c_shift_imm_args_t;
typedef struct { uint8_t rd_rs1; int32_t imm; } embive_c_andi_args_t;
typedef struct { uint8_t rs1; int32_t imm; } embive_c_branch_zero_args_t;
typedef struct { uint8_t rd_rs1; uint8_t rs2; } embive_c_jr_mv_add_args_t;
typedef struct { uint8_t rd_rs1; uint8_t rs2; } embive_c_binary_reg_args_t;
typedef struct { uint8_t rs2; int32_t imm; } embive_c_swsp_args_t;
typedef struct { int32_t imm; } embive_c_jump_args_t;

typedef struct {
    uint8_t rs1;
    uint8_t rs2;
    int32_t imm;
    uint8_t func;
} embive_branch_args_t;

typedef struct {
    uint8_t rd;
    int32_t imm;
} embive_u_args_t;

typedef struct {
    uint8_t rd;
    int32_t imm;
} embive_j_args_t;

typedef struct {
    uint8_t rd;
    uint8_t rs1;
    uint8_t rs2;
    uint16_t func;
} embive_r_args_t;

typedef enum {
    EMBIVE_CS_OP_KIND_NONE = 0,
    EMBIVE_CS_OP_KIND_WRITE,
    EMBIVE_CS_OP_KIND_SET,
    EMBIVE_CS_OP_KIND_CLEAR,
} embive_cs_op_kind_t;

typedef struct {
    embive_cs_op_kind_t kind;
    uint32_t value;
} embive_cs_op_t;

typedef enum {
    /* Compressed instructions */
    EMBIVE_OPCODE_C_ADDI4SPN = 0u,
    EMBIVE_OPCODE_C_LW = 1u,
    EMBIVE_OPCODE_C_SW = 2u,
    EMBIVE_OPCODE_C_ADDI = 3u,
    EMBIVE_OPCODE_C_JAL = 4u,
    EMBIVE_OPCODE_C_LI = 5u,
    EMBIVE_OPCODE_C_ADDI16SP = 6u,
    EMBIVE_OPCODE_C_LUI = 7u,
    EMBIVE_OPCODE_C_SRLI = 8u,
    EMBIVE_OPCODE_C_SRAI = 9u,
    EMBIVE_OPCODE_C_ANDI = 10u,
    EMBIVE_OPCODE_C_SUB = 11u,
    EMBIVE_OPCODE_C_XOR = 12u,
    EMBIVE_OPCODE_C_OR = 13u,
    EMBIVE_OPCODE_C_AND = 14u,
    EMBIVE_OPCODE_C_J = 15u,
    EMBIVE_OPCODE_C_BEQZ = 16u,
    EMBIVE_OPCODE_C_BNEZ = 17u,
    EMBIVE_OPCODE_C_SLLI = 18u,
    EMBIVE_OPCODE_C_LWSP = 19u,
    EMBIVE_OPCODE_C_JR_MV = 20u,
    EMBIVE_OPCODE_C_EBREAK_JALR_ADD = 21u,
    EMBIVE_OPCODE_C_SWSP = 22u,
    /* Other instructions (some groups) */
    EMBIVE_OPCODE_AUIPC = 23u,
    EMBIVE_OPCODE_BRANCH = 24u,
    EMBIVE_OPCODE_JAL = 25u,
    EMBIVE_OPCODE_JALR = 26u,
    EMBIVE_OPCODE_LOAD_STORE = 27u,
    EMBIVE_OPCODE_LUI = 28u,
    EMBIVE_OPCODE_OP_IMM = 29u,
    EMBIVE_OPCODE_OP_AMO = 30u,
    EMBIVE_OPCODE_SYSTEM_MISC_MEM = 31u,
} embive_opcode_t;

static void embive_zero_registers(embive_registers_t *registers) {
    memset(registers, 0, sizeof(*registers));
}

static uint32_t embive_get_misa(void) {
    return (EMBIVE_MACHINE_CONSTANT_MXL_32 << 30) |
        EMBIVE_MACHINE_CONSTANT_MISA_I |
        EMBIVE_MACHINE_CONSTANT_MISA_M |
        EMBIVE_MACHINE_CONSTANT_MISA_A;
}

static uint32_t embive_execute_cs_op(embive_cs_op_t op, uint32_t current) {
    switch (op.kind) {
        case EMBIVE_CS_OP_KIND_NONE:
            return current;
        case EMBIVE_CS_OP_KIND_WRITE:
            return op.value;
        case EMBIVE_CS_OP_KIND_SET:
            return current | op.value;
        case EMBIVE_CS_OP_KIND_CLEAR:
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

static embive_i_args_t embive_decode_i(uint32_t instruction) {
    embive_i_args_t decoded;

    decoded.rd_rs2 = (uint8_t)((instruction >> 10) & 0x1Fu);
    decoded.func = (uint8_t)((instruction >> 7) & 0x07u);
    decoded.rs1 = (uint8_t)((instruction >> 15) & 0x1Fu);
    decoded.imm = ((int32_t)instruction) >> 20;

    return decoded;
}

static embive_c_addi4spn_args_t embive_decode_c_addi4spn(uint32_t instruction) {
    embive_c_addi4spn_args_t decoded;
    decoded.rd = (uint8_t)(((instruction >> 5) & 0x07u) + EMBIVE_LAYOUT_COMPRESSED_REGISTER_OFFSET);
    decoded.imm = (int32_t)((instruction & (0xFFu << 8)) >> 6);
    return decoded;
}

static embive_c_lw_sw_args_t embive_decode_c_lw_sw(uint32_t instruction) {
    embive_c_lw_sw_args_t decoded;
    decoded.rd_rs2 = (uint8_t)(((instruction >> 5) & 0x07u) + EMBIVE_LAYOUT_COMPRESSED_REGISTER_OFFSET);
    decoded.rs1 = (uint8_t)(((instruction >> 8) & 0x07u) + EMBIVE_LAYOUT_COMPRESSED_REGISTER_OFFSET);
    decoded.imm = (int32_t)((instruction & (0x1Fu << 11)) >> 9);
    return decoded;
}

static embive_c_addi_li_args_t embive_decode_c_addi_li(uint32_t instruction) {
    embive_c_addi_li_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int16_t)(instruction & (0x3Fu << 10))) >> 10;
    return decoded;
}

static embive_c_addi16sp_args_t embive_decode_c_addi16sp(uint32_t instruction) {
    embive_c_addi16sp_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int16_t)(instruction & (0x3Fu << 10))) >> 6;
    return decoded;
}

static embive_c_lui_args_t embive_decode_c_lui(uint32_t instruction) {
    embive_c_lui_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int32_t)(int16_t)(instruction & (0x3Fu << 10))) << 2;
    return decoded;
}

static embive_c_shift_imm_args_t embive_decode_c_slli(uint32_t instruction) {
    embive_c_shift_imm_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 10);
    return decoded;
}

static embive_c_lwsp_args_t embive_decode_c_lwsp(uint32_t instruction) {
    embive_c_lwsp_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 8);
    return decoded;
}

static embive_c_shift_imm_args_t embive_decode_c_shift_imm(uint32_t instruction) {
    embive_c_shift_imm_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 10);
    return decoded;
}

static embive_c_andi_args_t embive_decode_c_andi(uint32_t instruction) {
    embive_c_andi_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = ((int16_t)(instruction & (0x3Fu << 10))) >> 10;
    return decoded;
}

static embive_c_branch_zero_args_t embive_decode_c_branch_zero(uint32_t instruction) {
    embive_c_branch_zero_args_t decoded;
    decoded.rs1 = (uint8_t)(((instruction >> 5) & 0x07u) + EMBIVE_LAYOUT_COMPRESSED_REGISTER_OFFSET);
    decoded.imm = ((int16_t)(instruction & (0xFFu << 8))) >> 7;
    return decoded;
}

static embive_c_jr_mv_add_args_t embive_decode_c_jr_mv_add(uint32_t instruction) {
    embive_c_jr_mv_add_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.rs2 = (uint8_t)((instruction >> 10) & 0x1Fu);
    return decoded;
}

static embive_c_binary_reg_args_t embive_decode_c_binary_reg(uint32_t instruction) {
    embive_c_binary_reg_args_t decoded;
    decoded.rd_rs1 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.rs2 = (uint8_t)((instruction >> 10) & 0x1Fu);
    return decoded;
}

static embive_c_swsp_args_t embive_decode_c_swsp(uint32_t instruction) {
    embive_c_swsp_args_t decoded;
    decoded.rs2 = (uint8_t)((instruction >> 5) & 0x1Fu);
    decoded.imm = (int32_t)((instruction & (0x3Fu << 10)) >> 8);
    return decoded;
}

static embive_c_jump_args_t embive_decode_c_jump(uint32_t instruction) {
    embive_c_jump_args_t decoded;
    decoded.imm = ((int16_t)(instruction & (0x7FFu << 5))) >> 4;
    return decoded;
}

static embive_branch_args_t embive_decode_branch(uint32_t instruction) {
    embive_branch_args_t decoded;

    decoded.imm = ((int32_t)(instruction & (0xFFFu << 20))) >> 19;
    decoded.func = (uint8_t)((instruction >> 7) & 0x07u);
    decoded.rs1 = (uint8_t)((instruction >> 10) & 0x1Fu);
    decoded.rs2 = (uint8_t)((instruction >> 15) & 0x1Fu);

    return decoded;
}

static embive_u_args_t embive_decode_u(uint32_t instruction) {
    embive_u_args_t decoded;

    decoded.rd = (uint8_t)((instruction >> 7) & 0x1Fu);
    decoded.imm = (int32_t)(instruction & (0xFFFFFu << 12));

    return decoded;
}

static embive_j_args_t embive_decode_j(uint32_t instruction) {
    embive_j_args_t decoded;

    decoded.rd = (uint8_t)((instruction >> 7) & 0x1Fu);
    decoded.imm = ((int32_t)(instruction & (0xFFFFFu << 12))) >> 11;

    return decoded;
}

static embive_r_args_t embive_decode_r(uint32_t instruction) {
    embive_r_args_t decoded;

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
    if ((addr >= EMBIVE_CS_ADDR_MCOUNTINHIBIT && addr <= EMBIVE_CS_ADDR_MHPMEVENT31H) ||
        (addr >= EMBIVE_CS_ADDR_MCYCLE && addr <= EMBIVE_CS_ADDR_MHPMCOUNTER31H) ||
        (addr >= EMBIVE_CS_ADDR_MVENDORID && addr <= EMBIVE_CS_ADDR_MCONFIGPTR)) {
        *ret = 0;
        return EMBIVE_OK;
    }

    switch (addr) {
        case EMBIVE_CS_ADDR_MSTATUS:
            *ret = registers->mstatus;
            registers->mstatus = (uint8_t)(embive_execute_cs_op(op, *ret) & EMBIVE_MACHINE_CONSTANT_MSTATUS_MASK);
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MISA:
            *ret = embive_get_misa();
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MIE:
            *ret = ((uint32_t)registers->mie_embive) << EMBIVE_INTERRUPT_CODE;
            registers->mie_embive =
                (uint8_t)((embive_execute_cs_op(op, *ret) & EMBIVE_MACHINE_CONSTANT_MIE_MIP_EMBIVE_MASK) != 0u);
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MTVEC:
            *ret = registers->mtvec;
            registers->mtvec = embive_execute_cs_op(op, *ret) & ~EMBIVE_MACHINE_CONSTANT_MTVEC_MODE_MASK;
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MSTATUSH:
            *ret = 0;
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MSCRATCH:
            *ret = registers->mscratch;
            registers->mscratch = embive_execute_cs_op(op, *ret);
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MEPC:
            *ret = registers->mepc;
            registers->mepc = embive_execute_cs_op(op, *ret) & ~EMBIVE_MACHINE_CONSTANT_MEPC_BIT0_MASK;
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MCAUSE:
            *ret = registers->mcause;
            registers->mcause = embive_execute_cs_op(op, *ret);
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MTVAL:
            *ret = (uint32_t)registers->mtval;
            registers->mtval = (int32_t)embive_execute_cs_op(op, *ret);
            return EMBIVE_OK;
        case EMBIVE_CS_ADDR_MIP:
            *ret = ((uint32_t)registers->mip_embive) << EMBIVE_INTERRUPT_CODE;
            registers->mip_embive =
                (uint8_t)((embive_execute_cs_op(op, *ret) & EMBIVE_MACHINE_CONSTANT_MIE_MIP_EMBIVE_MASK) != 0u);
            return EMBIVE_OK;
        default:
            return EMBIVE_ERR_INVALID_CS_REGISTER;
    }
}

static bool embive_interrupt_enabled(const embive_registers_t *registers) {
    return registers->mie_embive != 0u &&
        (registers->mstatus & EMBIVE_MACHINE_CONSTANT_MSTATUS_MIE) != 0u;
}

static void embive_trap_entry(
    embive_interpreter_t *interpreter,
    int32_t value
) {
    if ((interpreter->registers.mstatus & EMBIVE_MACHINE_CONSTANT_MSTATUS_MIE) != 0u) {
        interpreter->registers.mstatus |= EMBIVE_MACHINE_CONSTANT_MSTATUS_MPIE;
    } else {
        interpreter->registers.mstatus &= (uint8_t)~EMBIVE_MACHINE_CONSTANT_MSTATUS_MPIE;
    }

    interpreter->registers.mstatus &= (uint8_t)~EMBIVE_MACHINE_CONSTANT_MSTATUS_MIE;
    interpreter->registers.mcause = EMBIVE_MACHINE_CONSTANT_MCAUSE_INTERRUPT | EMBIVE_INTERRUPT_CODE;
    interpreter->registers.mepc = interpreter->program_counter;
    interpreter->registers.mtval = value;
    interpreter->registers.mip_embive = 1u;
    interpreter->program_counter = interpreter->registers.mtvec & ~EMBIVE_MACHINE_CONSTANT_MTVEC_MODE_MASK;
}

static uint32_t embive_trap_return(embive_interpreter_t *interpreter) {
    if ((interpreter->registers.mstatus & EMBIVE_MACHINE_CONSTANT_MSTATUS_MPIE) != 0u) {
        interpreter->registers.mstatus |= EMBIVE_MACHINE_CONSTANT_MSTATUS_MIE;
    } else {
        interpreter->registers.mstatus &= (uint8_t)~EMBIVE_MACHINE_CONSTANT_MSTATUS_MIE;
    }

    return interpreter->registers.mepc;
}

static embive_error_t embive_execute_system_misc_mem(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_i_args_t decoded = embive_decode_i(instruction);
    uint32_t csr_value = 0;
    embive_cs_op_t op = {EMBIVE_CS_OP_KIND_NONE, 0};

    if (decoded.func == EMBIVE_SYSTEM_FUNC_MISC) {
        switch ((uint32_t)decoded.imm) {
            case EMBIVE_SYSTEM_IMM_ECALL:
                interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_CALLED;
                return EMBIVE_OK;
            case EMBIVE_SYSTEM_IMM_EBREAK:
                interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_HALTED;
                return EMBIVE_OK;
            case EMBIVE_SYSTEM_IMM_FENCEI:
                interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_RUNNING;
                return EMBIVE_OK;
            case EMBIVE_SYSTEM_IMM_WFI:
                interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
                *state = EMBIVE_STATE_WAITING;
                return EMBIVE_OK;
            case EMBIVE_SYSTEM_IMM_MRET:
                interpreter->program_counter = embive_trap_return(interpreter);
                *state = EMBIVE_STATE_RUNNING;
                return EMBIVE_OK;
            default:
                return EMBIVE_ERR_INVALID_INSTRUCTION;
        }
    }

    switch (decoded.func) {
        case EMBIVE_SYSTEM_FUNC_CSRRW:
            op.kind = EMBIVE_CS_OP_KIND_WRITE;
            op.value = (uint32_t)interpreter->registers.x[decoded.rs1];
            break;
        case EMBIVE_SYSTEM_FUNC_CSRRS:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_KIND_SET;
                op.value = (uint32_t)interpreter->registers.x[decoded.rs1];
            }
            break;
        case EMBIVE_SYSTEM_FUNC_CSRRC:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_KIND_CLEAR;
                op.value = (uint32_t)interpreter->registers.x[decoded.rs1];
            }
            break;
        case EMBIVE_SYSTEM_FUNC_CSRRWI:
            op.kind = EMBIVE_CS_OP_KIND_WRITE;
            op.value = decoded.rs1;
            break;
        case EMBIVE_SYSTEM_FUNC_CSRRSI:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_KIND_SET;
                op.value = decoded.rs1;
            }
            break;
        case EMBIVE_SYSTEM_FUNC_CSRRCI:
            if (decoded.rs1 != 0u) {
                op.kind = EMBIVE_CS_OP_KIND_CLEAR;
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

    interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_op_imm(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_i_args_t decoded = embive_decode_i(instruction);
    int32_t rs1 = interpreter->registers.x[decoded.rs1];
    int32_t result = 0;
    uint32_t shamt = ((uint32_t)decoded.imm) & 0x1Fu;

    if (decoded.rd_rs2 != 0u) {
        switch (decoded.func) {
            case EMBIVE_OP_IMM_FUNC_ADDI:
                result = rs1 + decoded.imm;
                break;
            case EMBIVE_OP_IMM_FUNC_SLLI:
                result = (int32_t)((uint32_t)rs1 << shamt);
                break;
            case EMBIVE_OP_IMM_FUNC_SLTI:
                result = rs1 < decoded.imm ? 1 : 0;
                break;
            case EMBIVE_OP_IMM_FUNC_SLTIU:
                result = ((uint32_t)rs1 < (uint32_t)decoded.imm) ? 1 : 0;
                break;
            case EMBIVE_OP_IMM_FUNC_XORI:
                result = rs1 ^ decoded.imm;
                break;
            case EMBIVE_OP_IMM_FUNC_SRLI_SRAI:
                if ((decoded.imm & (1 << 10)) != 0) {
                    result = rs1 >> shamt;
                } else {
                    result = (int32_t)(((uint32_t)rs1) >> shamt);
                }
                break;
            case EMBIVE_OP_IMM_FUNC_ORI:
                result = rs1 | decoded.imm;
                break;
            case EMBIVE_OP_IMM_FUNC_ANDI:
                result = rs1 & decoded.imm;
                break;
            default:
                return EMBIVE_ERR_INVALID_INSTRUCTION;
        }

        interpreter->registers.x[decoded.rd_rs2] = result;
    }

    interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_load_store(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_i_args_t decoded = embive_decode_i(instruction);
    uint32_t address = (uint32_t)interpreter->registers.x[decoded.rs1] + (uint32_t)decoded.imm;
    uint8_t u8_value = 0;
    uint16_t u16_value = 0;
    uint32_t u32_value = 0;
    int32_t rs2 = 0;
    embive_error_t err = EMBIVE_OK;

    switch (decoded.func) {
        case EMBIVE_LOAD_STORE_FUNC_LB:
            err = embive_load_u8(interpreter, address, &u8_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)(int8_t)u8_value;
            break;
        case EMBIVE_LOAD_STORE_FUNC_LH:
            err = embive_load_u16(interpreter, address, &u16_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)(int16_t)u16_value;
            break;
        case EMBIVE_LOAD_STORE_FUNC_LW:
            err = embive_load_u32(interpreter, address, &u32_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)u32_value;
            break;
        case EMBIVE_LOAD_STORE_FUNC_LBU:
            err = embive_load_u8(interpreter, address, &u8_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)u8_value;
            break;
        case EMBIVE_LOAD_STORE_FUNC_LHU:
            err = embive_load_u16(interpreter, address, &u16_value);
            if (err != EMBIVE_OK) {
                return err;
            }
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)u16_value;
            break;
        case EMBIVE_LOAD_STORE_FUNC_SB:
            rs2 = interpreter->registers.x[decoded.rd_rs2];
            err = embive_store_u8(interpreter, address, (uint8_t)rs2);
            if (err != EMBIVE_OK) {
                return err;
            }
            break;
        case EMBIVE_LOAD_STORE_FUNC_SH:
            rs2 = interpreter->registers.x[decoded.rd_rs2];
            err = embive_store_u16(interpreter, address, (uint16_t)rs2);
            if (err != EMBIVE_OK) {
                return err;
            }
            break;
        case EMBIVE_LOAD_STORE_FUNC_SW:
            rs2 = interpreter->registers.x[decoded.rd_rs2];
            err = embive_store_u32(interpreter, address, (uint32_t)rs2);
            if (err != EMBIVE_OK) {
                return err;
            }
            break;
        default:
            return EMBIVE_ERR_INVALID_INSTRUCTION;
    }

    interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
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
    embive_r_args_t decoded = embive_decode_r(instruction);
    int32_t rs1 = interpreter->registers.x[decoded.rs1];
    int32_t rs2 = interpreter->registers.x[decoded.rs2];
    int32_t result = 0;

    switch (decoded.func) {
        case EMBIVE_OP_AMO_FUNC_ADD:
            result = rs1 + rs2;
            break;
        case EMBIVE_OP_AMO_FUNC_SUB:
            result = rs1 - rs2;
            break;
        case EMBIVE_OP_AMO_FUNC_SLL:
            result = (int32_t)(((uint32_t)rs1) << ((uint32_t)rs2 & 31u));
            break;
        case EMBIVE_OP_AMO_FUNC_SLT:
            result = rs1 < rs2 ? 1 : 0;
            break;
        case EMBIVE_OP_AMO_FUNC_SLTU:
            result = ((uint32_t)rs1 < (uint32_t)rs2) ? 1 : 0;
            break;
        case EMBIVE_OP_AMO_FUNC_XOR:
            result = rs1 ^ rs2;
            break;
        case EMBIVE_OP_AMO_FUNC_SRL:
            result = (int32_t)(((uint32_t)rs1) >> ((uint32_t)rs2 & 31u));
            break;
        case EMBIVE_OP_AMO_FUNC_SRA:
            result = rs1 >> ((uint32_t)rs2 & 31u);
            break;
        case EMBIVE_OP_AMO_FUNC_OR:
            result = rs1 | rs2;
            break;
        case EMBIVE_OP_AMO_FUNC_AND:
            result = rs1 & rs2;
            break;
        case EMBIVE_OP_AMO_FUNC_MUL:
            result = (int32_t)((uint32_t)rs1 * (uint32_t)rs2);
            break;
        case EMBIVE_OP_AMO_FUNC_MULH:
            result = (int32_t)(((int64_t)rs1 * (int64_t)rs2) >> 32);
            break;
        case EMBIVE_OP_AMO_FUNC_MULHSU:
            result = (int32_t)(((int64_t)rs1 * (int64_t)(uint32_t)rs2) >> 32);
            break;
        case EMBIVE_OP_AMO_FUNC_MULHU:
            result = (int32_t)(((uint64_t)(uint32_t)rs1 * (uint64_t)(uint32_t)rs2) >> 32);
            break;
        case EMBIVE_OP_AMO_FUNC_DIV:
            result = embive_div_signed(rs1, rs2);
            break;
        case EMBIVE_OP_AMO_FUNC_DIVU:
            result = rs2 == 0 ? -1 : (int32_t)((uint32_t)rs1 / (uint32_t)rs2);
            break;
        case EMBIVE_OP_AMO_FUNC_REM:
            result = embive_rem_signed(rs1, rs2);
            break;
        case EMBIVE_OP_AMO_FUNC_REMU:
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
                case EMBIVE_OP_AMO_FUNC_LR:
                    interpreter->has_reservation = 1u;
                    interpreter->reservation_addr = address;
                    interpreter->reservation_value = result;
                    break;
                case EMBIVE_OP_AMO_FUNC_SC:
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
                case EMBIVE_OP_AMO_FUNC_SWAP:
                    err = embive_store_u32(interpreter, address, (uint32_t)rs2);
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOADD:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result + rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOXOR:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result ^ rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOAND:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result & rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOOR:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result | rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOMIN:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result < rs2 ? result : rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOMAX:
                    err = embive_store_u32(interpreter, address, (uint32_t)(result > rs2 ? result : rs2));
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOMINU:
                    err = embive_store_u32(interpreter, address, ((uint32_t)result < (uint32_t)rs2) ? (uint32_t)result : (uint32_t)rs2);
                    if (err != EMBIVE_OK) return err;
                    break;
                case EMBIVE_OP_AMO_FUNC_AMOMAXU:
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

    interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
}

static embive_error_t embive_execute_branch(
    embive_interpreter_t *interpreter,
    uint32_t instruction,
    embive_state_t *state
) {
    embive_branch_args_t decoded = embive_decode_branch(instruction);
    int32_t rs1 = interpreter->registers.x[decoded.rs1];
    int32_t rs2 = interpreter->registers.x[decoded.rs2];
    bool branch = false;

    switch (decoded.func) {
        case EMBIVE_BRANCH_FUNC_BEQ:
            branch = rs1 == rs2;
            break;
        case EMBIVE_BRANCH_FUNC_BNE:
            branch = rs1 != rs2;
            break;
        case EMBIVE_BRANCH_FUNC_BLT:
            branch = rs1 < rs2;
            break;
        case EMBIVE_BRANCH_FUNC_BGE:
            branch = rs1 >= rs2;
            break;
        case EMBIVE_BRANCH_FUNC_BLTU:
            branch = (uint32_t)rs1 < (uint32_t)rs2;
            break;
        case EMBIVE_BRANCH_FUNC_BGEU:
            branch = (uint32_t)rs1 >= (uint32_t)rs2;
            break;
        default:
            return EMBIVE_ERR_INVALID_INSTRUCTION;
    }

    if (branch) {
        interpreter->program_counter += (uint32_t)decoded.imm;
    } else {
        interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
    }

    *state = EMBIVE_STATE_RUNNING;
    return EMBIVE_OK;
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
    embive_opcode_t opcode;
    embive_error_t err = embive_load_u32(
        interpreter,
        interpreter->program_counter,
        &instruction
    );
    if (err != EMBIVE_OK) {
        return err;
    }

    opcode = (embive_opcode_t)(instruction & EMBIVE_LAYOUT_OPCODE_MASK);

    switch (opcode) {
        case EMBIVE_OPCODE_C_ADDI4SPN: {
            embive_c_addi4spn_args_t decoded = embive_decode_c_addi4spn(instruction);
            int32_t sp = interpreter->registers.x[2];
            if (decoded.imm == 0) {
                return EMBIVE_ERR_ILLEGAL_INSTRUCTION;
            }
            interpreter->registers.x[decoded.rd] = sp + decoded.imm;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_LW: {
            embive_c_lw_sw_args_t decoded = embive_decode_c_lw_sw(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[decoded.rs1] + (uint32_t)decoded.imm;
            uint32_t value = 0;
            err = embive_load_u32(interpreter, address, &value);
            if (err != EMBIVE_OK) return err;
            interpreter->registers.x[decoded.rd_rs2] = (int32_t)value;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_SW: {
            embive_c_lw_sw_args_t decoded = embive_decode_c_lw_sw(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[decoded.rs1] + (uint32_t)decoded.imm;
            err = embive_store_u32(interpreter, address, (uint32_t)interpreter->registers.x[decoded.rd_rs2]);
            if (err != EMBIVE_OK) return err;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_ADDI: {
            embive_c_addi_li_args_t decoded = embive_decode_c_addi_li(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] += decoded.imm;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_JAL: {
            embive_c_jump_args_t decoded = embive_decode_c_jump(instruction);
            interpreter->registers.x[1] =
                (int32_t)(interpreter->program_counter + EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE);
            interpreter->program_counter += (uint32_t)decoded.imm;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_LI: {
            embive_c_addi_li_args_t decoded = embive_decode_c_addi_li(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] = decoded.imm;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_ADDI16SP: {
            embive_c_addi16sp_args_t decoded = embive_decode_c_addi16sp(instruction);
            interpreter->registers.x[2] += decoded.imm;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_LUI: {
            embive_c_lui_args_t decoded = embive_decode_c_lui(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] = decoded.imm;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_SRLI: {
            embive_c_shift_imm_args_t decoded = embive_decode_c_shift_imm(instruction);
            if (decoded.rd_rs1 != 0u) {
                interpreter->registers.x[decoded.rd_rs1] =
                    (int32_t)(((uint32_t)interpreter->registers.x[decoded.rd_rs1]) >> (uint32_t)decoded.imm);
            }
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_SRAI: {
            embive_c_shift_imm_args_t decoded = embive_decode_c_shift_imm(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] >>= (uint32_t)decoded.imm;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_ANDI: {
            embive_c_andi_args_t decoded = embive_decode_c_andi(instruction);
            if (decoded.rd_rs1 != 0u) interpreter->registers.x[decoded.rd_rs1] &= decoded.imm;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_SUB:
        case EMBIVE_OPCODE_C_XOR:
        case EMBIVE_OPCODE_C_OR:
        case EMBIVE_OPCODE_C_AND: {
            embive_c_binary_reg_args_t decoded = embive_decode_c_binary_reg(instruction);
            int32_t rs2 = interpreter->registers.x[decoded.rs2];
            if (decoded.rd_rs1 != 0u) {
                if (opcode == EMBIVE_OPCODE_C_SUB) interpreter->registers.x[decoded.rd_rs1] -= rs2;
                else if (opcode == EMBIVE_OPCODE_C_XOR) interpreter->registers.x[decoded.rd_rs1] ^= rs2;
                else if (opcode == EMBIVE_OPCODE_C_OR) interpreter->registers.x[decoded.rd_rs1] |= rs2;
                else interpreter->registers.x[decoded.rd_rs1] &= rs2;
            }
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_J: {
            embive_c_jump_args_t decoded = embive_decode_c_jump(instruction);
            interpreter->program_counter += (uint32_t)decoded.imm;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_BEQZ:
        case EMBIVE_OPCODE_C_BNEZ: {
            embive_c_branch_zero_args_t decoded = embive_decode_c_branch_zero(instruction);
            bool cond = interpreter->registers.x[decoded.rs1] == 0;
            if (opcode == EMBIVE_OPCODE_C_BNEZ) cond = !cond;
            if (cond) interpreter->program_counter += (uint32_t)decoded.imm;
            else interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_SLLI: {
            embive_c_shift_imm_args_t decoded = embive_decode_c_slli(instruction);
            if (decoded.rd_rs1 != 0u) {
                interpreter->registers.x[decoded.rd_rs1] =
                    (int32_t)(((uint32_t)interpreter->registers.x[decoded.rd_rs1]) << (uint32_t)decoded.imm);
            }
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_LWSP: {
            embive_c_lwsp_args_t decoded = embive_decode_c_lwsp(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[2] + (uint32_t)decoded.imm;
            uint32_t value = 0;
            err = embive_load_u32(interpreter, address, &value);
            if (err != EMBIVE_OK) return err;
            interpreter->registers.x[decoded.rd_rs1] = (int32_t)value;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_JR_MV: {
            embive_c_jr_mv_add_args_t decoded = embive_decode_c_jr_mv_add(instruction);
            if (decoded.rs2 == 0u) {
                interpreter->program_counter = (uint32_t)interpreter->registers.x[decoded.rd_rs1];
            } else {
                interpreter->registers.x[decoded.rd_rs1] = interpreter->registers.x[decoded.rs2];
                interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            }
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_EBREAK_JALR_ADD: {
            embive_c_jr_mv_add_args_t decoded = embive_decode_c_jr_mv_add(instruction);
            if (decoded.rs2 == 0u) {
                if (decoded.rd_rs1 == 0u) {
                    interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
                    *state = EMBIVE_STATE_HALTED;
                    return EMBIVE_OK;
                }
                interpreter->registers.x[1] =
                    (int32_t)(interpreter->program_counter + EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE);
                interpreter->program_counter = (uint32_t)interpreter->registers.x[decoded.rd_rs1];
            } else {
                interpreter->registers.x[decoded.rd_rs1] += interpreter->registers.x[decoded.rs2];
                interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            }
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_C_SWSP: {
            embive_c_swsp_args_t decoded = embive_decode_c_swsp(instruction);
            uint32_t address = (uint32_t)interpreter->registers.x[2] + (uint32_t)decoded.imm;
            err = embive_store_u32(interpreter, address, (uint32_t)interpreter->registers.x[decoded.rs2]);
            if (err != EMBIVE_OK) return err;
            interpreter->program_counter += EMBIVE_LAYOUT_COMPRESSED_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_AUIPC:
        {
            embive_u_args_t decoded = embive_decode_u(instruction);
            if (decoded.rd != 0u) {
                interpreter->registers.x[decoded.rd] =
                    (int32_t)(interpreter->program_counter + (uint32_t)decoded.imm);
            }
            interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_BRANCH:
            return embive_execute_branch(interpreter, instruction, state);
        case EMBIVE_OPCODE_JAL:
        {
            embive_j_args_t decoded = embive_decode_j(instruction);
            if (decoded.rd != 0u) {
                interpreter->registers.x[decoded.rd] =
                    (int32_t)(interpreter->program_counter + EMBIVE_LAYOUT_INSTRUCTION_SIZE);
            }
            interpreter->program_counter += (uint32_t)decoded.imm;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_JALR:
        {
            embive_i_args_t decoded = embive_decode_i(instruction);
            int32_t rs1 = interpreter->registers.x[decoded.rs1];
            if (decoded.rd_rs2 != 0u) {
                interpreter->registers.x[decoded.rd_rs2] =
                    (int32_t)(interpreter->program_counter + EMBIVE_LAYOUT_INSTRUCTION_SIZE);
            }
            interpreter->program_counter = (uint32_t)rs1 + (uint32_t)decoded.imm;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_LOAD_STORE:
            return embive_execute_load_store(interpreter, instruction, state);
        case EMBIVE_OPCODE_LUI:
        {
            embive_u_args_t decoded = embive_decode_u(instruction);
            if (decoded.rd != 0u) {
                interpreter->registers.x[decoded.rd] = decoded.imm;
            }
            interpreter->program_counter += EMBIVE_LAYOUT_INSTRUCTION_SIZE;
            *state = EMBIVE_STATE_RUNNING;
            return EMBIVE_OK;
        }
        case EMBIVE_OPCODE_OP_IMM:
            return embive_execute_op_imm(interpreter, instruction, state);
        case EMBIVE_OPCODE_OP_AMO:
            return embive_execute_op_amo(interpreter, instruction, state);
        case EMBIVE_OPCODE_SYSTEM_MISC_MEM:
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
    syscall->nr = interpreter->registers.x[EMBIVE_REGISTER_INDEX_A7];
    memcpy(
        syscall->args,
        &interpreter->registers.x[EMBIVE_REGISTER_INDEX_A0],
        sizeof(syscall->args)
    );
    return EMBIVE_OK;
}

void embive_set_syscall_result(
    embive_interpreter_t *interpreter,
    int32_t error_code,
    int32_t value
) {
    interpreter->registers.x[EMBIVE_REGISTER_INDEX_A0] = error_code;
    interpreter->registers.x[EMBIVE_REGISTER_INDEX_A1] = value;
}
