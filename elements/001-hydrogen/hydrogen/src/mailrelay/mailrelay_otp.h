/*
 * Mail Relay OTP generation and verification primitives.
 *
 * Phase 8: generate, hash, constant-time compare, generate_and_send, verify.
 */

#ifndef MAILRELAY_OTP_H
#define MAILRELAY_OTP_H

#include <stdbool.h>
#include <stddef.h>

#include <src/mailrelay/mailrelay_queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Lookup 066: Mail OTP Purpose */
#define MAIL_OTP_PURPOSE_LOGIN_MFA      0
#define MAIL_OTP_PURPOSE_EMAIL_VERIFY   1
#define MAIL_OTP_PURPOSE_PASSWORD_RESET 2

/** Lookup 067: Mail OTP Status */
#define MAIL_OTP_STATUS_ACTIVE                  0
#define MAIL_OTP_STATUS_CONSUMED                1
#define MAIL_OTP_STATUS_EXPIRED                 2
#define MAIL_OTP_STATUS_MAX_ATTEMPTS_EXCEEDED   3

/** Default OTP policy (matches MailRelay.Otp defaults). */
#define MAIL_OTP_DEFAULT_DIGITS          6
#define MAIL_OTP_DEFAULT_EXPIRY_SECONDS  300
#define MAIL_OTP_DEFAULT_MAX_ATTEMPTS    5

/** Allowed Digits range (also in hydrogen_config_schema.json). */
#define MAIL_OTP_MIN_DIGITS  4
#define MAIL_OTP_MAX_DIGITS  10

/** SHA-256 hex digest length including NUL. */
#define MAIL_OTP_HASH_HEX_LEN 65

/** Template key for OTP mail (seeded by migration 1261). */
#define MAIL_OTP_TEMPLATE_KEY "auth.otp_code"

/**
 * Injectable random source for deterministic Unity tests.
 * Signature matches utils_random_bytes. NULL restores default.
 */
typedef bool (*mailrelay_otp_random_fn)(unsigned char* buffer, size_t length);

void mailrelay_otp_set_random_fn(mailrelay_otp_random_fn fn);

/**
 * Generate a numeric OTP code of the given length.
 *
 * @param digits  Code length (MAIL_OTP_MIN_DIGITS..MAIL_OTP_MAX_DIGITS).
 * @param out     Caller buffer of at least digits+1 bytes; receives digits
 *                ASCII digits and a trailing NUL. Never logs the code.
 * @return true on success.
 */
bool mailrelay_otp_generate_code(int digits, char* out, size_t out_size);

/**
 * SHA-256 hash of the plaintext code as lowercase hex (64 chars + NUL).
 *
 * @param code  NUL-terminated plaintext OTP (must not be empty).
 * @param out   Buffer of at least MAIL_OTP_HASH_HEX_LEN bytes.
 * @return true on success.
 */
bool mailrelay_otp_hash_code(const char* code, char* out, size_t out_size);

/**
 * Constant-time equality for two NUL-terminated strings of equal length.
 * Returns false if either is NULL or lengths differ.
 */
bool mailrelay_otp_constant_time_equal(const char* a, const char* b);

/**
 * Wipe a buffer that may hold plaintext OTP material.
 */
void mailrelay_otp_wipe(void* buf, size_t len);

/**
 * Request to generate, store, and mail an OTP.
 * Plaintext code is never returned to the caller.
 */
typedef struct MailRelayOtpSendRequest {
    const char* email;           /**< Required recipient. */
    long long account_id;        /**< Optional; 0 if unknown. */
    int purpose_a66;             /**< MAIL_OTP_PURPOSE_*. */
    int digits;                  /**< 0 = MailRelay.Otp.Digits or default. */
    int expiry_seconds;          /**< 0 = MailRelay.Otp.ExpirySeconds or default. */
    int max_attempts;            /**< 0 = MailRelay.Otp.MaxAttempts or default. */
    const char* from;            /**< Optional From override. */
    const char* app_name;        /**< Optional %APP_NAME%. */
    const char* server_name;     /**< Optional %SERVER_NAME%. */
    const char* request_id;      /**< Optional %REQUEST_ID%. */
    int priority;                /**< Queue priority for the mail. */
} MailRelayOtpSendRequest;

/**
 * Result of a successful generate_and_send (no plaintext).
 */
typedef struct MailRelayOtpSendResponse {
    long long otp_id;            /**< Stored row id when known; 0 if not returned. */
    char* message_id;            /**< Caller-owned mail message id (may be NULL). */
    char* status;                /**< Caller-owned mail status (e.g. "queued"). */
} MailRelayOtpSendResponse;

void mailrelay_otp_send_response_init(MailRelayOtpSendResponse* resp);
void mailrelay_otp_send_response_free(MailRelayOtpSendResponse* resp);

/**
 * Generate OTP, hash, insert mail_otp_codes, enqueue auth.otp_code template,
 * wipe plaintext. Never logs or returns the code.
 *
 * @param req  Send request.
 * @param resp Out: otp_id / message_id / status on success. Init and free with helpers.
 * @param err  Error buffer for MAIL_* strings.
 * @param err_cap Capacity of err.
 * @return MAILRELAY_OK on success.
 */
MailRelayStatus mailrelay_otp_generate_and_send(const MailRelayOtpSendRequest* req,
                                                MailRelayOtpSendResponse* resp,
                                                char* err,
                                                size_t err_cap);

/**
 * Request to verify a submitted OTP (email + purpose lookup).
 * Plaintext code is wiped after hashing; never logged.
 */
typedef struct MailRelayOtpVerifyRequest {
    const char* email;           /**< Required. */
    int purpose_a66;             /**< MAIL_OTP_PURPOSE_*. */
    const char* code;            /**< Submitted plaintext OTP. */
} MailRelayOtpVerifyRequest;

/**
 * Result of a successful verify (no plaintext).
 */
typedef struct MailRelayOtpVerifyResponse {
    long long otp_id;
    long long account_id;
} MailRelayOtpVerifyResponse;

void mailrelay_otp_verify_response_init(MailRelayOtpVerifyResponse* resp);

/**
 * Verify OTP: get active (113) → C expiry check → hash/compare →
 * consume (114) or increment (115) / mark max (128).
 * Never logs or returns the submitted code.
 *
 * @return MAILRELAY_OK on success; MAILRELAY_INVALID_ARGS for logical
 *         failures (MAIL_OTP_* in err); MAILRELAY_PERSIST_FAILED on DB errors.
 */
MailRelayStatus mailrelay_otp_verify(const MailRelayOtpVerifyRequest* req,
                                     MailRelayOtpVerifyResponse* resp,
                                     char* err,
                                     size_t err_cap);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_OTP_H */
