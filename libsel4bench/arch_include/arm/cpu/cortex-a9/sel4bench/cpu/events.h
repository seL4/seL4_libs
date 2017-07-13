/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#pragma once
/*
 * event definitions
 * events specific to the Cortex-A9
 */

/**
 * Java bytecode execute (Approximate)
 * Counts the number of Java bytecodes being decoded, including speculative
 * ones.
 */
#define SEL4BENCH_EVENT_JAVA_BYTECODE_EXECUTE    0x40

/**
 * Software Java bytecode executed (Approximate)
 * Counts the number of software java bytecodes being decoded, including
 * speculative ones.
 */
#define SEL4BENCH_EVENT_SW_JAVA_BYTECODE_EXECUTE 0x41

/**
 * Jazelle backward branches executed (Approximate)
 * Counts the number of Jazelle taken branches being executed. This includes
 * the branches that are flushed because of a previous load/store which aborts
 * late.
 */
#define SEL4BENCH_EVENT_JAZELLE_BW_BRANCHES      0x42

/**
 * Coherent linefill miss (Precise)
 * Counts the number of coherent linefill requests performed by the Cortex-A9
 * processor which also miss in all the other Cortex-A9 processors, meaning
 * that the request is sent to the external memory.
 */
#define SEL4BENCH_EVENT_LINEFILL_MISS            0x50

/**
 * Coherent linefill hit (Precise)
 * Counts the number of coherent linefill requests performed by the Cortex-A9
 * processor which hit in another Cortex-A9 processor, meaning that the
 * linefill data is fetched directly from the relevant Cortex-A9 cache.
 */
#define SEL4BENCH_EVENT_LINEFILL_HIT             0x51

/**
 * Instruction cache dependent stall cycles (Approximate)
 * Counts the number of cycles where the processor is ready to accept new
 * instructions, but does not receive any because of the instruction side not
 * being able to provide any and the instruction cache is currently performing
 * at least one linefill.
 */
#define SEL4BENCH_EVENT_ICACHE_STALL             0x60

/**
 * Data cache dependent stall cycles (Approximate)
 * Counts the number of cycles where the core has some instructions that it
 * cannot issue to any pipeline, and the Load Store unit has at least one
 * pending linefill request, and no pending TLB requests.
 */
#define SEL4BENCH_EVENT_DCACHE_STALL             0x61

/**
 * Main TLB miss stall cycles (Approximate)
 * Counts the number of cycles where the processor is stalled waiting for the
 * completion of translation table walks from the main TLB. The processor
 * stalls can be because of the instruction side not being able to provide the
 * instructions, or to the data side not being able to provide the necessary
 * data, because of them waiting for the main TLB translation table walk to
 * complete.
 */
#define SEL4BENCH_EVENT_TLBMISS_STALL            0x62

/**
 * STREX passed (Precise)
 * Counts the number of STREX instructions architecturally executed and passed.
 */
#define SEL4BENCH_EVENT_STREX_PASSED             0x63

/**
 * STREX failed (Precise)
 * Counts the number of STREX instructions architecturally executed and failed.
 */
#define SEL4BENCH_EVENT_STREX_FAILED             0x64

/**
 * Data eviction (Precise)
 * Counts the number of eviction requests because of a linefill in the data
 * cache.
 */
#define SEL4BENCH_EVENT_DATA_EVICT               0x65

/**
 * Issue does not dispatch any instruction (Precise)
 * Counts the number of cycles where the issue stage does not dispatch any
 * instruction because it is empty or cannot dispatch any instructions.
 */
#define SEL4BENCH_EVENT_ISSUE_NO_DISPATCH        0x66

/**
 * Issue is empty (Precise)
 * Counts the number of cycles where the issue stage is empty.
 */
#define SEL4BENCH_EVENT_ISSUE_EMPTY              0x67

/**
 * Instructions coming out of the core renaming stage (Approximate)
 * Counts the number of instructions going through the Register Renaming stage.
 * This number is an approximate number of the total number of instructions
 * speculatively executed, and even more approximate of the total number of
 * instructions architecturally executed. The approximation depends mainly on
 * the branch misprediction rate. The renaming stage can handle two
 * instructions in the same cycle so the event is two bits long.
 */
#define SEL4BENCH_EVENT_RENAME_INST              0x68
/* The cortex-a9 manual states that this counter implements 0x8 */
#define SEL4BENCH_EVENT_EXECUTE_INSTRUCTION SEL4BENCH_EVENT_RENAME_INST

/**
 * Predictable function returns (Approximate)
 * Counts the number of procedure returns whose condition codes do not fail,
 * excluding all returns from exception. This count includes procedure returns
 * which are flushed because of a previous load/store which aborts late.
 */
#define SEL4BENCH_EVENT_PREDICTABLE_FUNCTION_RET 0x6E

/**
 * Main execution unit instructions (Approximate)
 * Counts the number of instructions being executed in the main execution
 * pipeline of the processor, the multiply pipeline and arithmetic logic unit
 * pipeline. The counted instructions are still speculative.
 */
#define SEL4BENCH_EVENT_MAIN_EXEC_INST           0x70

