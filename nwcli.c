#include "CommandParser/libcli.h"
#include "CommandParser/cmdtlv.h"
#include "cmdcodes.h"
#include "graph.h"
#include <stdio.h>

extern graph_t *topo;

/* Generic Topology Commands */
static int
show_nw_topology_handler(param_t *param, ser_buff_t *tlv_buf,
		op_mode enable_or_disable){

	int CMDCODE = -1;
	CMDCODE = EXTRACT_CMD_CODE(tlv_buf);

	switch(CMDCODE){

		case CMDCODE_SHOW_NW_TOPOLOGY:
			dump_nw_graph(topo);
			break;
		default:
			;
	}
}

extern void
send_arp_broadcast_request(node_t *node,
		interface_t *oif,
		char *ip_addr);

static int
arp_handler(param_t *param, ser_buff_t *tlv_buf,
		op_mode enable_or_disable){
	
	node_t *node;
	char *node_name;
	char *ip_addr;
	tlv_struct_t *tlv = NULL;

	TLV_LOOP_BEGIN(tlv_buf, tlv){

		if(strncmp(tlv->leaf_id, "node-name", strlen("node-name")) ==0)
			node_name = tlv->value;
		else if(strncmp(tlv->leaf_id, "ip-address", strlen("ip-address")) ==0)
			ip_addr = tlv->value;
	} TLV_LOOP_END;

	node = get_node_by_node_name(topo, node_name);
	send_arp_broadcast_request(node, NULL, ip_addr);
	return 0;
}

void
nw_init_cli(){
	
	init_libcli();

	param_t *show	= libcli_get_show_hook();
	param_t *debug	= libcli_get_debug_hook();
	param_t *config = libcli_get_config_hook();
	param_t *run	= libcli_get_run_hook();
	param_t *debug_show = libcli_get_debug_show_hook();
	param_t *root	= libcli_get_root();

	{
		/* show topology */
		static param_t topology;
		init_param(&topology, CMD, "topology", show_nw_topology_handler, 0, INVALID,
				0, "Dump Complete Network Topology");
		libcli_register_param(show, &topology);
		set_param_cmd_code(&topology, CMDCODE_SHOW_NW_TOPOLOGY);
	}
	support_cmd_negation(config);
}
