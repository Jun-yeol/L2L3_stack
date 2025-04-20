#include "comm.h"
#include "graph.h"
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h> /*for struct entry*/

static unsigned int udp_port_number = 40000;

static unsigned int
get_next_udp_port_number(){

	return udp_port_number++;
}

void
init_udp_socket(node_t *node){

	node->udp_port_number = get_next_udp_port_number();

	int udp_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct sockaddr_in node_addr;
	node_addr.sin_family		= AF_INET;
	node_addr.sin_port			= node->udp_port_number;
	node_addr.sin_addr.s_addr 	= INADDR_ANY;
	if (bind(udp_sock_fd, (struct sockaddr *)&node_addr, sizeof(struct sockaddr)) == -1)
	{
		printf("Error : socket bind failed for Node %s\n", node->node_name);
		return;
	}

	node->udp_sock_fd = udp_sock_fd;

}

static char recv_buffer[MAX_PACKET_BUFFER_SIZE];
static char send_buffer[MAX_PACKET_BUFFER_SIZE];

static void _pkt_receive(node_t *receving_node,
						char *pkt_with_aux_data,
						unsigned int pkt_size){

	char *recv_intf_name = pkt_with_aux_data;
	interface_t *recv_intf = get_node_if_by_name(receving_node, recv_intf_name);

	if(!recv_intf){
		printf("Error : Pkt recvd on unknown interface %s on node %s\n",
				recv_intf->if_name, receving_node->node_name);
		return;
	}

	pkt_receive(receving_node, recv_intf, pkt_with_aux_data + IF_NAME_SIZE,
			pkt_size - IF_NAME_SIZE);

}

static void *
_network_start_pkt_receiver_thread(void *arg){
	
	node_t *node;
	glthread_t *curr;

	fd_set active_sock_fd_set,
		   backup_sock_fd_set;

	int sock_max_fd = 0;
	int bytes_recvd = 0;

	graph_t *topo = (graph_t *)arg;

	int addr_len = sizeof(struct sockaddr);

	FD_ZERO(&active_sock_fd_set);
	FD_ZERO(&backup_sock_fd_set);

	struct sockaddr_in sender_addr;

	ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr){

		node = graph_glue_to_node(curr);

		if(!node->udp_sock_fd)
			continue;

		if(node->udp_sock_fd > sock_max_fd)
			sock_max_fd = node->udp_sock_fd;

		FD_SET(node->udp_sock_fd, &backup_sock_fd_set);
	} ITERATE_GLTHREAD_END(&topo->node_list, curr)

	/*토폴로지에서 모든 노드들의  데이터 수신 모니터링*/
	while(1)
	{
		/*모든 소켓을 활성화 하지 않는 상태로 설정*/
		// select로 활성화된 파일 디스크립터를 처리한후 이므로, 더이상 유효하지 않기때문에
		// active_sock_fd_set을 0으로 초기화
		memcpy(&active_sock_fd_set, &backup_sock_fd_set, sizeof(fd_set));

		/*소켓으로부터 데이터를 받을때마다 파일디스크립터가 활성화 되어 수신할 수 있는 상태가 됨*/
		select(sock_max_fd + 1, &active_sock_fd_set, NULL, NULL, NULL);

		ITERATE_GLTHREAD_BEGIN(&topo->node_list, curr){

			node = graph_glue_to_node(curr);

			if(FD_ISSET(node->udp_sock_fd, &active_sock_fd_set)){
				memset(recv_buffer, 0, MAX_PACKET_BUFFER_SIZE);
				bytes_recvd = recvfrom(node->udp_sock_fd, (char *)recv_buffer,
						MAX_PACKET_BUFFER_SIZE, 0,
						(struct sockaddr *)&sender_addr, &addr_len);
				_pkt_receive(node, recv_buffer, bytes_recvd);
			}

		} ITERATE_GLTHREAD_END(&topo->node_list, curr);
	}

}

