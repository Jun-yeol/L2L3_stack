#pragma once
#include "graph.h"

#define MAX_PACKET_BUFFER_SIZE 2048

int
send_pkt_out(char *pkt, unsigned int pkt_size, interface_t *oif);

int
pkt_receive(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size);

int
send_pkt_flood (node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size);

