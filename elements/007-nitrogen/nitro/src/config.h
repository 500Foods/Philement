#ifndef NITRO_CONFIG_H
   #define NITRO_CONFIG_H

   #include <jansson.h>

   #define NITRO_DEFAULT_PORT 27001

   typedef struct {
       char *id;
       char *name;
       int port;
   } nitro_config_t;

   nitro_config_t *nitro_config_load(const char *app_name);
   void nitro_config_save(const char *app_name, nitro_config_t *config);
   void nitro_config_free(nitro_config_t *config);

   #endif // NITRO_CONFIG_H
