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
#include <unistd.h>
#include <getopt.h>
#include <sys/un.h>
#include <fcntl.h>   // open
#include <sys/stat.h>  // level of access to files
#include <sys/file.h>  // flock

// for get opt
extern char *optarg;
extern int optind, opterr, optopt;

// initializing ports for both UDP and TCP + FD Storage
char* TCP_PORT = 0;
char* UDP_PORT = 0;
char* UNIX_TCP_SOCKET_PATH = NULL;
char* UNIX_UDP_SOCKET_PATH = NULL;
char* STORAGE_FILE = 0;

// alarm value
extern int alarm_timeout;

// flag for storage file
u_int8_t file_flag = 0;

// flags for -o -h -c
unsigned long long oxygen_input = 0;
unsigned long long hydrogen_input = 0;
unsigned long long carbon_input = 0;

int main(int argc, char*argv[])
{

     // Check if port was provided as a command-line argument
     if (argc < 4) {
        fprintf(stderr,"usage: ./drinks_bar.out -T/--tcp-port <int> -U/--udp-port <int> -s/--stream-path <UDS stream file path> -d/--datagram-path <UDS datagram filepath> (OPTIONAL: -o/--oxygen <int=0> -c/--carbon <int=0> -h/--hydrogen <int=0> -t/--timeout <int=0>\n");
        exit(1);
    }

    struct option longopts[] = {
        {"udp-port",required_argument,NULL,'U'},
        {"tcp-port",required_argument,NULL,'T'},
        {"oxygen",optional_argument,NULL,'o'},
        {"carbon",optional_argument,NULL,'c'},
        {"hydrogen",optional_argument,NULL,'h'},
        {"timeout",optional_argument,NULL,'t'},
        {"stream-path",optional_argument,NULL,'s'},
        {"datagram-path",optional_argument,NULL,'d'},
        {"save-file",optional_argument,NULL,'f'},
        {0,0,0,0}
    };

    // check then option you got from the user:
    int ret = getopt_long(argc, argv, ":U:T:d:s:o:c:h:t:f:", longopts, NULL);
    char *endptr; // for checking if the value is digit
    long val = 0;

    while(ret != -1){
        switch(ret){
            case 'f':{
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                STORAGE_FILE = optarg;
                file_flag =1;
            }
                break;
            case 'U': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || val <= 0 || val > 65535) {
                    fprintf(stderr,"ERROR: Invalid argument for UDP PORT\n");
                    exit(1);
                }
                UDP_PORT = optarg;
                break;
            }
            case 'T': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr,"ERROR: Invalid argument for TCP PORT\n");
                    exit(1);
                }
                TCP_PORT = optarg;
                break;
            }
            case 'd': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                UNIX_UDP_SOCKET_PATH = optarg;
                break;
            }
            case 's': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                UNIX_TCP_SOCKET_PATH = optarg;
                break;
            }
            case 'o': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr,"ERROR: Invalid argument for Oxygen\n");
                    exit(1);
                }
                oxygen_input = (unsigned long long)val;

                break;
            }
            case 'c': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr,"ERROR: Invalid argument for Carbon\n");
                    exit(1);
                }
                carbon_input = (unsigned long long)val;
                break;
            }
            case 'h': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr,"ERROR: Invalid argument for Hydrogen\n");
                    exit(1);
                }
                hydrogen_input = (unsigned long long)val;
                break;
            }
            case 't': {
                if (optarg == NULL) {
                    fprintf(stderr, "ERROR: Missing argument for option -%c\n", ret);
                    exit(1);
                }
                val = strtol(optarg, &endptr, 10);
                if (*endptr != '\0') {
                    fprintf(stderr,"ERROR: Invalid argument for Timeout\n");
                    exit(1);
                }
                alarm_timeout = (int)val;
                break;
            }
            default:
                fprintf(stderr,"ERROR: usage: ./drinks_bar.out -T/--tcp-port <int> -U/--udp-port <int> (OPTIONAL: -o/--oxygen <int=0> -c/--carbon <int=0> -h/--hydrogen <int=0> -t/--timeout <int=0>\n");
                exit(1);
        }
        ret = getopt_long(argc, argv, ":U:T:d:s:o:c:h:t:f:", longopts, NULL);
    }

    // if file flag is on, chec if file exists
    //  TRUE    - Export file to the storage and update each change into the file (ignore -o -h -c additions)
    //  FALSE   - Import current storage into the file and update each change into the file
    // ignore -h -o -c values , else don't.
    // *********************************
    // *                               *
    // * <CARBON> <OXYGEN> <HYDROGEN>  *
    // *                               *
    // *********************************
    int fd = open(STORAGE_FILE, O_RDWR, S_IRUSR | S_IWUSR);
    if(file_flag){
        if(fd != -1){ // IF FILE EXISTS
            int reader = read(fd, &warehouse, sizeof(AtomStorage));

            // FILE EXISTS BUT NO INPUT
            if( reader < 0){
                perror("server read");
                close(fd);
                exit(1);
            }

            // IF NO STRUCT SIZE, WRONG FORMAT, ERROR
            if (reader != sizeof(AtomStorage)) {
                perror("server read");
                close(fd);
                return 0;
            }

        }else{ // FILE DOESNT EXISTS, CREATE IT AND UPDATE ITS FIELD
            fprintf(stdout,"WARNING: File: %s, creating file and storing storage predefined input",STORAGE_FILE);
            fd = open(STORAGE_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

            // UPDATE THE CURRENT STORAGE FROM USER INPUT
            warehouse.carbon = carbon_input;
            warehouse.oxygen = oxygen_input;
            warehouse.hydrogen =  hydrogen_input;

            // LOCK THE FILE, IF ERROR, END PROCCESS
            if (flock(fd, LOCK_EX) == -1){
                perror("server flock");
                close(fd);
                exit(1);
            }

            // CHECK FOR WRITE ISSUES
            if(write(fd, &warehouse, sizeof(AtomStorage)) == -1){
                perror("server write");
                close(fd);
                exit(1);
            }

            // UNLOCK THE LOCK
            flock(fd, LOCK_UN);
        }
    }else{
        // No file flag, only user input
        warehouse.carbon = carbon_input;
        warehouse.oxygen = oxygen_input;
        warehouse.hydrogen =  hydrogen_input;
    }

    // Socket file descriptors
    int tcp_sockfd, new_fd;  // sockfd = listening socket, new_fd = client connection socket
    
    // Network address structures for server setup
    struct addrinfo tcp_hints, *tcp_servinfo, *tcp_p;  // hints = criteria, servinfo = results list, p = iterator
    struct sockaddr_storage their_addr;    // Storage for client's address information
    socklen_t sin_size;                    // Size of client address structure
    
    
    // Socket options and utility variables
    char s[INET6_ADDRSTRLEN];    // Buffer to store client IP address as string
    int rv;                      // Return value for getaddrinfo()

    // STEP 1: Configure server address criteria
    memset(&tcp_hints, 0, sizeof tcp_hints);
    tcp_hints.ai_family = AF_INET;      // Use IPv4
    tcp_hints.ai_socktype = SOCK_STREAM; // Use TCP (reliable, connection-oriented)
    tcp_hints.ai_flags = AI_PASSIVE;    // Use local machine's IP address

    // STEP 2: Get list of possible addresses to bind to
    if ((rv = getaddrinfo(NULL, TCP_PORT, &tcp_hints, &tcp_servinfo)) != 0) {
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

    // STEP 4: Start listening for client connections
    if (listen(tcp_sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Create UDP socket, same as above TCP configuration
    int udp_sockfd;
    struct addrinfo udp_hints, *udp_servinfo, *udp_p;
    memset(&udp_hints, 0, sizeof udp_hints);
    udp_hints.ai_family = AF_INET;
    udp_hints.ai_socktype = SOCK_DGRAM;
    udp_hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, UDP_PORT, &udp_hints, &udp_servinfo)) != 0) {
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

    int unix_tcp_sockfd;
    // UNIX DOMAIN SOCKETS CREATION
    if(UNIX_TCP_SOCKET_PATH != NULL){
    // START TCP UNIX DS
    struct sockaddr_un unix_tcp_addr;

    // create a unix tcp socket
    if((unix_tcp_sockfd = socket(AF_UNIX,SOCK_STREAM,0)) == -1){
        perror("unix tcp socket");
        exit(1);
    }

    // remove any existing socket file
    unlink(UNIX_TCP_SOCKET_PATH);

    // setup unix address
    memset(&unix_tcp_addr, 0, sizeof(unix_tcp_addr));
    unix_tcp_addr.sun_family = AF_UNIX;
    strcpy(unix_tcp_addr.sun_path, UNIX_TCP_SOCKET_PATH);

    // bind socket to given path
    if (bind(unix_tcp_sockfd,(struct sockaddr*)&unix_tcp_addr,sizeof(unix_tcp_addr)) == -1){
        perror("bind unix tcp");
        exit(1);
    }

    // now listen for any connection
    if(listen(unix_tcp_sockfd, BACKLOG) == -1){
        perror("listen unix tcp");
        exit(1);
    }

    // END TCP UNIX DS
    }

    int unix_udp_sockfd;
    if(UNIX_UDP_SOCKET_PATH != NULL){
    // START UDP UNIX DS
    struct sockaddr_un unix_udp_addr;

    // Create UNIX UDP socket
    if ((unix_udp_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("unix udp socket");
        exit(1);
    }

    // Remove any existing socket file
    unlink(UNIX_UDP_SOCKET_PATH);
  
    // Setup UNIX UDP address
    memset(&unix_udp_addr, 0, sizeof(unix_udp_addr));
    unix_udp_addr.sun_family = AF_UNIX;
    strcpy(unix_udp_addr.sun_path, UNIX_UDP_SOCKET_PATH);

    // Bind UDP socket
    if (bind(unix_udp_sockfd, (struct sockaddr*)&unix_udp_addr, sizeof(unix_udp_addr)) == -1) {
        perror("bind unix udp");
        exit(1);
    }
    
    // END UDP UNIX DS
    }
    
    // Array of pollfd structures to track file descriptors
    struct pollfd fds[MAX_CLIENTS + 5];  // +1 for the listening socket

    int nfds = 0;

    // Initialize the first pollfd to watch for incoming connections on the listener socket
    fds[0].fd = tcp_sockfd;
    fds[0].events = POLLIN;  // Watch for incoming data
    nfds++;

    fds[1].fd = udp_sockfd;
    fds[1].events = POLLIN;
    nfds++;

    fds[2].fd = STDIN_FILENO;
    fds[2].events = POLLIN;
    nfds++;

    if(UNIX_UDP_SOCKET_PATH != NULL){
    fds[3].fd = unix_udp_sockfd;
    fds[3].events = POLLIN;
    nfds++;
    }
    if(UNIX_TCP_SOCKET_PATH != NULL){
    fds[4].fd = unix_tcp_sockfd;
    fds[4].events = POLLIN;
    nfds++;
    }
    


    printf("server: waiting for connections...\n");

    signal(SIGALRM,alarm_handler);

    // STEP 6: Main server loop - accept and handle client connections
    while(1) {

            // START alarm // TODO CHANGE THE INT TO USER INPUT INT
            alarm(alarm_timeout);

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

        // KEYBOARD listening socket
        if (fds[i].fd == STDIN_FILENO && (fds[i].revents & POLLIN)) {

                    alarm(0); // RESET ALARM

                    char server_input[256] = {0}; // setting a new buffer for user input
                    char response[100] = {0};
                    
                    printf("KEYBOARD: ");

                    // chec if no error occured
                    if(fgets(server_input,sizeof(server_input),stdin) != NULL){
                        process_message(server_input,strlen(server_input)+1,KEYBOARD_HANDLE,response,sizeof(response), file_flag, fd);
                        printf("%s\n", response);

                    }

            continue;
        }
        
        // Handle UDP socket
        if (fds[i].fd == udp_sockfd && (fds[i].revents & POLLIN)) {

            alarm(0); // RESET ALARM

            char udp_buf[MAXDATASIZE];
            struct sockaddr_storage udp_client_addr;
            socklen_t udp_addr_len = sizeof udp_client_addr;
            int udp_numbytes = recvfrom(udp_sockfd, udp_buf, sizeof(udp_buf) - 1, 0,
                                        (struct sockaddr *)&udp_client_addr, &udp_addr_len);
            if (udp_numbytes > 0) {
                udp_buf[udp_numbytes] = '\0';

                // Process the message
                char response[256];
                process_message(udp_buf, udp_numbytes, UDP_HANDLE, response, sizeof(response), file_flag, fd);

                // Send response back to UDP client
                if (sendto(udp_sockfd, response, strlen(response), 0,
                        (struct sockaddr *)&udp_client_addr, udp_addr_len) == -1) {
                    perror("sendto");
                }
            }
            continue; // Done with UDP, continue to next fd
        }

        // TCP listening socket, handles connection establishments
        if (fds[i].fd == tcp_sockfd && (fds[i].revents & POLLIN)) {

            alarm(0); // RESET ALARM

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

                // UNIX TCP listening socket, handles connection establishments
        if (fds[i].fd == unix_tcp_sockfd && (fds[i].revents & POLLIN)) {

            alarm(0); // RESET ALARM

            // Accept new connection
            sin_size = sizeof(their_addr);
            // Accept incoming client connection (blocks until client connects)
            new_fd = accept(unix_tcp_sockfd, NULL, NULL);
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
                    printf("server: new UNIX TCP connection on socket %d\n",new_fd);
                } else {
                    // We're at capacity, reject this client
                    printf("server: too many clients, rejecting new connection\n");
                    close(new_fd);
                }
            }
        }

        // Handle data from existing TCP client
        else if ((fds[i].revents & POLLIN)&& fds[i].fd != unix_tcp_sockfd && fds[i].fd != udp_sockfd && fds[i].fd != unix_udp_sockfd && fds[i].fd != STDIN_FILENO) {

        alarm(0); // RESET ALARM

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
            process_message(buf, numbytes, TCP_HANDLE,response, sizeof(response), file_flag, fd);
            
            // send message to client
            if (send(fds[i].fd, response, strlen(response), 0) == -1) {
                perror("send");
                 }
            }
        }       

        // Handle UNIX UDP socket
        if (fds[i].fd == unix_udp_sockfd && (fds[i].revents & POLLIN)) {

            alarm(0); // RESET ALARM

            char udp_buf[MAXDATASIZE];
            struct sockaddr_un unix_udp_client_addr;
            socklen_t unix_udp_addr_len = sizeof unix_udp_client_addr;
            int unix_udp_numbytes = recvfrom(unix_udp_sockfd, udp_buf, sizeof(udp_buf) - 1, 0,
                                        (struct sockaddr *)&unix_udp_client_addr, &unix_udp_addr_len);
            if (unix_udp_numbytes > 0) {
                udp_buf[unix_udp_numbytes] = '\0';

                // Process the message
                char response[256];
                process_message(udp_buf, unix_udp_numbytes, UDP_HANDLE, response, sizeof(response), file_flag, fd);

                // Send response back to UDP client
                if (sendto(unix_udp_sockfd, response, strlen(response), 0,
                        (struct sockaddr *)&unix_udp_client_addr, unix_udp_addr_len) == -1) {
                    perror("sendto");
                    }
                }
            continue; // Done with UNIX_UDP, continue to next fd
            }
        // END OF ALARM
        }   

    }
    close(fd);
}