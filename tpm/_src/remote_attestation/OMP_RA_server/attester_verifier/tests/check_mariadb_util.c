#include <stdlib.h>
#include <unistd.h>
#include <check.h>
#include <string.h>
#include <time.h>
#include <mysql/mysql.h>

#include "maria_db.h"


static void init_s_conf(SVR_CONF **s_conf){
	*s_conf = (SVR_CONF  *)malloc(sizeof(SVR_CONF ));
	(*s_conf)->db_host= strdup("localhost");
	(*s_conf)->db_user= strdup("root");
	(*s_conf)->db_pwd= strdup("mysql_2016");
	(*s_conf)->db_name = strdup("test_sp_db");
	(*s_conf)->db_ra_tbl = strdup("test_item_item");
	(*s_conf)->db_his_tbl = strdup("test_item_history");
	(*s_conf)->db_ans_tbl = strdup("test_item_answer");
}

START_TEST(test_db_init)
{
	MYSQL *mysql = NULL, *mysql_c = NULL;
	char query[1024];
	int rc, r;

	SVR_CONF *s_conf=NULL; 
	init_s_conf(&s_conf);

	mysql = mysql_init(mysql);

	mysql_c = mysql_real_connect (mysql, s_conf->db_host, 
				s_conf->db_user, s_conf->db_pwd, NULL,       
				0, NULL, CLIENT_FOUND_ROWS  );
    ck_assert(mysql_c != NULL);
	
	sprintf(query, "CREATE DATABASE IF NOT EXISTS %s", s_conf->db_name);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "USE %s", s_conf->db_name);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "DROP TABLE IF EXISTS %s", s_conf->db_ra_tbl);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "CREATE TABLE %s(%s)", s_conf->db_ra_tbl, 
			"gw_name      VARCHAR(64) NOT NULL,  \
			 gw_id        VARCHAR(16) NOT NULL, \
			 mac_addr     VARCHAR(17) NOT NULL, \
			 ip           VARCHAR(15), \
			 port         INT NULL, \
			 s_time       TIMESTAMP NULL , \
			 f_time       TIMESTAMP NULL , \
			 ra_result    VARCHAR(6) NULL, \
			 ml_index     INT NULL, \
			 citizen      VARCHAR(3) NULL, \
			 retry_cnt    INT NULL, \
			 recv_pcr     VARCHAR(40) NULL, \
			 PRIMARY KEY(gw_id, mac_addr)"); 
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "DROP TABLE IF EXISTS %s", s_conf->db_his_tbl);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "CREATE TABLE %s (id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY) SELECT * FROM %s", s_conf->db_his_tbl, s_conf->db_ra_tbl);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "DROP TABLE IF EXISTS %s", s_conf->db_ans_tbl);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "CREATE TABLE %s(%s)", s_conf->db_ans_tbl,
			"id INT PRIMARY KEY AUTO_INCREMENT, composite     VARCHAR(40) NOT NULL"); 
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "INSERT INTO %s (gw_name, gw_id, mac_addr, ip, port, retry_cnt) VALUES ('%s', '%s', '%s', '%s', %d, 0)", s_conf->db_ra_tbl, "test_001", "3E6kjr49yn8IWgoo", "90:9F:33:EF:23:EA", "11.22.33.44", 8899);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "INSERT INTO %s (composite) VALUES ('%s')", s_conf->db_ans_tbl, "cc8488344f13e08c7a9b02b6d162b153c1b7f28f");
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	mysql_close (mysql);
}
END_TEST

START_TEST(test_db_ra_list)
{
	MYSQL *mysql = NULL;
	DEV_LIST *dev_list=NULL; 
	int dev_num;
	char query[1024];
	int rc;
	SVR_CONF *s_conf=NULL; 
	init_s_conf(&s_conf);

	rc = DB_connect(&mysql, s_conf);
    ck_assert_int_eq(rc, 0);

	rc = DB_get_ra_list(mysql, s_conf, &dev_list, &dev_num);
    ck_assert_int_eq(rc, 0);
    ck_assert_int_eq(dev_num, 1);
	ck_assert_str_eq(dev_list[0].uuid, "3E6kjr49yn8IWgoo");
	ck_assert_str_eq(dev_list[0].ipaddress, "11.22.33.44");
	ck_assert_str_eq(dev_list[0].port, "8899");

	mysql_close (mysql);
}
END_TEST

