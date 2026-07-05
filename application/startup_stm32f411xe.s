.syntax unified
.cpu cortex-m4
.thumb

.global Reset_Handler
.global g_pfnVectors

.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss
.word _estack

.section .isr_vector,"a",%progbits
g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0,0,0,0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler
  /* IRQ0..IRQ85 - keep all slots so vector table size is correct */
  .rept 86
  .word Default_Handler
  .endr

.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_estack
  mov sp, r0

  /* copy .data from flash to ram */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b copy_data_loop_check
copy_data_loop:
  ldr r4, [r2, r3]
  str r4, [r0, r3]
  adds r3, r3, #4
copy_data_loop_check:
  adds r4, r0, r3
  cmp r4, r1
  bcc copy_data_loop

  /* zero .bss */
  ldr r0, =_sbss
  ldr r1, =_ebss
  movs r2, #0
  b zero_bss_loop_check
zero_bss_loop:
  str r2, [r0]
  adds r0, r0, #4
zero_bss_loop_check:
  cmp r0, r1
  bcc zero_bss_loop

  bl main
  b .

.section .text.Default_Handler,"ax",%progbits
Default_Handler:
  b Default_Handler

.macro def_irq_handler name
  .weak \name
  .set \name, Default_Handler
.endm

def_irq_handler NMI_Handler
def_irq_handler HardFault_Handler
def_irq_handler MemManage_Handler
def_irq_handler BusFault_Handler
def_irq_handler UsageFault_Handler
def_irq_handler SVC_Handler
def_irq_handler DebugMon_Handler
def_irq_handler PendSV_Handler
def_irq_handler SysTick_Handler
