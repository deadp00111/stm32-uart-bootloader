

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08004000, LENGTH = 480K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 128K
}

SECTIONS
{
    .text : {
        KEEP(*(.isr_vector))    
        *(.text*)               
    } > FLASH
}