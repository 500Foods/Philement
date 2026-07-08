/*
 * UUID generation utility.
 *
 * Provides a common, collision-resistant UUID-like string generator that can be
 * used by any subsystem. Previously lived in the webserver upload module; it
 * was moved here so subsystems such as Mail Relay can use it without depending
 * on webserver internals.
 */

#ifndef UTILS_UUID_H
#define UTILS_UUID_H

#include <src/globals.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Generate a collision-resistant UUID-like string.
 *
 * The output is written into uuid_str, which must be at least UUID_STR_LEN
 * bytes (defined in globals.h).
 */
void generate_uuid(char *uuid_str);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_UUID_H */
