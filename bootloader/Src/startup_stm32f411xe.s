    .syntax unified
    .cpu cortex-m4
    .thumb

    .section .isr_vector
    .word   0x20020000      /* Stack top */
    .word   _start          /* Reset handler */

    .section .text
    .global _start
_start:
    bl      main
    b       .
    