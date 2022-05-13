#include "helpers.h"


void usage_of_server() {
	fprintf(stderr, "Usage: ./server server_port\n");
	exit(0);
}

void usage_of_client()
{
	fprintf(stderr, "Usage: ./subscriber client_id server_ip server_port\n");
	exit(0);
}

void get_fd_max(int &fd_max, fd_set *clients_fds) {
    int max = 0;
    for (int j = 3; j <= fd_max; ++j) {
        if (FD_ISSET(j, clients_fds)) {
            max = j;
        }
    }
    fd_max = max;
}

void send_stored_messages(int new_sockfd, unordered_map<string, vector<struct tcp_packet>> &stored_messages_id, char buff[1600], unordered_set<string> &disconnected_ids) {
	vector<struct tcp_packet> stored_messages = stored_messages_id[buff];
	// the client was previously disconnected
	if (disconnected_ids.find(buff) != disconnected_ids.end()) {
		// client was reconnected
		disconnected_ids.erase(buff);
		
		// send messages the client did not receive
		for (struct tcp_packet message : stored_messages) {
		send(new_sockfd, message.size, 10, 0);
		send(new_sockfd, (char*)&message, atoi(message.size), 0);
		}
		// delete the stored messages after sending them
		stored_messages_id.erase(buff);
	}
	
}

void convert_tcp_to_udp(struct tcp_packet* tcp_message, struct udp_packet* udp_message, struct sockaddr_in new_tcp_addr) {
	DIE(udp_message->data_type > 3, "Invalid type for conversion.");

	long long integer_number;
	double real_number;

	strcpy(tcp_message->ip, inet_ntoa(new_tcp_addr.sin_addr));
    tcp_message->port = ntohs(new_tcp_addr.sin_port);

	strncpy(tcp_message->topic, udp_message->topic, 50);
	tcp_message->topic[50] = '\0';

	switch (udp_message->data_type) {
		case 0:
		// received an integer number
			DIE(udp_message->data[0] > 1, "Incorrect sign byte.");

			integer_number = ntohl(*(uint32_t *) (udp_message->data + 1));
			// conversion for a negative number
			if (udp_message->data[0] != 0) {
				integer_number = -integer_number;
			}
			// save number in tcp message
			sprintf(tcp_message->data_payload, "%lld", integer_number);
			strcpy(tcp_message->data_type, "INT");
			break;

		case 1:
		// received a short number
			integer_number = ntohs(*(uint16_t*)(udp_message->data));
			real_number = integer_number * 1.0;
			// save number in tcp message
			sprintf(tcp_message->data_payload, "%.2f", real_number / 100);
			strcpy(tcp_message->data_type, "SHORT_REAL");
			break;

		case 2:
		// received a float number
			DIE(udp_message->data[0] > 1, "Incorrect sign byte.");

			real_number = ntohl(*(uint32_t *) (udp_message->data + 1)) / pow(10, udp_message->data[5]);
			// conversion for a negative number
			if (udp_message->data[0] != 0) {
				real_number = -real_number;
			}
			// save number in tcp message
			sprintf(tcp_message->data_payload, "%lf", real_number);
			strcpy(tcp_message->data_type, "FLOAT");
			break;

		default:
		// received a string
			strcpy(tcp_message->data_type, "STRING");
			// save string in tcp message
			strcpy(tcp_message->data_payload, udp_message->data);
			break;
	}

	// save the length of the packet
	sprintf(tcp_message->size, "%ld", sizeof(struct tcp_packet) - MAX_LEN + strlen(tcp_message->data_payload));

}

void send_messages(unordered_map<string, int> &ids_to_sockets, 
 							unordered_set<string> &clients_id, unordered_map<string, unordered_map<string, int>> &topics_sf,
							struct tcp_packet *tcp_message) {
	// for every connected and subscribed client sends a tcp message
	for (auto sf : topics_sf[tcp_message->topic]) {
		if (clients_id.find(sf.first) != clients_id.end()) {
			send(ids_to_sockets[sf.first], tcp_message->size, 10, 0);
			send(ids_to_sockets[sf.first], (char *) tcp_message, atoi(tcp_message->size), 0);
		}
	}
}

void close_all_sockets(fd_set *fds, int fd_max) {
	for (int i = 2; i <= fd_max; i++) {
		if (FD_ISSET(i, fds)) {
			close(i);
		}
	}
}


