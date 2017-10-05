<!---
  Copyright 2017, Data61
  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
  ABN 41 687 119 230.

  This software may be distributed and modified according to the terms of
  the BSD 2-Clause license. Note that NO WARRANTY is provided.
  See "LICENSE_BSD2.txt" for details.

    @TAG(DATA61_BSD)
-->

libsel4utils
=============

libsel4utils provides OS-like utilities for building benchmarks and applications on seL4.
Although the library attempts to be policy agnostic, some decisions must be made.

This library is intended to be used to quickly get tests, benchmarks and prototypes off the ground
with minimal effort. It's meant to contain commonly useful stuff, that will be actively
maintained.
It can also be used as a reference for 'how-to' implement OS mechanisms on seL4.

Utilities provided by this library:

  * threads
  * processes
  * elf loading
  * virtual memory management
  * stack switching
  * debugging tools

No allocator is provided, although any allocator that implements the seL4 vka interface can be used
(we recommend libsel4allocman).

Dependencies
------------------

libsel4utils depends on libsel4vka, libsel4vspace, libutils, libelf, libcpio, libsel4.

Repository overview
-------------------

*include/sel4utils*:

  * elf.h -- elf loading.
  * mapping.h -- page mapping.
  * process.h -- process creation, deletion.
  * profile.h -- profiling.
  * strerror.h -- for printing seL4 error codes.
  * stack.h -- switch to a newly allocated stack.
  * thread.h -- threads (kernel threads) creation, deletion.
  * util.h -- includes utilities from libutils.
  * vspace.h -- virtual memory management (implements vspace interface)
  * vspace_internal.h -- virtual memory management internals, for hacking the above.

*arch_include/sel4utils*:

  * util.h -- utils to assist in writing arch independent code.

Configuration options
----------------------

* `SEL4UTILS_STACK_SIZE` -- the default stack size to use for processes and threads.
* `SEL4UTILS_CSPACE_SIZE_BITS` -- the default cspace size for new processes (threads use the current
                                cspace).

License
========

The files in this repository are release under standard open source licenses.
Please see individual file headers and the `LICENSE_BSD2`.txt file for details.
