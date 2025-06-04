// filepath: /home/parallels/Documents/OS_TA2/LVL6/src/elements.c
#include "../include/elements.h"
#include <string.h>

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