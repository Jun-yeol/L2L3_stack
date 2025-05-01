#include "net.h"
#include "graph.h"
#include "utils.h"
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/* Just some Random numberr generator*/
static unsigned int
hash_code(void *ptr, unsigned int size){
	unsigned int value=0, i=0;
	char *str = (char*)ptr;
	while(i<size)
	{
		value += *str;
		value *= 97;
		str++;
		i++;
	}

	return value;
}

/*Heuristics, Assign a unique mac address to interface */
void
interface_assign_mac_address(interface_t *interface){

	node_t *node = interface->att_node;


	if(!node)
		return;
	
	/*유일한 MAC 주소값을 위한 hash code*/
	unsigned int hash_code_val1 = hash_code(node->node_name, NODE_NAME_SIZE);
	unsigned int hash_code_val2 = hash_code(interface->if_name, IF_NAME_SIZE);
	unsigned int hash_combined = hash_code_val1 * hash_code_val2;

	unsigned char *mac = IF_MAC(interface);

	mac[0] = 0x02; // 제조사 임의값 
	mac[1] = (hash_combined >> 0) & 0xFF;
	mac[2] = (hash_combined >> 8) & 0xFF;
	mac[3] = (hash_combined >> 16) & 0xFF;
	mac[4] = (hash_combined >> 24) & 0xFF;
	mac[5] = (hash_code_val2 & 0xFF); 
}

bool_t node_set_loopback_address(node_t *node, char *ip_addr){

	assert(ip_addr);

	node->node_nw_props.is_lb_addr_config = TRUE;
	strncpy(NODE_LO_ADDR(node), ip_addr, 16);
	NODE_LO_ADDR(node)[15] = '\0';

	return TRUE;
}

bool_t node_set_intf_ip_address(node_t *node, char *local_if,
								char *ip_addr, char mask) {

	interface_t *interface = get_node_if_by_name(node, local_if);
	if(!interface) assert(0);

	strncpy(IF_IP(interface), ip_addr, 16);
	IF_IP(interface)[15] = '\0';
	interface->intf_nw_props.mask = mask;
	interface->intf_nw_props.is_ipadd_config = TRUE;

	return TRUE;
}

bool_t node_unset_intf_ip_address(node_t *node, char *local_if){

	return TRUE;
}

interface_t *
node_get_matching_subnet_interface(node_t *node, char *ip_addr){
	
	interface_t *interface = NULL;

	char intf_subnet[16];
	char cmp_subnet[16];
	char mask;
	
	for(int i = 0; i < MAX_INTF_PER_NODE; ++i)
	{
		interface = node->intf[i];
		if(!interface) return NULL;

		if(interface->intf_nw_props.is_ipadd_config == FALSE)
			continue;

		mask = interface->intf_nw_props.mask;

		memset(intf_subnet, 0, 16);
		memset(cmp_subnet, 0, 16);
		apply_mask(IF_IP(interface), mask, intf_subnet); 
		apply_mask(ip_addr, mask, cmp_subnet); 

		if(strncmp(intf_subnet, cmp_subnet, 16) == 0)
			return interface;

	}

}



void dump_nw_graph(graph_t *graph){

	node_t *node;
	glthread_t *curr;
	interface_t *interface;

	printf("Topology Name = %s\n", graph->topology_name);

	ITERATE_GLTHREAD_BEGIN(&graph->node_list, curr){

		node = graph_glue_to_node(curr);

		if(!node) return;

		dump_node_nw_props(node);
		for(unsigned int i = 0 ; i < MAX_INTF_PER_NODE; i++){
			interface = node->intf[i];
			if(!interface) continue;
			dump_intf_props(interface);
		}
	} ITERATE_GLTHREAD_END(&graph->node_list, curr);
}

void dump_node_nw_props(node_t *node){
	
	printf("\n Node Name = %s \n", node->node_name);
	printf("\t node flags : %u", node->node_nw_props.flags);
	if(node->node_nw_props.is_lb_addr_config)
	{
		printf("\t lo addr : %s/32", NODE_LO_ADDR(node));
	}
	printf("\n");
}

void dump_intf_props(interface_t *interface){

	if(interface->intf_nw_props.is_ipadd_config){
		printf("\t IP Addr = %s/%u", IF_IP(interface), interface->intf_nw_props.mask);
		printf("\t MAC : %u:%u:%u:%u:%u:%u\n",
				IF_MAC(interface)[0], IF_MAC(interface)[1],
				IF_MAC(interface)[2], IF_MAC(interface)[3],
				IF_MAC(interface)[4], IF_MAC(interface)[5]);
	}
	
}

unsigned int
convert_ip_from_str_to_int(char *ip_addr){

	unsigned int binary_ip =0;
	assert(ip_addr);

	inet_pton(AF_INET, ip_addr, &binary_ip);
	binary_ip = htonl(binary_ip);

	return binary_ip;
}

void
convert_ip_from_int_to_str(unsigned int ip_addr, char *output_buffer){

	inet_ntop(AF_INET, &ip_addr, output_buffer, 16);

	output_buffer[16] = '\0';
}

char *
pkt_buffer_shift_right(char *pkt, unsigned int pkt_size,
				unsigned int total_buffer_size){

	char *pkt_right = NULL;
	char *buffer_start = pkt;

	if( pkt < buffer_start || pkt + pkt_size > buffer_start + total_buffer_size
			|| pkt_size > total_buffer_size)
		return NULL;
	
	pkt_right = pkt + (total_buffer_size - pkt_size);
	memmove(pkt_right, pkt, pkt_size);
	memset(curr, 0, pkt_size);

	return pkt_right;
}
