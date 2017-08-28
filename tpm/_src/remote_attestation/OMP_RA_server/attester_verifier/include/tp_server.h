#ifndef _TP_SERVER_H_
#define _TP_SERVER_H_

void trigger_handler(void);
void *timer_handler(void *);
void *sms_handler(void *);
void *rsp_handler(void *);
int verify_response(MYSQL *mysql, SVR_CONF *s_conf, BYTE* resp, char *gw_id, time_t mtime);

#endif // _TP_SERVER_H_

