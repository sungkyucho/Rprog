#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <mysql/mysql.h>

#include "maria_db.h"
#include "tp_maria_db.h"
#include "dbg_macros.h"
#include "tp_ra_type.h"


/**
 * @brief insert updated data to history table  
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param target column name of primary key to qeury 
 * @param value primary key value to query  
*/
static int db_write_history(MYSQL *mysql, SVR_CONF *s_conf, char *target, char *value){
	char query[1024];
	int rc;

	sprintf(query, "INSERT INTO %s SELECT NULL, gw_id, dev_id, rsp_time, ra_result, ra_reason FROM %s WHERE %s='%s'", s_conf->db_his_tbl, s_conf->db_ra_tbl, target, value);
//	dbg_printf(DDBG, "query:%s\n", query);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		dbg_printf(DERROR, "Can't insert into history table\n");
		return EXIT_FAILURE;
	} 
	return EXIT_SUCCESS;
}


/**
 * @brief query registed devices info 
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param dev_list pointer to store devices info 
 * @param dev_num pointer to store number of registered devices
*/
static int db_select_list(MYSQL *mysql, SVR_CONF *s_conf, ra_request **dev_list, int *dev_num){
	MYSQL_ROW row;
	MYSQL_RES *result;
	ra_request *trigger_list;

	char query[512];
	int rc;
	int cnt;
	int fields, rows=0, row_index=0;

	sprintf(query, "SELECT gw_id, dev_id, ml_index from %s", s_conf->db_ra_tbl);
//	dbg_printf(DDBG, "query:%s\n", query);
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

	trigger_list = (ra_request *)malloc(sizeof(ra_request) * rows);
	memset(trigger_list, 0x00, sizeof(ra_request) * rows);
	*dev_num = rows;

	while( ( row = mysql_fetch_row( result ) ))
	{
		for( cnt = 0 ; cnt < fields ; ++cnt){
			if(cnt == 0){  
				snprintf(trigger_list[row_index].gw_id, LEN_GW_ID+1, "%s", row[cnt]);
			}
			else if(cnt == 1){ 
				snprintf(trigger_list[row_index].dev_id, LEN_DEVICE_ID+1, "%s", row[cnt]);
			}
			else if(cnt == 2){ 
				int idx = atoi(row[cnt]);
				trigger_list[row_index].pcr_select[23 >> 3]  |= 1 << (23 & 7); 
				if(idx == 9){
					trigger_list[row_index].pcr_select[4 >> 3]  |= 1 << (4 & 7); 
					trigger_list[row_index].pcr_select[5 >> 3]  |= 1 << (5 & 7); 
				}
				else if(idx == 10){
					trigger_list[row_index].pcr_select[4 >> 3]  |= 1 << (4 & 7); 
					trigger_list[row_index].pcr_select[6 >> 3]  |= 1 << (6 & 7); 
				}
				else if(idx == 11){
					trigger_list[row_index].pcr_select[5 >> 3]  |= 1 << (5 & 7); 
					trigger_list[row_index].pcr_select[6 >> 3]  |= 1 << (6 & 7); 
				}
			}
		}
		row_index++;
	}

	*dev_list = trigger_list;
	mysql_free_result(result);
			  
	return EXIT_SUCCESS;
}

