CROSS ?= riscv64-unknown-elf
CC := $(CROSS)-gcc
LD := $(CROSS)-ld
OBJCOPY := $(CROSS)-objcopy
OBJDUMP := $(CROSS)-objdump
QEMU := qemu-system-riscv64

CFLAGS := -march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -mno-relax -Wall -Wextra -Werror -O2 -Iinclude -Ikernel
ASFLAGS := -march=rv64imac_zicsr_zifencei -mabi=lp64 -mno-relax -Iinclude
LDFLAGS := -T linker.ld -nostdlib -static -melf64lriscv --no-relax
USER_CFLAGS := -march=rv64imac_zicsr_zifencei -mabi=lp64 -mcmodel=medany -ffreestanding -fno-builtin -fno-stack-protector -fno-pie -mno-relax -Wall -Wextra -Werror -O2 -Iinclude -Iuser
USER_LDFLAGS := -T user/user.ld -nostdlib -static -melf64lriscv --no-relax --no-warn-rwx-segments

KERNEL_C_OBJS := \
    kernel/main.o kernel/uart.o kernel/string.o kernel/spinlock.o \
    kernel/vm.o kernel/proc.o kernel/syscall.o kernel/trap.o \
    kernel/timer.o kernel/ramfs.o

KERNEL_ASM_OBJS := kernel/boot.o kernel/entry.o kernel/swtch.o
KERNEL_OBJS := $(KERNEL_C_OBJS) $(KERNEL_ASM_OBJS) user/sh_blob.o

.PHONY: all clean qemu qemu-test disasm user-disasm verify

all: kernel.elf kernel.bin

user/sh.o: user/sh.c
	$(CC) $(USER_CFLAGS) -c $< -o $@

user/syscall.o: user/syscall.S
	$(CC) $(ASFLAGS) -Iinclude -c $< -o $@

user/sh.elf: user/sh.o user/syscall.o user/user.ld
	$(LD) $(USER_LDFLAGS) -o $@ user/sh.o user/syscall.o

user/sh.bin: user/sh.elf
	$(OBJCOPY) -O binary $< $@

user/sh_blob.o: user/sh.bin
	$(OBJCOPY) -I binary -O elf64-littleriscv -B riscv $< $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/%.o: kernel/%.S
	$(CC) $(ASFLAGS) -c $< -o $@

kernel.elf: $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

qemu: kernel.elf
	$(QEMU) -machine virt -nographic -bios default -kernel kernel.elf -m 128M

qemu-test: kernel.elf user/sh.bin
	bash tests/test.sh

verify: all
	@command -v $(CC) >/dev/null 2>&1 || { echo "ERROR: $(CC) not found. Run ./setup_ubuntu.sh"; exit 1; }
	@command -v $(QEMU) >/dev/null 2>&1 || { echo "NOTE: $(QEMU) not installed; static checks continue."; }
	@file kernel.elf | grep -q 'RISC-V' || { echo "ERROR: kernel.elf is not a RISC-V ELF"; exit 1; }
	@test $$(wc -c < user/sh.bin) -le $$((4*4096)) || { echo "ERROR: user shell exceeds USER_IMAGE_PAGES"; exit 1; }
	@$(OBJDUMP) -t kernel.elf | grep -E -q '^0*80200000.* _start$$' || { echo "ERROR: kernel _start is not linked at 0x80200000"; exit 1; }
	@$(OBJDUMP) -t kernel.elf | grep -q '__bss_start' || { echo "ERROR: missing BSS start symbol"; exit 1; }
	@$(OBJDUMP) -t kernel.elf | grep -q '__bss_end' || { echo "ERROR: missing BSS end symbol"; exit 1; }
	@$(OBJDUMP) -t kernel.elf | grep -q 'user_trap_entry' || { echo "ERROR: missing user trap entry"; exit 1; }
	@$(OBJDUMP) -t kernel.elf | grep -q '_binary_user_sh_bin_start' || { echo "ERROR: missing embedded user image"; exit 1; }
	@$(OBJDUMP) -t user/sh.elf | grep -E -q '^0*40000000.*_start$$' || { echo "ERROR: user _start is not linked at USER_BASE (0x40000000)"; exit 1; }
	@echo "Static verification: PASS (RISC-V ELF, trap symbols, embedded shell, user entry address)"


disasm: kernel.elf
	$(OBJDUMP) -d kernel.elf > kernel.disasm

user-disasm: user/sh.elf
	$(OBJDUMP) -d user/sh.elf > user/sh.disasm

clean:
	rm -f kernel/*.o user/*.o user/*.elf user/*.bin user/sh.disasm kernel.elf kernel.bin kernel.disasm tests/qemu-output.txt
