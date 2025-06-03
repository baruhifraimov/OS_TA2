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

/**
 * @brief Calculates the maximum number of WATER molecules that can be formed
 *        from the given amounts of carbon, oxygen, and hydrogen atoms.
 *        (WATER: H2O, needs 2 hydrogen and 1 oxygen per molecule)
 * 
 * @param oxygen   Number of oxygen atoms
 * @param hydrogen Number of hydrogen atoms
 * @return Maximum number of water molecules that can be formed
 */
unsigned long long get_water_num(unsigned long long oxygen, unsigned long long hydrogen);

/**
 * @brief Calculates the maximum number of CARBON DIOXIDE molecules that can be formed
 *        from the given amounts of carbon and oxygen atoms.
 *        (CO2: needs 1 carbon and 2 oxygen per molecule)
 * 
 * @param carbon   Number of carbon atoms
 * @param oxygen   Number of oxygen atoms
 * @return Maximum number of carbon dioxide molecules that can be formed
 */
unsigned long long get_carbonDio_num(unsigned long long carbon, unsigned long long oxygen);

/**
 * @brief Calculates the maximum number of ALCOHOL molecules that can be formed
 *        from the given amounts of carbon, oxygen, and hydrogen atoms.
 *        (ALCOHOL: C2H6O, needs 2 carbon, 6 hydrogen, 1 oxygen per molecule)
 * 
 * @param carbon   Number of carbon atoms
 * @param oxygen   Number of oxygen atoms
 * @param hydrogen Number of hydrogen atoms
 * @return Maximum number of alcohol molecules that can be formed
 */
unsigned long long get_alcohol_num(unsigned long long carbon, unsigned long long oxygen, unsigned long long hydrogen);

/**
 * @brief Calculates the maximum number of GLUCOSE molecules that can be formed
 *        from the given amounts of carbon, oxygen, and hydrogen atoms.
 *        (GLUCOSE: C6H12O6, needs 6 carbon, 12 hydrogen, 6 oxygen per molecule)
 * 
 * @param carbon   Number of carbon atoms
 * @param oxygen   Number of oxygen atoms
 * @param hydrogen Number of hydrogen atoms
 * @return Maximum number of glucose molecules that can be formed
 */
unsigned long long get_glucose_num(unsigned long long carbon, unsigned long long oxygen, unsigned long long hydrogen);

/**
 * @brief Alarm handle, handles the SIGALRM signal
 * 
 */
void alarm_handler(int signum);