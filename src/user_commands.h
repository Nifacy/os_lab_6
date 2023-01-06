#ifndef __USER_COMMANDS_H__
#define __USER_COMMANDS_H__


typedef enum {
    USER_CREATE,
    USER_PING,
    USER_REMOVE,
    USER_EXEC_START,
    USER_EXEC_STOP,
    USER_EXEC_TIME,
    USER_CLOSE
} UserCommandType;


typedef struct {
    int command_id;
    UserCommandType type;
    int worker_id; // -1 if not setted
} UserCommand;

#endif