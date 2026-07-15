# Static functions report: hydrogen/src (mailrelay + scripting)

Scope: `elements/001-hydrogen/hydrogen/src/mailrelay` and `.../src/scripting`.
Excludes `examples/` and the benign `static` in `hydrogen.h`.

## Summary

- **146** `static` function declarations (some files declare a prototype + define it -> see below).
- **86** unique function names; **116** unique (name,file) pairs.
- **0 true cross-file collisions** within scope: all 30 repeated names are prototype+definition pairs in the *same* `.c` file.
- **0 collisions** with the rest of `hydrogen/src` — safe to un-`static` from a linker standpoint.
- Caveat: removing `static` alone does not make a function callable from Unity tests; it also needs a visible declaration (header).

## Per-file listing

### `http_client.c`

- L129: `static long resolve_timeout(int requested) {`
- L99: `static struct OidcRpHttpResponse* try_test_injection(const char* url) {`

### `http_pool.c`

- L110: `static void scripting_http_worker_process_one(ScriptingHttpPool* pool,`
- L175: `static bool scripting_http_worker_should_exit(ScriptingHttpPool* pool) {`
- L38: `static void* scripting_http_worker_thread(void* arg);`
- L39: `static void scripting_http_worker_process_one(ScriptingHttpPool* pool,`
- L41: `static bool scripting_http_worker_should_exit(ScriptingHttpPool* pool);`
- L59: `static char* serialize_headers(const OidcRpHttpResponse* resp) {`
- L84: `static void* scripting_http_worker_thread(void* arg) {`

### `lua_context.c`

- L156: `static int H_lua_panic(lua_State* L) {`
- L33: `static void H_lua_open_sandboxed_libraries(lua_State* L);`
- L34: `static int H_lua_panic(lua_State* L);`
- L93: `static void H_lua_open_sandboxed_libraries(lua_State* L) {`

### `lua_hook.c`

- L77: `static void H_lua_progress_hook_fn(lua_State* L, lua_Debug* ar) {`

### `mailrelay.c`

- L265: `static void insert_callback(MailRelayRepoResult* result, void* user_data) {`
- L26: `static void recovery_callback(MailRelayRepoResult* result, void* user_data) {`
- L279: `static bool mailrelay_persist_message(const MailRelayMessage* msg,`

### `mailrelay_debounce.c`

- L102: `static MailRelayDebounceEntry* create_entry(const MailRelayMessage* msg,`
- L25: `static MailRelayDebounceEntry* create_entry(const MailRelayMessage* msg,`
- L458: `static void* mailrelay_debounce_thread(void* arg) {`

### `mailrelay_events.c`

- L116: `static void mailrelay_event_free_rate_limits(void) {`
- L135: `static void mailrelay_event_push_event_table(lua_State* L,`
- L188: `static char** mailrelay_event_read_string_array(lua_State* L,`
- L236: `static char* mailrelay_event_read_string(lua_State* L, const char* field_name) {`
- L248: `static int mailrelay_event_read_int(lua_State* L,`
- L267: `static bool mailrelay_event_read_params(lua_State* L,`
- L297: `static void mailrelay_event_free_string_array(char** arr, int count) {`
- L318: `static bool mailrelay_event_dispatch_request(lua_State* L, char* err, size_t err_cap) {`
- L406: `static bool mailrelay_event_run_handler(const char* source,`
- L470: `static const MailEventRule* mailrelay_event_find_rule(const char* event_key) {`
- L489: `static const char* mailrelay_event_resolve_source(const char* event_key,`
- L75: `static bool mailrelay_event_check_rate_limit(const char* event_key,`

### `mailrelay_message.c`

- L150: `static bool add_recipient(char* arr[], int* count, const char* addr) {`

### `mailrelay_otp.c`

- L142: `static void otp_insert_callback(MailRelayRepoResult* result, void* user_data) {`
- L168: `static int otp_resolve_digits(const MailRelayOtpSendRequest* req) {`
- L178: `static int otp_resolve_expiry(const MailRelayOtpSendRequest* req) {`
- L188: `static int otp_resolve_max_attempts(const MailRelayOtpSendRequest* req) {`
- L198: `static bool otp_purpose_valid(int purpose) {`
- L391: `static long long otp_json_int64(json_t* obj, const char* key) {`
- L405: `static void otp_json_copy_string(json_t* obj, const char* key, char* out, size_t out_size) {`
- L419: `static void otp_get_active_callback(MailRelayRepoResult* result, void* user_data) {`
- L463: `static void otp_write_callback(MailRelayRepoResult* result, void* user_data) {`
- L479: `static bool otp_parse_iso_time(const char* s, time_t* out) {`
- L513: `static void otp_set_err(char* err, size_t err_cap, const char* msg) {`

