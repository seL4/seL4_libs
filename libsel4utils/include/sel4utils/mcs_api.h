/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* This file provides backwards compatible API wrappers for seemlessly switching between the MCS kernel
   and non-MCS kernel without too many #ifdefs and without confusing cscope and other analysis tools.

   Of course, if any of the new functionality of the MCS kernel is leveraged, the switch won't be
   seemless.
*/

#include <autoconf.h>
#include <sel4utils/gen_config.h>
#include <sel4/sel4.h>
#include <sel4utils/sel4_zf_logif.h>

#ifdef CONFIG_KERNEL_MCS
#define CONFIG_TIMER_TICK_MS CONFIG_BOOT_THREAD_TIME_SLICE
#else
#define CONFIG_BOOT_THREAD_TIME_SLICE CONFIG_TIMER_TICK_MS
#endif

static inline seL4_MessageInfo_t api_wait(seL4_CPtr ep, seL4_Word *badge)
{
#ifdef CONFIG_KERNEL_MCS
    return seL4_Wait(ep, badge);
#else
    return seL4_Recv(ep, badge);
#endif
}

static inline seL4_MessageInfo_t api_recv(seL4_CPtr ep, seL4_Word *badge, UNUSED seL4_CPtr reply)
{
#ifdef CONFIG_KERNEL_MCS
    return seL4_Recv(ep, badge, reply);
#else
    return seL4_Recv(ep, badge);
#endif
}

static inline seL4_MessageInfo_t api_nbrecv(seL4_CPtr ep, seL4_Word *badge, UNUSED seL4_CPtr reply)
{
#ifdef CONFIG_KERNEL_MCS
    return seL4_NBRecv(ep, badge, reply);
#else
    return seL4_NBRecv(ep, badge);
#endif
}

static inline void api_reply(UNUSED seL4_CPtr reply, seL4_MessageInfo_t msg)
{
#ifdef CONFIG_KERNEL_MCS
    seL4_Send(reply, msg);
#else
    seL4_Reply(msg);
#endif
}

static inline seL4_MessageInfo_t api_reply_recv(seL4_CPtr ep, seL4_MessageInfo_t msg, seL4_Word *badge,
                                                UNUSED seL4_CPtr reply)
{
#ifdef CONFIG_KERNEL_MCS
    return seL4_ReplyRecv(ep, msg, badge, reply);
#else
    return seL4_ReplyRecv(ep, msg, badge);
#endif
}

static inline seL4_MessageInfo_t api_nbsend_recv(UNUSED seL4_CPtr send, UNUSED seL4_MessageInfo_t info,
                                                 UNUSED seL4_CPtr recv, UNUSED seL4_Word *badge,
                                                 UNUSED seL4_CPtr reply)
{
    ZF_LOGF_IF(!config_set(CONFIG_KERNEL_MCS), "Not available on non MCS kernel");
#ifdef CONFIG_KERNEL_MCS
    return seL4_NBSendRecv(send, info, recv, badge, reply);
#else
    return seL4_MessageInfo_new(0, 0, 0, 0);
#endif
}

static inline seL4_MessageInfo_t api_nbsend_wait(UNUSED seL4_CPtr send, UNUSED seL4_MessageInfo_t info,
                                                 UNUSED seL4_CPtr recv, UNUSED seL4_Word *badge)
{
    ZF_LOGF_IF(!config_set(CONFIG_KERNEL_MCS), "Not available on non MCS kernel");
#ifdef CONFIG_KERNEL_MCS
    return seL4_NBSendWait(send, info, recv, badge);
#else
    return seL4_MessageInfo_new(0, 0, 0, 0);
#endif
}

