/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <sel4/sel4.h>
#include <iwdg.h>

#ifdef CONFIG_ALLOW_SMC_CALLS

#define IWDG_SMC          0xbc000000

#define IWDG_INIT         0
#define IWDG_SET_TIMEOUT  1
#define IWDG_ENABLE       2
#define IWDG_PET          3
#define IWDG_GET_TIMELEFT 4

seL4_Error stm32_iwdg_init(uint32_t *min_timeout, uint32_t *max_timeout)
{
    seL4_ARM_SMCContext smc_args = {0};
    int error;

    smc_args.x0 = IWDG_SMC;
    smc_args.x1 = IWDG_INIT;

    error = seL4_ARM_SMC_Call(seL4_CapSMC, &smc_args, &smc_args);
    if (error != seL4_NoError) {
        return error;
    }
    if (smc_args.x0 != 0) {
        return seL4_IllegalOperation;
    }
    *min_timeout = smc_args.x1; /* minimum timeout value in seconds */
    *max_timeout = smc_args.x2; /* maximum timeout value in seconds */

    return seL4_NoError;
}

seL4_Error stm32_iwdg_set_timeout(uint32_t timeout)
{
    seL4_ARM_SMCContext smc_args = {0};
    int error;

    smc_args.x0 = IWDG_SMC;
    smc_args.x1 = IWDG_SET_TIMEOUT;
    smc_args.x2 = timeout; /* configure timeout in seconds */

    error = seL4_ARM_SMC_Call(seL4_CapSMC, &smc_args, &smc_args);
    if (error != seL4_NoError) {
        return error;
    }
    if (smc_args.x0 != 0) {
        return seL4_InvalidArgument;
    }
    return seL4_NoError;
}

seL4_Error stm32_iwdg_enable(bool enable)
{
    seL4_ARM_SMCContext smc_args = {0};

    smc_args.x0 = IWDG_SMC;
    smc_args.x1 = IWDG_ENABLE;
    smc_args.x2 = enable;

    return seL4_ARM_SMC_Call(seL4_CapSMC, &smc_args, &smc_args);
}

seL4_Error stm32_iwdg_kick(void)
{
    seL4_ARM_SMCContext smc_args = {0};

    smc_args.x0 = IWDG_SMC;
    smc_args.x1 = IWDG_PET;

    return seL4_ARM_SMC_Call(seL4_CapSMC, &smc_args, &smc_args);
}

seL4_Error stm32_iwdg_get_timeleft(uint32_t *timeleft)
{
    seL4_ARM_SMCContext smc_args = {0};
    int error;

    smc_args.x0 = IWDG_SMC;
    smc_args.x1 = IWDG_GET_TIMELEFT;

    error = seL4_ARM_SMC_Call(seL4_CapSMC, &smc_args, &smc_args);
    if (error) {
        return error;
    }
    if (smc_args.x0 != 0) {
        return seL4_IllegalOperation; /* not supported (optional) or disabled */
    }
    *timeleft = smc_args.x1;
    return seL4_NoError;
}

#endif /* CONFIG_ALLOW_SMC_CALLS */
