/*
 * Mail Relay JSON Configuration Loading
 *
 * This module handles loading mail relay configuration from JSON,
 * including environment variable substitution and validation.
 */

#ifndef JSON_MAIL_RELAY_H
#define JSON_MAIL_RELAY_H

#include <jansson.h>
#include "../config.h"

/*
 * Load mail relay configuration from JSON
 *
 * This function loads the mail relay configuration from a JSON object,
 * handling environment variable substitution and validation.
 *
 * JSON Structure:
 * {
 *   "MailRelay": {
 *     "Enabled": true,
 *     "ListenPort": 587,
 *     "Workers": 2,
 *     "QueueSettings": {
 *       "MaxQueueSize": 1000,
 *       "RetryAttempts": 3,
 *       "RetryDelaySeconds": 300
 *     },
 *     "OutboundServers": [
 *       {
 *         "Host": "${env.SMTP_SERVER1_HOST}",
 *         "Port": "${env.SMTP_SERVER1_PORT}",
 *         "Username": "${env.SMTP_SERVER1_USER}",
 *         "Password": "${env.SMTP_SERVER1_PASS}",
 *         "UseTLS": true
 *       }
 *     ]
 *   }
 * }
 *
 * @param root The root JSON object
 * @param config The configuration structure to populate
 * @return true on success, false on failure
 */
bool load_json_mail_relay(json_t* root, AppConfig* config);

#endif /* JSON_MAIL_RELAY_H */