#ifndef __MESSAGE_QUERY_H__
#define __MESSAGE_QUERY_H__


#include <stddef.h>


#define WORKER_OK 0
#define WORKER_ERROR_TIMEOUT 1
#define WORKER_ERROR_INTERNAL 2


typedef struct {
    int id;
    void* socket;
    void* context;
} MessageQueue;


void mq_create(MessageQueue* mq);
void mq_connect(MessageQueue* mq, int mq_id);
int mq_id(MessageQueue* mq);
void mq_send(MessageQueue* mq, void* data, size_t size);
void mq_recv(MessageQueue* mq, void** buff, size_t* size);
void mq_set_timeout(MessageQueue* mq, int timeout);
void mq_close(MessageQueue* mq);


#endif