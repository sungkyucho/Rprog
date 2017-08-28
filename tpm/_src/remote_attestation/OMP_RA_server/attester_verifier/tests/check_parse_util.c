#include <stdlib.h>
#include <check.h>
#include <string.h>
#include <unistd.h>

#include "ra_com.h"
#include "parse_util.h"


START_TEST(test_parse_server_conf)
{
	SVR_CONF *s_conf = (SVR_CONF  *)malloc(sizeof(SVR_CONF ));
    int r;

    r = parse_server_conf("../conf/tp_server.conf", s_conf, "=");
    ck_assert_int_eq(r, 1);
    ck_assert_str_eq(s_conf->db_host, "localhost");
    ck_assert_str_eq(s_conf->db_user, "root");
    ck_assert_str_eq(s_conf->db_pwd, "mysql_2016");
    ck_assert_str_eq(s_conf->db_name, "sp_db");
    ck_assert_str_eq(s_conf->db_ra_tbl, "ra_item");
    ck_assert_str_eq(s_conf->db_his_tbl, "ra_history");
    ck_assert_str_eq(s_conf->db_ans_tbl, "ra_answer");
//    ck_assert_str_eq(s_conf->open_port, "8000");
    ck_assert_int_eq(s_conf->t_interval, 30);
//    ck_assert_int_eq(s_conf->pcr_idx_sum, 1 << 5 | 1 << 16);
//    ck_assert(s_conf->pcr_select[0] == 1 << 5);
//    ck_assert(s_conf->pcr_select[2] == 1 << 0);

	free(s_conf);
}
END_TEST

/*
START_TEST(test_parse_recv_conf)
{
	R_CONF *r_conf = (R_CONF  *)malloc(sizeof(R_CONF ));
    int r;

    r = parse_recv_conf("../conf/ra_receiver.conf", r_conf);
    ck_assert_int_eq(r, 1);
    ck_assert_int_eq(r_conf->port, 8001);
    ck_assert_str_eq(r_conf->caaddr, "192.168.0.101:8888");
    ck_assert_str_eq(r_conf->interface, "wlan0");

	free(r_conf);
}
END_TEST
*/


Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Setting");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_parse_server_conf);
//    tcase_add_test(tc_core, test_parse_recv_conf);
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
