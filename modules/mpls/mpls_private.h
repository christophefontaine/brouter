// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#ifndef _MPLS_PRIVATE
#define _MPLS_PRIVATE

#include <gr_iface.h>
#include <gr_mbuf.h>

#include <stdint.h>

#define MAX_NEXT_HOPS (1 << 16)

GR_MBUF_PRIV_DATA_TYPE(mpls_mbuf_data, {
	union {
		const struct iface *input_iface;
		const struct iface *output_iface;
	};
	uint16_t label;
});

struct mpls_nexthop {
	int32_t ref_count;
	uint16_t vrf_id;
	uint16_t label;
	const struct iface *iface;
	struct ip4_net addr;
};

#endif
