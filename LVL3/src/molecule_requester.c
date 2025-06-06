/**
 * @file atom_supplier.c
 * @brief Client side network application that connects to a server and receives data
 * https://beej.us/guide/bgnet/html/split-wide/client-server-background.html#a-simple-stream-client
 * @date 2025-05-31
 */

#include <stdio.h>      // Standard input/output functions (printf, fprintf, etc.)
#include <stdlib.h>     // General utilities (exit, malloc, etc.)
#include <unistd.h>     // UNIX standard functions (close, etc.)
#include <errno.h>      // Error number definitions
#include <string.h>     // String manipulation functions (memset, etc.)
#include <netdb.h>      // Network database operations (getaddrinfo, etc.)
#include <sys/types.h>  // Various data type definitions 
#include <netinet/in.h> // Internet address family structures and constants
#include <sys/socket.h> // Socket API definitions
#include <ctype.h>
#include <arpa/inet.h>  // Functions for manipulating IP addresses (inet_ntop, etc.)
#include "../include/const.h"
#include "../include/functions/atom_supplier_funcs.h"

int main(int argc, char *argv[])
{
    int sockfd;                     // Socket file descriptor
    int numbytes;                   // Number of bytes received 
    char buf[MAXDATASIZE];          // Buffer to store received data
    struct addrinfo hints;          // Criteria for address selection
    struct addrinfo *servinfo;      // Linked list of results from getaddrinfo
    int rv;                         // Return value for getaddrinfo

    // Check if all the needed args was provided as a command-line argument
    if (argc != 3) {
        fprintf(stderr,"usage: ./atom_supplier.out <IP or hostname> <port>\n");
        exit(1);
    }

	 // check that the argv[1] is only a number, else throw an error and exit
	 for (int i = 0; i < strlen(argv[2]); i++) {
        if (!isdigit(argv[2][i])) {
            fprintf(stderr, "Error: port must be a number\n");
            exit(1);
        }
    }

	const char* IP = argv[1]; // The IP number the client will connect to on the server
    const char* PORT = argv[2]; // The port number the client will connect to on the server


    // Initialize hints structure with zeros
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // Allow either IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // Use UDP dgram sockets

    // Get address information for the server using the provided hostname
    if ((rv = getaddrinfo(IP, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    
    // create a UDP socket
    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0){
        perror("SOCKET CREATION FAILED");
        exit(1);
    }

    memset(&server_addr, 0,addr_len);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(PORT));

    inet_pton(AF_INET,IP,&server_addr.sin_addr);

    // GET USER INPUT
    unsigned int amount;
    char element[20];
    ask_requester(&amount, element, sizeof(element));

    // Format the message to send to server: ADD {atom} {amount}
    char send_buf[MAXDATASIZE];
    snprintf(send_buf, MAXDATASIZE, "DELIVER %s %u", element, amount);

    // Send the formatted request to server
    if (sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&server_addr, addr_len) == -1) {
        perror("sendto");
        close(sockfd);
        exit(1);
    }

    printf("client: sent request --> '%s'\n", send_buf);

    if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE-1, 0, (struct sockaddr*)&server_addr, &addr_len)) == -1) {
        perror("recv");
        exit(1);
    }

    // Null-terminate the received data to make it a valid string
    buf[numbytes] = '\0';

    // Print the message received from the server
    printf("client: received '%s'\n", buf);

    // Clean up by closing the socket
    close(sockfd);

    return 0;
}