/**
 * @brief update nonce column
 *        nonce will use to verify attestant's response
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param gw_id attestant's gateway id 
 * @param nonce generated by attestater   
*/
static int db_update_nonce(MYSQL *mysql, SVR_CONF *s_conf, char * gw_id, char *nonce){
	char query[1024];
	int rc;

	sprintf(query, "UPDATE %s SET nonce='%s' WHERE gw_id='%s'", s_conf->db_ra_tbl, nonce, gw_id);
//	dbg_printf(DINFO, "query:%s\n", query);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		return EXIT_FAILURE;
	} 

	if(EXIT_FAILURE == db_write_history(mysql, s_conf, "gw_id", gw_id))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/**
 * @brief update RA result data  
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param gw_id attestant's gateway id 
 * @param reason why RA failed 
 * @param result RA result  
 * @param tf RA finished time 
*/
static int db_update_data(MYSQL *mysql, SVR_CONF *s_conf, char * gw_id, int reason, uint8_t result, unsigned long tf){
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
	
	sprintf(query, "UPDATE %s SET rsp_time=FROM_UNIXTIME(%lu), ra_result='%s', ra_reason='%d' WHERE gw_id='%s'", s_conf->db_ra_tbl, tf, str_result, reason, gw_id);
//	dbg_printf(DINFO, "query:%s\n", query);
	if ((rc=mysql_query(mysql, query))) {
		check_mysql_rc(rc, mysql);
		return EXIT_FAILURE;
	} 

	if(EXIT_FAILURE == db_write_history(mysql, s_conf, "gw_id", gw_id))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

/**
 * @brief query PCR composite answer  
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param answer pointer to store PCR answer 
 * @param fw_ver firmware version to use primary key
 * @param ml_index pointer to store pcr measurement index 
*/
static int db_select_pcr_answer(MYSQL *mysql, SVR_CONF *s_conf, unsigned char *answer, char *fw_ver, int ml_index){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char query[512];
	int rc;

	sprintf(query, "SELECT pcr%d FROM %s where fw_ver='%s'", ml_index, s_conf->db_ans_tbl, fw_ver);
//	dbg_printf(DINFO, "query:%s\n", query);
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
		if(0 == strcmp(row[0], "")){
			dbg_printf(DERROR, "empty string in pcr%d\n", ml_index);
			return EXIT_FAILURE;
		}
		hexstr_to_hex(row[0], answer, 20);
	}
	mysql_free_result(result);
	return EXIT_SUCCESS;
}

/**
 * @brief query firmware version of registed attestant
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param gw_id attestant's gateway id 
 * @param fw_ver pointer to store firmware version 
 * @param ml_index pointer to store pcr measurement index 
*/
static int db_select_fw_version(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id, char ** fw_ver, int *ml_index){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char query[512];
	int rc;
	int fw_len;
	char firmware[18];

	sprintf(query, "SELECT fw_ver, ml_index FROM %s where gw_id='%s'", s_conf->db_ra_tbl, gw_id);
//	dbg_printf(DINFO, "query:%s\n", query);
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
		fw_len = strlen(row[0]);
		snprintf(firmware, fw_len + 1, "%s", row[0]);
		*ml_index = strtol(row[1], (char **)NULL, 10);
	}
	*fw_ver = strdup(firmware);
	mysql_free_result(result);
	return EXIT_SUCCESS;
}

/**
 * @brief query nonce of registed attestant
 * @return EXIT_SUCCESS on success, EXIT_FAILURE otherwize
 * @param mysql db connection to query 
 * @param s_conf server configuration  
 * @param gw_id attestant's gateway id 
 * @param nonce pointer to store nonce 
*/
int DB_tp_get_nonce(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id, unsigned char *nonce){
	MYSQL_ROW row;
	MYSQL_RES *result;

	char query[512];
	int rc;

	sprintf(query, "SELECT nonce FROM %s where gw_id='%s'", s_conf->db_ra_tbl, gw_id);
//	dbg_printf(DINFO, "query:%s\n", query);
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
		hexstr_to_hex(row[0], nonce, 20);
	}
	mysql_free_result(result);
	return EXIT_SUCCESS;
}


int DB_tp_get_ra_list(MYSQL *mysql, SVR_CONF *s_conf, ra_request**dev_list, int *dev_num){
	return db_select_list(mysql, s_conf, dev_list, dev_num);
}

int DB_tp_update_device_info(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id, int reason, uint8_t result, unsigned long tf ){
	return db_update_data(mysql, s_conf, gw_id, reason, result, tf);
}

int DB_tp_update_nonce(MYSQL *mysql, SVR_CONF *s_conf, char *gw_id, char *nonce){
	return db_update_nonce(mysql, s_conf, gw_id, nonce);
}

int DB_tp_get_pcr_answer(MYSQL *mysql, SVR_CONF *s_conf, unsigned char *answer, char *gw_id){
	char * fw_ver=NULL;
	int ml_index;
	int ret;
	if(EXIT_SUCCESS != db_select_fw_version(mysql, s_conf, gw_id, &fw_ver, &ml_index)){
		return EXIT_FAILURE;
	}

	ret = db_select_pcr_answer(mysql, s_conf, answer, fw_ver, ml_index);
	if(fw_ver) free(fw_ver);
	return ret;
}