#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mysql/mysql.h>

#include "maria_db.h"
#include "dbg_macros.h"


/*
static int db_use_database(MYSQL *mysql, char * db_name){
	char query[1024];
	int rc;

	sprintf(query, "USE %s", db_name);
	dbg_printf(DINFO, "query:%s\n", query);
	if ((rc = mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		printf("query:%s\n", query);
		return EXIT_FAILURE;
	} 
	return EXIT_SUCCESS;
}
*/

static int db_write_history(MYSQL *mysql, SVR_CONF *s_conf, char *target, char *value){
	char query[1024];
	int rc;

	sprintf(query, "INSERT INTO %s SELECT NULL, gw_name, gw_id, mac_addr, ip, port, s_time, f_time, ra_result, ml_index, citizen, retry_cnt, recv_pcr FROM %s WHERE %s='%s'", s_conf->db_his_tbl, s_conf->db_ra_tbl, target, value);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		dbg_printf(DERROR, "Can't insert into history table\n");
		return EXIT_FAILURE;
	} 
	return EXIT_SUCCESS;
}

static int db_get_retry_cnt(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char query[512];
	int rc;
	int cnt=-1;

	sprintf(query, "SELECT retry_cnt FROM %s WHERE gw_id='%s'", s_conf->db_ra_tbl, gw_id);
	dbg_printf(DINFO, "query:%s\n", query);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		return cnt;
	}
	result = mysql_store_result(mysql);
	if(!result){
		dbg_printf(DERROR, "Invalid result set\n");
		return cnt;
	}

	while( ( row = mysql_fetch_row( result ) ))
	{
		if( row[0] == NULL) {
			mysql_free_result(result);
			return 0;
		}
		cnt = atoi(row[0]); 
	}
	mysql_free_result(result);
	dbg_printf(DINFO, "retry count:%d\n", cnt);
	return cnt;
}

static int db_select_list(MYSQL *mysql, SVR_CONF *s_conf, DEV_LIST **dev_list, int *dev_num){
	MYSQL_ROW row;
	MYSQL_RES *result;
	DEV_LIST *trigger_list;

	char query[512];
	int rc;
	int cnt;
	int fields, rows=0, row_index=0;
	char ip[16];
	char port[6];
	char gw_id[17];

	sprintf(query, "SELECT gw_id, ip, port from %s", s_conf->db_ra_tbl);
	dbg_printf(DDBG, "query:%s\n", query);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		return EXIT_FAILURE;
	}
//	result= mysql_use_result(mysql);
	result= mysql_store_result(mysql);
	if(!result){
		dbg_printf(DERROR, "Invalid result set\n");
		return EXIT_FAILURE;
	}
	fields = mysql_num_fields(result) ;

	rows = mysql_num_rows(result);
	if(rows <= 0){
		dbg_printf(DERROR, "mysql_num_rows failed\n");
		mysql_free_result(result);
		return EXIT_FAILURE;
	}
	if(rows > 65535)
		rows = 65535;

	trigger_list = (DEV_LIST *)malloc(sizeof(DEV_LIST) * rows);
	*dev_num = rows;

	while( ( row = mysql_fetch_row( result ) ))
	{
		for( cnt = 0 ; cnt < fields ; ++cnt){
			if(cnt == 1){ // IP
				snprintf(trigger_list[row_index].ipaddress, sizeof(ip), "%s", row[cnt]);
			}
			else if(cnt == 2){ //port
				snprintf(trigger_list[row_index].port, sizeof(port), "%s", row[cnt]);
			}
			else if(cnt == 0){ // gw_id 
				snprintf(trigger_list[row_index].uuid, sizeof(gw_id), "%s", row[cnt]);
			}
		}
		row_index++;
	}

	*dev_list = trigger_list;
	mysql_free_result(result);
			  
	return EXIT_SUCCESS;
}

