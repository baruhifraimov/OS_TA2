/**
 * @file atom_warehouse.c
 * @brief Server side - Atom warehouse management system
 * This server accepts commands to add atoms (CARBON, OXYGEN, HYDROGEN) and tracks their counts.
 * Uses IO multiplexing to handle multiple clients in parallel via forking.
 * 
 * The commands (messages) that will be accepted from client side in unsigned_int:
 * 	ADD CAROB <#>
 * 	ADD OXYGEN <#>
 * 	ADD HYDROGEN <#>
 * 
 * After each command print to stdout the current num of atoms for each type
 * 
 * For an invalid command, print ERROR
 * 
 * https://beej.us/guide/bgnet/html/split-wide/client-server-background.html#a-simple-stream-server
 * @date 2025-05-31
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include "../include/const.h"
#include "../include/functions/atom_warehouse_funcs.h"
#include <poll.h>

int main(int argc, char*argv[])
{
     // Check if port was provided as a command-line argument
     if (argc != 2) {
        fprintf(stderr,"usage: ./atom_warehouse.out <port>\n");
        exit(1);
    }

    // check that the argv[1] is only a number, else throw an error and exit
    for (int i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            fprintf(stderr, "Error: port must be a number\n");
            exit(1);
        }
    }

    // Create PORT number that have been provided from user
    const char* PORT = argv[1];

    // Socket file descriptors
    int tcp_sockfd, new_fd;  // sockfd = listening socket, new_fd = client connection socket
    
    // Network address structures for server setup
    struct addrinfo tcp_hints, *tcp_servinfo, *tcp_p;  // hints = criteria, servinfo = results list, p = iterator
    struct sockaddr_storage their_addr;    // Storage for client's address information
    socklen_t sin_size;                    // Size of client address structure
    
    
    // Socket options and utility variables
    int yes=1;                    // Used for setsockopt to enable address reuse
    char s[INET6_ADDRSTRLEN];    // Buffer to store client IP address as string
    int rv;                      // Return value for getaddrinfo()

    // STEP 1: Configure server address criteria
    memset(&tcp_hints, 0, sizeof tcp_hints);
    tcp_hints.ai_family = AF_INET;      // Use IPv4
    tcp_hints.ai_socktype = SOCK_STREAM; // Use TCP (reliable, connection-oriented)
    tcp_hints.ai_flags = AI_PASSIVE;    // Use local machine's IP address

    // STEP 2: Get list of possible addresses to bind to
    if ((rv = getaddrinfo(NULL, PORT, &tcp_hints, &tcp_servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }


    // STEP 3: Try to create socket and bind to first available address
    for(tcp_p = tcp_servinfo; tcp_p != NULL; tcp_p = tcp_p->ai_next) {
        // Create socket with the address family, type, and protocol
        if ((tcp_sockfd = socket(tcp_p->ai_family, tcp_p->ai_socktype, tcp_p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;  // Try next address if socket creation fails
        }

        // Allow socket address to be reused (prevents "Address already in use" error)
        if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // Bind socket to the address and port
        if (bind(tcp_sockfd, tcp_p->ai_addr, tcp_p->ai_addrlen) == -1) {
            close(tcp_sockfd);
            perror("server: bind");
            continue;  // Try next address if bind fails
        }

        break;  // Success! Exit the loop
    }

    freeaddrinfo(tcp_servinfo); // Clean up the address list

    // Check if we successfully bound to an address
    if (tcp_p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }



    // Create UDP socket, same as above TCP configuration
    int udp_sockfd;
    struct addrinfo udp_hints, *udp_servinfo, *udp_p;
    memset(&udp_hints, 0, sizeof udp_hints);
    udp_hints.ai_family = AF_INET;
    udp_hints.ai_socktype = SOCK_DGRAM;
    udp_hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &udp_hints, &udp_servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo (UDP): %s\n", gai_strerror(rv));
        exit(1);
    }

    for(udp_p = udp_servinfo; udp_p != NULL; udp_p = udp_p->ai_next) {
        if ((udp_sockfd = socket(udp_p->ai_family, udp_p->ai_socktype, udp_p->ai_protocol)) == -1) {
            perror("server: udp socket");
            continue;
        }
        if (bind(udp_sockfd, udp_p->ai_addr, udp_p->ai_addrlen) == -1) {
            close(udp_sockfd);
            perror("server: udp bind");
            continue;
        }
        break;
    }
    freeaddrinfo(udp_servinfo);

    if (udp_p == NULL) {
        fprintf(stderr, "server: failed to bind UDP socket\n");
        exit(1);
    }



    // STEP 4: Start listening for client connections
    if (listen(tcp_sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }


    // Array of pollfd structures to track file descriptors
    struct pollfd fds[BACKLOG + 1];  // +1 for the listening socket

    // Initialize the first pollfd to watch for incoming connections on the listener socket
    fds[0].fd = tcp_sockfd;
    fds[0].events = POLLIN;  // Watch for incoming data
    fds[1].fd = udp_sockfd;
    fds[1].events = POLLIN;
    int nfds = 2;  // Number of file descriptors to monitor the first TCP and UDP

    printf("server: waiting for connections...\n");

    // STEP 6: Main server loop - accept and handle client connections
    // TODO: NEED TO CHANGE TO IO MUX INSTEAD OF FORK PER CLIENT
    while(1) {
        sin_size = sizeof their_addr;
        
        // Wait for activity on the sockets (blocks until activity occurs)
        int poll_count = poll(fds, nfds, -1);  // -1 means wait indefinitely

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }


        // Loop through all file descriptors to check for events
        for (int i = 0; i < nfds; i++) {
            // Skip if this fd has no events
            if (fds[i].revents == 0) {continue;}

            // Handle errors on this file descriptor
            if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
               // Only close and remove if this is NOT the listening socket or UDP socket
                if (fds[i].fd != tcp_sockfd && fds[i].fd != udp_sockfd) {
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                } else {
                    // For listening/UDP sockets, just print error and continue
                    fprintf(stderr, "Critical error on listening or UDP socket (fd %d). Server should exit or restart!\n", fds[i].fd);
                    // Optionally: exit(1);
                }
                continue;
            }

        // // TCP listening socket
        if (fds[i].fd == tcp_sockfd && (fds[i].revents & POLLIN)) {
            // Accept new connection
            sin_size = sizeof their_addr;
            // Accept incoming client connection (blocks until client connects)
            new_fd = accept(tcp_sockfd, (struct sockaddr *)&their_addr, &sin_size);
            if (new_fd == -1) {
                perror("accept");
            } else {
                // Make sure we have room for a new client
                if (nfds < MAX_CLIENTS + 1) {
                    // Add the new connection to our array
                    fds[nfds].fd = new_fd;
                    fds[nfds].events = POLLIN;
                    nfds++;
                    
                    // Print connection info
                    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr),
                              s, sizeof s);
                    printf("server: new connection from %s on socket %d\n", s, new_fd);
                } else {
                    // We're at capacity, reject this client
                    printf("server: too many clients, rejecting new connection\n");
                    close(new_fd);
                }
            }
        }

        // Handle UDP socket
        if (fds[i].fd == udp_sockfd && (fds[i].revents & POLLIN)) {
            char udp_buf[MAXDATASIZE];
            struct sockaddr_storage udp_client_addr;
            socklen_t udp_addr_len = sizeof udp_client_addr;
            int udp_numbytes = recvfrom(udp_sockfd, udp_buf, sizeof(udp_buf) - 1, 0,
                                        (struct sockaddr *)&udp_client_addr, &udp_addr_len);
            if (udp_numbytes > 0) {
                udp_buf[udp_numbytes] = '\0';

                // Process the message
                char response[256];
                process_message(udp_buf, udp_numbytes, UDP_HANDLE, response, sizeof(response));

                // Send response back to UDP client
                if (sendto(udp_sockfd, response, strlen(response), 0,
                        (struct sockaddr *)&udp_client_addr, udp_addr_len) == -1) {
                    perror("sendto");
                }
            }
            continue; // Done with UDP, continue to next fd
        }

         // Handle data from existing TCP client
         else if (fds[i].revents & POLLIN) {
            // This is a client socket with data to read
            char buf[MAXDATASIZE];
            int numbytes;

            // Receive data
            numbytes = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);

            if (numbytes < 1) {
                // Error or connection closed
                if (numbytes == 0) {
                    // Connection closed normally
                    // printf("server: socket %d hung up\n", fds[i].fd);
                } else {
                    perror("recv");
                }
                
                // Remove this fd from the array (by replacing him with the last fd)
                // Only remove if this is NOT the listening socket or UDP socket
                if (fds[i].fd != tcp_sockfd && fds[i].fd != udp_sockfd) {
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                }
            } else {
                // We have data from a client
                buf[numbytes] = '\0';
                // printf("server: received '%s' on socket %d\n", buf, fds[i].fd);
                
                // Process the message
                char response[256];
                process_message(buf, numbytes, TCP_HANDLE,response, sizeof(response));
                
                // send message to client
                if (send(fds[i].fd, response, strlen(response), 0) == -1) {
                    perror("send");
                }
            }
        }
    }
}
}