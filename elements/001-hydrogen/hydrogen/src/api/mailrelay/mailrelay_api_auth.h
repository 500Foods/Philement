/*
 * Mail Relay API Authorization Helpers
 *
 * Shared role-check logic for the Mail Relay REST endpoints. Resolves a
 * human-readable role name (e.g. "mail_send") to its database role_id and
 * then checks the JWT `roles` claim, which stores role_ids as a
 * comma-separated list of integers.
 */

#ifndef MAILRELAY_API_AUTH_H
#define MAILRELAY_API_AUTH_H

#include <stdbool.h>

#include <src/api/auth/auth_service.h>
#include <src/mailrelay/mailrelay_repository.h>  // For MailRelayRepoResult

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Return true if the JWT claims contain the named role.
 *
 * The role name is resolved to a role_id at request time via QueryRef #127
 * (Get Role By Name). If resolution fails or the role is not present in the
 * JWT `roles` claim, the function returns false.
 */
bool mailrelay_api_has_role(const jwt_claims_t* claims, const char* role_name);

/*
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly.
 */
void mailrelay_api_role_lookup_callback(MailRelayRepoResult* result, void* user_data);
bool mailrelay_api_has_role_id(const char* roles, const char* role_id_str);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_API_AUTH_H */
