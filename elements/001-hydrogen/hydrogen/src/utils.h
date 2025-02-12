/*
 * Utility functions and constants for the Hydrogen printer.
 * 
 * Provides common functionality used across the application:
 * - Random ID generation using consonants for readability
 * - Shared constants and macros
 */

#ifndef UTILS_H
#define UTILS_H

// System Libraries
#include <stddef.h>


#define ID_CHARS "BCDFGHKPRST"
#define ID_LEN 5

void generate_id(char *buf, size_t len);

#endif // UTILS_H
