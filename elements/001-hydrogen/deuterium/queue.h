#ifndef QUEUE_H
#define QUEUE_H

#include <jansson.h>

typedef struct queue_t queue_t;

queue_t* queue_init(void);
void queue_push(queue_t* queue, json_t* item);
json_t* queue_pop(queue_t* queue);
void queue_free(queue_t* queue);

#endif
