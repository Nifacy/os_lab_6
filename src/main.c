#include "./worker_tree/worker_tree.h"
#include "./worker/worker.h"
#include "./queue/queue.h"
#include "user_commands.h"
#include "./mq/mq.h"
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>


MessageQueue callback;
Worker root_worker;
WorkerTree tree;
int command_counter = 1;


WorkerResult base_result(UserCommand cmd, MessageType message_type, ResultType result_type)
{
    WorkerResult result = {
        .message_id = cmd.command_id,
        .worker_id = cmd.worker_id,
        .message_type = message_type,
        .result_type = result_type
    };

    return result;
}


void send_to_output(WorkerResult* result, size_t result_size)
{
    MessageQueue output;
    mq_connect(&output, mq_id(&callback));
    mq_send(&output, result, result_size);
    mq_close(&output);
}


void send_not_found_result(UserCommand cmd, MessageType type)
{
    WorkerResult result = base_result(cmd, type, NOT_FOUND);
    send_to_output(&result, sizeof(result));
}


void send_already_exists_result(UserCommand cmd, MessageType type)
{
    WorkerResult result = base_result(cmd, type, ALREADY_EXISTS);
    send_to_output(&result, sizeof(result));
}


int get_command_id()
{
    return command_counter++;
}


UserCommand scan_user_command()
{
    char command_type[256];
    int command_id = get_command_id();
    UserCommand command;
    
    printf("(%d) > ", command_id);
    scanf("%s", command_type);

    if(!strcmp(command_type, "create"))
    {
        command.type = USER_CREATE;
        scanf("%d", &command.worker_id);
    }

    if(!strcmp(command_type, "remove"))
    {
        command.type = USER_REMOVE;
        scanf("%d", &command.worker_id);
    }

    if(!strcmp(command_type, "ping"))
    {
        command.type = USER_PING;
        command.worker_id = -1;
    }

    if(!strcmp(command_type, "exec"))
    {
        scanf("%d", &command.worker_id);
        char action[256];
        scanf("%s", action);

        if(!strcmp(action, "start")) command.type = USER_EXEC_START;
        if(!strcmp(action, "stop")) command.type = USER_EXEC_STOP;
        if(!strcmp(action, "time")) command.type = USER_EXEC_TIME;
    }

    if(!strcmp(command_type, "close"))
    {
        command.type = USER_CLOSE;
        command.worker_id = -1;
    }

    command.command_id = command_id;
    return command;
}


void result_type_repr(char* str, ResultType type)
{
    switch(type)
    {
        case RESULT_OK: strcpy(str, " : ok"); break;
        case RESULT_UNAVAILABLE: strcpy(str, " : error : node is unavailable"); break;
        case NOT_FOUND: strcpy(str, " : error : not found"); break;
        case ALREADY_EXISTS: strcpy(str, " : error : already exists"); break;
        case RESULT_PING: strcpy(str, " : ok"); break;
    }
}

void handle_result_messages()
{
    WorkerResult* result;
    size_t result_size;
    char result_type_str[256];

    mq_recv(&callback, (void **) &result, &result_size);
    result_type_repr(result_type_str, result->result_type);
    
    printf("[Main] <<< (%d)", result->message_id);

    if(result->message_type == TIMER_TIME || result->message_type == TIMER_START || result->message_type == TIMER_STOP)
    {
        printf(" : %d", result->worker_id);
    }

    printf("%s", result_type_str);

    if(result->result_type == NOT_FOUND || result->result_type == ALREADY_EXISTS || result->result_type == RESULT_UNAVAILABLE)
    {
        printf("\n");
        return;
    }

    if(result->result_type == RESULT_PING)
    {
        PingResult* ping_result = (PingResult*) result;

        printf(": ");
        while(!queue_empty(&ping_result->unavailable_nodes))
        {
            int* unavailable_node_id;
            queue_pop(&ping_result->unavailable_nodes, (void**) &unavailable_node_id, NULL);
            printf("%d; ", *unavailable_node_id);
        }
    }

    if(result->message_type == CREATE)
    {
        CreateResult* create_result = (CreateResult*) result;
        printf(" : %d", create_result->worker_pid);
    }

    if(result->message_type == TIMER_TIME)
    {
        TimeResult* time_result = (TimeResult*) result;
        printf(" : %lu", time_result->time);
    }

    printf("\n");
}


void handle_create_command(UserCommand cmd)
{
    Result add_node_result;
    CreateMessage message;

    if(tree_exists(&tree, cmd.worker_id))
    {
        send_already_exists_result(cmd, CREATE);
        return;
    }

    add_node_result = tree_create_node(&tree, cmd.worker_id);

    message.message_id = cmd.command_id;
    message.receiver_id = add_node_result.parent;
    message.callback_id = mq_id(&callback);
    message.type = CREATE;
    message.created_worker_id = add_node_result.child;

    worker_send_message(&root_worker, (WorkerMessage*) &message, sizeof(message));
}


