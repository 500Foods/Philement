   #ifndef NITRO_UTILS_H
   #define NITRO_UTILS_H

   #include <stddef.h>

   #define NITRO_ID_CHARS "BCDFGHKPRST"
   #define NITRO_ID_LEN 5

   void nitro_generate_id(char *buf, size_t len);
   void nitro_log(const char *level, const char *fmt, ...);

   #endif // NITRO_UTILS_H
