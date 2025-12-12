/* ENTRY tells the linker that Reset_Handler is the start of the program */
ENTRY(Reset_Handler)

/* Define memory regions of nRF52840 */
MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 1024K /* r - read, x - execute */
  RAM   (rwx): ORIGIN = 0x20000000, LENGTH = 256K  /* r - read, w - write, x - execute */
}

/* Initial stack pointer = top of RAM */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  /* ---- Vector table ---- */
  .isr_vector :
  {
    KEEP(*(.isr_vector))   /* Keep vector table from startup.s */
  } > FLASH

  /* ---- Code (.text) + read-only data ---- */
  .text :
  {
    *(.text*)
    *(.rodata*)
  } > FLASH

  /* ---- Initialized data (.data) ---- */
  .data : AT(ADDR(.text) + SIZEOF(.text))
  {
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM
  _sidata = LOADADDR(.data);

  /* ---- Zero-initialized data (.bss) ---- */
  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM
}

