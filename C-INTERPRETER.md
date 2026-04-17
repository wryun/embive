# C Interpreter API

This document describes how to use the standalone C interpreter in:

- `c/include/embive.h`
- `c/src/embive.c`

The interpreter executes already-transpiled Embive bytecode. It does not parse
ELF files itself.

## Memory model

The host provides two buffers:

- `code`
  Mapped at guest address `0x00000000`
- `ram`
  Mapped at guest address `EMBIVE_RAM_OFFSET` (`0x80000000`)

The interpreter does not allocate memory internally. Guest stack, data, and any
guest-side allocator all live inside the RAM buffer you provide.

## Lifecycle

1. Allocate an `embive_interpreter_t`.
2. Call `embive_init`.
3. Set `interpreter.program_counter` if needed.
4. Repeatedly call `embive_run`.
5. When the returned state is:
   - `EMBIVE_STATE_RUNNING`: call `embive_run` again
   - `EMBIVE_STATE_CALLED`: handle the syscall, then continue
   - `EMBIVE_STATE_WAITING`: optionally inject an interrupt, then continue
   - `EMBIVE_STATE_HALTED`: stop

`instruction_limit` controls how many instructions `embive_run` executes per
call. Use `0` to run until the guest changes state.

A minimal example is available in `examples/cinterp.c`.

## Notes

- `embive_reset` resets registers, PC, and LR/SC reservation state, but keeps
  guest memory intact.
- `embive_get_syscall` reads syscall number from guest `a7` and arguments from
  `a0..a6`.
- `embive_set_syscall_result` writes back to guest `a0` and `a1`.
- The interpreter struct is intentionally plain data. Hosts may inspect fields
  like `program_counter` and `registers` directly.
