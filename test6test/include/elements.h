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
	SOFT_DRINK,		// = 7
	VODKA,			// = 8
	CHAMPAGNE,		// = 9
	UNKNOWN
} Element, *PElement;

/**
 * @brief Translates strings to enums for better use
 * 
 * @param str The Element
 * @return Element enum
 */
Element element_type_from_str(const char *str);