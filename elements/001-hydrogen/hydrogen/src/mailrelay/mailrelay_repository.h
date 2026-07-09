/*
 * Mail Relay Repository
 *
 * Callback-based access to the Mail Relay QueryRefs (093-126). The repository
 * builds typed JSON parameter objects and executes them through a swappable
 * seam so unit tests can mock the database layer.
 */

#ifndef MAILRELAY_REPOSITORY_H
#define MAILRELAY_REPOSITORY_H

// System includes
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Third-party includes
#include <jansson.h>

// Project includes
#include <src/config/config_mail_relay.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_result.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * QueryRef identifiers
 * -------------------------------------------------------------------------- */

#define MAILRELAY_QREF_QUEUE_INSERT           93
#define MAILRELAY_QREF_QUEUE_GET_BY_UUID      94
#define MAILRELAY_QREF_QUEUE_GET_BY_IDEMPOTENCY 95
#define MAILRELAY_QREF_QUEUE_SELECT_NEXT_PENDING 96
#define MAILRELAY_QREF_QUEUE_MARK_SENDING     97
#define MAILRELAY_QREF_QUEUE_MARK_SENT        98
#define MAILRELAY_QREF_QUEUE_MARK_FAILED      99
#define MAILRELAY_QREF_QUEUE_RESCHEDULE      100
#define MAILRELAY_QREF_QUEUE_RECOVER_STALE   101
#define MAILRELAY_QREF_ATTEMPT_INSERT        102
#define MAILRELAY_QREF_TEMPLATE_GET_BY_KEY   103
#define MAILRELAY_QREF_TEMPLATE_LIST_ACTIVE  104
#define MAILRELAY_QREF_TEMPLATE_INSERT       105
#define MAILRELAY_QREF_TEMPLATE_UPDATE       106
#define MAILRELAY_QREF_TEMPLATE_SOFT_DELETE  107
#define MAILRELAY_QREF_EVENT_INSERT          108
#define MAILRELAY_QREF_EVENT_LIST_PENDING    109
#define MAILRELAY_QREF_EVENT_MARK_PROCESSED  110
#define MAILRELAY_QREF_EVENT_MARK_SUPPRESSED 111
#define MAILRELAY_QREF_OTP_INSERT            112
#define MAILRELAY_QREF_OTP_GET_ACTIVE        113
#define MAILRELAY_QREF_OTP_CONSUME           114
#define MAILRELAY_QREF_OTP_INCREMENT_ATTEMPTS 115
#define MAILRELAY_QREF_OTP_EXPIRE_OLD        116
#define MAILRELAY_QREF_OTP_GET_BY_ID         117
#define MAILRELAY_QREF_ROUTE_GET_BY_SENDER_DOMAIN 118
#define MAILRELAY_QREF_ROUTE_LIST_ACTIVE     119
#define MAILRELAY_QREF_ROUTE_INSERT          120
#define MAILRELAY_QREF_ROUTE_UPDATE          121
#define MAILRELAY_QREF_ROUTE_SOFT_DELETE     122
#define MAILRELAY_QREF_CLEANUP_QUEUE         123
#define MAILRELAY_QREF_CLEANUP_EVENTS        124
#define MAILRELAY_QREF_CLEANUP_ATTEMPTS      125
#define MAILRELAY_QREF_CLEANUP_OTP           126
#define MAILRELAY_QREF_ROLE_GET_BY_NAME      127

/* --------------------------------------------------------------------------
 * Result type
 * -------------------------------------------------------------------------- */

typedef enum {
    MAILRELAY_REPO_OK = 0,
    MAILRELAY_REPO_INVALID_ARGS,
    MAILRELAY_REPO_NO_DATABASE,
    MAILRELAY_REPO_QUERY_NOT_FOUND,
    MAILRELAY_REPO_SUBMIT_FAILED,
    MAILRELAY_REPO_TIMEOUT,
    MAILRELAY_REPO_QUERY_ERROR,
    MAILRELAY_REPO_PARSE_ERROR
} MailRelayRepoStatus;

