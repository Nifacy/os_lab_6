#include "worker.h"
#include <zmq.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


void __send_unavailable_error(WorkerMessage* message, MessageQueue* callback)
{
    WorkerResult result = {
        .message_id = message->message_id,
        .worker_id = message->receiver_id,
        .message_type = message->type,
        .result_type = RESULT_UNAVAILABLE
    };

    mq_send(callback, &result, sizeof(result));
}


pid_t worker_create(Worker* worker, char* execute_path, int worker_id)
{
    MessageQueue callback;
    pid_t pid;

    mq_create(&callback);
    // log_debug("[WorkerCreate] Client: callback mq(%d)", mq_id(&callback));
    pid = fork();

    if(pid == 0)
    {
        char callback_id_str[256];
        char worker_id_str[256];

        // log_debug("Prepair data for worker...");
        sprintf(callback_id_str, "%d", mq_id(&callback));
        sprintf(worker_id_str, "%d", worker_id);

        char* args[] = {execute_path, worker_id_str, callback_id_str, NULL};
        execv(execute_path, args);

        exit(2);
    }

    if(pid > 0)
    {
        CreateResult* recv_data;
        size_t recv_data_size;
        pid_t created_worker_pid;
        
        // log_debug("[Client] Waiting worker init...");
        mq_recv(&callback, (void **) &recv_data, &recv_data_size);
        assert(recv_data != NULL);

        // log_debug("[Client] Worker queue id: %d", recv_data->worker_mq_id);
        mq_connect(&worker->mq, recv_data->worker_mq_id);
        worker->id = worker_id;
        worker->pid = recv_data->worker_pid;
        
        free(recv_data);
        return worker->pid;
    }

    if(pid < 0)
    {
        exit(2);
    }
}


void worker_send_message(Worker* worker, WorkerMessage* message, size_t message_size)
{
    // debug("[WorkerLib] message size: %ld\n", message_size);

    MessageQueue local_callback, callback;
    WorkerMessage* message_copy;
    void* recv_data;
    size_t recv_data_size;


    mq_connect(&callback, message->callback_id);

    mq_create(&local_callback);
    mq_set_timeout(&local_callback, 1000);

    message_copy = (WorkerMessage *) malloc(message_size);
    memcpy(message_copy, message, message_size);
    message_copy->callback_id = mq_id(&local_callback);

    mq_send(&worker->mq, message_copy, message_size);
    mq_recv(&local_callback, &recv_data, &recv_data_size);

    if(recv_data == NULL)
        __send_unavailable_error(message, &callback);
    else
    {
        mq_send(&callback, recv_data, recv_data_size);
        free(recv_data);
    }

    free(message_copy);
    mq_close(&local_callback);
    mq_close(&callback);
}


void worker_delete(Worker* worker)
{
    mq_close(&worker->mq);
}