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
#include "const.hpp"
#include "atom_warehouse.hpp"
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
    int sockfd, new_fd;  // sockfd = listening socket, new_fd = client connection socket
    
    // Network address structures for server setup
    struct addrinfo hints, *servinfo, *p;  // hints = criteria, servinfo = results list, p = iterator
    struct sockaddr_storage their_addr;    // Storage for client's address information
    socklen_t sin_size;                    // Size of client address structure
    
    
    // Socket options and utility variables
    int yes=1;                    // Used for setsockopt to enable address reuse
    char s[INET6_ADDRSTRLEN];    // Buffer to store client IP address as string
    int rv;                      // Return value for getaddrinfo()

    // STEP 1: Configure server address criteria
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // Use IPv4
    hints.ai_socktype = SOCK_STREAM; // Use TCP (reliable, connection-oriented)
    hints.ai_flags = AI_PASSIVE;    // Use local machine's IP address

    // STEP 2: Get list of possible addresses to bind to
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // STEP 3: Try to create socket and bind to first available address
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Create socket with the address family, type, and protocol
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;  // Try next address if socket creation fails
        }

        // Allow socket address to be reused (prevents "Address already in use" error)
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // Bind socket to the address and port
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;  // Try next address if bind fails
        }

        break;  // Success! Exit the loop
    }

    freeaddrinfo(servinfo); // Clean up the address list

    // Check if we successfully bound to an address
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    // STEP 4: Start listening for client connections
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }


    // Array of pollfd structures to track file descriptors
    struct pollfd fds[BACKLOG + 1];  // +1 for the listening socket
    int nfds = 1;  // Number of file descriptors to monitor (start with just the listening socket)

    // Initialize the first pollfd to watch for incoming connections on the listener socket
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;  // Watch for incoming data

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
            if (fds[i].revents == 0) {
                continue;
            }

        
        // Handle errors on this file descriptor
        if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        close(fds[i].fd);
        
        // Remove this fd from the array by copying the last one to its place
        fds[i] = fds[nfds - 1];
        nfds--;
        i--;  // Process the same index again since we moved an fd here
        continue;
        }

        if (fds[i].fd == sockfd && (fds[i].revents & POLLIN)) {
            // Accept new connection
            sin_size = sizeof their_addr;
            // Accept incoming client connection (blocks until client connects)
            new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
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

         // Handle data from existing client
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
                
                // Close the socket
                close(fds[i].fd);
                
                // Remove this fd from the array (by replacing him with the last fd)
                fds[i] = fds[nfds - 1];
                nfds--;
                i--;  // Process the same index again since we moved an fd here
            } else {
                // We have data from a client
                buf[numbytes] = '\0';
                // printf("server: received '%s' on socket %d\n", buf, fds[i].fd);
                
                // Process the message (with shared counters because no fork)
                process_message(buf, numbytes);
                
                // Send response back to client
                char response[100];
                snprintf(response, sizeof(response), 
                         "Recieved message");
                         
                if (send(fds[i].fd, response, strlen(response), 0) == -1) {
                    perror("send");
                }
            }
        }
    }
}
}