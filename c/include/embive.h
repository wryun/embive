#ifndef EMBIVE_H
#define EMBIVE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Embive C interpreter API
 *
 * Typical usage:
 * 1. Provide transpiled Embive bytecode in `code` and a writable guest RAM
 *    buffer in `ram`.
 * 2. Call `embive_init`.
 * 3. Set `interpreter.program_counter` if the guest entry point is not 0.
 *    The Rust test harness sets it to `EMBIVE_RAM_OFFSET` when executing
 *    transpiled binaries directly from RAM.
 * 4. Call `embive_run` in a loop and react to the returned state:
 *    - EMBIVE_STATE_RUNNING: instruction limit reached; call `embive_run` again
 *    - EMBIVE_STATE_CALLED: read syscall arguments with `embive_get_syscall`,
 *      handle the syscall in the host, then write the result back with
 *      `embive_set_syscall_result`
 *    - EMBIVE_STATE_WAITING: optionally deliver an interrupt with
 *      `embive_interrupt`
 *    - EMBIVE_STATE_HALTED: guest execution finished
 *
 * The interpreter does not allocate memory. Guest stack/data/heap all live
 * inside the RAM slice provided by the host.
 */

#define EMBIVE_CPU_REGISTER_COUNT 32U
#define EMBIVE_RAM_OFFSET 0x80000000u
#define EMBIVE_INTERRUPT_CODE 16u
#define EMBIVE_SYSCALL_ARGS 7u

/* Guest execution state returned by `embive_run` and `embive_step`. */
typedef enum {
    EMBIVE_STATE_RUNNING = 0,
    EMBIVE_STATE_CALLED = 1,
    EMBIVE_STATE_WAITING = 2,
    EMBIVE_STATE_HALTED = 3,
} embive_state_t;

/* Error codes returned by the C API. */
typedef enum {
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
} embive_error_t;

/* Guest register state owned by the interpreter. */
typedef struct {
    /* General-purpose x0..x31 registers. */
    int32_t x[EMBIVE_CPU_REGISTER_COUNT];
    /* Supported machine CSRs. */
    uint32_t mtvec;
    uint32_t mscratch;
    uint32_t mepc;
    uint32_t mcause;
    int32_t mtval;
    uint8_t mie_embive;
    uint8_t mip_embive;
    uint8_t mstatus;
} embive_registers_t;

/* Guest memory view used by the interpreter. */
typedef struct {
    /* Code is mapped at address 0x00000000. */
    const uint8_t *code;
    size_t code_len;
    /* RAM is mapped at address EMBIVE_RAM_OFFSET. */
    uint8_t *ram;
    size_t ram_len;
} embive_memory_t;

/* Complete interpreter instance. */
typedef struct {
    /* Current guest PC. */
    uint32_t program_counter;
    /* Architectural register state. */
    embive_registers_t registers;
    /* Host-provided code and RAM buffers. */
    embive_memory_t memory;
    /* Max instructions per `embive_run` call. 0 means "run until state change". */
    uint32_t instruction_limit;
    /* LR/SC reservation state for atomic instructions. */
    uint8_t has_reservation;
    uint32_t reservation_addr;
    int32_t reservation_value;
} embive_interpreter_t;

/* Syscall payload mirrored from guest registers a7 and a0..a6. */
typedef struct {
    int32_t nr;
    int32_t args[EMBIVE_SYSCALL_ARGS];
} embive_syscall_t;

/*
 * Initialize an interpreter instance.
 *
 * `code` and `ram` are borrowed for the lifetime of the interpreter and must
 * remain valid and writable/readable as appropriate.
 */
void embive_init(
    embive_interpreter_t *interpreter,
    const uint8_t *code,
    size_t code_len,
    uint8_t *ram,
    size_t ram_len,
    uint32_t instruction_limit
);

/* Reset PC, registers, and atomic reservation state. Memory contents are kept. */
void embive_reset(embive_interpreter_t *interpreter);

/*
 * Run until either:
 * - the guest changes state (syscall, wait, halt), or
 * - `instruction_limit` instructions have executed.
 *
 * Returns `EMBIVE_OK` on a normal state transition and stores the resulting
 * state in `*state`.
 */
embive_error_t embive_run(
    embive_interpreter_t *interpreter,
    embive_state_t *state
);

/* Execute exactly one instruction and return the resulting guest state. */
embive_error_t embive_step(
    embive_interpreter_t *interpreter,
    embive_state_t *state
);

/*
 * Deliver a host interrupt to a waiting guest.
 *
 * The guest must have enabled interrupts via its CSRs first, otherwise
 * `EMBIVE_ERR_INTERRUPT_NOT_ENABLED` is returned.
 */
embive_error_t embive_interrupt(
    embive_interpreter_t *interpreter,
    int32_t value
);

/* Read the pending syscall number and arguments from guest registers. */
embive_error_t embive_get_syscall(
    const embive_interpreter_t *interpreter,
    embive_syscall_t *syscall
);

/*
 * Translate a guest address into a host-accessible contiguous byte slice.
 *
 * On success:
 * - `*slice` points at the first byte for `address`
 * - `*len` is the number of contiguous bytes remaining in that mapped region
 *
 * The returned slice is borrowed from the interpreter's code or RAM buffers.
 */
embive_error_t embive_memory_slice(
    const embive_interpreter_t *interpreter,
    uint32_t address,
    const uint8_t **slice,
    size_t *len
);

/*
 * Write a syscall result back to guest registers:
 * - `error_code` goes to a0
 * - `value` goes to a1
 */
void embive_set_syscall_result(
    embive_interpreter_t *interpreter,
    int32_t error_code,
    int32_t value
);

#ifdef __cplusplus
}
#endif

#endif
