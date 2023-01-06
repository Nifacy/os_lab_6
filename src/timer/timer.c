#include "timer.h"
#include <sys/time.h>
#include <stddef.h>


Timemark __get_time()
{
    struct timeval timemark;
    gettimeofday(&timemark, NULL);
    return timemark.tv_sec * 1000LL + timemark.tv_usec / 1000;
}


void __timer_update(Timer* timer)
{
    Timemark curr_timemark = __get_time();
    timer->passed_time += (curr_timemark - timer->last_timemark);
    timer->last_timemark = curr_timemark;
}


void timer_init(Timer* timer)
{
    timer->passed_time = 0;
    timer->last_timemark = __get_time();
    timer->is_active = 0;
}


void timer_start(Timer* timer)
{
    timer->last_timemark = __get_time();
    timer->is_active = 1;
}


void timer_stop(Timer* timer)
{
    __timer_update(timer);
    timer->is_active = 0;
}


Milliseconds timer_time(Timer* timer)
{
    if(timer->is_active) __timer_update(timer);
    return timer->passed_time;
}
