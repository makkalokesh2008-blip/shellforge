OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
  . = 0x80200000;

  .text : ALIGN(0x1000)
  {
    *(.text.boot)
    *(.text .text.*)
  }

  .rodata : ALIGN(0x1000)
  {
    *(.rodata .rodata.*)
  }

  .data : ALIGN(0x1000)
  {
    *(.data .data.*)
    *(.sdata .sdata.*)
  }

  .bss : ALIGN(0x1000)
  {
    __bss_start = .;
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(8);
    __bss_end = .;
  }

  . = ALIGN(0x1000);
  __kernel_end = .;
}
