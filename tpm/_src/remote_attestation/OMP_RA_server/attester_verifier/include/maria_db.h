#ifndef _MARIA_DB_H_
#define _MARIA_DB_H_

#include <stdio.h>
#include <mysql/mysql.h>
#include "ra_server.h"

#define check_mysql_rc(rc, mysql) \
	if (rc)\
{\
	  printf("Error (%d): %s (%d) in %s line %d\n", rc, mysql_error(mysql), \
			         mysql_errno(mysql), __FILE__, __LINE__);\
	  return(0);\
}
#define CHECK_BIT(byte, bit) \
	byte & (1 << bit)?'1':'0'

typedef	struct registered_device_list{
	char ipaddress[16];
	char port[6];
	char uuid[17];
}DEV_LIST;

int DB_connect(MYSQL **mysql, SVR_CONF* s_conf);
int DB_request_ra(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id);
int DB_get_ra_list(MYSQL *mysql, SVR_CONF * s_conf, DEV_LIST **, int *);
int DB_update_device_info(MYSQL *mysql, SVR_CONF * s_conf, char *mac_addr, uint8_t , unsigned long t2, unsigned char *);
int DB_get_pcr_answer(MYSQL *mysql, SVR_CONF *s_conf, unsigned char *);
#endif 