void handle_remove_command(UserCommand cmd)
{
    Result remove_node_result;
    DeleteMessage message;

    if(!tree_exists(&tree, cmd.worker_id))
    {
        send_not_found_result(cmd, DELETE);
        return;
    }

    tree_remove_node(&tree, cmd.worker_id);

    message.message_id = cmd.command_id;
    message.receiver_id = cmd.worker_id;
    message.callback_id = mq_id(&callback);
    message.type = DELETE,

    worker_send_message(&root_worker, (WorkerMessage*) &message, sizeof(message));
}


void handle_exec_start_command(UserCommand cmd)
{
    StartTimerMessage message = {
        .message_id = cmd.command_id,
        .receiver_id = cmd.worker_id,
        .callback_id = mq_id(&callback),
        .type = TIMER_START
    };

    if(!tree_exists(&tree, cmd.worker_id))
    {
        send_not_found_result(cmd, TIMER_START);
        return;
    }

    worker_send_message(&root_worker, (WorkerMessage*) &message, sizeof(message));
}


void handle_exec_stop_command(UserCommand cmd)
{
    StopTimerMessage message = {
        .message_id = cmd.command_id,
        .receiver_id = cmd.worker_id,
        .callback_id = mq_id(&callback),
        .type = TIMER_STOP
    };

    if(!tree_exists(&tree, cmd.worker_id))
    {
        send_not_found_result(cmd, TIMER_STOP);
        return;
    }

    worker_send_message(&root_worker, (WorkerMessage*) &message, sizeof(message));
}

void handle_exec_time_command(UserCommand cmd)
{
    GetTimeMessage message = {
        .message_id = cmd.command_id,
        .receiver_id = cmd.worker_id,
        .callback_id = mq_id(&callback),
        .type = TIMER_TIME
    };

    if(!tree_exists(&tree, cmd.worker_id))
    {
        send_not_found_result(cmd, TIMER_TIME);
        return;
    }

    worker_send_message(&root_worker, (WorkerMessage*) &message, sizeof(message));
}

void handle_ping_command(UserCommand cmd)
{
    Queue nodes, unavailable_nodes;
    MessageQueue local_callback, output;
    PingResult result;

    queue_init(&nodes);
    queue_init(&unavailable_nodes);
    mq_create(&local_callback);
    mq_connect(&output, mq_id(&callback));

    tree_get_nodes(&tree, &nodes);

    while(!queue_empty(&nodes))
    {
        int* worker_id;

        queue_pop(&nodes, (void**) &worker_id, NULL);

        PingMessage ping_message = {
            .message_id = -1,
            .receiver_id = *worker_id,
            .callback_id = mq_id(&local_callback),
            .type = PING
        };
        WorkerResult* ping_result;
        size_t ping_result_size;

        worker_send_message(&root_worker, (WorkerMessage*) &ping_message, sizeof(ping_message));
        mq_recv(&local_callback, (void**) &ping_result, &ping_result_size);

        if(ping_result->result_type == RESULT_UNAVAILABLE)
            queue_push(&unavailable_nodes, (void*) worker_id, sizeof(*worker_id));
    }

    result.message_id = cmd.command_id;
    result.worker_id = -1;
    result.message_type = PING;
    result.result_type = RESULT_PING;
    memcpy(&result.unavailable_nodes, &unavailable_nodes, sizeof(unavailable_nodes));

    mq_send(&output, &result, sizeof(result));
}


void handle_close_command(UserCommand cmd)
{
    MessageQueue local_callback;

    mq_create(&local_callback);

    DeleteMessage message = {
        .message_id = cmd.command_id,
        .receiver_id = root_worker.id,
        .callback_id = mq_id(&local_callback),
        .type = DELETE
    };

    worker_send_message(&root_worker, (WorkerMessage*) &message, sizeof(message));
    waitpid(root_worker.pid, NULL, 0);
    // mq_close(&local_callback);

    exit(0);
}


int main(void)
{
    mq_create(&callback);
    tree_init(&tree);

    worker_create(&root_worker, "worker", -1);
    tree_create_node(&tree, root_worker.id);

    while(1)
    {
        UserCommand cmd = scan_user_command();

        switch(cmd.type)
        {
            case USER_CREATE: handle_create_command(cmd); print_tree(&tree); break;
            case USER_PING: handle_ping_command(cmd); break;
            case USER_REMOVE: handle_remove_command(cmd); print_tree(&tree); break;
            case USER_EXEC_START: handle_exec_start_command(cmd); break;
            case USER_EXEC_STOP: handle_exec_stop_command(cmd); break;
            case USER_EXEC_TIME: handle_exec_time_command(cmd); break;
            case USER_CLOSE: handle_close_command(cmd); break;
        }

        handle_result_messages();
    }
}