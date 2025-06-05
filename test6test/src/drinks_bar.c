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

#define _POSIX_C_SOURCE 200112L

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
#include "../include/atom_warehouse_funcs.h"
#include <poll.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/un.h>
#include <fcntl.h>   // open
#include <sys/stat.h>  // level of access to files
#include <sys/file.h>  // flock
#include <limits.h>
#include "../include/elements.h"

Element element_type_from_str(const char *str) {
    if (strcmp(str, "CARBON") == 0) return CARBON;
    if (strcmp(str, "OXYGEN") == 0) return OXYGEN;
    if (strcmp(str, "HYDROGEN") == 0) return HYDROGEN;
    if (strcmp(str, "WATER") == 0) return WATER;
    if (strcmp(str, "CARBONDIOXIDE") == 0) return CARBON_DIOXIDE;
    if (strcmp(str, "GLUCOSE") == 0) return GLUCOSE;
    if (strcmp(str, "ALCOHOL") == 0) return ALCOHOL;
    if (strcmp(str, "SOFT DRINK") == 0) return SOFT_DRINK;
    if (strcmp(str, "VODKA") == 0) return VODKA;
    if (strcmp(str, "CHAMPAGNE") == 0) return CHAMPAGNE;
    return UNKNOWN;
}

// Global warehouse instance
AtomStorage warehouse = {0};
int alarm_timeout = 0;

void save_to_file(int fd){

    // LOCK THE FILE, IF ERROR, END PROCCESS
    if (flock(fd, LOCK_EX) == -1){
        perror("function flock");
        close(fd);
        exit(1);
    }

    // Seek to beginning of file
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(1);
    }

    // check if write failed
    if(write(fd, &warehouse, sizeof(AtomStorage)) == -1){
        perror("writre failed");
        close(fd);
        exit(1);
    }

    // UNLOCK THE LOCK
    flock(fd, LOCK_UN);
}

void reload_from_file(int fd){

    // LOCK THE FILE, IF ERROR, END PROCCESS
    if (flock(fd, LOCK_EX) == -1){
        perror("function flock");
        close(fd);
        exit(1);
    }

    // Seek to beginning of file
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(1);
    }

    // check if write failed
    if(read(fd, &warehouse, sizeof(AtomStorage)) == -1){
        perror("read failed");
        close(fd);
        exit(1);
    }

    // UNLOCK THE LOCK
    flock(fd, LOCK_UN);
}

void init_warehouse(unsigned long long c, unsigned long long o, unsigned long long h) {
    warehouse.carbon = c;
    warehouse.oxygen = o;
    warehouse.hydrogen = h;
}

unsigned long long get_water_num(unsigned long long oxygen, unsigned long long hydrogen){
    unsigned long long counter = 0;
    while(oxygen > 1 && hydrogen > 2){
        oxygen -= 1;
        hydrogen -= 2;
        counter++;
    }
    return counter;
}

unsigned long long get_carbonDio_num(unsigned long long carbon, unsigned long long oxygen){
    unsigned long long counter = 0;
    while(carbon > 1 && oxygen > 2){
        carbon -= 1;
        oxygen -= 2;
        counter++;
    }
    return counter;
}

unsigned long long get_alcohol_num(unsigned long long carbon, unsigned long long oxygen, unsigned long long hydrogen){
    unsigned long long counter = 0;
    while(carbon > 2 && oxygen > 1 && hydrogen > 6){
        carbon -= 2;
        hydrogen -=6;
        oxygen -=1;
        counter++;
    }
    return counter;
}

unsigned long long get_glucose_num(unsigned long long carbon, unsigned long long oxygen, unsigned long long hydrogen){
    unsigned long long counter = 0;
    while(carbon > 6 && oxygen > 6 && hydrogen > 12){
        carbon -= 6;
        hydrogen -= 12;
        oxygen -= 6;
        counter++;
    }
    return counter;
}


void print_storage(){
    printf("\nCARBON #:%lld \nOXYGEN #:%lld \nHYDROGEN #:%lld\n",warehouse.carbon, warehouse.oxygen, warehouse.hydrogen);
    return;
}

void format_storage(char *out, size_t out_size) {
    snprintf(out, out_size, "CARBON: %lld\nOXYGEN: %lld\nHYDROGEN: %lld\n", warehouse.carbon, warehouse.oxygen, warehouse.hydrogen);
}