/**
 * Second execution unit instructions (Approximate)
 * Counts the number of instructions being executed in the processor second
 * execution pipeline (ALU). The counted instructions are still speculative.
 */
#define SEL4BENCH_EVENT_SECOND_EXEC_INST         0x71

/**
 * Load/Store Instructions (Approximate)
 * Counts the number of instructions being executed in the Load/Store unit.
 * The counted instructions are still speculative.
 */
#define SEL4BENCH_EVENT_LOADSTORE_INST           0x72

/**
 * Floating-point instructions (Approximate)
 * Counts the number of Floating-point instructions going through the Register
 * Rename stage. Instructions are still speculative in this stage. Two
 * floating-point instructions can be renamed in the same cycle so the event is
 * two bits long.
 */
#define SEL4BENCH_EVENT_FLOAT_INST               0x73

/**
 * NEON instructions (Approximate)
 * Counts the number of NEON instructions going through the Register Rename
 * stage. Instructions are still speculative in this stage. Two NEON
 * instructions can be renamed in the same cycle so the event is two bits long.
 */
#define SEL4BENCH_EVENT_NEON_INST                0x74

/**
 * Processor stalls because of PLDs (Approximate)
 * Counts the number of cycles where the processor is stalled because PLD slots
 * are all full.
 */
#define SEL4BENCH_EVENT_PLD_STALL                0x80

/**
 * Processor stalled because of a write to memory (Approximate)
 * Counts the number of cycles when the processor is stalled and the data side
 * is stalled too because it is full and executing writes to the external
 * memory.
 */
#define SEL4BENCH_EVENT_WRITE_STALL              0x81

/**
 * Processor stalled because of instruction side main TLB miss (Approximate)
 * Counts the number of stall cycles because of main TLB misses on requests
 * issued by the instruction side.
 */
#define SEL4BENCH_EVENT_ITLBMISS_STALL           0x82

/**
 * Processor stalled because of data side main TLB miss (Approximate)
 * Counts the number of stall cycles because of main TLB misses on requests
 * issued by the data side.
 */
#define SEL4BENCH_EVENT_DTLBMISS_STALL           0x83

/**
 * Processor stalled because of instruction micro TLB miss (Approximate)
 * Counts the number of stall cycles because of micro TLB misses on the
 * instruction side. This event does not include main TLB miss stall cycles
 * that are already counted in the corresponding main TLB event.
 */
#define SEL4BENCH_EVENT_IUTLBMISS_STALL          0x84

/**
 * Processor stalled because of data micro TLB miss (Approximate)
 * Counts the number of stall cycles because of micro TLB misses on the data
 * side. This event does not include main TLB miss stall cycles that are
 * already counted in the corresponding main TLB event.
 */
#define SEL4BENCH_EVENT_DUTLBMISS_STALL          0x85

/**
 * Processor stalled because of DMB (Approximate)
 * Counts the number of stall cycles because of the execution of a DMB memory
 * barrier. This includes all DMB instructions being executed, even
 * speculatively.
 */
#define SEL4BENCH_EVENT_DMB_STALL                0x86

/**
 * Integer clock enabled (Approximate)
 * Counts the number of cycles during which the integer core clock is enabled.
 */
#define SEL4BENCH_EVENT_INTEGER_CLOCK_ENABLED    0x8A

/**
 * Data Engine clock enabled (Approximate)
 * Counts the number of cycles during which the Data Engine clock is enabled.
 */
#define SEL4BENCH_EVENT_DATA_CLOCK_ENABLED       0x8B

/**
 * ISB instructions (Precise)
 * Counts the number of ISB instructions architecturally executed.
 */
#define SEL4BENCH_EVENT_ISB_INST                 0x90

/**
 * DSB instructions (Precise)
 * Counts the number of DSB instructions architecturally executed.
 */
#define SEL4BENCH_EVENT_DSB_INST                 0x91

/**
 * DMB instructions (Approximate)
 * Counts the number of DMB instructions speculatively executed.
 */
#define SEL4BENCH_EVENT_DMB_INST                 0x92

/**
 * External interrupts (Approximate)
 * Counts the number of external interrupts executed by the processor.
 */
#define SEL4BENCH_EVENT_EXT_IRQ                  0x93

/**
 * PLE cache line request completed. (Precise)
 */
#define SEL4BENCH_EVENT_PLE_CACHELINE_COMPLETED  0xA0

/**
 * PLE cache line request skipped. (Precise)
 */
#define SEL4BENCH_EVENT_PLE_CACHELINE_SKIPPED    0xA1

/**
 * PLE FIFO flush. (Precise)
 */
#define SEL4BENCH_EVENT_PLE_FIFO_FLUSH           0xA2

/**
 * PLE request completed.
 */
#define SEL4BENCH_EVENT_PLE_COMPLETED            0xA3

/**
 * PLE FIFO overflow. (Precise)
 */
#define SEL4BENCH_EVENT_PLE_FIFO_OVERFLOW        0xA4

/**
 * PLE request programmed. (Precise)
 */
#define SEL4BENCH_EVENT_PLE_REQUESTS             0xA5
