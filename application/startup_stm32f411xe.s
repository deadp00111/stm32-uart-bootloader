    .syntax unified
    .cpu cortex-m4
    .thumb

    // Vector table: first entry = stack top, second = reset handler 
    .section .isr_vector
    .word   0x20020000      //Stack pointer = top of 128KB SRAM 
    .word   _start          // Reset handler address 

    .section .text
    .global _start
_start:
    bl      main            // Call C main() 
    b       .               // Loop forever if main returns 