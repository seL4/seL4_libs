/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *
 * ARM Cortex-A55 performance monitoring events
 *
 */

#pragma once

// Events are defined in the Cortex-A55 TRM. Most recent version as of today is
// "ARM 100442_0200_01_en" (2022-Aug-31, "Second release for r2p0"),
// section 4.2.4 "PMU events". The definitions for the Cortex-A55 core are
// aligned with the ARM Architecture Reference Manual for ARMv8. Most recent
// version as of today is "ARM DDI 0487I.a" (2022-Aug-19, "Updated Armv9 EAC
// release incorporating BRBE, ETE, and TRBE"). See section D11.11.2 "The PMU
// event number space and common events"

#define SEL4BENCH_EVENT_BUS_ACCESS_LD         0x60 /* BUS_ACCESS_RD */
#define SEL4BENCH_EVENT_BUS_ACCESS_ST         0x61 /* BUS_ACCESS_WR */
#define SEL4BENCH_EVENT_BR_INDIRECT_SPEC      0x7A
#define SEL4BENCH_EVENT_EXC_IRQ               0x86
#define SEL4BENCH_EVENT_EXC_FIQ               0x87
