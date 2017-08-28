#ifndef _TIMER_H_
#define _TIMER_H_

typedef enum TimerError {
     TIMER_ALREADY_CREATED_ERR = 1,
     TIMER_COULD_NOT_CREATE_ERR,  
     TIMER_NOT_CREATED_ERR,      
     TIMER_COULD_NOT_DELETE_ERR,
     TIMER_COULD_NOT_START_ERR,
     TIMER_COULD_NOT_STOP_ERR, 
     TIMER_COULD_NOT_SET_HANDLER_ERR  
} TimerError;

typedef enum TimerStatus {
    TIMER_UNINIT,      
    TIMER_CREATED,    
    TIMER_ARMED,     
    TIMER_DISARMED  
} TimerStatus;

int sp_timer_create();
int sp_timer_register_handler(void (*pfunc)(void));
int sp_timer_start(const double start, const double interval);
int sp_timer_stop();
int sp_timer_delete();

#endif          
