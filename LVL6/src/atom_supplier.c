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
#include <unistd.h>
#include <getopt.h>
#include <sys/un.h>

// for get opt
extern char *optarg;
extern int optind, opterr, optopt;

const char* IP; // The IP number the client will connect to on the server
const char* PORT; // The port number the client will connect to on the server
const char* PATH; // Path for the AF_UNIX prot

// ARGUMENTS FLAGS
int flag_f = 0;
int flag_h = 0;
int flag_p = 0;

int main(int argc, char *argv[])
{
    int sockfd;                     // Socket file descriptor
    int numbytes;                   // Number of bytes received 
    char buf[MAXDATASIZE];          // Buffer to store received data
    struct addrinfo hints;          // Criteria for address selection
    struct addrinfo *servinfo;      // Linked list of results from getaddrinfo
    struct addrinfo *p;             // Pointer to traverse servinfo list
    int rv;                         // Return value for getaddrinfo
    char s[INET6_ADDRSTRLEN];       // Buffer to store string representation of IP address

    // Check if all the needed args was provided as a command-line argument
    if (argc < 3) {
        fprintf(stderr,"usage: ./atom_supplier.out -h <IP/hostname> -p <port> OR -f <UDS socket file path>\n");
        exit(1);
    }


    // check then option you got from the user:
    int ret = getopt(argc, argv, ":p:h:f:");
    char *endptr; // for checking if the value is digit
    long val = 0;

      while(ret != -1){
        switch(ret){
            case 'p': {
                if(flag_f){
                    fprintf(stderr, "ERROR: cannot use -f and (-p, -h) together\n");
                    exit(1);
                }
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || val <= 0 || val > 65535) {
                    fprintf(stderr,"ERROR: Invalid argument for PORT\n");
                    exit(1);
                }
                flag_p = 1;
                PORT = optarg;
                break;
            }
            case 'h': {
                if(flag_f){
                    fprintf(stderr, "ERROR: cannot use -f and (-p, -h) together\n");
                    exit(1);
                }
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                flag_h = 1;
                IP = optarg;
                break;
            }
            case 'f': {
                if(flag_h || flag_p){
                    fprintf(stderr, "ERROR: cannot use -f and (-p, -h) together\n");
                    exit(1);
                }
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                flag_f = 1;
                PATH = optarg;
                break;
            }
        }
        ret = getopt(argc, argv, ":p:h:f:");
    }

    if((flag_p !=1 && flag_h ==1) || (flag_h != 1 && flag_p ==1)){
        fprintf(stderr, "ERROR: -h should come with -p, vice versa \n");
        exit(1);
    }else if(flag_p == 0 && flag_f == 0 && flag_h == 0){
        fprintf(stderr, "ERROR: no valid input \n");
        exit(1);
    }



    if(flag_h && flag_p){
        // Initialize hints structure with zeros
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;    // Allow either IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM; // Use TCP stream sockets

        // Get address information for the server using the provided hostname
        if ((rv = getaddrinfo(IP, PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }



        // Loop through all the results and connect to the first server we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            // Try to create a socket with the current address info
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;  // If socket creation fails, try the next address
            }

            // Convert the IP address to a readable string format and display it
            inet_ntop(p->ai_family,
                get_in_addr((struct sockaddr *)p->ai_addr),
                s, sizeof s);
            printf("client: attempting connection to %s\n", s);

            // Attempt to connect to the server
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                perror("client: connect");
                close(sockfd);  // Close the socket if connection fails
                continue;       // Try the next address
            }

            break;  // If we get here, we successfully connected
        }

        // If p is NULL, it means we couldn't connect to any address
        if (p == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            return 2;
        }

        // Convert connected server's address to string and print it
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                s, sizeof s);
        printf("client: connected to %s\n", s);

        freeaddrinfo(servinfo); // Free the linked list of addresses, no longer needed
    }else if(flag_f){
        struct sockaddr_un addr;
        // create fd for the UNIX stream socket
        sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

        // check if failed to create FD
        if(sockfd == -1){
            perror("unix stream socket");
            exit(1);
        }
        
        // preparing server address struct
        memset(&addr,0,sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, PATH);

        if(connect(sockfd,(struct sockaddr*)&addr,sizeof(addr)) == -1 ){
            perror("connect");
            exit(1);
        }
    }
        // GET USER INPUT
        unsigned long long amount;
        char atom[10];
        ask_supplier(&amount, atom, sizeof(atom));

        // Format the message to send to server: ADD {atom} {amount}
        char send_buf[MAXDATASIZE];
        snprintf(send_buf, MAXDATASIZE, "ADD %s %llu", atom, amount);

    

        if(flag_h && flag_p){
        // Send the formatted request to server
        if (send(sockfd, send_buf, strlen(send_buf), 0) == -1) {
            perror("send");
            close(sockfd);
            exit(1);
        }

        printf("client: sent request --> '%s'\n", send_buf);

        // Receive data from the server
        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        // Null-terminate the received data to make it a valid string
        buf[numbytes] = '\0';
    } else if(flag_f){
        if (write(sockfd, send_buf, strlen(send_buf)) == -1) {
            perror("write");
            close(sockfd);
            exit(1);
        }

        printf("client: sent request --> '%s'\n", send_buf);

        // Receive data from the server
        if ((numbytes = read(sockfd, buf, MAXDATASIZE-1)) == -1) {
            perror("read");
            exit(1);
        }
    }

    // Print the message received from the server
    printf("client: received '%s'\n", buf);

    // Clean up by closing the socket
    close(sockfd);
    return 0;
}