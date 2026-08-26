#ifndef HYDROGEN_MCP_HTTP_H
#define HYDROGEN_MCP_HTTP_H

#include <stdbool.h>
#include <sys/socket.h>
#include <microhttpd.h>

#include <src/config/config_mcp.h>
#include <src/mcp/mcp_auth.h>

bool mcp_start_listen(const MCPConfig *cfg);
void mcp_stop_listen(void);
bool mcp_is_listening(void);
int mcp_http_thread_pool_size(void);

bool mcp_url_is_path(const char *url, const char *path);
bool mcp_url_is_healthz(const char *url, const char *path);
bool mcp_url_is_prm(const char *url, const char *path);
bool mcp_origin_allowed(const char *origin, const MCPConfig *cfg);
bool mcp_fill_bind_addr(const MCPConfig *cfg, struct sockaddr_storage *out);
enum MHD_Result mcp_send_prm(struct MHD_Connection *connection, const MCPConfig *cfg);

enum MHD_Result mcp_handle_request(void *cls,
                                   struct MHD_Connection *connection,
                                   const char *url,
                                   const char *method,
                                   const char *version,
                                   const char *upload_data,
                                   size_t *upload_data_size,
                                   void **con_cls);

#define MCP_UPLOAD_MAGIC 0x4D435055u

typedef struct McpHttpUpload {
    unsigned int magic;
    char *data;
    size_t size;
    size_t capacity;
    bool overflow;
} McpHttpUpload;

bool mcp_http_upload_is(const void *con_cls);
McpHttpUpload *mcp_http_upload_new(void);
bool mcp_http_upload_append(McpHttpUpload *upload, const char *data, size_t len, int max_body);
void mcp_http_upload_free(void **con_cls);
enum MHD_Result mcp_queue_rpc_response(struct MHD_Connection *connection,
                                       unsigned int status,
                                       char *body,
                                       bool take_ownership,
                                       const char *session_id);
enum MHD_Result mcp_http_handle_post(struct MHD_Connection *connection,
                                     const MCPConfig *cfg,
                                     const McpAuthResult *auth,
                                     const char *body,
                                     size_t body_len);
enum MHD_Result mcp_http_handle_delete(struct MHD_Connection *connection,
                                       const MCPConfig *cfg,
                                       const McpAuthResult *auth);

#endif
