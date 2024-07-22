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
	IP_OUTPUT,
	NO_ROUTE,
	TTL_EXCEEDED,
	EDGE_COUNT,
};

static uint16_t
mpls_input_process(struct rte_graph *graph, struct rte_node *node, void **objs, uint16_t nb_objs) {
	const struct iface *iface;
	struct rte_mpls_hdr *mpls;
	struct rte_ipv4_hdr *ip;
	struct rte_mbuf *mbuf;
	struct nexthop *nh;
	rte_edge_t next;
	uint16_t i;
	uint8_t ttl;

	for (i = 0; i < nb_objs; i++) {
		mbuf = objs[i];
		mpls = rte_pktmbuf_mtod(mbuf, struct rte_mpls_hdr *);
		rte_pktmbuf_adj(mbuf, sizeof(*mpls));

		iface = eth_input_mbuf_data(mbuf)->iface;
		ttl = mpls->ttl;
		nh = NULL;

		if (unlikely(ttl <= 1 && !mpls->bs)) {
			next = TTL_EXCEEDED;
			goto next_packet;
		}
		mpls = rte_pktmbuf_mtod(mbuf, struct rte_mpls_hdr *);
		mpls->ttl = ttl - 1;

		if (mpls->bs) {
			ip = rte_pktmbuf_mtod(mbuf, struct rte_ipv4_hdr *);
			// Do we mix mpls vrf and ip vrf ?
			nh = ip4_route_lookup(iface->vrf_id, ip->dst_addr);

			ip_output_mbuf_data(mbuf)->input_iface = iface;
			ip_output_mbuf_data(mbuf)->nh = nh;
			// TODO: check RFC to know if we have to copy
			// MPLS TTL to IP TTL

			if (nh == NULL)
				next = NO_ROUTE;
			else
				next = IP_OUTPUT;
			goto next_packet;
		}

		mpls_mbuf_data(mbuf)->input_iface = iface;
		mpls_mbuf_data(mbuf)->label = mpls->tag_lsb | (mpls->tag_msb << 8);
		next = FORWARD;

next_packet:
		rte_node_enqueue_x1(graph, node, next, mbuf);
	}
	return nb_objs;
}

static void mpls_input_register(void) {
	gr_eth_input_add_type(rte_cpu_to_be_16(RTE_ETHER_TYPE_MPLS), "mpls_input");
}

static struct rte_node_register mpls_input_node = {
	.name = "mpls_input",

	.process = mpls_input_process,

	.nb_edges = EDGE_COUNT,
	.next_nodes = {
		[FORWARD] = "mpls_forward",
		[IP_OUTPUT] = "ip_forward",
		[NO_ROUTE] = "mpls_input_no_route",
		[TTL_EXCEEDED] = "mpls_input_ttl_exceeded",
	},
};

static struct gr_node_info mpls_input_info = {
	.node = &mpls_input_node,
	.register_callback = mpls_input_register,
};

GR_NODE_REGISTER(mpls_input_info);

GR_DROP_REGISTER(mpls_input_no_route);
GR_DROP_REGISTER(mpls_input_ttl_exceeded);
