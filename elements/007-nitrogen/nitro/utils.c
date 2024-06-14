   #include "utils.h"
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <time.h>
   #include <stdarg.h>

   void nitro_generate_id(char *buf, size_t len) {
       if (len < NITRO_ID_LEN + 1) {
           nitro_log("ERROR", "Buffer too small for ID");
           return;
       }

       static int seeded = 0;
       if (!seeded) {
           srand(time(NULL));
           seeded = 1;
       }

       size_t nitro_id_chars_len = strlen(NITRO_ID_CHARS);
       for (int i = 0; i < NITRO_ID_LEN; i++) {
           buf[i] = NITRO_ID_CHARS[rand() % nitro_id_chars_len];
       }
       buf[NITRO_ID_LEN] = '\0';
   }

   void nitro_log(const char *level, const char *fmt, ...) {
       va_list args;
       va_start(args, fmt);
       fprintf(stderr, "[%s] ", level);
       vfprintf(stderr, fmt, args);
       fprintf(stderr, "\n");
       va_end(args);
   }
