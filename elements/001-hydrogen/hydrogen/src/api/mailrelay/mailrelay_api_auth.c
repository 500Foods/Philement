/*
 * Mail Relay API Authorization Helpers
 */

// Project includes
#include <src/hydrogen.h>

#include <src/api/auth/auth_service.h>
#include <src/mailrelay/mailrelay_repository.h>

// Local includes
#include "mailrelay_api_auth.h"

// Standard includes
#include <stdio.h>
#include <string.h>

typedef struct {
    bool found;
    int  role_id;
} RoleLookupContext;

 void mailrelay_api_role_lookup_callback(MailRelayRepoResult* result, void* user_data) {
    RoleLookupContext* ctx = (RoleLookupContext*)user_data;
    if (!result || !ctx) {
        return;
    }
    if (result->status != MAILRELAY_REPO_OK || !result->data) {
        ctx->found = false;
        return;
    }

    json_t* arr = result->data;
    if (!json_is_array(arr)) {
        ctx->found = false;
        return;
    }

    json_t* row = json_array_get(arr, 0);
    if (!row) {
        ctx->found = false;
        return;
    }

    json_t* role_json = json_object_get(row, "role_id");
    if (!role_json) role_json = json_object_get(row, "ROLE_ID");
    if (role_json && json_is_integer(role_json)) {
        ctx->found   = true;
        ctx->role_id = (int)json_integer_value(role_json);
    }
}

 bool mailrelay_api_has_role_id(const char* roles, const char* role_id_str) {
    if (!roles || !role_id_str || role_id_str[0] == '\0') {
        return false;
    }

    size_t role_len = strlen(role_id_str);
    const char* p = roles;

    while (*p != '\0') {
        while (*p == ' ' || *p == '\t') {
            p++;
        }

        if (strncmp(p, role_id_str, role_len) == 0) {
            char next = p[role_len];
            if (next == '\0' || next == ',' || next == ' ' || next == '\t') {
                return true;
            }
        }

        while (*p != '\0' && *p != ',') {
            p++;
        }
        if (*p == ',') {
            p++;
        }
    }

    return false;
}

bool mailrelay_api_has_role(const jwt_claims_t* claims, const char* role_name) {
    if (!claims || !role_name || role_name[0] == '\0') {
        return false;
    }

    RoleLookupContext ctx = {0};
    if (!mailrelay_repo_role_get_by_name(role_name, mailrelay_api_role_lookup_callback, &ctx)) {
        return false;
    }
    if (!ctx.found) {
        return false;
    }

    char role_id_str[32];
    snprintf(role_id_str, sizeof(role_id_str), "%d", ctx.role_id);

    return mailrelay_api_has_role_id(claims->roles, role_id_str);
}
