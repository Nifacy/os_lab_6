#ifndef __WORKER_H__
#define __WORKER_H__


#include "../mq/mq.h"
#include "../queue/queue.h"
#include <stdbool.h>
#include <unistd.h>


/* Worker types */


typedef struct {
    int id;
    MessageQueue mq;
    pid_t pid;
} Worker;


typedef enum {
    CREATE,
    PING,
    DELETE,
    TIMER_START,
    TIMER_STOP,
    TIMER_TIME
} MessageType;


typedef enum {
    RESULT_OK,
    RESULT_UNAVAILABLE,
    RESULT_PING,
    NOT_FOUND,
    ALREADY_EXISTS
} ResultType;


/* Incoming messages */


typedef struct {
    int message_id;
    int receiver_id;
    int callback_id;
    MessageType type;
} WorkerMessage;


typedef struct {
    int message_id;
    int receiver_id;
    int callback_id;
    MessageType type;
    int created_worker_id;
} CreateMessage;


typedef struct {
    int message_id;
    int receiver_id;
    int callback_id;
    MessageType type;
} PingMessage;


typedef struct {
    int message_id;
    int receiver_id;
    int callback_id;
    MessageType type;
} DeleteMessage;


typedef struct {
    int message_id;
    int receiver_id;
    int callback_id;
    MessageType type;
} StartTimerMessage;


typedef struct {
    int message_id;
    int receiver_id;
    int callback_id;
    MessageType type;
} StopTimerMessage;


typedef struct {
    int message_id;
    int receiver_id;
    int callback_id;
    MessageType type;
} GetTimeMessage;


/* Result messages */


typedef struct {
    int message_id;
    int worker_id;
    MessageType message_type;
    ResultType result_type;
} WorkerResult;


typedef struct {
    int message_id;
    int worker_id;
    MessageType message_type;
    ResultType result_type;
    pid_t worker_pid;
    int worker_mq_id;
} CreateResult;


typedef struct {
    int message_id;
    int worker_id;
    MessageType message_type;
    ResultType result_type;
    unsigned long time;
} TimeResult;


typedef struct {
    int message_id;
    int worker_id;
    MessageType message_type;
    ResultType result_type;
    Queue unavailable_nodes;
} PingResult;


/* Methods */


pid_t worker_create(Worker* worker, char* execute_path, int worker_id);
void worker_send_message(Worker* worker, WorkerMessage* message, size_t message_size);
void worker_delete(Worker* worker);

#endif