void process_message(char* buf, size_t size_buf, u_int8_t sock_handle, char *response, size_t response_size, int file_flag, int fd){
    if(file_flag){
        reload_from_file(fd);
        }
    // Early validation for ADD missing arguments
    char cmdTemp[10] = {0}, elemTemp[20] = {0}, extraTemp[20] = {0};
    int tokenCount = sscanf(buf, "%9s %19s %19s", cmdTemp, elemTemp, extraTemp);
    if (strcmp(cmdTemp, "ADD") == 0 && tokenCount < 3) {
        fprintf(stdout, "ERROR: Invalid ADD command format\n");
        snprintf(response, response_size, "ERROR: Invalid ADD command format\n");
        return;
    }

    // Parse the command
    char cmd[10] = {0}, element_str[20] = {0}, element_str2[20] = {0};
    Element element;
    int amount;

    // if  we got exactly three elements, continue
    if (sscanf(buf, "%s %s %d",cmd,element_str,&amount) == 3){
        element = element_type_from_str(element_str);
        // check if its ADD and TCP
        if(!strcmp(cmd,"ADD") && sock_handle == TCP_HANDLE){
            switch(element){
                case 0:
                warehouse.carbon += amount;
                    break;
                case 1:
                warehouse.oxygen += amount;
                    break;
                case 2:
                warehouse.hydrogen += amount;
                    break;
                default:
                    fprintf(stdout,"ERROR: Unkown atom type\n");
                    snprintf(response, response_size, 
                        "ERROR: Unkown atom type\n");
                    return;
            }
            if(file_flag){
            save_to_file(fd);
            }
            format_storage(response, response_size);
            // Print the storage to server console
            print_storage();
        }
        // check if its DELIVER and UDP
        else if(!strcmp(cmd,"DELIVER") && sock_handle == UDP_HANDLE){
            
            switch(element){
                case WATER:
                if(warehouse.hydrogen>=2*amount && warehouse.oxygen >=1*amount){
                    warehouse.hydrogen -= 2*amount;
                    warehouse.oxygen -= 1*amount;
                    snprintf(response, response_size, 
                        "#%d WATER DELIVERED",amount);
                }else{
                    fprintf(stderr,"Not enough atoms to make WATER");
                    snprintf(response, response_size, 
                        "ERROR: Not enough atoms to make WATER\n");
                }
                    break;
                case CARBON_DIOXIDE:
                if(warehouse.carbon >=1*amount && warehouse.oxygen>=2*amount){
                    warehouse.carbon -= 1*amount;
                    warehouse.oxygen -= 2*amount;
                    snprintf(response, response_size, 
                        "#%d CARBON DIOXIDE DELIVERED",amount);
                }else{
                    fprintf(stderr,"Not enough atoms to make CARBON DIOXIDE");
                    snprintf(response, response_size, 
                        "ERROR: Not enough atoms to make CARBON DIOXIDE\n");
                }
                    break;
                case GLUCOSE:
                if(warehouse.carbon >= 6*amount && warehouse.hydrogen >= 12*amount && warehouse.oxygen >= 6*amount){
                    warehouse.carbon -= 6*amount;
                    warehouse.hydrogen -= 12*amount;
                    warehouse.oxygen -= 6*amount;
                    snprintf(response, response_size, 
                        "#%d GLUCOSE DELIVERED", amount);
                } else{
                    fprintf(stderr,"Not enough atoms to make GLUCOSE");
                    snprintf(response, response_size, 
                        "ERROR: Not enough atoms to make GLUCOSE\n");
                }
                    break;
                case ALCOHOL:
                if(warehouse.carbon >= 2*amount && warehouse.hydrogen >= 6*amount && warehouse.oxygen >= 1*amount){
                    warehouse.carbon -= 2*amount;
                    warehouse.hydrogen -=6*amount;
                    warehouse.oxygen -=1*amount;
                    snprintf(response, response_size, 
                        "#%d ALCOHOL DELIVERED",amount);
                } else{
                    fprintf(stderr,"Not enough atoms to make ALCOHOL");
                    snprintf(response, response_size, 
                        "ERROR: Not enough atoms to make ALCOHOL\n");
                }
                    break;
                default:
                    fprintf(stdout,"ERROR: Unkown mulecule type\n");
                    snprintf(response, response_size, 
                        "ERROR: Unkown mulecule type\n");
                    return;
                    }
                    if(file_flag){
                        save_to_file(fd);
                    }
                printf("\n-- UPDATE --\n");
                print_storage();
            }
    }else if(sscanf(buf, "%s %s %s",cmd,element_str, element_str2) && strcmp(cmd,"GEN") == 0){
        // cjeck if we got anther word
        if(strlen(element_str2) > 1){
            strcat(element_str, " ");
            strcat(element_str, element_str2);
        }
        element = element_type_from_str(element_str);
        
        if(sock_handle == KEYBOARD_HANDLE){
            unsigned long long min = INT_MAX ;
            unsigned long long temp_water = get_water_num(warehouse.oxygen,warehouse.hydrogen);
            unsigned long long temp_alcohol = get_alcohol_num(warehouse.carbon,warehouse.oxygen,warehouse.hydrogen);
            unsigned long long temp_carbonDio = get_carbonDio_num(warehouse.carbon,warehouse.oxygen);
            unsigned long long temp_glucose = get_glucose_num(warehouse.carbon,warehouse.oxygen,warehouse.hydrogen);
            switch(element){
                case SOFT_DRINK:
                    if(min > temp_water){
                        min = temp_water;
                    }
                    else if(min > temp_carbonDio){
                        min = temp_carbonDio;
                    }
                    else if(min > temp_glucose){
                        min = temp_alcohol;
                    }
                    snprintf(response, response_size, 
                        "The Drink Bar is able to generate --> %lld Soft Drink's\n", min);
                    break;
                case VODKA:
                    if(min > temp_water){
                        min = temp_water;
                    }
                    else if(min > temp_glucose){
                        min = temp_glucose;
                    }
                    else if(min > temp_alcohol){
                        min = temp_alcohol;
                    }
                snprintf(response, response_size, 
                    "The Drink Bar is able to generate --> %lld Vodka's\n", min);
                    break;
                case CHAMPAGNE:
                    if(min > temp_water){
                        min = temp_water;
                    }
                    else if(min > temp_carbonDio){
                        min = temp_carbonDio;
                    }
                    else if(min > temp_alcohol){
                        min = temp_alcohol;
                    }
                    snprintf(response, response_size, 
                        "The Drink Bar is able to generate --> %lld Champagne's\n", min);
                    break;
                default:
                    fprintf(stdout,"ERROR: Unkown drink type\n");
                    snprintf(response, response_size, 
                        "ERROR: Unkown drink type\n");
                    return;
            }
            if (sock_handle != KEYBOARD_HANDLE){
            format_storage(response, response_size);
            print_storage();    // Print the storage to server console
            }

        }
    }else {                // no ADD no DELIVER? unkown
            fprintf(stdout,"ERROR: Unkown command\n");
            snprintf(response, response_size, 
                "ERROR: Unknown command\n");
        }
    }

