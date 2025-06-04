#pragma once
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
#include "../const.h"

/**
 * @brief Prompts user to select atom type and quantity, 
 * storing results in the provided parameters
 * 
 * @param amount Amount of the attoms
 * @param atom The specified atom
 * @param atom_size The size of the string
 */
void ask_supplier(unsigned long long* amount, char* atom, size_t atom_size);

/**
 * @brief Prompts user to select atom type and quantity, 
 * storing results in the provided parameters
 * 
 * @param amount Amount of the attoms
 * @param atom The specified atom
 * @param atom_size The size of the string
 */
void ask_requester(unsigned long long* amount, char* atom, size_t atom_size);

/**
 * Helper function to handle both IPv4 and IPv6 addresses.
 * Returns pointer to the address in the sockaddr structure.
 * 
 * @param sa Pointer to sockaddr structure (could be IPv4 or IPv6)
 * @return Pointer to the IP address part of the structure
 */
void *get_in_addr(struct sockaddr *sa);