### `mailrelay_producer.c`

- L113: `static bool add_recipients_arrays(MailRelayMessage* msg,`
- L156: `static MailRelayStatus producer_check_runtime(char* err, size_t err_cap) {`
- L170: `static bool producer_try_idempotency(const char* idempotency_key,`
- L201: `static bool producer_resolve_from_reply(const char* req_from,`
- L232: `static MailRelayStatus producer_enqueue_message(MailRelayMessage* msg,`
- L273: `static MailRelayStatus mailrelay_send_template_default(const MailRelaySendTemplateRequest* req,`
- L391: `static MailRelayStatus mailrelay_send_direct_default(const MailRelaySendDirectRequest* req,`
- L48: `static void idempotency_callback(MailRelayRepoResult* result, void* user_data);`
- L51: `static const char* status_from_a63(int status_a63) {`
- L66: `static void idempotency_callback(MailRelayRepoResult* result, void* user_data) {`

### `mailrelay_queue.c`

- L12: `static int timespec_compare(const struct timespec* a, const struct timespec* b) {`
- L20: `static bool item_is_due(const MailRelayQueueItem* item, const struct timespec* now) {`
- L26: `static MailRelayQueueItem* find_first_due(MailRelayQueue* queue, const struct timespec* now) {`
- L37: `static MailRelayQueueItem* find_earliest(MailRelayQueue* queue) {`
- L49: `static void remove_item(MailRelayQueue* queue, MailRelayQueueItem* target) {`

### `mailrelay_render.c`

- L105: `static void render_mime_part(char** buf, size_t* len, size_t* cap, const char* boundary, const char* part_type, const char* body) {`
- L115: `static void render_rfc822_date(char* buf, size_t cap, time_t t) {`
- L16: `static void render_grow(char** buf, const size_t* len, size_t* cap, size_t extra) {`
- L30: `static void render_mem(char** buf, size_t* len, size_t* cap, const char* data, size_t data_len) {`
- L37: `static void render_str(char** buf, size_t* len, size_t* cap, const char* s) {`
- L41: `static void render_header(char** buf, size_t* len, size_t* cap, const char* name, const char* value) {`
- L57: `static void render_header_list(char** buf, size_t* len, size_t* cap, const char* name, char* const* arr, int count) {`
- L83: `static char* render_boundary(const char* message_id) {`

### `mailrelay_repository.c`

- L288: `static json_t* repo_params_new(void) {`
- L309: `static bool repo_add_string(json_t* root, const char* name, const char* value) {`
- L323: `static bool repo_add_int(json_t* root, const char* name, int value) {`
- L334: `static bool repo_add_int64(json_t* root, const char* name, long long value) {`
- L349: `static bool repo_execute_json(int query_ref, json_t* params,`
- L378: `static bool repo_execute_empty(int query_ref,`
- L40: `static bool mailrelay_repo_default_execute(int query_ref,`
- L51: `static const char* mailrelay_repo_resolve_database(void) {`
- L77: `static void mailrelay_repo_invoke_callback(mailrelay_repo_callback_fn callback,`
- L98: `static bool mailrelay_repo_default_execute(int query_ref,`

### `mailrelay_smtp.c`

- L102: `static size_t smtp_read_cb(void* ptr, size_t size, size_t nmemb, void* userp) {`
- L120: `static size_t smtp_write_cb(const void* ptr, size_t size, size_t nmemb, void* userp) {`
- L133: `static long parse_smtp_code(const char* buf, size_t len, char* text_out, size_t text_cap) {`
- L20: `static int resolve_tls_mode(const OutboundServer* server);`
- L21: `static bool build_request(const MailRelayMessage* msg,`
- L26: `static size_t smtp_read_cb(void* ptr, size_t size, size_t nmemb, void* userp);`
- L27: `static size_t smtp_write_cb(const void* ptr, size_t size, size_t nmemb, void* userp);`
- L28: `static long parse_smtp_code(const char* buf, size_t len, char* text_out, size_t text_cap);`
- L34: `static int resolve_tls_mode(const OutboundServer* server) {`
- L44: `static bool build_request(const MailRelayMessage* msg,`

### `mailrelay_template.c`

