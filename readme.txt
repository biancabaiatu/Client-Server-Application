*******************************************************************************

                            323CA - BAIATU BIANCA-DANIELA

                COMMUNICATION PROTOCOLS - CLIENT-SERVER APPLICATION

                                    MAY 2022

*******************************************************************************

    The purpose of the programm is to simulate a client-server application in
order to manage messages. The server manages the connections with the clients.

    I. SERVER IMPLEMENTATION

    -> I used the I/O multiplexing in order to manage the inputs received from 
        the TCP or UDP clients and from STDIN. 
    -> I created sockets for every one of the above mentioned connections, 
        the TCP socket is passive, which means it listens to the connection 
        requests received from the TCP clients.
    -> The server keeps running until receiving an exit command from standard
        input, when all the connections and sockets are closed

    -> The server runs with the following logic:
        * creates the TCP and UDP sockets and associates them with the given
            ports
        * creates the file descriptors' set which contains the STDIN 
            descriptor and the communication sockets
        * when a TCP client connects to the server, the programm verifies 
            whether another client with the same id is already connected and 
            deactivates the Nagle algorithm on that socket
        * if there already is an active connection, the new socket is closed 
            and the connection interrupted
        * if there is no active connection, the client must be new or an old 
            client reconnecting
            In the case of reconnecting, the server sends the client all the 
            missed messages from the subscribed topic where the store and 
            forward flag is equal to 1.
        * if the UDP communication socket is active, the server receives a 
            datagram from an UDP client (which was already implemented in the 
            structure of the programm). The UDP datagram is converted to a TCP 
            packet structure, which is then being sent to all the online TCP 
            clients that are subscribed to the given topic. 
            In the case of a disconnected TCP client that was subscribed to 
            the topic with sf = 1, the message is being saved in an
            unordered_map that maps the key = client_id to the value = vector 
            of missed TCP messages.
        * if the server receives a message with no bytes from a client, the 
            client has disconnected and its id is added to the disconnected 
            set to treat afterwards.
        * in the last case, the server receives commands from a TCP client.
            The subscription to certain topics is managed accordingly 
            using the data structures
        * in the end all the sockets are closed and the server shuts down

    II. SUBSCRIBER IMPLEMENTATION

    -> The client keeps running until receiving an exit command from standard
        input, when the socket and the connection are closed
    -> The subscriber runs with the following logic:
        * a socket is created in order to manage the communication with the 
            server. The Nagle algorithm is also deactivated on that socket
        * apart from the exit command, the client can receive two types of 
            commands from the standard input:
            -> subscribe <TOPIC> <SF>: In this case the client wants to 
                subscribe for a topic with a certain sf. 
            -> unsubscribe <TOPIC>: In this case the client wants to 
                unsubscribe from a topic
        * if the received command respects the format, it is put in a 
            action_t structure and sent to the server


    -> To deal with the errors that may appear during the programm, I used
        the DIE macro from the laboratories.

    -> For every aspect and entity I created special structures with attribute 
        packed to keep all the information needed (TCP packet, UDP packet, 
        action <subscribe / unsubscribe>)

    -> In order to manage the data flux of the TCP protocol correctly and 
        efficiently, before sending a TCP packet the server first sends the 
        size of the TCP packet and then the packet to make it easier for the
        client to make sure the message is correct. The size of the TCP 
        packet is memorized in the tcp_oacket structure and is initialized 
        when converting UDP messages to TCP messages.