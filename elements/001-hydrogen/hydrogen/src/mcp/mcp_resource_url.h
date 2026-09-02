#ifndef MCP_RESOURCE_URL_H
#define MCP_RESOURCE_URL_H

#include <stdbool.h>

/*
 * Returns true if `url` is non-empty, uses https, and resolves to a host
 * that is *not* loopback, link-local, RFC1918, or otherwise unreachable
 * from an external provider (xAI/OpenAI).
 *
 * This is a pure string/IP check; it does not perform DNS resolution.
 * Internal DNS names that xAI cannot reach are the operator's
 * responsibility to avoid when configuring MCP.Resource.
 */
bool mcp_mcp_resource_url_is_reachable(const char *url);

#endif
