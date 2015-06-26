/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/* These symbols are provided by the linker. */
extern char __init_array_start[];
extern char __init_array_end[];

/* Run pre-main initialisation functions. This function is called from _start.
 */
void _init(void)
{
    /* Commented out because this functionality doesn't work during certain links.
     * It's unclear to me exactly why, but sometimes the linker doesn't give you
     * the externs declared above. This should probably eventually be repaired
     * using crtbegin.o and crtend.o.
     */
#if 0

    /* The 'ctors' section of the ELF file contains an array of pointers to
     * functions that have been declared as constructors. This array is
     * delineated by the __init_array_start and __init_array_end symbols and
     * already ordered by constructor priority by the compiler.
     *
     * XXX: This does not work on x86 because our toolchain seems to always
     * incorrectly set __init_array_end to the start of the array.
     */
    typedef void(*ctor_t)(void);
    for (ctor_t *f = (ctor_t*)__init_array_start; f < (ctor_t*)__init_array_end;
            f++) {
        (*f)();
    }
#endif
}
