// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#include "gr_mpls.h"
#include "mpls_private.h"

#include <gr_control.h>
#include <gr_iface.h>
#include <gr_log.h>

#include <rte_errno.h>
#include <rte_hash.h>
#include <rte_malloc.h>

#include <errno.h>

static struct mpls_nexthop *nh_array;
static struct rte_hash *nh_hash;

struct mpls_nexthop_key {
	uint16_t label;
	uint16_t vrf_id;
};

struct mpls_nexthop *mpls_nexthop_get(uint32_t idx) {
	return &nh_array[idx];
}

struct mpls_nexthop *mpls_nexthop_lookup(uint16_t vrf_id, uint16_t label) {
	struct mpls_nexthop_key k = {.vrf_id = vrf_id, .label = label};
	int32_t nh_idx;

	if ((nh_idx = rte_hash_lookup(nh_hash, &k)) < 0)
		return NULL;

	return &nh_array[nh_idx];
}

void mpls_nexthop_decref(struct mpls_nexthop *nh) {
	if (nh->ref_count <= 1) {
		struct mpls_nexthop_key key = {.label = nh->label, .vrf_id = nh->vrf_id};
		rte_hash_del_key(nh_hash, &key);
		memset(nh, 0, sizeof(*nh));
	} else {
		nh->ref_count--;
	}
}

void mpls_nexthop_incref(struct mpls_nexthop *nh) {
	nh->ref_count++;
}

int mpls_nexthop_add(uint16_t vrf_id, uint16_t label, uint16_t iface_id) {
	struct mpls_nexthop_key key = {.label = label, .vrf_id = vrf_id};
	int32_t nh_idx = rte_hash_add_key(nh_hash, &key);
	const struct iface *iface = iface_from_id(iface_id);

	if (nh_idx < 0)
		return errno_set(-nh_idx);
	if (iface == NULL)
		return errno_set(-ENODEV);

	nh_array[nh_idx].vrf_id = vrf_id;
	nh_array[nh_idx].label = label;
	nh_array[nh_idx].iface = iface;
	nh_array[nh_idx].ref_count = 1;

	/*
	*idx = nh_idx;
	*nh = &nh_array[nh_idx];
	*/
	return 0;
}

int mpls_nexthop_del(uint16_t vrf_id, uint16_t label) {
	struct mpls_nexthop *nh;

	nh = mpls_nexthop_lookup(vrf_id, label);
	if (nh == NULL)
		return errno_set(ENOENT);

	mpls_nexthop_decref(nh);

	return 0;
}

int mpls_nexthop_list(uint16_t vrf_id, uint32_t *nh_sz, struct mpls_nexthop *nhs) {
	struct mpls_nexthop *nh;
	const void *key;
	uint32_t iter;
	int32_t idx;
	void *data;

	*nh_sz = 0;
	iter = 0;
	while ((idx = rte_hash_iterate(nh_hash, &key, &data, &iter)) >= 0) {
		nh = mpls_nexthop_get(idx);
		if (nh->vrf_id != vrf_id && vrf_id != UINT16_MAX)
			continue;
		if (nhs) {
			nhs[*nh_sz].vrf_id = nh->vrf_id;
			nhs[*nh_sz].label = nh->label;
			nhs[*nh_sz].iface = nh->iface;
		}
		(*nh_sz)++;
	}

	return 0;
}

static void mpls_init(struct event_base *) {
	struct rte_hash_parameters params = {
		.name = "mpls_nh",
		.entries = MAX_NEXT_HOPS,
		.key_len = sizeof(struct mpls_nexthop_key),
		.extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY_LF
			| RTE_HASH_EXTRA_FLAGS_TRANS_MEM_SUPPORT,
	};
	nh_hash = rte_hash_create(&params);
	if (nh_hash == NULL)
		ABORT("rte_hash_create: %s", rte_strerror(rte_errno));

	nh_array = rte_calloc(
		"mpls_array",
		rte_hash_max_key_id(nh_hash) + 1,
		sizeof(struct mpls_nexthop),
		RTE_CACHE_LINE_SIZE
	);
	if (nh_array == NULL)
		ABORT("rte_calloc(mpls_array) failed");
}

static void mpls_fini(struct event_base *) {
	rte_hash_free(nh_hash);
	nh_hash = NULL;
	rte_free(nh_array);
	nh_array = NULL;
}

static struct gr_module mpls_module = {
	.name = "mpls nexthop",
	.init = mpls_init,
	.fini = mpls_fini,
	.fini_prio = 20000,
};

RTE_INIT(mpls_module_init) {
	gr_register_module(&mpls_module);
}
