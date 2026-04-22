#include "embive.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static uint8_t ram[4096];

static int puts_guest_string(const embive_interpreter_t *interpreter, uint32_t address) {
    uint8_t *string;
    size_t remaining;
    size_t i;

    if (embive_memory_slice(interpreter, address, &string, &remaining) != EMBIVE_OK) {
        return -1;
    }

    for (i = 0; i < remaining; ++i) {
        if (string[i] == 0) {
            if (fwrite(string, 1, i, stdout) != i) {
                return -1;
            }

            if (fputc('\n', stdout) == EOF) {
                return -1;
            }

            if (fflush(stdout) != 0) {
                return -1;
            }

            return 0;
        }
    }

    return -1;
}

static int run_guest(const uint8_t *code, size_t code_len, uint8_t *ram, size_t ram_len) {
    embive_interpreter_t interpreter;
    embive_state_t state;

    embive_init(&interpreter, code, code_len, ram, ram_len, 0);

    /* Optional: set a non-zero entry point. */
    interpreter.program_counter = 0;

    for (;;) {
        embive_error_t err = embive_run(&interpreter, &state);
        if (err != EMBIVE_OK) {
            return (int)err;
        }

        switch (state) {
            case EMBIVE_STATE_RUNNING:
                break;

            case EMBIVE_STATE_CALLED: {
                embive_syscall_t syscall;
                err = embive_get_syscall(&interpreter, &syscall);
                if (err != EMBIVE_OK) {
                    return (int)err;
                }

                /* Host syscall handling example. */
                if (syscall.nr == 1) {
                    embive_set_syscall_result(
                        &interpreter,
                        0,
                        syscall.args[0] + syscall.args[1]
                    );
                } else if (syscall.nr == 3) {
                    if (puts_guest_string(&interpreter, (uint32_t)syscall.args[0]) == 0) {
                        embive_set_syscall_result(&interpreter, 0, 0);
                    } else {
                        embive_set_syscall_result(&interpreter, 1, 0);
                    }
                } else {
                    embive_set_syscall_result(&interpreter, 2, 0);
                }
                break;
            }

            case EMBIVE_STATE_WAITING:
                err = embive_interrupt(&interpreter, 123);
                if (err != EMBIVE_OK) {
                    return (int)err;
                }
                break;

            case EMBIVE_STATE_HALTED:
                return EMBIVE_OK;
        }
    }
}

int main(int argc, char **argv) {
    int fd;
    struct stat st;
    void *mapped;
    int result;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <embive-bytecode>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Failed to open input file");
        return 1;
    }

    if (fstat(fd, &st) != 0) {
        perror("Failed to stat input file");
        close(fd);
        return 1;
    }

    if (st.st_size <= 0) {
        fprintf(stderr, "Input file is empty\n");
        close(fd);
        return 1;
    }

    mapped = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (mapped == MAP_FAILED) {
        perror("Failed to memory map input file");
        return 1;
    }

    result = run_guest((const uint8_t *)mapped, (size_t)st.st_size, ram, sizeof(ram));

    if (munmap(mapped, (size_t)st.st_size) != 0) {
        perror("Failed to unmap input file");
        return 1;
    }

    if (result != EMBIVE_OK) {
        fprintf(stderr, "Interpreter failed with error %d\n", result);
        return 1;
    }

    return 0;
}
