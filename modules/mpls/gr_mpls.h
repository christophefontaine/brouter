// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#ifndef _GR_MPLS
#define _GR_MPLS

#include <stdint.h>

#define GR_MPLS_MODULE 0xf00e

struct gr_mpls_nh {
	uint16_t vrf_id;
	uint16_t label;
	uint16_t iface_id;
};

#define GR_MPLS_NH_ADD REQUEST_TYPE(GR_MPLS_MODULE, 0x0001)

struct gr_mpls_nh_add_req {
	struct gr_mpls_nh nh;
	uint8_t exist_ok;
};

// struct gr_mpls_nh_add_resp { };

#define GR_MPLS_NH_DEL REQUEST_TYPE(GR_MPLS_MODULE, 0x0002)

struct gr_mpls_nh_del_req {
	uint16_t vrf_id;
	uint16_t label;
	uint8_t missing_ok;
};

// struct gr_mpls_nh_del_resp { };

#define GR_MPLS_NH_LIST REQUEST_TYPE(GR_MPLS_MODULE, 0x0003)

struct gr_mpls_nh_list_req {
	uint16_t vrf_id;
};

struct gr_mpls_nh_list_resp {
	uint16_t n_nhs;
	struct gr_mpls_nh nhs[/* n_nhs */];
};

struct mpls_nexthop *mpls_nexthop_get(uint32_t idx);
struct mpls_nexthop *mpls_nexthop_lookup(uint16_t vrf_id, uint16_t label);
int mpls_nexthop_add(uint16_t vrf_id, uint16_t label, uint16_t iface_id);
int mpls_nexthop_del(uint16_t vrf_id, uint16_t label);
int mpls_nexthop_list(uint16_t vrf_id, uint32_t *nh_sz, struct mpls_nexthop *nh);

void mpls_nexthop_incref(struct mpls_nexthop *nh);
void mpls_nexthop_decref(struct mpls_nexthop *nh);
#endif
