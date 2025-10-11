/*
 * Terminal Subsystem Header
 *
 * Defines the terminal subsystem interface for handling terminal-based interactions
 * through a web-based interface using xterm.js.
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <microhttpd.h>
#include <src/config/config_terminal.h>

// Terminal subsystem initialization and cleanup
bool init_terminal_support(TerminalConfig *config);
void cleanup_terminal_support(TerminalConfig *config);

// Terminal URL validation and request handling
bool is_terminal_request(const char *url, const TerminalConfig *config);
enum MHD_Result handle_terminal_request(struct MHD_Connection *connection,
                                      const char *url,
                                      const TerminalConfig *config);

/**
 * Terminal URL validator - wrapper for webserver integration
 * @param url The URL to validate
 * @return true if URL is handled by terminal subsystem
 */
bool terminal_url_validator(const char *url);

/**
 * Terminal request handler - wrapper for webserver integration
 * Compatible with MHD webserver callback requirements
 */
enum MHD_Result terminal_request_handler(void *cls,
                                       struct MHD_Connection *connection,
                                       const char *url,
                                       const char *method,
                                       const char *version,
                                       const char *upload_data,
                                       size_t *upload_data_size,
                                       void **con_cls);

#endif /* TERMINAL_H */
