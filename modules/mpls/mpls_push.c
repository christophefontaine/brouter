// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#include "gr_mpls.h"
#include "mpls_private.h"

#include <gr_datapath.h>
#include <gr_eth_input.h>
#include <gr_graph.h>
#include <gr_ip4_datapath.h>
#include <gr_log.h>
#include <gr_mbuf.h>

#include <rte_ether.h>
#include <rte_graph_worker.h>
#include <rte_ip.h>
#include <rte_mpls.h>

enum {
	FORWARD = 0,
	NO_HEADROOM,
	EDGE_COUNT,
};

static uint16_t
mpls_push_process(struct rte_graph *graph, struct rte_node *node, void **objs, uint16_t nb_objs) {
	const struct iface *iface;
	struct rte_mpls_hdr *mpls;
	struct rte_ipv4_hdr *ip;
	struct rte_mbuf *mbuf;
	struct nexthop *nh;
	rte_edge_t next;
	uint16_t i;

	for (i = 0; i < nb_objs; i++) {
		mbuf = objs[i];
		mpls = (struct rte_mpls_hdr *)rte_pktmbuf_prepend(mbuf, sizeof(*mpls));
		if (unlikely(mpls == NULL)) {
			rte_node_enqueue_x1(graph, node, NO_HEADROOM, mbuf);
			continue
		}

		mpls->ttl = 64;
		mpls->label = rte_cpu_to_be_16(mpls_mbuf_data(mbuf)->label);

		rte_node_enqueue_x1(graph, node, FORWARD, mbuf);
	}
	return nb_objs;
}

static void mpls_push_register(void) { }

static struct rte_node_register mpls_push_node = {
	.name = "mpls_push",

	.process = mpls_push_process,

	.nb_edges = EDGE_COUNT,
	.next_nodes = {
		[FORWARD] = "mpls_forward",
		[NO_HEADROOM] = "error_no_headroom",
	},
};

static struct gr_node_info mpls_push_info = {
	.node = &mpls_push_node,
	.register_callback = mpls_push_register,
};

GR_NODE_REGISTER(mpls_push_info);

GR_DROP_REGISTER(mpls_push_no_route);
GR_DROP_REGISTER(mpls_push_ttl_exceeded);
