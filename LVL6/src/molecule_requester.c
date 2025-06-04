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
     int rv;                         // Return value for getaddrinfo
 
      // Check if all the needed args was provided as a command-line argument
      if (argc < 3) {
         fprintf(stderr,"usage: ./atom_supplier.out -h <IP/hostname> -p <port> OR -f <UDS socket file path>\n");
         exit(1);
     }
 
 
     // check then option you got from the user:
     int ret = getopt(argc, argv, "p:h:f:");
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
         ret = getopt(argc, argv, "p:h:f:");
     }
 
     if((flag_p !=1 && flag_h ==1) || (flag_h != 1 && flag_p ==1)){
         fprintf(stderr, "ERROR: -h should come with -p, vice versa -%c\n", ret);
         exit(1);
     }
 
     struct sockaddr_un unix_server_addr;
     struct sockaddr_in udp_server_addr;
 
 
     if (flag_p && flag_h){
         // Initialize hints structure with zeros
         memset(&hints, 0, sizeof hints);
         hints.ai_family = AF_UNSPEC;    // Allow either IPv4 or IPv6
         hints.ai_socktype = SOCK_DGRAM; // Use UDP dgram sockets
 
         // Get address information for the server using the provided hostname
         if ((rv = getaddrinfo(IP, PORT, &hints, &servinfo)) != 0) {
             fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
             return 1;
         }
 
         // create a UDP socket
         sockfd = socket(AF_INET,SOCK_DGRAM,0);
         if(sockfd < 0){
             perror("SOCKET CREATION FAILED");
             exit(1);
         }
         
         socklen_t addr_len = sizeof(udp_server_addr);
         memset(&udp_server_addr, 0,addr_len);
         udp_server_addr.sin_family = AF_INET;
         udp_server_addr.sin_port = htons(atoi(PORT));
 
         inet_pton(AF_INET,IP,&udp_server_addr.sin_addr);
         
         }else if(flag_f){
            
             // create fd for the UNIX dgram socket
             sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
     
             // check if failed to create FD
             if(sockfd == -1){
                 perror("unix dgram socket");
                 exit(1);
             }
     
             // Bind client socket to a unique abstract address (not a file)
             struct sockaddr_un client_addr;
             memset(&client_addr, 0, sizeof(client_addr));
             client_addr.sun_family = AF_UNIX;
             client_addr.sun_path[0] = '\0'; // abstract namespace
             snprintf(&client_addr.sun_path[1], sizeof(client_addr.sun_path) - 1, "mreq_%d", getpid());
             socklen_t client_addr_len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(&client_addr.sun_path[1]);
             if (bind(sockfd, (struct sockaddr*)&client_addr, client_addr_len) == -1) {
                 perror("bind client");
                 close(sockfd);
                 exit(1);
             }

             // preparing server address struct
             memset(&unix_server_addr, 0, sizeof(unix_server_addr));
             unix_server_addr.sun_family = AF_UNIX;
             strncpy(unix_server_addr.sun_path, PATH, sizeof(unix_server_addr.sun_path) - 1);
             
         }
         // GET USER INPUT
         unsigned long long* amount = malloc(sizeof(unsigned long long));
         memset(amount,0,sizeof(*amount));
         char element[20];
         ask_requester(amount, element, sizeof(element));
 
         // Format the message to send to server: ADD {atom} {amount}
         char send_buf[MAXDATASIZE];
         snprintf(send_buf, MAXDATASIZE, "DELIVER %s %llu", element, *amount);
 
         if(flag_p && flag_h){
 
         socklen_t udp_addr_len = sizeof(udp_server_addr);
 
         // Send the formatted request to server
         if (sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&udp_server_addr, udp_addr_len) == -1) {
             perror("sendto");
             close(sockfd);
             exit(1);
         }
 
         printf("client: sent request via STREAM--> '%s'\n", send_buf);
 
         if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE-1, 0, (struct sockaddr*)&udp_server_addr, &udp_addr_len)) == -1) {
             perror("recv");
             exit(1);
         }
     }else if(flag_f){
 
         socklen_t unix_addr_len = sizeof(unix_server_addr);
 
         // Send the formatted request to server UNIX
         if (sendto(sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&unix_server_addr, unix_addr_len) == -1) {
             perror("sendto");
             close(sockfd);
             exit(1);
         }
 
         printf("client: sent request via UNIX --> '%s'\n", send_buf);
 
         if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE-1, 0, NULL, NULL)) == -1) {
             perror("recv");
             close(sockfd);
             exit(1);
         }
     }
 
         // Null-terminate the received data to make it a valid string
         buf[numbytes] = '\0';
 
         // Print the message received from the server
         printf("client: received '%s'\n", buf);
 
         // Clean up by closing the socket
         close(sockfd);
         free(amount);
         return 0;
     }
