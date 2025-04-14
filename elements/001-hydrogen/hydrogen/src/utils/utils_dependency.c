/*
 * Simplified library dependency checking utilities
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <dlfcn.h>
 #include <signal.h>
 #include <stdint.h>
 #include "utils_dependency.h"
 #include "../logging/logging.h"
 
 extern const char *jansson_version_str(void);
 
 typedef struct {
     const char *name;
     const char **paths;
     const char **version_funcs;
     const char *expected;
     bool is_core;
     bool required;
 } LibConfig;
 
 static const char *pthread_paths[] = {"libpthread.so", "/lib64/libpthread.so.0", "/usr/lib/libpthread.so", NULL};
 static const char *jansson_paths[] = {"libjansson.so", "/lib64/libjansson.so.4", "/usr/lib/libjansson.so", "/usr/lib/x86_64-linux-gnu/libjansson.so.4", NULL};
 static const char *microhttpd_paths[] = {"libmicrohttpd.so", "/lib64/libmicrohttpd.so.12", "/usr/lib/libmicrohttpd.so", "/usr/lib/x86_64-linux-gnu/libmicrohttpd.so.12", NULL};
 static const char *libm_paths[] = {"libm.so", "/lib64/libm.so.6", "/usr/lib/libm.so", NULL};
 static const char *libwebsockets_paths[] = {"libwebsockets.so", "/lib64/libwebsockets.so.19", "/usr/lib/libwebsockets.so", "/usr/lib/x86_64-linux-gnu/libwebsockets.so.19", NULL};
 static const char *openssl_paths[] = {"libssl.so", "/lib64/libssl.so.3", "/usr/lib/libssl.so", "/usr/lib/x86_64-linux-gnu/libssl.so.3", NULL};
 static const char *brotli_paths[] = {"libbrotlidec.so", "/lib64/libbrotlidec.so.1", "/usr/lib/libbrotlidec.so", "/usr/lib/x86_64-linux-gnu/libbrotlidec.so.1", NULL};
 static const char *libtar_paths[] = {"libtar.so", "/usr/lib64/libtar.so.1", "/usr/lib64/libtar.so", "/lib64/libtar.so.1", "/usr/lib/libtar.so", "/usr/lib/x86_64-linux-gnu/libtar.so", NULL};
 
 static const char *jansson_funcs[] = {"jansson_version_str", NULL};
 static const char *microhttpd_funcs[] = {"MHD_get_version", NULL};
 static const char *libwebsockets_funcs[] = {"lws_get_library_version", NULL};
 static const char *openssl_funcs[] = {"OpenSSL_version", "SSLeay_version", NULL};
 static const char *brotli_funcs[] = {"BrotliDecoderVersion", NULL};
 static const char *libtar_funcs[] = {"libtar_version", NULL};
 
 static const LibConfig lib_configs[] = {
     {"pthreads", pthread_paths, NULL, "1.0", true, true},
     {"jansson", jansson_paths, jansson_funcs, "2.13.1", false, true},
     {"microhttpd", microhttpd_paths, microhttpd_funcs, "1.0.1", false, false},
     {"libm", libm_paths, NULL, "2.0", true, true},
     {"libwebsockets", libwebsockets_paths, libwebsockets_funcs, "4.3.3", false, false},
     {"OpenSSL", openssl_paths, openssl_funcs, "3.2.4", false, false},
     {"libbrotlidec", brotli_paths, brotli_funcs, "1.1.0", false, false},
     {"libtar", libtar_paths, libtar_funcs, "1.2.20", false, false}
 };
 
 static const char *get_status_string(LibraryStatus status) {
     switch (status) {
         case LIB_STATUS_GOOD: return "Good";
         case LIB_STATUS_WARNING: return "Less Good";
         case LIB_STATUS_CRITICAL: return "Trouble awaits";
         default: return "Unknown";
     }
 }
 
 static LibraryStatus determine_status(const char *expected, const char *found, bool required) {
     if (!found || !strcmp(found, "None")) return required ? LIB_STATUS_CRITICAL : LIB_STATUS_WARNING;
     if (!strcmp(found, "NoVersionFound")) return required ? LIB_STATUS_WARNING : LIB_STATUS_GOOD;
     if (!expected || strstr(found, expected) != NULL) return LIB_STATUS_GOOD;
     return LIB_STATUS_WARNING;
 }
 
 static void log_status(const char *name, const char *expected, const char *found, const char *method, LibraryStatus status) {
     int level = (status == LIB_STATUS_GOOD) ? LOG_LEVEL_STATE :
                 (status == LIB_STATUS_WARNING) ? LOG_LEVEL_ALERT :
                 (status == LIB_STATUS_CRITICAL) ? LOG_LEVEL_FATAL : LOG_LEVEL_ERROR;
     log_this("DepCheck", "%s Expecting: %s Found: %s (%s) Status: %s",
              level, name, expected ? expected : "(default)", found ? found : "None",
              method, get_status_string(status));
 }
 
 static const char *get_version(const LibConfig *config, char *buffer, size_t size, const char **method) {
     if (!config || !buffer || !size || !method) return "None";
     buffer[0] = '\0';
     *method = "N/A";
 
     if (config->is_core) {
         *method = "COR";
         return config->expected;
     }
 
     for (const char **path = config->paths; *path; path++) {
         void *handle = dlopen(*path, RTLD_LAZY);
         if (!handle) {
//             log_this("DepCheck", "Failed to open %s at %s: %s", LOG_LEVEL_DEBUG, config->name, *path, dlerror());
             continue;
         }
 
         const char *version = "NoVersionFound";
         *method = "DLO";
 
         if (config->version_funcs) {
             for (const char **func_name = config->version_funcs; *func_name; func_name++) {
                 if (strcmp(config->name, "jansson") == 0 && strcmp(*func_name, "jansson_version_str") == 0) {
                     const char *temp = jansson_version_str();
                     if (temp) {
                         size_t len = strnlen(temp, size - 1);
                         if (len > 0 && len < size - 1) {
                             strncpy(buffer, temp, len);
                             buffer[len] = '\0';
                             version = buffer;
                             *method = "API";
//                             log_this("DepCheck", "%s: Found version %s via direct call", LOG_LEVEL_DEBUG, config->name, version);
                             dlclose(handle);
                             return version;
                         }
                     }
                     continue;
                 }
 
                 dlerror();
                 void *func_ptr = dlsym(handle, *func_name);
                 const char *err = dlerror();
                 if (err || !func_ptr) {
//                     log_this("DepCheck", "%s: dlsym(%s) failed: %s", LOG_LEVEL_DEBUG, config->name, *func_name, err ? err : "NULL");
                     continue;
                 }
 
                 if (strcmp(config->name, "libtar") == 0 && strcmp(*func_name, "libtar_version") == 0) {
                     const char *version_str = (const char *)func_ptr;
//                     log_this("DepCheck", "%s: Raw version_str at %p", LOG_LEVEL_DEBUG, config->name, func_ptr);
                     if (version_str) {
                         volatile char probe = *version_str; // Force read to catch segfault
                         if (probe != '\0') {
                             size_t len = strnlen(version_str, size - 1);
//                             log_this("DepCheck", "%s: Version string length: %zu", LOG_LEVEL_DEBUG, config->name, len);
                             if (len > 0 && len < size - 1) {
                                 strncpy(buffer, version_str, len);
                                 buffer[len] = '\0';
                                 version = buffer;
                                 *method = "SYM";
//                                 log_this("DepCheck", "%s: Found version %s via %s (data symbol)", LOG_LEVEL_DEBUG, config->name, version, *func_name);
                                 dlclose(handle);
                                 return version;
                             }
                         }
                     }
                     version = "NoVersionFound";
//                     log_this("DepCheck", "%s: %s is empty or inaccessible", LOG_LEVEL_DEBUG, config->name, *func_name);
                     dlclose(handle);
                     return version;
                 }
 
                 if (strcmp(config->name, "libbrotlidec") == 0 && strcmp(*func_name, "BrotliDecoderVersion") == 0) {
                     uint32_t (*brotli_func)(void);
                     *(void **)(&brotli_func) = func_ptr;
                     uint32_t ver = brotli_func();
                     snprintf(buffer, size, "%u.%u.%u",
                              ver >> 24, (ver >> 12) & 0xFFF, ver & 0xFFF);
                     version = buffer;
                     *method = "SYM";
//                     log_this("DepCheck", "%s: Found version %s via %s", LOG_LEVEL_DEBUG, config->name, version, *func_name);
                     dlclose(handle);
                     return version;
                 }
 
                 union {
                     const char *(*void_func)(void);
                     const char *(*int_func)(int);
                     void *ptr;
                 } func;
                 func.ptr = func_ptr;
 
                 const char *temp = NULL;
                 if (strcmp(config->name, "OpenSSL") == 0) {
                     temp = func.int_func(0);
                 } else {
                     temp = func.void_func();
                 }
                 if (!temp) {
//                     log_this("DepCheck", "%s: Version function %s returned NULL", LOG_LEVEL_DEBUG, config->name, *func_name);
                     continue;
                 }
 
                 size_t len = strnlen(temp, size - 1);
                 if (len > 0 && len < size - 1) {
                     bool valid = true;
                     volatile char probe;
                     for (size_t i = 0; i < len; i++) {
                         probe = temp[i];
                         if (probe < 32 || probe > 126) {
                             valid = false;
                             break;
                         }
                     }
                     if (valid) {
                         strncpy(buffer, temp, len);
                         buffer[len] = '\0';
                         version = buffer;
                         *method = "SYM";
//                         log_this("DepCheck", "%s: Found version %s via %s", LOG_LEVEL_DEBUG, config->name, version, *func_name);
                         dlclose(handle);
                         return version;
                     }
                 }
                 version = "NoVersionFound";
             }
         }
         dlclose(handle);
         return version;
     }
     if (strcmp(config->name, "libtar") == 0) {
//         log_this("DepCheck", "libtar not found; installed at /usr/lib64/libtar.so.1? Run 'ldd ./hydrogen' and 'sudo ldconfig'", LOG_LEVEL_DEBUG);
     }
     return "None";
 }
 
 bool is_library_available(const char *lib_name) {
     if (!lib_name) return false;
     void *handle = dlopen(lib_name, RTLD_LAZY);
     if (handle) {
         dlclose(handle);
         return true;
     }
     return false;
 }
 
 void check_library_dependency(const char *name, const char *expected_with_v, bool is_required) {
     if (!name) return;
     const char *expected = expected_with_v && expected_with_v[0] == 'v' ? expected_with_v + 1 : expected_with_v;
 
     for (size_t i = 0; i < sizeof(lib_configs) / sizeof(lib_configs[0]); i++) {
         if (strcmp(lib_configs[i].name, name)) continue;
         char buffer[256];
         const char *method;
         const char *found = get_version(&lib_configs[i], buffer, sizeof(buffer), &method);
         LibraryStatus status = lib_configs[i].is_core ? LIB_STATUS_GOOD :
                                determine_status(expected, found, is_required);
         log_status(name, expected, found, method, status);
         return;
     }
     log_status(name, expected, "None", "N/A", is_required ? LIB_STATUS_CRITICAL : LIB_STATUS_WARNING);
 }
 
 int check_library_dependencies(const AppConfig *config) {
     log_this("DepCheck", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
     log_this("DepCheck", "DEPENDENCY CHECK", LOG_LEVEL_STATE);
     int critical_count = 0;
 
     bool web = config ? (config->webserver.enable_ipv4 || config->webserver.enable_ipv6) : false;
     bool ws = config ? config->websocket.enabled : false;
     bool sec = config ? (web || ws) : false;
     bool print = config ? config->print.enabled : false;
 
     for (size_t i = 0; i < sizeof(lib_configs) / sizeof(lib_configs[0]); i++) {
         LibConfig lib = lib_configs[i];
         lib.required = lib.is_core || (strcmp(lib.name, "microhttpd") == 0 && web) ||
                        (strcmp(lib.name, "libbrotlidec") == 0 && web) ||
                        (strcmp(lib.name, "libwebsockets") == 0 && ws) ||
                        (strcmp(lib.name, "OpenSSL") == 0 && sec) ||
                        (strcmp(lib.name, "libtar") == 0 && print);
 
         char buffer[256];
         const char *method;
         const char *found = get_version(&lib, buffer, sizeof(buffer), &method);
         LibraryStatus status = lib.is_core ? LIB_STATUS_GOOD : determine_status(lib.expected, found, lib.required);
         log_status(lib.name, lib.expected, found, method, status);
         if (status == LIB_STATUS_CRITICAL && lib.required) critical_count++;
     }
     log_this("DepCheck", "Completed dependency check, critical issues: %d", LOG_LEVEL_STATE, critical_count);
     return critical_count;
 }
 
 LibraryHandle *load_library(const char *lib_name, int dlopen_flags) {
     if (!lib_name) return NULL;
     LibraryHandle *handle = calloc(1, sizeof(LibraryHandle));
     if (!handle) return NULL;
 
     handle->name = strdup(lib_name);
     handle->handle = dlopen(lib_name, dlopen_flags);
     handle->is_loaded = handle->handle != NULL;
     handle->status = handle->is_loaded ? LIB_STATUS_GOOD : LIB_STATUS_WARNING;
     handle->version = handle->is_loaded ? "Unknown" : "None";
     return handle;
 }
 
 bool unload_library(LibraryHandle *handle) {
     if (!handle) return false;
     bool success = true;
     if (handle->is_loaded && dlclose(handle->handle) != 0) success = false;
     free((void*)handle->name);
     free(handle);
     return success;
 }
 
 void *get_library_function(LibraryHandle *handle, const char *function_name) {
     if (!handle || !handle->is_loaded || !function_name) return NULL;
     dlerror();
     void *func = dlsym(handle->handle, function_name);
     return dlerror() ? NULL : func;
 }