static inline seL4_Error api_tcb_configure(seL4_CPtr tcb, seL4_CPtr ep, UNUSED seL4_Word epdata,
                                           UNUSED seL4_CapRights_t eprights, UNUSED seL4_CPtr timeout_ep,
                                           UNUSED seL4_Word tfdata, UNUSED seL4_CapRights_t tfrights, UNUSED seL4_CPtr sc,
                                           seL4_CPtr cspace, seL4_Word cdata, seL4_CPtr vspace, seL4_Word vdata,
                                           seL4_Word ipc_buffer_addr, seL4_CPtr ipc_buffer_cap)
{
#ifdef CONFIG_KERNEL_MCS
    seL4_Error error = seL4_TCB_SetSpace(tcb, ep, epdata, eprights, cspace, cdata, vspace, vdata);
    if (!error) {
        error = seL4_TCB_SetIPCBuffer(tcb, ipc_buffer_addr, ipc_buffer_cap);
    }
    if (!error) {
        error = seL4_TCB_SetTimeoutEndpoint(tcb, timeout_ep, tfdata, tfrights);
    }
    if (!error && sc != seL4_CapNull) {
        error = seL4_SchedContext_Bind(sc, tcb);
    }
    return error;
#else
    return seL4_TCB_Configure(tcb, ep, cspace, cdata, vspace, vdata,
                              ipc_buffer_addr, ipc_buffer_cap);
#endif
}

static inline seL4_Error api_tcb_set_sched_params(seL4_CPtr tcb, seL4_CPtr auth, seL4_Word prio,
                                                  seL4_Word mcp, UNUSED seL4_CPtr sc, UNUSED seL4_CPtr ep,
                                                  UNUSED seL4_Word epdata, UNUSED seL4_CapRights_t eprights)
{
#ifdef CONFIG_KERNEL_MCS
    return seL4_TCB_SetSchedParams(tcb, auth, mcp, prio, sc, ep, epdata, eprights);
#else
    return seL4_TCB_SetSchedParams(tcb, auth, mcp, prio);
#endif

}

static inline seL4_Error api_tcb_set_space(seL4_CPtr tcb, seL4_CPtr ep, seL4_Word epdata,
                                           seL4_CapRights_t eprights, seL4_CPtr cspace,
                                           seL4_Word cdata, seL4_CPtr vspace, seL4_Word vdata)
{
#ifdef CONFIG_KERNEL_MCS
    return seL4_TCB_SetSpace(tcb, ep, epdata, eprights, cspace, cdata, vspace, vdata);
#else
    return seL4_TCB_SetSpace(tcb, ep, cspace, cdata, vspace, vdata);
#endif
}

static inline seL4_Error api_sc_bind(UNUSED seL4_CPtr sc, UNUSED seL4_CPtr tcb)
{
    if (!config_set(CONFIG_KERNEL_MCS)) {
        return (seL4_Error) - ENOSYS;
    }
#ifdef CONFIG_KERNEL_MCS
    return seL4_SchedContext_Bind(sc, tcb);
#endif
}

static inline seL4_Error api_sc_unbind_object(UNUSED seL4_CPtr sc, UNUSED seL4_CPtr tcb)
{
    if (!config_set(CONFIG_KERNEL_MCS)) {
        return (seL4_Error) - ENOSYS;
    }
#ifdef CONFIG_KERNEL_MCS
    return seL4_SchedContext_UnbindObject(sc, tcb);
#endif
}

static inline seL4_Error api_sc_unbind(UNUSED seL4_CPtr sc)
{
    if (!config_set(CONFIG_KERNEL_MCS)) {
        return (seL4_Error) - ENOSYS;
    }
#ifdef CONFIG_KERNEL_MCS
    return seL4_SchedContext_Unbind(sc);
#endif
}

static inline seL4_SchedContext_Consumed_t api_sc_consumed(UNUSED seL4_CPtr sc)
{
    if (!config_set(CONFIG_KERNEL_MCS)) {
        return (seL4_SchedContext_Consumed_t) {
            .error = (seL4_Error) - ENOSYS
        };
    }
#ifdef CONFIG_KERNEL_MCS
    return seL4_SchedContext_Consumed(sc);
#endif
}

static inline seL4_Error api_sched_ctrl_configure(UNUSED seL4_CPtr sched_ctrl, UNUSED seL4_CPtr sc,
                                                  UNUSED seL4_Time budget, UNUSED seL4_Time period,
                                                  UNUSED seL4_Word refills, UNUSED seL4_Word badge)
{
    if (!config_set(CONFIG_KERNEL_MCS)) {
        return (seL4_Error) - ENOSYS;
    }
#ifdef CONFIG_KERNEL_MCS
    return seL4_SchedControl_Configure(sched_ctrl, sc, budget, period, refills, badge);
#endif
}