typedef struct {
    MailRelayRepoStatus status;
    const char* error_message;   /**< Static error description; do not free. */
    json_t* data;                  /**< Borrowed Jansson value (array/object). */
    int affected_rows;
} MailRelayRepoResult;

typedef void (*mailrelay_repo_callback_fn)(MailRelayRepoResult* result, void* user_data);

/*
 * Execution seam. The default implementation resolves the configured Mail
 * Relay database, looks up the QueryRef in the QTC, submits the query, waits
 * for the result, and invokes the callback. Unit tests can replace this seam
 * to assert on parameters and inject results without a live database.
 */
typedef bool (*mailrelay_repo_execute_fn)(int query_ref,
                                          const char* params_json,
                                          mailrelay_repo_callback_fn callback,
                                          void* user_data);

void mailrelay_repo_set_executor(mailrelay_repo_execute_fn executor);
mailrelay_repo_execute_fn mailrelay_repo_get_executor(void);

/* --------------------------------------------------------------------------
 * Parameter structs and helper functions for each QueryRef
 * -------------------------------------------------------------------------- */

typedef struct {
    const char* message_uuid;
    int priority;
    const char* template_key;
    const char* from_addr;
    const char* reply_to;
    const char* recipients_json;
    const char* subject;
    const char* body_text;
    const char* body_html;
    const char* headers_json;
    const char* idempotency_key;
    const char* next_attempt_at;   /**< ISO 8601 timestamp or NULL. */
} MailRelayRepoQueueInsert;

bool mailrelay_repo_queue_insert(const MailRelayRepoQueueInsert* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data);

typedef struct {
    const char* message_uuid;
} MailRelayRepoQueueGetByUuid;

bool mailrelay_repo_queue_get_by_uuid(const MailRelayRepoQueueGetByUuid* params,
                                      mailrelay_repo_callback_fn callback,
                                      void* user_data);

typedef struct {
    const char* idempotency_key;
} MailRelayRepoQueueGetByIdempotency;

bool mailrelay_repo_queue_get_by_idempotency(
    const MailRelayRepoQueueGetByIdempotency* params,
    mailrelay_repo_callback_fn callback,
    void* user_data);

bool mailrelay_repo_queue_select_next_pending(mailrelay_repo_callback_fn callback,
                                              void* user_data);

typedef struct {
    long long queue_id;
    const char* instance_id;
    const char* claim_token;
} MailRelayRepoQueueMarkSending;

bool mailrelay_repo_queue_mark_sending(const MailRelayRepoQueueMarkSending* params,
                                       mailrelay_repo_callback_fn callback,
                                       void* user_data);

typedef struct {
    long long queue_id;
    int smtp_code;
    const char* smtp_text;
} MailRelayRepoQueueMarkSent;

bool mailrelay_repo_queue_mark_sent(const MailRelayRepoQueueMarkSent* params,
                                    mailrelay_repo_callback_fn callback,
                                    void* user_data);

typedef struct {
    long long queue_id;
    int smtp_code;
    const char* smtp_text;
} MailRelayRepoQueueMarkFailed;

bool mailrelay_repo_queue_mark_failed(const MailRelayRepoQueueMarkFailed* params,
                                      mailrelay_repo_callback_fn callback,
                                      void* user_data);

typedef struct {
    long long queue_id;
    const char* next_attempt_at;
    int attempts;
} MailRelayRepoQueueReschedule;

bool mailrelay_repo_queue_reschedule(const MailRelayRepoQueueReschedule* params,
                                     mailrelay_repo_callback_fn callback,
                                     void* user_data);

typedef struct {
    const char* stale_before;      /**< ISO 8601 timestamp. */
} MailRelayRepoQueueRecoverStale;

bool mailrelay_repo_queue_recover_stale(const MailRelayRepoQueueRecoverStale* params,
                                        mailrelay_repo_callback_fn callback,
                                        void* user_data);

