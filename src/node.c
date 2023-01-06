#include "./mq/mq.h"
#include "./worker/worker.h"
#include "./timer/timer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>


int id;
MessageQueue mq;
Timer timer;

#define WORKERS_AMOUNT 2
int created_workers = 0;
Worker child_workers[WORKERS_AMOUNT];


void delete_child_worker(int worker_id)
{
    int i;
    int found = 0;

    for(i = 0; i < created_workers; i++)
    {
        if(child_workers[i].id == worker_id)
        {
            // worker_delete(&child_workers[i]);
            found = 1;
            break;
        }
    }
            
    for(int j = i; j < created_workers - 1; j++)
    {
        memcpy(&child_workers[j], &child_workers[j + 1], sizeof(child_workers[j]));
    }

    if(found) created_workers--;
    if(created_workers < 0) created_workers = 0;
}


void init_worker(int worker_id, int callback_id)
{
    MessageQueue callback;
    int worker_mq_id;

    // log_debug("[Worker #%d] Init worker mq...", worker_id);
    mq_create(&mq);
    id = worker_id;
    // log_debug("[Worker #%d] Worker mq id: %d", worker_id, mq_id(&mq));

    // log_debug("[Worker #%d] Connect to callback mq(%d)", worker_id, callback_id);
    mq_connect(&callback, callback_id);

    // log_debug("[Worker #%d] Send data to callback mq...", worker_id);
    CreateResult result = {
        .worker_id = id,
        .message_type = CREATE,
        .result_type = RESULT_OK,
        .worker_pid = getpid(),
        .worker_mq_id = mq_id(&mq)
    };

    mq_send(&callback, &result, sizeof(result));
    // log_debug("[Worker #%d] Sended!", worker_id);

    timer_init(&timer);
}


void handle_ping_message(PingMessage* message)
{
    MessageQueue callback;
    WorkerResult result = {
        .message_id = message->message_id,
        .worker_id = id,
        .message_type = message->type,
        .result_type = RESULT_OK
    };

    mq_connect(&callback, message->callback_id);
    mq_send(&callback, &result, sizeof(result));
    
    mq_close(&callback);
}


void handle_create_message(CreateMessage* message)
{
    MessageQueue callback;
    CreateResult result = {
        .message_id = message->message_id,
        .worker_id = id,
        .message_type = message->type,
        .result_type = RESULT_OK,
        .worker_pid = -1,
        .worker_mq_id = -1
    };

    mq_connect(&callback, message->callback_id);

    if(created_workers >= WORKERS_AMOUNT)
    {
        mq_close(&callback);
        exit(2);
    }

    result.worker_pid = worker_create(&child_workers[created_workers], "worker", message->created_worker_id);
    result.worker_mq_id = mq_id(&child_workers[created_workers].mq);
    created_workers++;

    mq_send(&callback, &result, sizeof(result));
    mq_close(&callback);
}


void handle_delete_message(DeleteMessage* message)
{
    DeleteMessage local_message;
    MessageQueue local_callback, callback;
    WorkerResult result = {
        .message_id = message->message_id,
        .worker_id = id,
        .message_type = message->type,
        .result_type = RESULT_OK
    };

    mq_create(&local_callback);
    mq_connect(&callback, message->callback_id);
    
    memcpy(&local_message, message, sizeof(local_message));
    local_message.callback_id = mq_id(&local_callback);

    // delete child workers
    for(int i = 0; i < created_workers; i++)
    {
        local_message.receiver_id = child_workers[i].id;
        mq_send(&child_workers[i].mq, &local_message, sizeof(local_message));
    }

    // delete current worker
    mq_send(&callback, &result, sizeof(result));

    mq_close(&local_callback);
    mq_close(&callback);
    exit(0);
}


void handle_timer_start_message(StartTimerMessage* message)
{
    MessageQueue callback;
    WorkerResult result = {
        .message_id = message->message_id,
        .worker_id = id,
        .message_type = message->type,
        .result_type = RESULT_OK
    };

    mq_connect(&callback, message->callback_id);
    
    timer_start(&timer);

    mq_send(&callback, &result, sizeof(result));
    mq_close(&callback);
}


void handle_timer_stop_message(StopTimerMessage* message)
{
    MessageQueue callback;
    WorkerResult result = {
        .message_id = message->message_id,
        .worker_id = id,
        .message_type = message->type,
        .result_type = RESULT_OK
    };

    mq_connect(&callback, message->callback_id);

    timer_stop(&timer);

    mq_send(&callback, &result, sizeof(result));
    mq_close(&callback);
}


void handle_timer_time_message(GetTimeMessage* message)
{
    MessageQueue callback;
    TimeResult result = {
        .message_id = message->message_id,
        .worker_id = id,
        .message_type = message->type,
        .result_type = RESULT_OK,
        .time = 0
    };

    mq_connect(&callback, message->callback_id);

    result.time = timer_time(&timer);

    mq_send(&callback, &result, sizeof(result));
    mq_close(&callback);
}


void delegate_message(WorkerMessage* message, size_t message_size)
{
    MessageQueue callback, local_callback;
    
    mq_connect(&callback, message->callback_id);
    mq_create(&local_callback);
    mq_set_timeout(&local_callback, 100);
    
    message->callback_id = mq_id(&local_callback);

    // send message to child workers
    for(int i = 0; i < created_workers; i++)
        mq_send(&child_workers[i].mq, message, message_size);

    int res = 0;

    // wait first successfull result form child nodes
    for(int i = 0; i < created_workers; i++)
    {
        WorkerResult* recv_data;
        size_t recv_data_size;

        mq_recv(&local_callback, (void **) &recv_data, &recv_data_size);
        
        if(recv_data == NULL)
            continue;

        if(recv_data->result_type != RESULT_UNAVAILABLE)
        {
            mq_send(&callback, recv_data, recv_data_size);
            mq_close(&local_callback);
            free(recv_data);
            res = 1;
            break;
        }

        free(recv_data);
    }

    // remove from buffer if command 'DELETE'
    if(message->type == DELETE)
    {
        delete_child_worker(message->receiver_id);
    }

    if(res) return;


    // if all child nodes' results are failed
    WorkerResult result = {
        .message_id = message->message_id,
        .worker_id = message->receiver_id,
        .message_type = message->type,
        .result_type = RESULT_UNAVAILABLE
    };

    mq_send(&callback, &result, sizeof(result));
}


int main(int argc, char* argv[])
{
    WorkerMessage* message;
    size_t message_size;

    init_worker(atoi(argv[1]), atoi(argv[2]));
    
    while(1)
    {
        mq_recv(&mq, (void **) &message, &message_size);

        if(message->receiver_id != id)
        {
            delegate_message(message, message_size);
            free(message);
            continue;
        }

        switch(message->type)
        {
            case PING:
                handle_ping_message((PingMessage *) message);
            break;

            case CREATE:
                handle_create_message((CreateMessage *) message);
            break;

            case DELETE:
                handle_delete_message((DeleteMessage *) message);
            break;

            case TIMER_START:
                handle_timer_start_message((StartTimerMessage *) message);
            break;

            case TIMER_STOP:
                handle_timer_stop_message((StopTimerMessage *) message);
            break;

            case TIMER_TIME:
                handle_timer_time_message((GetTimeMessage *) message);
            break;
        }

        free(message);
    }

    return 0;
}