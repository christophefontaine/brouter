// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#include "gr_mpls.h"
#include "mpls_private.h"

#include <gr_datapath.h>
#include <gr_eth_output.h>
#include <gr_graph.h>
#include <gr_iface.h>
#include <gr_log.h>
#include <gr_mbuf.h>

#include <rte_ether.h>
#include <rte_graph_worker.h>
#include <rte_ip.h>
#include <rte_mpls.h>

enum {
	OUTPUT = 0,
	INVALID,
	EDGE_COUNT,
};

static struct mpls_nexthop *mpls_lookup(uint16_t, uint16_t) {
	return NULL;
}

static uint16_t mpls_forward_process(
	struct rte_graph *graph,
	struct rte_node *node,
	void **objs,
	uint16_t nb_objs
) {
	const struct iface *iface;
	//struct rte_mpls_hdr *mpls;
	struct rte_mbuf *mbuf;
	struct mpls_nexthop *nh;
	rte_edge_t next;
	uint16_t label;
	uint16_t i;

	for (i = 0; i < nb_objs; i++) {
		mbuf = objs[i];
		//	mpls = rte_pktmbuf_mtod(mbuf, struct rte_mpls_hdr *);
		iface = mpls_mbuf_data(mbuf)->input_iface;
		label = mpls_mbuf_data(mbuf)->label;

		nh = mpls_lookup(iface->vrf_id, label);
		next = OUTPUT;

		mpls_mbuf_data(mbuf)->output_iface = nh->iface;
		rte_node_enqueue_x1(graph, node, next, mbuf);
	}
	return nb_objs;
}

static void mpls_forward_register(void) { }

static struct rte_node_register mpls_forward_node = {
	.name = "mpls_forward",

	.process = mpls_forward_process,

	.nb_edges = EDGE_COUNT,
	.next_nodes = {
		//[OUTPUT] = "mpls_output",
		[OUTPUT] = "mpls_forward_unsupported",
		[INVALID] = "mpls_forward_invalid",
	},
};

static struct gr_node_info mpls_forward_info = {
	.node = &mpls_forward_node,
	.register_callback = mpls_forward_register,
};

GR_NODE_REGISTER(mpls_forward_info);

GR_DROP_REGISTER(mpls_forward_invalid);
GR_DROP_REGISTER(mpls_forward_unsupported);
