/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4/sel4.h>

/* A counter is an index to a performance counter on a platform.
 * The max counter index is sizeof(seL4_Word) */
typedef seL4_Word counter_t;
/* A counter_bitfield is used to select multiple counters.
 * Each bit corresponds to a counter id */
typedef seL4_Word counter_bitfield_t;

/* An event id is the hardware id of an event.
 * See the events.h for your architecture for specific events, caveats,
 * gotchas, and other trickery. */
typedef seL4_Word event_id_t;
