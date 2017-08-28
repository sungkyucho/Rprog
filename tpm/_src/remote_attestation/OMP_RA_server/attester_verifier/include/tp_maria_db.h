#ifndef _TP_MARIA_DB_H_
#define _TP_MARIA_DB_H_

#include "tp_ra_type.h" 

typedef	struct tp_registered_device_list{
	char gw_id[17];
	char dev_id[17];
	char ml_index[4];
	char nonce[21];
}TP_DEV_LIST;

int DB_tp_get_ra_list(MYSQL *mysql, SVR_CONF * s_conf, ra_request**, int *);
int DB_tp_update_device_info(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id, int reason, uint8_t result, unsigned long tf);
int DB_tp_update_nonce(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id, char *nonce);
int DB_tp_get_pcr_answer(MYSQL *mysql, SVR_CONF *s_conf, unsigned char *answer, char *gw_id);
int DB_tp_get_nonce(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id, unsigned char *nonce);
#endif
