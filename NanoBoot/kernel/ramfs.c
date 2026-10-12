#include "common.h"
#include "string.h"
#include "uart.h"
#include "ramfs.h"

struct ramfile {
    const char *name;
    const char *data;
};

static const struct ramfile files[] = {
    {"README.txt", "NanoBoot RAMFS\nThis file is stored in kernel memory.\n"},
    {"hello.txt", "Hello from the NanoBoot in-memory filesystem!\n"},
    {"design.txt", "RISC-V + QEMU virt + Sv39 + traps + syscalls + round-robin.\n"},
};

void ramfs_ls(void)
{
    uart_puts("Files:\n");
    for (size_t i = 0; i < NELEM(files); ++i) {
        uart_puts("  ");
        uart_puts(files[i].name);
        uart_putc('\n');
    }
}

int ramfs_cat(const char *name)
{
    for (size_t i = 0; i < NELEM(files); ++i) {
        if (strcmp(name, files[i].name) == 0) {
            uart_puts(files[i].data);
            return 0;
        }
    }
    uart_puts("cat: file not found\n");
    return -1;
}
