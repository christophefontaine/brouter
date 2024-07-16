// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#ifndef _GR_CONTROL_OUTPUT_PATH
#define _GR_CONTROL_OUTPUT_PATH

#include "gr_mbuf.h"

#include <rte_graph.h>

#include <time.h>

#define CONTROL_OUTPUT_UNKNOWN (uint8_t)0xff
typedef void (*control_output_cb_t)(struct rte_mbuf *);

GR_MBUF_PRIV_DATA_TYPE(control_output_mbuf_data, {
	control_output_cb_t callback;
	clock_t timestamp;
});

struct gr_control_output_msg {
	control_output_cb_t callback;
	struct rte_mbuf *mbuf;
} __rte_packed;

void signal_control_ouput_message(void);
#endif
