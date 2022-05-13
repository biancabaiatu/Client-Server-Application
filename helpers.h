#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <string>
#include <iostream>

using namespace std;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1552
#define MAX_CLIENTS	10000
#define TOPIC_SIZE 	51
#define ID_LEN 11
#define MAX_LEN 1501


struct __attribute__((packed)) tcp_packet {
    uint16_t port;
    char ip[16];
    char topic[TOPIC_SIZE];
    char data_type[11];
    char data_payload[MAX_LEN];
	char size[10];
};

struct __attribute__((packed)) udp_packet {
    char topic[50];
    uint8_t data_type;
    char data[MAX_LEN];
};

struct __attribute__((packed)) action_t {
	int action_type;
	char action_topic[TOPIC_SIZE];
	int sf;
};

/**
 * @brief This functions tells the user how to open the server
 * 
 */
void usage_of_server();

/**
 * @brief This functions tells the user how to open the client
 * 
 */
void usage_of_client();

/**
 * @brief This function finds the maximum file descriptor
 * 
 * @param fd_max the maximum file descriptor
 * @param clients_fds active file descriptors
 */
void get_fd_max(int &fd_max, fd_set *clients_fds);

/**
 * @brief This functions sends old messages to an old client
 * reconnecting and with the sf = 1
 * @param new_sockfd the client's socket
 * @param stored_messages_id map with key = client_id and value = vector of messages
 * @param buff the client's id
 * @param disconnected_ids set that contains the ids of disconnected clients
 */
void send_stored_messages(int new_sockfd, unordered_map<string, vector<struct tcp_packet>> &stored_messages_id, char buff[1600], unordered_set<string> &disconnected_ids);

/**
 * @brief This function converts an udp message to a tcp message
 * 
 * @param tcp_message to be constructed
 * @param udp_message used for constructing tcp message
 * @param new_tcp_addr address of the new tcp message
 */
void convert_tcp_to_udp(struct tcp_packet* tcp_message, struct udp_packet* udp_message, struct sockaddr_in new_tcp_addr);

/**
 * @brief This function sends a tcp message to all connected clients
 * 
 * @param ids_to_sockets 
 * @param clients_id 
 * @param topics_sf 
 * @param tcp_message 
 */
void send_messages(unordered_map<string, int> &ids_to_sockets, unordered_set<string> &clients_id, unordered_map<string, unordered_map<string, int>> &topics_sf,
							struct tcp_packet *tcp_message);

void close_all_sockets(fd_set *fds, int fd_max);

#endif