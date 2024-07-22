// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Christophe Fontaine

#include "gr_mpls.h"

#include <gr_api.h>
#include <gr_cli.h>
#include <gr_cli_iface.h>
#include <gr_ip4.h>
#include <gr_net_types.h>
#include <gr_table.h>

#include <ecoli.h>
#include <libsmartcols.h>

#include <errno.h>
#include <stdint.h>

static cmd_status_t mpls_add(const struct gr_api_client *c, const struct ec_pnode *p) {
	struct gr_mpls_nh_add_req req = {0};
	struct gr_iface iface;
	uint64_t label;

	if (arg_u64(p, "LABEL", &label) < 0) {
		errno = EINVAL;
		return CMD_ERROR;
	}
	req.nh.label = (uint32_t)label;

	if (iface_from_name(c, arg_str(p, "IFACE"), &iface) < 0)
		return CMD_ERROR;
	req.nh.iface_id = iface.id;

	if (gr_api_client_send_recv(c, GR_MPLS_NH_ADD, sizeof(req), &req, NULL) < 0)
		return CMD_ERROR;

	return CMD_SUCCESS;
}

static cmd_status_t mpls_del(const struct gr_api_client *c, const struct ec_pnode *p) {
	struct gr_mpls_nh_del_req req = {.missing_ok = true};
	struct gr_iface iface;
	uint64_t label;

	if (arg_u64(p, "LABEL", &label) < 0) {
		errno = EINVAL;
		return CMD_ERROR;
	}
	req.label = (uint32_t)label;

	if (iface_from_name(c, arg_str(p, "IFACE"), &iface) < 0)
		return CMD_ERROR;
	req.vrf_id = iface.vrf_id;

	if (gr_api_client_send_recv(c, GR_MPLS_NH_DEL, sizeof(req), &req, NULL) < 0)
		return CMD_ERROR;

	return CMD_SUCCESS;
}

static cmd_status_t mpls_list(const struct gr_api_client *c, const struct ec_pnode *p) {
	struct gr_mpls_nh_list_req req = {.vrf_id = UINT16_MAX};
	struct libscols_table *table = scols_new_table();
	const struct gr_mpls_nh_list_resp *resp;
	struct gr_iface iface;
	void *resp_ptr = NULL;

	(void)p;

	if (table == NULL)
		return CMD_ERROR;
	if (arg_u16(p, "VRF", &req.vrf_id) < 0 && errno != ENOENT) {
		scols_unref_table(table);
		return CMD_ERROR;
	}
	if (gr_api_client_send_recv(c, GR_MPLS_NH_LIST, sizeof(req), &req, &resp_ptr) < 0) {
		scols_unref_table(table);
		return CMD_ERROR;
	}

	resp = resp_ptr;

	scols_table_new_column(table, "VRF", 0, 0);
	scols_table_new_column(table, "LABEL", 0, 0);
	scols_table_new_column(table, "IFACE", 0, 0);
	scols_table_set_column_separator(table, "  ");

	for (size_t i = 0; i < resp->n_nhs; i++) {
		struct libscols_line *line = scols_table_new_line(table, NULL);
		const struct gr_mpls_nh *nh = &resp->nhs[i];

		scols_line_sprintf(line, 0, "%u", nh->vrf_id);
		scols_line_sprintf(line, 1, "%d", nh->label);
		if (iface_from_id(c, nh->iface_id, &iface) == 0)
			scols_line_sprintf(line, 2, "%s", iface.name);
		else
			scols_line_sprintf(line, 2, "%u", nh->iface_id);
	}

	scols_print_table(table);
	scols_unref_table(table);
	free(resp_ptr);

	return CMD_SUCCESS;
}

static int ctx_init(struct ec_node *root) {
	int ret;

	ret = CLI_COMMAND(
		CLI_CONTEXT(root, CTX_ADD, CTX_ARG("mpls", "Associate a next hop to an interface")),
		"nexthop label LABEL iface IFACE",
		mpls_add,
		"Add a new next hop.",
		with_help("Label", ec_node_uint("LABEL", 3, UINT16_MAX, 10)),
		with_help("Output interface.", ec_node_dyn("IFACE", complete_iface_names, NULL))
	);
	if (ret < 0)
		return ret;
	ret = CLI_COMMAND(
		CLI_CONTEXT(
			root, CTX_DEL, CTX_ARG("mpls", "Disassociate a next hop to an interface")
		),
		"nexthop label LABEL iface IFACE",
		mpls_del,
		"Delete a next hop.",
		with_help("Label", ec_node_uint("LABEL", 3, UINT16_MAX, 10)),
		with_help("Output interface.", ec_node_dyn("IFACE", complete_iface_names, NULL))
	);
	if (ret < 0)
		return ret;
	ret = CLI_COMMAND(
		CLI_CONTEXT(root, CTX_SHOW, CTX_ARG("mpls", "Show mpls next hop list")),
		"nexthop [vrf VRF]",
		mpls_list,
		"List all next hops.",
		with_help("MPLS routing domain ID.", ec_node_uint("VRF", 0, UINT16_MAX - 1, 10))
	);
	if (ret < 0)
		return ret;

	return 0;
}

static struct gr_cli_context ctx = {
	.name = "mpls nexthop",
	.init = ctx_init,
};

static void __attribute__((constructor, used)) init(void) {
	register_context(&ctx);
}
