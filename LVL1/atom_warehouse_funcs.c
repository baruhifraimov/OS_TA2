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
#include "const.h"
#include "atom_warehouse_funcs.h"

// At the beginning of atom_warehouse_funcs.c, add these definitions:
unsigned long long carbon = 0;
unsigned long long oxygen = 0;
unsigned long long hydrogen = 0;

void print_storage(){
    printf("CARBON #:%lld \nOXYGEN #:%lld \nHYDROGEN #:%lld\n",carbon, oxygen, hydrogen);
    return;
}

void process_message(char* buf, size_t size_buf){
    // Parse the command
    char cmd[10], atom_type[20];
    int amount;

    // already invalid if it shorter than 9
    if(size_buf < 9){
        fprintf(stdout, "ERROR: Message too short, invalid");
        return;
    }

    // if  we got exactly three elements, continue
    if (sscanf(buf, "%s %s %d",cmd,atom_type,&amount) == 3){
        if(!strcmp(cmd,"ADD")){
            // Update the appropriate counter based on atom type
            if (strcmp(atom_type, "CARBON") == 0) {
                carbon += amount;
            } else if (strcmp(atom_type, "OXYGEN") == 0) {
                oxygen += amount;
            } else if (strcmp(atom_type, "HYDROGEN") == 0) {
                hydrogen += amount;
            } else {
                fprintf(stdout,"ERROR: Unkown atom type\n");
                return;
            }
            // Print the storage to server console
            print_storage();
        }
    }else {
        fprintf(stdout,"ERROR: invalid command format\n");
        return;
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
