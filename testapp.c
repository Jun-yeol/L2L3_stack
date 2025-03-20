#include "graph.h"
#include "comm.h"
#include <stdio.h>
#include "CommandParser/libcli.h"

extern void nw_init_cli();
extern graph_t *build_first_topo();
graph_t *topo = NULL;

int main(int argc, char **argv){
	
	nw_init_cli();
	topo = build_first_topo();

	sleep(2);

	node_t *srcnode = get_node_by_node_name(topo, "R0_re");
	interface_t *outIntf = get_node_if_by_name(srcnode, "eth0/0");

	char msg[] = "Hello, how are you\0";
	send_pkt_out(msg, strlen(msg), outIntf);

	start_shell();
	return 0;
}
