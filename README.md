<!---
  Copyright 2017, Data61
  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
  ABN 41 687 119 230.

  This software may be distributed and modified according to the terms of
  the BSD 2-Clause license. Note that NO WARRANTY is provided.
  See "LICENSE_BSD2.txt" for details.

    @TAG(DATA61_BSD)
-->

seL4 Libraries
==============

A collection of libraries for working on seL4.

* libsel4allocman: an allocator for managing virtual memory, malloc memory and cspaces.
* libsel4bench: a library with utilities for benchmarking on seL4.
* libsel4debug: a library with utilities for debugging on seL4. Only useful when debugging a userlevel app; potentially hacky.
* libsel4muslcsys: a library to support muslc for the root task.
* libsel4platsupport: a wrapper around libplatsupport specificially for seL4.
* libsel4simple: an interface which abstracts over the boot environment of a seL4 application.
* libsel4simple-default: an implementation of simple for the master branch of the kernel.
* libsel4simple-experimental: an implementatoin of simple for the experimental branch of the kernel.
* libsel4sync: a synchronisation library that uses notifications to construct basic locks.
* libsel4test: a very basic test infrastructure library.
* libsel4utils: a library OS - Commonly used stuff, actively maintained: implements threads, processes, elf loading, virtual memory management etc.
* libsel4vka: an allocation interface for seL4.
* libsel4vspace: a virtual memory management interface for seL4.

License
========

The files in this repository are release under standard open source licenses.
Please see individual file headers and the `LICENSE_BSD2`.txt file for details.
