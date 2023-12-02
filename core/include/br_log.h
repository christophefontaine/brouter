// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2023 Robin Jarry

#ifndef _BR_CORE_LOG
#define _BR_CORE_LOG

#include <rte_log.h>

extern int br_rte_log_type;
#define RTE_LOGTYPE_BR br_rte_log_type

#define LOG(level, fmt, ...)                                                                       \
	do {                                                                                       \
		_Static_assert(                                                                    \
			!__builtin_strchr(fmt, '\n'), "This log format string contains a \\n"      \
		);                                                                                 \
		RTE_LOG(level, BR, "%s: " fmt "\n", __func__ __VA_OPT__(, ) __VA_ARGS__);          \
	} while (0)

#endif