static int db_update_data(MYSQL *mysql, SVR_CONF *s_conf, char * mac_addr, uint8_t result, unsigned long tf, unsigned char *pcrcomposite){
	char query[1024];
	int i, rc, num=0;
	char str_result[6];

	for(i=BIT_RA_SIG_VERIFY; i<BIT_RA_MAX; i++){
		snprintf(str_result + i, 2, "%c", CHECK_BIT(result, i));
		if(CHECK_BIT(result, i) == '1') num ++;
	}
	if(num == BIT_RA_MAX){ // all success 
		snprintf(str_result + BIT_RA_MAX, 2, "%c", '1');
	}
	else{
		snprintf(str_result + BIT_RA_MAX, 2, "%c", '0');
	}
	
	sprintf(query, "UPDATE %s SET f_time=FROM_UNIXTIME(%lu), ra_result='%s', ml_index='%d', retry_cnt=0, recv_pcr='%s' WHERE mac_addr='%s'", s_conf->db_ra_tbl, tf, str_result, s_conf->pcr_idx_sum, pcrcomposite, mac_addr);
	dbg_printf(DINFO, "query:%s\n", query);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		return EXIT_FAILURE;
	} 

	if(EXIT_FAILURE == db_write_history(mysql, s_conf, "mac_addr", mac_addr))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
	// UPDATE timestamp- use "CURRENT_TIMESTAMP()" or like this "INSERT INTO t3(id)  VALUES (1),(2);"

}

static int db_update_req_time(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id){
	char query[1024];
	int rc, retry;
	unsigned long cur_time; 
	cur_time = (unsigned long)time(NULL);

	retry = db_get_retry_cnt(mysql, s_conf, gw_id);
	if(retry == -1)
		return EXIT_FAILURE;

	dbg_printf(DINFO, "query:%s\n", query);
	sprintf(query, "UPDATE %s SET s_time=FROM_UNIXTIME(%lu), retry_cnt='%d' WHERE gw_id='%s'", s_conf->db_ra_tbl, cur_time, (retry+1), gw_id);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		return EXIT_FAILURE;
	} 

	if(EXIT_FAILURE ==  db_write_history(mysql, s_conf, "gw_id", gw_id))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
	// UPDATE timestamp- use "CURRENT_TIMESTAMP()" or like this "INSERT INTO t3(id)  VALUES (1),(2);"

}
static int db_select_pcr_answer(MYSQL *mysql, SVR_CONF *s_conf, unsigned char *answer){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char query[512];
	int rc;

	sprintf(query, "SELECT composite FROM %s", s_conf->db_ans_tbl);
	dbg_printf(DINFO, "query:%s\n", query);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		return EXIT_FAILURE;
	}
	result = mysql_store_result(mysql);
	if(!result){
		dbg_printf(DERROR, "Invalid result set\n");
		return EXIT_FAILURE;
	}

	while( ( row = mysql_fetch_row( result ) ))
	{
		hexstr_to_hex(row[0], answer, 20);
	}
	mysql_free_result(result);
	return EXIT_SUCCESS;
}

int DB_connect(MYSQL **mysql, SVR_CONF* s_conf)
{
	MYSQL *n_mysql = NULL;

	*mysql = mysql_init(n_mysql);

	if (!*mysql) {
		dbg_printf(DERROR, "mysql Init failed\n");
		return EXIT_FAILURE;
	}

	if (!mysql_real_connect (*mysql, s_conf->db_host, 
				s_conf->db_user, s_conf->db_pwd, s_conf->db_name,
				0,          /* port number, 0 for default */
				NULL,       /* socket file or named pipe name */
				CLIENT_FOUND_ROWS /* connection flags */ ))
	{
		dbg_printf(DERROR, "Connection failed\n");
		mysql_close(*mysql);
		return EXIT_FAILURE;
	} 
	dbg_printf(DINFO, "Connection Success\n");

	return EXIT_SUCCESS;
}

int DB_request_ra(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id){
	return db_update_req_time(mysql, s_conf, gw_id);
}

int DB_get_ra_list(MYSQL *mysql, SVR_CONF *s_conf, DEV_LIST **dev_list, int *dev_num){
	return db_select_list(mysql, s_conf, dev_list, dev_num);
}

int DB_update_device_info(MYSQL *mysql, SVR_CONF *s_conf, char *mac_addr, uint8_t result, unsigned long tf, unsigned char *pcrcomposite){
	return db_update_data(mysql, s_conf, mac_addr, result, tf, pcrcomposite);
}

int DB_get_pcr_answer(MYSQL *mysql, SVR_CONF *s_conf, unsigned char *answer){
	return db_select_pcr_answer(mysql, s_conf, answer);
}
