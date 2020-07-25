    .macro JT symbol, address
    .section ".text.\symbol\()", "ax"
    .global \symbol
\symbol\():
    .balign 4
    ldr pc, [pc, #-4]
    .word 0x\address
    .endm

    .arch armv4

    JT strcpy, 01BB5AAC
    JT strncpy, 01BB5ABC
