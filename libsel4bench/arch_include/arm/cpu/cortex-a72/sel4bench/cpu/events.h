/*
 * ARM Cortex-A72 performance monitoring events
 *
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// Events are defined in the Cortex-A72 TRM. Most recent version as of today is
// "ARM 100095_0003_06_en" (2016-Dec-01, "Second release for r0p3"),
// section 11.8 "Performance Monitor Unit", "Events". The definitions for the
// Cortex-A72 core are aligned with the ARM Architecture Reference Manual for
// ARMv8. Most recent version as of today is "ARM DDI 0487G.a" (2021-Jan-22,
// "Initial Armv8.7 EAC release"). See section K3.1 "Arm recommendations for
// IMPLEMENTATION DEFINED event numbers"

#define SEL4BENCH_EVENT_BUS_ACCESS_LD         0x60
#define SEL4BENCH_EVENT_BUS_ACCESS_ST         0x61

#define SEL4BENCH_EVENT_BR_INDIRECT_SPEC      0x7A

#define SEL4BENCH_EVENT_EXC_IRQ               0x86
#define SEL4BENCH_EVENT_EXC_FIQ               0x87
