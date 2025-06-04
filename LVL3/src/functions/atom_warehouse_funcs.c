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
#include <limits.h>
#include "../../include/const.h"
#include "../../include/functions/atom_warehouse_funcs.h"
#include "../../include/elements.h"

// At the beginning of atom_warehouse_funcs.c, add these definitions:
unsigned long long carbon = 0;
unsigned long long oxygen = 0;
unsigned long long hydrogen = 0;

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
    printf("\nCARBON #:%lld \nOXYGEN #:%lld \nHYDROGEN #:%lld\n",carbon, oxygen, hydrogen);
    return;
}

void format_storage(char *out, size_t out_size) {
    snprintf(out, out_size, "CARBON: %lld\nOXYGEN: %lld\nHYDROGEN: %lld\n", carbon, oxygen, hydrogen);
}


void process_message(char* buf, size_t size_buf, u_int8_t sock_handle, char *response, size_t response_size){
    // Parse the command
    char cmd[10] = {0}, element_str[20] = {0}, element_str2[20] = {0};
    Element element;
    int amount;

    // already invalid if it shorter than 9
    if(size_buf < 9){
        fprintf(stdout, "ERROR: Message too short, invalid");
        return;
    }

    // if  we got exactly three elements, continue
    if (sscanf(buf, "%s %s %d",cmd,element_str,&amount) == 3 || (sscanf(buf, "%s %s",cmd,element_str) && strcmp(cmd,"GEN") == 0)){
        element = element_type_from_str(element_str);
        // check if its ADD and TCP
        if(!strcmp(cmd,"ADD") && sock_handle == TCP_HANDLE){
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
        else if(!strcmp(cmd,"DELIVER") && sock_handle == UDP_HANDLE){
            switch(element){
                case WATER:
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
                case CARBON_DIOXIDE:
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
                case GLUCOSE:
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
                case ALCOHOL:
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
                    }
                printf("\n-- UPDATE --\n");
                print_storage();
            }
        
        else if(sscanf(buf, "%s %s %s",cmd,element_str, element_str2) && strcmp(cmd,"GEN") == 0){
            // cjeck if we got anther word
                if(strlen(element_str2) > 1){
                    strcat(element_str, " ");
                    strcat(element_str, element_str2);
                }
            element = element_type_from_str(element_str);

            unsigned long long min = INT_MAX ;
            unsigned long long temp_water = get_water_num(oxygen,hydrogen);
            unsigned long long temp_alcohol = get_alcohol_num(carbon,oxygen,hydrogen);
            unsigned long long temp_carbonDio = get_carbonDio_num(carbon,oxygen);
            unsigned long long temp_glucose = get_glucose_num(carbon,oxygen,hydrogen);
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
                        min = temp_carbonDio;
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

        }else {                // no ADD no DELIVER? unkown
            fprintf(stdout,"ERROR: Unkown command\n");
            snprintf(response, response_size, 
                "ERROR: Unknown command\n");
        }
    }else {                  // Not in the requested format

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
