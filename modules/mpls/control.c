// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#include "gr_mpls.h"
#include "mpls_private.h"

#include <gr_api.h>
#include <gr_control.h>
#include <gr_log.h>
#include <gr_net_types.h>
#include <gr_queue.h>

static struct api_out mpls_add(const void *request, void **) {
	const struct gr_mpls_nh_add_req *req = request;
	struct iface *iface;
	int ret;

	if ((iface = iface_from_id(req->nh.iface_id)) == NULL) {
		return api_out(errno, 0);
	}

	ret = mpls_nexthop_add(req->nh.vrf_id, req->nh.label, iface->id);
	return api_out(-ret, 0);
}

static struct api_out mpls_del(const void *request, void **) {
	const struct gr_mpls_nh_del_req *req = request;
	int ret;

	ret = mpls_nexthop_del(req->vrf_id, req->label);
	return api_out(-ret, 0);
}

static struct api_out mpls_list(const void *request, void **response) {
	const struct gr_mpls_nh_list_req *req = request;
	struct gr_mpls_nh_list_resp *resp;
	struct mpls_nexthop *nh;
	uint32_t nh_sz = 0;
	uint32_t len;
	int ret;

	if ((ret = mpls_nexthop_list(req->vrf_id, &nh_sz, NULL)) < 0) {
		return api_out(-ret, 0);
	}

	len = sizeof(*resp) + nh_sz * sizeof(struct gr_mpls_nh);
	resp = calloc(len, 1);
	nh = calloc(nh_sz, sizeof(*nh));
	if (!resp || !nh) {
		free(nh);
		free(resp);
		return api_out(ENOMEM, 0);
	}
	resp->n_nhs = nh_sz;

	if ((ret = mpls_nexthop_list(req->vrf_id, &nh_sz, nh)) < 0) {
		return api_out(-ret, 0);
	}

	for (uint16_t i = 0; i < nh_sz; i++) {
		resp->nhs[i].vrf_id = nh[i].vrf_id;
		resp->nhs[i].label = nh[i].label;
		resp->nhs[i].iface_id = nh[i].iface->id;
	}
	*response = resp;

	free(nh);
	return api_out(0, len);
}

static struct gr_api_handler mpls_add_handler = {
	.name = "mpls nexthop add",
	.request_type = GR_MPLS_NH_ADD,
	.callback = mpls_add,
};
static struct gr_api_handler mpls_del_handler = {
	.name = "mpls nexthop del",
	.request_type = GR_MPLS_NH_DEL,
	.callback = mpls_del,
};
static struct gr_api_handler mpls_list_handler = {
	.name = "mpls nexthop list",
	.request_type = GR_MPLS_NH_LIST,
	.callback = mpls_list,
};

RTE_INIT(control_mpls_init) {
	gr_register_api_handler(&mpls_add_handler);
	gr_register_api_handler(&mpls_del_handler);
	gr_register_api_handler(&mpls_list_handler);
}
