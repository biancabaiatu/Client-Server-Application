#include "helpers.h"


int main(int argc, char *argv[]) {

    // disable buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // check if there are enough parameters for input
    if (argc < 2) {
		usage_of_server();
	}

    int sock_tcp, sock_udp, new_sockfd;
    int port_nr, nagle_flag, ret;
	char buffer[BUFLEN];
	struct sockaddr_in tcp_addr, udp_addr, new_tcp_addr;
    
	socklen_t tcp_len = sizeof(struct sockaddr_in);
	socklen_t udp_len = sizeof(struct sockaddr_in);

    // map that stores ids of clients to a topic and the client's sf for
    // that topic
    unordered_map<string, unordered_map<string, int>> topic_sf;

    // set of the active clients' ids
    unordered_set<string> clients_id;

    // map of active clients by socket file descriptor with value id
    unordered_map<int, string> online_clients_sockfd;

    // map that makes a connection between a client's id and socket
    // file descriptor
    unordered_map<string, int> ids_to_sockets;

    // map that stores the message for a client's id on a topic with sf = 1
    // when the client has disconnected
    unordered_map<string, vector<struct tcp_packet>> stored_messages;

    // set of ids of the clients that have disconnected from server
    unordered_set<string> disconnected_ids;

	fd_set read_fds;	// reading set used in select()
	fd_set tmp_fds;		// temporarily used set
	int fd_max;			// maximum value of a file descriptor

	// empty the reading fds set and the temporary fds set
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    // create the TCP socket
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "Unable to create TCP socket");

    // create the UDP socket
    sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sock_udp < 0, "Unable to create UDP socket");

    // find the port
	port_nr = atoi(argv[1]);
	DIE(port_nr == 0, "Unable to convert port number.");

    // initialize the data for the TCP socket
	memset((char *) &tcp_addr, 0, sizeof(tcp_addr));
	tcp_addr.sin_family = AF_INET;
	tcp_addr.sin_port = htons(port_nr);
	tcp_addr.sin_addr.s_addr = INADDR_ANY;

    // initialize the data for the UDP socket
    memset((char *) &udp_addr, 0, sizeof(udp_addr));
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(port_nr);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

    // bind the TCP socket
	ret = bind(sock_tcp, (struct sockaddr *) &tcp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Unable to bind TCP socket");

    // bind the UDP socket
    ret = bind(sock_udp, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "Unable to bind UDP socket");

    // listen on the TCP socket
	ret = listen(sock_tcp, MAX_CLIENTS);
	DIE(ret < 0, "Unable to listen on the TCP socket");

	// add both sockets to the read_fds set
	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
    FD_SET(0, &read_fds);

	fd_max = sock_tcp;
    int exit_opt = 0;

	while (!exit_opt) {
		tmp_fds = read_fds; 
		
		ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

        // search the sockets that send data
		for (int i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

                // receives data from STDIN
                if (i == 0) {
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);

                    // the message can only be the exit command
                    // that stops the programm
                    if (strcmp(buffer, "exit\n") == 0) {
                        exit_opt = 1;
                    } else {
                        printf("Server can only execute exit command\n");
                        continue;
                    }
                } else if (i == sock_udp) {
                    // received data from one of the udp clients
					memset(buffer, 0, BUFLEN);
					ret = recvfrom(sock_udp, buffer, BUFLEN, 0, (struct sockaddr *) &new_tcp_addr, &udp_len);
					DIE(ret < 0, "Unable to receive from the UDP socket.");

                    struct tcp_packet tcp_message;
                    struct udp_packet* udp_message;

                    // cast the received data to a udp_packet
                    udp_message = (struct udp_packet *) buffer;

                    // convert the udp message to a tcp packet
                    convert_tcp_to_udp(&tcp_message, udp_message, new_tcp_addr);
                    // send the message to the client
                    send_messages(ids_to_sockets, clients_id, topic_sf, &tcp_message);
                    
                    // save the message for a disconnected client with sf = 1
                    for (auto sf : topic_sf[tcp_message.topic]) {
			            if (sf.second == 1) {
				            if (stored_messages.find(sf.first) != stored_messages.end()) {
					            stored_messages[sf.first].push_back(tcp_message);
				            } else {
                                vector<struct tcp_packet> stored;
                                stored.push_back(tcp_message);
                                stored_messages[sf.first] = stored;
				            }
			            }
		            }
	
				} else if (i == sock_tcp) {
					// received a tcp packet
					new_sockfd = accept(sock_tcp, (struct sockaddr *) &new_tcp_addr, &tcp_len);
					DIE(new_sockfd < 0, "accept");

                    // disable Nagle algorithm for the socket
                    nagle_flag = 1;
                    setsockopt(new_sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle_flag, sizeof(int));

					// add the socket to the reading sockets' set
					FD_SET(new_sockfd, &read_fds);
					
                    // update the greatest file descriptor
                    if (new_sockfd > fd_max) { 
						fd_max = new_sockfd;
					}
                    
                    // receive the client's ID
                    memset(buffer, 0, BUFLEN);
                    ret = recv(new_sockfd, buffer, BUFLEN, 0);
                    DIE(ret < 0, "Unable to receive client ID");

                    // search for the received id
                    if (clients_id.find(buffer) != clients_id.end()) {
                        printf("Client %s already connected.\n", buffer);
                        FD_CLR(new_sockfd, &read_fds);
                        // close the new socket because there is another client with same id
                        close(new_sockfd);

                        // update the maximum file descriptor
                        get_fd_max(fd_max, &read_fds);
                        continue;
                    } else {
                        // whether the client is reconnecting or new we save its info
                        online_clients_sockfd[new_sockfd] = string(buffer);
                        clients_id.insert(string(buffer));
                        ids_to_sockets[string(buffer)] = new_sockfd;
                        
                        printf("New client %s connected from %s:%d.\n", buffer,
                         inet_ntoa(new_tcp_addr.sin_addr), ntohs(new_tcp_addr.sin_port));

                        // if the client is reconnecting we send the old messages
                        send_stored_messages(new_sockfd, stored_messages, buffer, disconnected_ids);
                    }


				} else {
                	// received tcp data from a subscriber
                    memset(buffer, 0, BUFLEN);
                    ret = recv(i, buffer, sizeof(struct action_t), 0);
                    DIE(ret < 0, "Unable to receive command from subscriber.");

                    if (ret == 0) {
                        // the client stopped the connection
                        cout<<"Client "<<online_clients_sockfd[i]<<" disconnected.\n";
                        disconnected_ids.insert(online_clients_sockfd[i]);
                        ids_to_sockets.erase(online_clients_sockfd[i]);
                        clients_id.erase(online_clients_sockfd[i]);
                        online_clients_sockfd.erase(i);

                        FD_CLR(i, &read_fds);

                        // update the maximum file descriptor
                        get_fd_max(fd_max, &read_fds);
                        // close the current socket
                        close(i);
                    } else {
                        // the subscriber wants to perform an action
                        struct action_t* action = (struct action_t *) buffer;
                        if (action->action_type == 1) {
                            // the clients subscribes for a topic
                            topic_sf[action->action_topic][online_clients_sockfd[i]] = action->sf;
                        } else {
                            // the client unsubscribes from a topic
                            topic_sf[action->action_topic].erase(online_clients_sockfd[i]);
                        }
                    }
                }
			}
		}
	}

    // close all the active sockets
    close_all_sockets(&read_fds, fd_max);

	return 0;
}