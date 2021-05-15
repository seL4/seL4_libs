/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/zf_log.h>
#include <utils/zf_log_if.h>
#include <sel4utils/strerror.h>

/*  sel4_zf_logif.h:
 * This file contains some convenience macros built on top of the ZF_LOG
 * library, to reduce source size and improve single-line readability.
 *
 * ZF_LOG?_IF(condition, fmt, ...):
 *  These will call the relevant ZF_LOG?() macro if "condition" evaluates to
 *  true at runtime.
 *
 */

#define ZF_LOGD_IFERR(err, fmt, ...) \
	if ((err) != seL4_NoError) \
		{ ZF_LOGD("[Err %s]:\n\t" fmt, sel4_strerror(err), ## __VA_ARGS__); }

#define ZF_LOGI_IFERR(err, fmt, ...) \
	if ((err) != seL4_NoError) \
		{ ZF_LOGI("[Err %s]:\n\t" fmt, sel4_strerror(err), ## __VA_ARGS__); }

#define ZF_LOGW_IFERR(err, fmt, ...) \
	if ((err) != seL4_NoError) \
		{ ZF_LOGW("[Err %s]:\n\t" fmt, sel4_strerror(err), ## __VA_ARGS__); }

#define ZF_LOGE_IFERR(err, fmt, ...) \
	if ((err) != seL4_NoError) \
		{ ZF_LOGE("[Err %s]:\n\t" fmt, sel4_strerror(err), ## __VA_ARGS__); }

#define ZF_LOGF_IFERR(err, fmt, ...) \
	if ((err) != seL4_NoError) \
		{ ZF_LOGF("[Err %s]:\n\t" fmt, sel4_strerror(err), ## __VA_ARGS__); }