typedef struct {
    long long queue_id;
    int attempt_number;
    int server_index;
    bool success;
    int smtp_code;
    const char* smtp_text;
    long long duration_ms;
    const char* error_class;
} MailRelayRepoAttemptInsert;

bool mailrelay_repo_attempt_insert(const MailRelayRepoAttemptInsert* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data);

typedef struct {
    const char* template_key;
} MailRelayRepoTemplateGetByKey;

bool mailrelay_repo_template_get_by_key(const MailRelayRepoTemplateGetByKey* params,
                                        mailrelay_repo_callback_fn callback,
                                        void* user_data);

bool mailrelay_repo_template_list_active(mailrelay_repo_callback_fn callback,
                                         void* user_data);

typedef struct {
    const char* template_key;
    const char* name;
    int status_a64;
    const char* subject_template;
    const char* text_template;
    const char* html_template;
    const char* collection;        /**< JSON string or NULL. */
} MailRelayRepoTemplateInsert;

bool mailrelay_repo_template_insert(const MailRelayRepoTemplateInsert* params,
                                    mailrelay_repo_callback_fn callback,
                                    void* user_data);

typedef struct {
    const char* template_key;
    const char* name;
    int status_a64;
    const char* subject_template;
    const char* text_template;
    const char* html_template;
    const char* collection;        /**< JSON string or NULL. */
} MailRelayRepoTemplateUpdate;

bool mailrelay_repo_template_update(const MailRelayRepoTemplateUpdate* params,
                                    mailrelay_repo_callback_fn callback,
                                    void* user_data);

typedef struct {
    const char* template_key;
} MailRelayRepoTemplateSoftDelete;

bool mailrelay_repo_template_soft_delete(const MailRelayRepoTemplateSoftDelete* params,
                                       mailrelay_repo_callback_fn callback,
                                       void* user_data);

typedef struct {
    const char* event_key;
    int status_a65;
    const char* template_key;
    const char* from_addr;
    const char* reply_to;
    const char* recipients_json;
    const char* subject;
    const char* body_text;
    const char* body_html;
    const char* headers_json;
    const char* params_json;
    const char* debounce_key;
    const char* idempotency_key;
    int priority;
} MailRelayRepoEventInsert;

bool mailrelay_repo_event_insert(const MailRelayRepoEventInsert* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data);

bool mailrelay_repo_event_list_pending(mailrelay_repo_callback_fn callback,
                                       void* user_data);

typedef struct {
    long long event_id;
    int status_a65;
    long long queue_id;
} MailRelayRepoEventMarkProcessed;

bool mailrelay_repo_event_mark_processed(const MailRelayRepoEventMarkProcessed* params,
                                         mailrelay_repo_callback_fn callback,
                                         void* user_data);

typedef struct {
    long long event_id;
} MailRelayRepoEventMarkSuppressed;

bool mailrelay_repo_event_mark_suppressed(const MailRelayRepoEventMarkSuppressed* params,
                                          mailrelay_repo_callback_fn callback,
                                          void* user_data);

typedef struct {
    const char* code_hash;
    const char* email;
    long long account_id;
    int purpose_a66;
    int status_a67;
    const char* expiry_at;         /**< ISO 8601 timestamp. */
    int max_attempts;
} MailRelayRepoOtpInsert;

bool mailrelay_repo_otp_insert(const MailRelayRepoOtpInsert* params,
                               mailrelay_repo_callback_fn callback,
                               void* user_data);

typedef struct {
    const char* email;
    int purpose_a66;
} MailRelayRepoOtpGetActive;

bool mailrelay_repo_otp_get_active(const MailRelayRepoOtpGetActive* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data);

typedef struct {
    long long otp_id;
} MailRelayRepoOtpConsume;

bool mailrelay_repo_otp_consume(const MailRelayRepoOtpConsume* params,
                                mailrelay_repo_callback_fn callback,
                                void* user_data);

typedef struct {
    long long otp_id;
} MailRelayRepoOtpIncrementAttempts;

