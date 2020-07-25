#include <NewtonScript.h>

__attribute__((naked)) void einstein_puts(const char *str)
{
    asm("stmdb   sp!, {r1, lr}\n"
        "ldr     lr, id0\n"
        "mov     r1, r0\n"
        "mcr     p10, 0, lr, c0, c0\n"
        "ldmia   sp!, {r1, pc}\n"
        "id0:\n"
        ".word      0x11a\n");
}

const char *msg = "Hello, World!";

extern "C" Ref Hello(RefArg inRcvr)
{
    char buffer[32];
    buffer[sizeof(buffer) - 1] = 0;
    strncpy(buffer, msg, sizeof(buffer) - 1);
    einstein_puts(buffer);
    return MAKEINT(0);
}