void sigchld_handler(int s)
{
    (void)s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void alarm_handler(int signum){
    fprintf(stdout,"Server didnt recieved any input in the past %d seconds\nTERMINATING!\n", alarm_timeout);
    exit(0);
}

// for get opt
extern char *optarg;
extern int optind, opterr, optopt;

// initializing ports for both UDP and TCP + FD Storage
char* TCP_PORT = 0;
char* UDP_PORT = 0;
char* UNIX_TCP_SOCKET_PATH = NULL;
char* UNIX_UDP_SOCKET_PATH = NULL;
char* STORAGE_FILE = 0;

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
            if( reader <= 0){
                fprintf(stderr,"ERROR: FILE EXISTS, NO INPUT\n");
                close(fd);
                exit(1);
            }

            // IF NO STRUCT SIZE, WRONG FORMAT, ERROR
            if (reader != sizeof(AtomStorage)) {
                fprintf(stderr,"ERROR: WRONG FORMAT\n");
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
    struct pollfd fds[MAX_CLIENTS + 5];

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

            // START alarm
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

            // Skip if no events
            if (fds[i].revents == 0) continue;

            // Handle data from existing TCP and UNIX TCP clients first
            if ((fds[i].revents & POLLIN) && 
                fds[i].fd != tcp_sockfd && fds[i].fd != unix_tcp_sockfd &&
                fds[i].fd != udp_sockfd && fds[i].fd != unix_udp_sockfd &&
                fds[i].fd != STDIN_FILENO) {

                alarm(0);
                char buf[MAXDATASIZE];
                int numbytes = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                if (numbytes < 1) {
                    if (numbytes == 0) {
                        // client closed
                    } else {
                        perror("recv");
                    }
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1]; nfds--; i--;
                } else {
                    buf[numbytes] = '\0';
                    printf("server: received '%s' on socket %d\n", buf, fds[i].fd);
                    char response[256];
                    process_message(buf, numbytes, TCP_HANDLE, response, sizeof(response), file_flag, fd);
                    if (send(fds[i].fd, response, strlen(response), 0) == -1) perror("send");
                    else printf("server: sent response to socket %d\n", fds[i].fd);
                }
                continue;
            }

            // Handle errors (exclude POLLHUP if POLLIN was handled)
            if ((fds[i].revents & (POLLERR | POLLNVAL)) ||
                ((fds[i].revents & POLLHUP) && !(fds[i].revents & POLLIN))) {
                if (fds[i].fd != tcp_sockfd && fds[i].fd != udp_sockfd) {
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1]; nfds--; i--;
                } else {
                    fprintf(stderr, "Critical error on listening or UDP socket (fd %d)\n", fds[i].fd);
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
                if (nfds < MAX_CLIENTS + 5) {
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
            continue;
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
            } else if (nfds < MAX_CLIENTS + 5) {
                fds[nfds].fd     = new_fd;
                fds[nfds].events = POLLIN;
                nfds++;
                printf("server: new UNIX TCP connection on socket %d\n", new_fd);
            } else {
                printf("server: too many clients, rejecting new connection\n");
                close(new_fd);
            }
            continue; 
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