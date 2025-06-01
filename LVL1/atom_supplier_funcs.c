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
#include "const.h"
#include "atom_supplier_funcs.h"

void ask_client(unsigned int* amount, char* atom, size_t atom_size) {
    int index = 0;
    
    memset(atom, 0, atom_size);  // Safe clearing
    
    printf("Enter your desired choice:\n(1) CARBON\n(2) OXYGEN\n(3) HYDROGEN\n");
    if (scanf("%d", &index) != 1) {
        fprintf(stderr, "Error: Invalid input\n");
        return;
    }
    
    printf("Enter your desired amount:\n");
    if (scanf("%d", amount) != 1) {  // %llu for unsigned long long
        fprintf(stderr, "Error: Invalid amount\n");
        return;
    }
    
    switch(index) {
        case 1: strncpy(atom, "CARBON", atom_size-1); break;
        case 2: strncpy(atom, "OXYGEN", atom_size-1); break;
        case 3: strncpy(atom, "HYDROGEN", atom_size-1); break;
        default: 
            fprintf(stderr, "Error: Invalid selection\n");
            return;
    }
    atom[atom_size-1] = '\0';  // Ensure null-termination
}

void *get_in_addr(struct sockaddr *sa)
{
    // Check if the address is IPv4
    if (sa->sa_family == AF_INET) {
        // If IPv4, return the address from sockaddr_in structure
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    // Otherwise, assume IPv6 and return the address from sockaddr_in6 structure
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}