ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08004000, LENGTH = 496K
  RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH

  .text : {
    *(.text*)
    *(.rodata*)
    _etext = .;
  } > FLASH

  .data : {
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM AT> FLASH
  _sidata = LOADADDR(.data);

  .bss : {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM

  ._user_heap_stack : {
    . = ALIGN(8);
    . = . + 0x400;
    . = . + 0x400;
    . = ALIGN(8);
  } > RAM
}
