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
#include "../../include/const.h"
#include "../../include/functions/atom_warehouse_funcs.h"
#include "../../include/elements.h"

// At the beginning of atom_warehouse_funcs.c, add these definitions:
unsigned long long carbon = 0;
unsigned long long oxygen = 0;
unsigned long long hydrogen = 0;

void print_storage(){
    printf("\nCARBON #:%lld \nOXYGEN #:%lld \nHYDROGEN #:%lld\n",carbon, oxygen, hydrogen);
    return;
}

void format_storage(char *out, size_t out_size) {
    snprintf(out, out_size, "CARBON: %lld\nOXYGEN: %lld\nHYDROGEN: %lld\n", carbon, oxygen, hydrogen);
}


void process_message(char* buf, size_t size_buf, u_int8_t sock_handle, char *response, size_t response_size){
    // Parse the command
    char cmd[10], element_str[20];
    Element element;
    int amount;

    // already invalid if it shorter than 9
    if(size_buf < 9){
        fprintf(stdout, "ERROR: Message too short, invalid");
        return;
    }

    // if  we got exactly three elements, continue
    if (sscanf(buf, "%s %s %d",cmd,element_str,&amount) == 3){
        element = element_type_from_str(element_str);
        // check if its ADD and TCP
        if(!strcmp(cmd,"ADD") && sock_handle == 0){
            switch(element){
                case 0:
                    carbon += amount;
                    break;
                case 1:
                    oxygen += amount;
                    break;
                case 2:
                    hydrogen += amount;
                    break;
                default:
                    fprintf(stdout,"ERROR: Unkown atom type\n");
                    snprintf(response, response_size, 
                        "ERROR: Unkown atom type\n");
                    return;
            }
            format_storage(response, response_size);
            // Print the storage to server console
            print_storage();
        }
        // check if its DELIVER and UDP
        else if(!strcmp(cmd,"DELIVER") && sock_handle == 1){
            switch(element){
                case 3:
                if(hydrogen>=2*amount && oxygen >=1*amount){
                    hydrogen -= 2*amount;
                    oxygen -= 1*amount;
                    snprintf(response, response_size, 
                        "#%d WATER DELIVERED",amount);
                }else{
                    fprintf(stderr,"Not enough atoms to make WATER");
                    snprintf(response, response_size, 
                        "ERROR: Not enough atoms to make WATER\n");
                }
                    break;
                case 4:
                if(carbon >=1*amount && oxygen>=2*amount){
                    carbon -= 1*amount;
                    oxygen -= 2*amount;
                    snprintf(response, response_size, 
                        "#%d CARBON DIOXIDE DELIVERED",amount);
                }else{
                    fprintf(stderr,"Not enough atoms to make CARBON DIOXIDE");
                    snprintf(response, response_size, 
                        "ERROR: Not enough atoms to make CARBON DIOXIDE\n");
                }
                    break;
                case 5:
                if(carbon >= 6*amount && hydrogen >= 12*amount && oxygen >= 6*amount){
                    carbon -= 6*amount;
                    hydrogen -= 12*amount;
                    oxygen -= 6*amount;
                    snprintf(response, response_size, 
                        "#%d GLUCOSE DELIVERED", amount);
                } else{
                    fprintf(stderr,"Not enough atoms to make GLUCOSE");
                    snprintf(response, response_size, 
                        "ERROR: Not enough atoms to make GLUCOSE\n");
                }
                    break;
                case 6:
                if(carbon >= 2*amount && hydrogen >= 6*amount && oxygen >= 1*amount){
                    carbon -= 2*amount;
                    hydrogen -=6*amount;
                    oxygen -=1*amount;
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

                printf("\n-- UPDATE --\n");
                print_storage();
            }
        }else {
            // no ADD no DELIVER? unkown
            fprintf(stdout,"ERROR: Unkown command\n");
            snprintf(response, response_size, 
                "ERROR: Unknown command\n");
        }
    }else {
        // Not in the requested format
        fprintf(stdout,"ERROR: invalid format\n");
        snprintf(response, response_size, 
            "ERROR: invalid format\n");
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
