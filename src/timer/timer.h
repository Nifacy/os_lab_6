#ifndef __TIMER_H__
#define __TIMER_H__


typedef unsigned long Milliseconds;
typedef unsigned long Timemark;


typedef struct {
    Milliseconds passed_time;
    Timemark last_timemark;
    int is_active;
} Timer;


void timer_init(Timer* timer);
void timer_start(Timer* timer);
void timer_stop(Timer* timer);
Milliseconds timer_time(Timer* timer);


#endif