/* Stubs for clpru-specific constructs so clangd can parse PRU C code.
 * Included only via compile_flags.txt (-include), never by the real build. */

#ifndef __TI_COMPILER_VERSION__

#define __far
#define __attribute__(x)
#define __delay_cycles(x) ((void)(x))

volatile unsigned int __R30, __R31;

#endif
