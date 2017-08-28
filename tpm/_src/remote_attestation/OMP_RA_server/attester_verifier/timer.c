#include <signal.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "timer.h"


static TimerStatus t_status = TIMER_UNINIT;
static timer_t sp_timer;
static void (*handler_func)(void) = NULL;
static time_t get_whole_seconds(const double seconds) {
    return (time_t) ((long) seconds);
}

TimerStatus get_status(){
	return t_status;
}
static long get_fractional_nanoseconds(const double seconds) {
    const double whole_secs = (double) get_whole_seconds(seconds);
    const double fractional_part = seconds - whole_secs;
    return (long) (fractional_part * 1000000000);
}

void sig_alarm_handler(int signum) {
    assert(signum == SIGALRM);
    handler_func();
}

int sp_timer_create() {
    struct sigevent sevp;

    if ( t_status != TIMER_UNINIT ) {
        return TIMER_ALREADY_CREATED_ERR;
    }

    sevp.sigev_notify = SIGEV_THREAD_ID;
    sevp.sigev_signo = SIGALRM;
    sevp.sigev_value.sival_int = 0;
	sevp._sigev_un._tid  = syscall(SYS_gettid);

    if ( timer_create(CLOCK_REALTIME, &sevp, &sp_timer) != 0 ) {
        return TIMER_COULD_NOT_CREATE_ERR;
    }

    t_status = TIMER_CREATED;
    return 0;
}


int sp_timer_register_handler(void (*pfunc)(void)){
    struct sigaction sa;

    sa.sa_handler = sig_alarm_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if ( sigaction(SIGALRM, &sa, NULL) != 0 ) {
        return TIMER_COULD_NOT_SET_HANDLER_ERR;
    }

    handler_func = pfunc;
    return 0;
}


int sp_timer_start(const double start, const double interval) {
    struct itimerspec its;

    its.it_interval.tv_sec = get_whole_seconds(interval);
    its.it_interval.tv_nsec = get_fractional_nanoseconds(interval);
    its.it_value.tv_sec = get_whole_seconds(start);
    its.it_value.tv_nsec = get_fractional_nanoseconds(start);

    if ( timer_settime(sp_timer, 0, &its, NULL) != 0 ) {
        return TIMER_COULD_NOT_START_ERR;
    }

    t_status = TIMER_ARMED;
    return 0;
}


int sp_timer_stop() {
    struct itimerspec its;

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;

    if ( timer_settime(sp_timer, 0, &its, NULL) != 0 ) {
        return TIMER_COULD_NOT_STOP_ERR;
    }

    t_status = TIMER_DISARMED;
    return 0;
}


int sp_timer_delete() {
    if ( t_status == TIMER_UNINIT ) {
        return TIMER_NOT_CREATED_ERR;
    }

    if ( timer_delete(sp_timer) != 0 ) {
        return TIMER_COULD_NOT_DELETE_ERR;
    }

    t_status = TIMER_UNINIT;
    return 0;
}