void
network_start_pkt_receiver_thread(graph_t *topo){

	pthread_attr_t attr;
	pthread_t recv_pkt_thread;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // thread를 분리하여 독립적으로 자원을 해제 할 수 있음.

	pthread_create(&recv_pkt_thread, &attr,
				_network_start_pkt_receiver_thread,
				(void *)topo);
}

static int
_send_pkt_out(int sock_fd, char *pkt_data, unsigned int pkt_size,
		unsigned int dst_udp_port_no){

	int rc;
	struct sockaddr_in dest_addr;

	struct hostent *host = (struct hostent *) gethostbyname("127.0.0.1");
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = dst_udp_port_no;
	dest_addr.sin_addr = *((struct in_addr*)host->h_addr);

	rc = sendto(sock_fd, pkt_data, pkt_size, 0,
			(struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

	return rc;
}

int
send_pkt_out(char *pkt, unsigned int pkt_size,
		interface_t *interface) {

	int rc = 0;

	node_t *sending_node = interface->att_node;
	node_t *nbr_node = get_nbr_node(interface); // 목적지 노드

	if(!nbr_node)
		return -1;

	unsigned int dst_udp_port_no = nbr_node->udp_port_number;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(sock < 0){
		printf("Error : Sending socket Creation failed, errno = %d", errno);
		return -1;
	}

	// 로컬 인터페이스에서 링크 반대쪽에있는 인터페이스에 대한 포인터
	interface_t *other_interface = &interface->link->intf1 == interface ? \
								   &interface->link->intf2 : &interface->link->intf1;

	memset(send_buffer, 0, MAX_PACKET_BUFFER_SIZE);

	char *pkt_with_aux_data = send_buffer;

	strncpy(pkt_with_aux_data, other_interface->if_name, IF_NAME_SIZE);

	pkt_with_aux_data[IF_NAME_SIZE - 1] = '\0';

	// 보낼 데이터:[[intf_name][ pkt_data ]]
	memcpy(pkt_with_aux_data + IF_NAME_SIZE, pkt, pkt_size);

	rc = _send_pkt_out(sock, pkt_with_aux_data, pkt_size + IF_NAME_SIZE,
			dst_udp_port_no);

	close(sock);
	return rc;

}

extern void
layer2_frame_recv(node_t *node, interface_t *interface,
		char *pkt, unsigned int pkt_size);

int
pkt_receive(node_t *node, interface_t *interface, char *pkt, unsigned int pkt_size){

	/* 데이터 링크 계층 진입점
	 * 패킷이 외부에서 내부로 들어오는 곳
	 */

	pkt = pkt_buffer_shift_right(pkt, pkt_size,
			MAX_PACKET_BUFFER_SIZE - IF_NAME_SIZE);

	layer2_frame_recv(node, interface, pkt, pkt_size);

	return pkt_size;

}

int
send_pkt_flood (node_t *node, interface_t *exempted_intf, char *pkt, unsigned int pkt_size){

	int rc = 0;
	int index = 0;
	interface_t *interface = NULL;

	for(index; index < MAX_INTF_PER_NODE; ++index)
	{
		interface = node->intf[index];
		
		if(!interface)
			continue;

		if(interface == exempted_intf)
			continue;
		
		send_pkt_out(pkt, pkt_size, interface);

	}

	return 0;
}

int
send_pkt_flood_l2_intf_only(node_t *node,
		interface_t *exempted_intf,
		char *pkt, unsigned int pkt_size){

	int rc = 0;
	interface_t *interface = NULL;

	for(int index = 0; index < MAX_INTF_PER_NODE; ++index){

		interface = node->intf[index];

		if(!interface)
			continue;

		if(interface == exempted_intf ||
				!IS_INTF_L3_MODE(interface))
			continue;

		send_pkt_out(pkt, pkt_size, interface);
	}

	return 0;
}
