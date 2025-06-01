#pragma once

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
#include "../const.h"

#define BACKLOG 10   // Maximum number of pending client connections in the queue

// Warehouse supply, (unsigned long long = 10^18)
extern unsigned long long carbon;
extern unsigned long long oxygen;
extern unsigned long long hydrogen;

/**
 * @brief Prints the whole warehouse storage
 */
void print_storage();

/**
 * @brief processes a message buffer containing a command, 
 * an atom type, and an amount. It validates the message format, 
 * updates counters for specific atom types (CARBON, OXYGEN, HYDROGEN) 
 * if the command is "ADD," and prints the updated storage, 
 * while handling errors for invalid formats or unknown atom types.
 * 
 * @param buf The message we want to handle
 * @param size_buf Size of the message
* @param sock_handle Determines if UDP or TCP
* @param response the response we want to send to the client
* @param response_size size of the response
*/
void process_message(char* buf, size_t size_buf, u_int8_t sock_handle, char *response, size_t response_size);

/**
 * @brief Signal handler for cleaning up zombie child processes
 * When a child process (handling a client) terminates, this prevents zombie processes
 * by properly reaping the dead child processes using waitpid()
 */
void sigchld_handler(int s);

/**
 * @brief Helper function to extract IP address from sockaddr structure
 * Works with both IPv4 and IPv6 addresses by checking the address family
 * Returns pointer to the actual IP address part of the structure
 */
void *get_in_addr(struct sockaddr *sa);
