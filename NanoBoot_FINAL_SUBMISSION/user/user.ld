OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS
{
  . = 0x40000000;
  .text : ALIGN(16) { *(.text.start) *(.text .text.*) }
  .rodata : ALIGN(16) { *(.rodata .rodata.*) }
  .data : ALIGN(16) { *(.data .data.*) }
  .bss : ALIGN(16) { *(.bss .bss.*) *(COMMON) }
}
