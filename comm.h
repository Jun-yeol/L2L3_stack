#pragma once
#include "graph.h"

#define MAX_PACKET_BUFFER_SIZE 2048

int
send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *oif);

int
pkt_receive(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size);

int
send_pkt_flood (node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size);

int
send_pkt_flood_l2_intf_only(node_t *node,
		interface_t *exempted_intf, /*Inteface on which the frame was recvd by L2 switch*/
		char *pkt, unsigned int pkt_size);

