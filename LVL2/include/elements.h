#pragma once
#include <string.h>

typedef enum {
	CARBON,			// = 0
	OXYGEN,			// = 1
	HYDROGEN,		// = 2
	WATER,			// = 3
	CARBON_DIOXIDE,	// = 4
	GLUCOSE,		// = 5
	ALCOHOL,		// = 6
	UNKNOWN
} Element, *PElement;

/**
 * @brief Translates strings to enums for better use
 * 
 * @param str The Element
 * @return Element enum
 */
Element element_type_from_str(const char *str) {
    if (strcmp(str, "CARBON") == 0) return CARBON;
    if (strcmp(str, "OXYGEN") == 0) return OXYGEN;
    if (strcmp(str, "HYDROGEN") == 0) return HYDROGEN;
	if (strcmp(str, "WATER") == 0) return WATER;
    if (strcmp(str, "CARBONDIOXIDE") == 0) return CARBON_DIOXIDE;
    if (strcmp(str, "GLUCOSE") == 0) return GLUCOSE;
	if (strcmp(str, "ALCOHOL") == 0) return ALCOHOL;

    return UNKNOWN;
}