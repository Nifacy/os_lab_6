#include "mq.h"
#include <zmq.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <error.h>


int listeners_counter = 0;
static char* __mq_server_address = "0.0.0.0";
static int __max_endpoint_size = 256;


#define HANDLE_ERROR \
    exit(errno)


int __auto_bind(void* socket)
{
    char endpoint[__max_endpoint_size];
    char addr[__max_endpoint_size];
    size_t endpoint_size = __max_endpoint_size;
    int port;
    
    sprintf(endpoint, "tcp://%s:*", __mq_server_address);
    
    if(zmq_bind(socket, endpoint) != 0)
        return -1;

    zmq_getsockopt(socket, ZMQ_LAST_ENDPOINT, &endpoint, &endpoint_size);
    sscanf(endpoint, "tcp://0.0.0.0:%d", &port); // TODO: добавить зависимость от __mq_server_address

    return port;
}


void mq_create(MessageQueue* mq)
{
    mq->context = zmq_ctx_new();
    mq->socket = zmq_socket(mq->context, ZMQ_PULL);
    zmq_setsockopt(mq->socket, ZMQ_IDENTITY, &listeners_counter, sizeof(listeners_counter));
    listeners_counter++;

    mq->id = __auto_bind(mq->socket);

    if(mq->id == -1) { HANDLE_ERROR; }
}


void mq_connect(MessageQueue* mq, int mq_id)
{
    char endpoint[__max_endpoint_size];
    
    mq->context = zmq_ctx_new();
    mq->socket = zmq_socket(mq->context, ZMQ_PUSH);
    mq->id = mq_id;

    sprintf(endpoint, "tcp://%s:%d", __mq_server_address, mq_id);
    int res = zmq_connect(mq->socket, endpoint); assert(res == 0);
}


int mq_id(MessageQueue* mq)
{
    return mq->id;
}


void mq_send(MessageQueue* mq, void* data, size_t size)
{
    zmq_msg_t message;
    int res;
    
    if(zmq_msg_init(&message) != 0) { HANDLE_ERROR; }
    if(zmq_msg_init_size(&message, size) != 0) { HANDLE_ERROR; }
    
    memcpy(zmq_msg_data(&message), data, size);

    if(zmq_msg_send(&message, mq->socket, 0) == -1) { HANDLE_ERROR; }
    if(zmq_msg_close(&message) != 0) { HANDLE_ERROR; }
}


// TODO: добавить обратное значение + код для timeout
// TODO: добавить NULL поля
void mq_recv(MessageQueue* mq, void** buff, size_t* size)
{
    zmq_msg_t message;
    int res;

    if(zmq_msg_init(&message) != 0) { HANDLE_ERROR; }
    if(zmq_msg_recv(&message, mq->socket, 0) == -1)
    {
        if(errno == EAGAIN)
        {
            *buff = NULL;
            *size = -1;
            return;
        } else { HANDLE_ERROR; }
    }

    *size = zmq_msg_size(&message);
    *buff = malloc(*size);
    memcpy(*buff, zmq_msg_data(&message), *size);

    if(zmq_msg_close(&message) != 0) { HANDLE_ERROR; }
}


void mq_set_timeout(MessageQueue* mq, int timeout)
{
    int res;
    if(zmq_setsockopt(mq->socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout))) { HANDLE_ERROR; }
    if(zmq_setsockopt(mq->socket, ZMQ_SNDTIMEO, &timeout, sizeof(timeout))) { HANDLE_ERROR; }
}


void mq_close(MessageQueue* mq)
{
    zmq_close(mq->socket);
    zmq_ctx_destroy(mq->context);
}