START_TEST(test_db_ra_req)
{
	MYSQL *mysql = NULL;
	DEV_LIST *dev_list=NULL; 
	int dev_num;
	char query[1024];
	int rc;
	SVR_CONF *s_conf=NULL; 
	MYSQL_RES *result;
	MYSQL_ROW row;
	int fields, rows, row_index=0;

	init_s_conf(&s_conf);

	rc = DB_connect(&mysql, s_conf);
    ck_assert_int_eq(rc, 0);

	rc = DB_request_ra(mysql, s_conf, "3E6kjr49yn8IWgoo");
    ck_assert_int_eq(rc, 0);

	sprintf(query, "SELECT * from %s", s_conf->db_ra_tbl);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	result= mysql_store_result(mysql);
    ck_assert(result != NULL);

	fields = mysql_num_fields(result) ;
    ck_assert_int_eq(fields, 12);

	rows = mysql_num_rows(result);
    ck_assert_int_eq(rows, 1);

	while( ( row = mysql_fetch_row( result ) )){
		ck_assert_str_eq(row[0], "test_001");
		ck_assert_str_eq(row[10], "1");
	}
	mysql_free_result(result);

	sprintf(query, "SELECT * from %s", s_conf->db_his_tbl);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	result= mysql_store_result(mysql);
    ck_assert(result != NULL);

	fields = mysql_num_fields(result) ;
    ck_assert_int_eq(fields, 13);

	rows = mysql_num_rows(result);
    ck_assert_int_eq(rows, 1);

	while( ( row = mysql_fetch_row( result ) )){
		ck_assert_str_eq(row[1], "test_001");
		ck_assert_str_eq(row[2], "3E6kjr49yn8IWgoo");
		ck_assert_str_eq(row[3], "90:9F:33:EF:23:EA");
		ck_assert_str_eq(row[4], "11.22.33.44");
		ck_assert_str_eq(row[5], "8899");
		ck_assert_str_eq(row[11], "1");
	}
	mysql_free_result(result);
	mysql_close (mysql);
}
END_TEST

START_TEST(test_db_update_device_info)
{
	MYSQL *mysql = NULL;
	DEV_LIST *dev_list=NULL; 
	int dev_num;
	char query[1024];
	char mac_addr[]="90:9F:33:EF:23:EA";

	int rc;
	SVR_CONF *s_conf=NULL; 
	MYSQL_RES *result;
	MYSQL_ROW row;
	int fields, rows, row_index=0;
	unsigned long fin_time; 
	unsigned char pcrResp[] = "cc8488344f13e08c7a9b02b6d162b153c1b7f28f";
	uint8_t ra_result = 1<<0 | 1<<1 | 1<<2 | 1<<3; 

	init_s_conf(&s_conf);

	rc = DB_connect(&mysql, s_conf);
    ck_assert_int_eq(rc, 0);

	fin_time = (unsigned long)time(NULL);
	rc = DB_update_device_info(mysql, s_conf, mac_addr, ra_result, fin_time, pcrResp);
    ck_assert_int_eq(rc, 0);

	sprintf(query, "SELECT * from %s", s_conf->db_ra_tbl);
	rc = mysql_query(mysql, query);
    ck_assert_int_eq(rc, 0);

	result= mysql_store_result(mysql);
    ck_assert(result != NULL);

	fields = mysql_num_fields(result) ;
    ck_assert_int_eq(fields, 12);

	rows = mysql_num_rows(result);
    ck_assert_int_eq(rows, 1);

	while( ( row = mysql_fetch_row( result ) )){
		ck_assert_str_eq(row[0], "test_001");
		ck_assert_str_eq(row[7], "1111");
		ck_assert_str_eq(row[10], "0");
		ck_assert_str_eq(row[11], "cc8488344f13e08c7a9b02b6d162b153c1b7f28f");
	}

	mysql_free_result(result);
	mysql_close (mysql);
}
END_TEST

START_TEST(test_db_pcr_answer)
{
	MYSQL *mysql = NULL;
	SVR_CONF *s_conf=NULL; 
	char query[1024];
	int rc;
	unsigned char * pcrAnswer=malloc(41);
	unsigned char * answer=malloc(21);

	init_s_conf(&s_conf);

	rc = DB_connect(&mysql, s_conf);
    ck_assert_int_eq(rc, 0);

	rc = DB_get_pcr_answer(mysql, s_conf, pcrAnswer);
    ck_assert_int_eq(rc, 0);
	hexstr_to_hex("cc8488344f13e08c7a9b02b6d162b153c1b7f28f", answer, 20);
    ck_assert(0 == memcmp(pcrAnswer, answer, 20));
	free(pcrAnswer);
	free(answer);
	mysql_close (mysql);
}
END_TEST

Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Setting");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_db_init);
    tcase_add_test(tc_core, test_db_ra_list);
    tcase_add_test(tc_core, test_db_ra_req);
    tcase_add_test(tc_core, test_db_update_device_info);
    tcase_add_test(tc_core, test_db_pcr_answer);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = setting_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed ==0) ? EXIT_SUCCESS: EXIT_FAILURE;
}
/* vi: set ai sw=4 ts=8 sts=4: */
