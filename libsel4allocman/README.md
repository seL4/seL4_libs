<!---
  Copyright 2017, Data61
  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
  ABN 41 687 119 230.

  This software may be distributed and modified according to the terms of
  the BSD 2-Clause license. Note that NO WARRANTY is provided.
  See "LICENSE_BSD2.txt" for details.

    @TAG(DATA61_BSD)
-->

libsel4allocman
===============

libsel4allocman provides a way to allocate different kinds of resources from the
'base' resource type in seL4, which is untyped memory.

It provides by an implementation of the `vka_t` interface as well as a native
interface for accessing features not exposed by vka.

Allocation overview
-------------------

Allocation is complex due to the circular dependencies that exist on allocating resources. These dependencies
are loosely described as

  * Capability slots: Allocated from untypeds, book kept in memory
  * Untypeds / other objects (including frame objects): Allocated from other untypeds, into capability slots,
    book kept in memory
  * memory: Requires frame object

Note that these dependencies and complications only exist if you want to be able to track, book keep, free and reuse
all allocations. If you want a simpler 'allocate only' system, that does not require book keeping to support free,
then these problems don't exist.

Initially, because of these circular dependencies, nothing can be allocated. The static pool provides a way to inject
one of the dependencies and try and start the cycle.

The static and virtual pools can be thought of as just being another heap. Allocman uses these to allocate book
keeping data in preference to the regular C heap due to usage in environments where the C heap is either not
implemented or is implemented using allocman.

Two pools are needed, one static and one virtual, because the virtual pool (along with the rest of allocman) require
memory in order to bootstrap. Once bootstrapping is done then provided the system does not run out of memory enough
space in the virtual pool will also be reserved to ensure that if the virtual pool needs to grow, there is enough
memory for any book keeping required.
