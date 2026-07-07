/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * Validated against Arm Cortex-A76 TRM, version 0401, table 18-1 "PMU events".
 *
 * https://developer.arm.com/documentation/100798/0401/Performance-Monitoring-Unit/PMU-events
 */

#pragma once
/* BUS_ACCESS_RD in table 18-1 */
#define SEL4BENCH_EVENT_BUS_ACCESS_LD         0x60
/* BUS_ACCESS_WR in table 18-1 */
#define SEL4BENCH_EVENT_BUS_ACCESS_ST         0x61

#define SEL4BENCH_EVENT_BR_INDIRECT_SPEC      0x7A

#define SEL4BENCH_EVENT_EXC_IRQ               0x86
#define SEL4BENCH_EVENT_EXC_FIQ               0x87
