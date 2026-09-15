/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdbool.h>
#include <stdint.h>

#ifdef CONFIG_ALLOW_SMC_CALLS
seL4_Error stm32_iwdg_init(uint32_t *min_timeout, uint32_t *max_timeout);
seL4_Error stm32_iwdg_enable(bool enable);
seL4_Error stm32_iwdg_kick(void);
seL4_Error stm32_iwdg_set_timeout(uint32_t timeout);
seL4_Error stm32_iwdg_get_timeleft(uint32_t *timeleft);
#endif