bool mailrelay_repo_otp_increment_attempts(const MailRelayRepoOtpIncrementAttempts* params,
                                             mailrelay_repo_callback_fn callback,
                                             void* user_data);

typedef struct {
    const char* expiry_cutoff_at;  /**< ISO 8601 timestamp. */
} MailRelayRepoOtpExpireOld;

bool mailrelay_repo_otp_expire_old(const MailRelayRepoOtpExpireOld* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data);

typedef struct {
    long long otp_id;
} MailRelayRepoOtpGetById;

bool mailrelay_repo_otp_get_by_id(const MailRelayRepoOtpGetById* params,
                                  mailrelay_repo_callback_fn callback,
                                  void* user_data);

typedef struct {
    const char* sender_domain;
} MailRelayRepoRouteGetBySenderDomain;

bool mailrelay_repo_route_get_by_sender_domain(
    const MailRelayRepoRouteGetBySenderDomain* params,
    mailrelay_repo_callback_fn callback,
    void* user_data);

bool mailrelay_repo_route_list_active(mailrelay_repo_callback_fn callback,
                                      void* user_data);

typedef struct {
    int status_a68;
    const char* source_network;
    const char* sender_domain;
    const char* sender_pattern;
    const char* recipient_domain;
    const char* recipient_pattern;
    bool auth_required;
    bool require_tls;
    const char* template_key;
    const char* rewrite_from;
    const char* rewrite_to;
    const char* add_recipients_json;
    int priority;
    int sort_seq;
} MailRelayRepoRouteInsert;

bool mailrelay_repo_route_insert(const MailRelayRepoRouteInsert* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data);

typedef struct {
    long long route_id;
    int status_a68;
    const char* source_network;
    const char* sender_domain;
    const char* sender_pattern;
    const char* recipient_domain;
    const char* recipient_pattern;
    bool auth_required;
    bool require_tls;
    const char* template_key;
    const char* rewrite_from;
    const char* rewrite_to;
    const char* add_recipients_json;
    int priority;
    int sort_seq;
} MailRelayRepoRouteUpdate;

bool mailrelay_repo_route_update(const MailRelayRepoRouteUpdate* params,
                                 mailrelay_repo_callback_fn callback,
                                 void* user_data);

typedef struct {
    long long route_id;
} MailRelayRepoRouteSoftDelete;

bool mailrelay_repo_route_soft_delete(const MailRelayRepoRouteSoftDelete* params,
                                      mailrelay_repo_callback_fn callback,
                                      void* user_data);

typedef struct {
    const char* cutoff_at;         /**< ISO 8601 timestamp. */
} MailRelayRepoCleanupQueue;

bool mailrelay_repo_cleanup_queue(const MailRelayRepoCleanupQueue* params,
                                  mailrelay_repo_callback_fn callback,
                                  void* user_data);

typedef struct {
    const char* cutoff_at;         /**< ISO 8601 timestamp. */
} MailRelayRepoCleanupEvents;

bool mailrelay_repo_cleanup_events(const MailRelayRepoCleanupEvents* params,
                                   mailrelay_repo_callback_fn callback,
                                   void* user_data);

typedef struct {
    const char* cutoff_at;         /**< ISO 8601 timestamp. */
} MailRelayRepoCleanupAttempts;

bool mailrelay_repo_cleanup_attempts(const MailRelayRepoCleanupAttempts* params,
                                     mailrelay_repo_callback_fn callback,
                                     void* user_data);

typedef struct {
    const char* cutoff_at;         /**< ISO 8601 timestamp. */
} MailRelayRepoCleanupOtp;

bool mailrelay_repo_cleanup_otp(const MailRelayRepoCleanupOtp* params,
                              mailrelay_repo_callback_fn callback,
                              void* user_data);

/* --------------------------------------------------------------------------
 * Role QueryRefs
 * -------------------------------------------------------------------------- */

bool mailrelay_repo_role_get_by_name(const char* name,
                                     mailrelay_repo_callback_fn callback,
                                     void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_REPOSITORY_H */
