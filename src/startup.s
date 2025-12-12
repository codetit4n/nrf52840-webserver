    .syntax unified
    .cpu cortex-m4
    .thumb

    .extern main         /* main() lives in C code */

    .global Reset_Handler
    .global Default_Handler

/* ================================
 *  Vector table
 * ================================ */

    .section .isr_vector, "a", %progbits

    .word _estack          /* Initial stack pointer (from linker.ld) */
    .word Reset_Handler    /* Reset handler */
    .word Default_Handler  /* NMI */
    .word Default_Handler  /* HardFault */
    .word Default_Handler  /* MemManage */
    .word Default_Handler  /* BusFault */
    .word Default_Handler  /* UsageFault */
    .word 0                /* Reserved */
    .word 0                /* Reserved */
    .word 0                /* Reserved */
    .word 0                /* Reserved */
    .word Default_Handler  /* SVCall */
    .word Default_Handler  /* DebugMonitor */
    .word 0                /* Reserved */
    .word Default_Handler  /* PendSV */
    .word Default_Handler  /* SysTick */

    /* Simple filler for interrupts we don't use yet */
    .rept 48
    .word Default_Handler
    .endr

/* ================================
 *  Reset handler
 *  - sets up .data and .bss
 *  - calls main()
 * ================================ */

    .section .text.Reset_Handler, "ax", %progbits
Reset_Handler:
    /* Copy .data from FLASH (_sidata) to RAM (_sdata .. _edata) */

    ldr   r0, =_sidata    /* r0 = source address in FLASH */
    ldr   r1, =_sdata     /* r1 = destination in RAM */
    ldr   r2, =_edata     /* r2 = end of .data in RAM */

1:  cmp   r1, r2
    bcc   2f             /* if r1 < r2, copy more */
    b     3f             /* else done copying */

2:  ldr   r3, [r0], #4   /* load *r0 into r3, r0 += 4 */
    str   r3, [r1], #4   /* store r3 into *r1, r1 += 4 */
    b     1b

3:
    /* Zero .bss (_sbss .. _ebss) */

    ldr   r0, =_sbss
    ldr   r1, =_ebss

4:  cmp   r0, r1
    bcc   5f             /* if r0 < r1, still zeroing */
    b     6f             /* else done */

5:  movs  r2, #0
    str   r2, [r0], #4   /* store 0, r0 += 4 */
    b     4b

6:
    /* Now jump into C world */
    bl    main

    /* If main returns, just loop forever */
7:  b     7b

/* ================================
 * Default handler
 * ================================ */
    .section .text.Default_Handler, "ax", %progbits
Default_Handler:
    b Default_Handler

