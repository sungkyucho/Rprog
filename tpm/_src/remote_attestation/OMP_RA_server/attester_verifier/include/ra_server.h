#ifndef _RA_SERVER_H_
#define _RA_SERVER_H_

#include <stdio.h>
#include <stdarg.h>
#include <getopt.h>
#include "ra_com.h"
#include "parse_util.h"

#define PCR_DIGEST_LEN 20

#define QUOTE2_COMPOSITE_IDX_NONCE        6
#define QUOTE2_COMPOSITE_IDX_SELECT_CNT   26
#define QUOTE2_COMPOSITE_IDX_SELECT_NUM   28 
#define QUOTE2_COMPOSITE_IDX_PCR_DIGEST   32 
#define QUOTE2_COMPOSITE_IDX_TPM_VERSION  54
#define QUOTE2_COMPOSITE_IDX_TPM_SW_REV   56
#define QUOTE2_COMPOSITE_IDX_TPM_MANUFCT  61
#define QUOTE2_COMPOSITE_IDX_TPM_BLD_NUM  67 
#define QUOTE2_COMPOSITE_IDX_TPM_ID       72 

/*
#define BIT_RA_SIG_VERIFY 0
#define BIT_RA_NONCE_MATCH 1
#define BIT_RA_PCR_MATCH 2
*/

typedef enum ra_result_bit{
	BIT_RA_SIG_VERIFY=0,
	BIT_RA_NONCE_MATCH,
	BIT_RA_PCR_MATCH,
	BIT_RA_MAX
}RA_RESULT_BIT;

typedef struct s_thread_handler{
	int socket;
	int thread_num;
	
	SVR_CONF  *s_config;
}S_Thread_Handler;

void trigger_handler(void);
void *timer_handler(void *);
void *sms_handler(void *);
void *svr_conn_handler(void *handler);

#endif // _RA_SERVER_H_

