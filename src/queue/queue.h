#ifndef __QUEUE_H__
#define __QUEUE_H__


#include <stdlib.h>
#include <stdbool.h>


typedef struct __QElement {
    void* value;
    size_t value_size;
    struct __QElement* next;
} QElement;


typedef struct {
    QElement* head;
} Queue;



void queue_init(Queue* q);
void queue_push(Queue* q, void* value, size_t value_size);
void queue_pop(Queue* q, void** value, size_t* value_size);
bool queue_empty(Queue* q);
void queue_delete(Queue* q);


#endif