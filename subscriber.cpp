#include "helpers.h"


int main(int argc, char *argv[])
{

    // disable buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
    char new_buffer[sizeof(struct tcp_packet) + 1];
	fd_set read_fds, tmp_fds;
    int bytes = 0, length;

    struct action_t action;

    // check if there are enough parameters for input
	if (argc < 3) {
		usage_of_client();
	}

  	// empty the reading fds set and the temporary fds set
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    // create the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Unable to connect to socket.");

    // initialize the data for the socket
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton failed");

    // try to connect to the server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "Unable to connect.");

	// add socket to the read_fds set
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);

    // disable Nagle algorithm for the socket
    int nagle_flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle_flag, sizeof(int));

    // send the client's id to the server
    ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
    DIE(ret < 0, "Unable to send id.");

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Unable to select a socket.");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// read data from standard input
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

            // the exit command was sent, close connection to server
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

            // the subscribe command was received => construct an action
            // with type subscribe(1) for the received topic
            if (strncmp(buffer, "subscribe", strlen("subscribe")) == 0) {
                action.action_type = 1;
                char *p;

                // preprocess subscribe command
                p = strtok(buffer, " ");

                if (p) {
                    p = strtok(NULL, " ");
                    strcpy(action.action_topic, p);
                    p = strtok(NULL, " ");

                    if (p) {
                        action.sf = atoi(p);
                        printf("Subscribed to topic.\n");
                    }
                    
                }
            }

            // the unsubscribe command was received => construct an action
            // with type unsubscribe(0) from the received topic
            if (strncmp(buffer, "unsubscribe", strlen("unsubscribe")) == 0) {
                action.action_type = 0;
                char *p;

                // preprocess subscribe command
                p = strtok(buffer, " ");

                if (p) {

                    p = strtok(NULL, " ");

                    if (p) {
                        strncpy(action.action_topic, p, 50);
                        action.sf = 3;
                        printf("Unsubscribed from topic.\n");
                    } 
                }
            }

            // send the command to the server
            ret = send(sockfd, (char *)&action, sizeof(struct action_t), 0);
            DIE(ret < 0, "Unable to send subscription to server.");

		} else if (FD_ISSET(sockfd, &tmp_fds)) {
            // received data from server
			memset(new_buffer, 0, sizeof(struct tcp_packet) + 1);
            // receive the size of the data
			ret = recv(sockfd, new_buffer, 10, 0);
            DIE(ret < 0, "Unable to receive size from server.");

            // the server has stopped the connection
            if (ret == 0) {
                break;
            } else {
                bytes = 0;
                length = atoi(new_buffer);
                memset(new_buffer, 0, sizeof(struct tcp_packet) + 1);
                // receive the package
                while (bytes < length) {
                    ret = recv(sockfd, new_buffer + bytes, length, 0);
                    DIE(ret < 0, "Unable to receive byte from server.");
                    
                    if (ret == 0) {
                        break;
                    }
                    bytes = bytes + ret;
                }
                // construct a tcp packet from the received data
                struct tcp_packet* received_packet = (struct tcp_packet *)new_buffer;
                printf("%s:%d - %s - %s - %s\n", received_packet->ip, received_packet->port,
                   received_packet->topic, received_packet->data_type, received_packet->data_payload);
            }
		}
	}

    // close the socket of the client
	close(sockfd);

	return 0;
}