- L122: `static bool output_append_char(MailRelayTemplateOutput* out,`
- L130: `static void output_free(MailRelayTemplateOutput* out) {`
- L143: `static const char* resolve_builtin(const char* name,`
- L176: `static bool parse_macro(const char* template_text,`
- L240: `static bool is_valid_macro_name(const char* name, char* err, size_t err_cap) {`
- L28: `static bool is_macro_name_char(char c);`
- L29: `static bool output_append(MailRelayTemplateOutput* out,`
- L34: `static bool output_append_char(MailRelayTemplateOutput* out,`
- L38: `static void output_free(MailRelayTemplateOutput* out);`
- L39: `static const char* resolve_builtin(const char* name,`
- L46: `static bool parse_macro(const char* template_text,`
- L510: `static void preview_render_callback(MailRelayRepoResult* result, void* user_data) {`
- L55: `static bool is_valid_macro_name(const char* name, char* err, size_t err_cap);`
- L74: `static void preview_render_callback(MailRelayRepoResult* result, void* user_data);`
- L77: `static bool is_macro_name_char(char c) {`
- L83: `static bool output_append(MailRelayTemplateOutput* out,`

### `mailrelay_test_seams.c`

- L10: `static time_t default_seam_time(void) {`
- L14: `static void default_seam_message_id(char* buffer, size_t buflen, const char* app_name) {`

### `mailrelay_workers.c`

- L23: `static void* mailrelay_worker_thread(void* arg) {`

### `orchestrator.c`

- L103: `static void* orchestrator_thread_main(void* arg) {`
- L170: `static void orchestrator_set_shutdown_and_join(void) {`
- L293: `static char* orchestrator_build_params_json(const char* group_name,`
- L326: `static char* orchestrator_extract_code_from_result(const char* data_json) {`
- L544: `static const char* orchestrator_resolve_database(void) {`
- L637: `static void orchestrator_load_configured_blocking(void) {`
- L672: `static void* orchestrator_loader_main(void* arg) {`
- L82: `static void* orchestrator_thread_main(void* arg);`
- L83: `static void   orchestrator_set_shutdown_and_join(void);`
- L84: `static void   orchestrator_load_configured_blocking(void);`
- L85: `static void*  orchestrator_loader_main(void* arg);`
- L86: `static const char* orchestrator_resolve_database(void);`

### `scoreboard.c`

- L121: `static bool generate_unique_id(const Scoreboard* sb, char out[ID_LEN + 1]) {`
- L35: `static bool is_terminal_status(ScoreboardJobStatus status) {`
- L43: `static void entry_clear_owned(ScoreboardEntry* entry) {`
- L81: `static void timespec_clear(struct timespec* ts) {`
- L89: `static void timespec_now(struct timespec* ts) {`
- L99: `static bool entries_grow_if_needed(Scoreboard* sb) {`

### `scripting_api_llm.c`

- L35: `static DatabaseQueue* resolve_llm_db_queue(const char* db_name, char** err_out) {`
- L83: `static ChatEngineConfig* resolve_llm_engine(const char* model_name, const char* db_name) {`
- L96: `static char* build_llm_request_json(const char* prompt, int max_tokens, double temperature) {`

### `scripting_api_mail_notify.c`

- L107: `static void mail_parse_init(MailLuaParse* p) {`
- L113: `static bool parse_recipient_field(lua_State* L, int table_idx, const char* field,`
- L210: `static bool parse_params_table(lua_State* L, int table_idx, MailRelayTemplateParams* params,`
- L242: `static bool parse_optional_string_field(lua_State* L, int table_idx, const char* field,`
- L284: `static bool parse_mail_message(lua_State* L, MailLuaParse* p) {`
- L439: `static void status_to_mail_error(MailRelayStatus status, const char* producer_err,`
- L57: `static void free_mail_parse(MailLuaParse* p) {`

### `scripting_api_scoreboard.c`

- L230: `static int bytecode_dump_writer(lua_State* L, const void* p, size_t sz, void* ud) {`

### `scripting_handle.c`

- L44: `static int H_Handle_gc(lua_State* L) {`

### `script_registry.c`

- L69: `static bool registry_grow_if_needed(ScriptRegistry* reg) {`

### `source_cache.c`

- L79: `static bool source_cache_grow_if_needed(SourceCache* cache) {`

### `worker_pool.c`

- L330: `static bool scripting_worker_should_exit(ScriptingWorkerPool* pool) {`
- L344: `static void* scripting_worker_thread(void* arg) {`
- L379: `static void scripting_worker_process_one(ScriptingWorkerPool* pool,`
- L72: `static void* scripting_worker_thread(void* arg);`
- L73: `static void scripting_worker_process_one(ScriptingWorkerPool* pool,`
- L75: `static bool scripting_worker_should_exit(ScriptingWorkerPool* pool);`
- L76: `static void scripting_signal_waiter_if_present(const char* job_id);`
- L96: `static void scripting_signal_waiter_if_present(const char* job_id) {`

## Collision detail

Names declared as both prototype and definition in the same file (NOT real collisions)
