#include <stdlib.h>
#include <unistd.h>
#include "ra_com.h"

#include <check.h>

/* for memset() and memcmp() */
#include <string.h>

START_TEST(test_hexstr_to_hex)
{
    char *hexstr;
	unsigned char *hexbytes;
    int r;

	hexstr = malloc(5);
	hexbytes = malloc(5);
	memset(hexstr, 0x00, 5);
	memset(hexbytes, 0x00, 5);

	snprintf(hexstr, 5, "3FA6");
    r = hexstr_to_hex(hexstr, hexbytes, 2);
    ck_assert_int_eq(r, 1);
    ck_assert(hexbytes[0] == 0x3F);
    ck_assert(hexbytes[1] == 0xA6);

    r = hexstr_to_hex(hexstr, hexbytes, 0);
    ck_assert_int_eq(r, 0);

	free(hexbytes);
	free(hexstr);
}
END_TEST

START_TEST(test_hex_to_hexstr)
{
    char *hexstr;
	unsigned char *hexbytes;
    int r;

	hexstr = malloc(5);
	hexbytes = malloc(5);
	memset(hexstr, 0x00, 5);
	memset(hexbytes, 0x4A, 5);

    r = hex_to_hexstr(hexstr, hexbytes, 0);
    ck_assert_int_eq(r, 0);

    r = hex_to_hexstr(hexstr, hexbytes, 2);
    ck_assert_int_eq(r, 1);
    ck_assert_str_eq(hexstr, "4a4a");

	free(hexbytes);
	free(hexstr);
}
END_TEST

START_TEST(test_ipaddr_valid)
{
    char ipaddr[16];
    int r;

	snprintf(ipaddr, sizeof(ipaddr), "192.168.0.32");
    r = is_valid_ip(ipaddr);
    ck_assert_int_eq(r, 1);

	snprintf(ipaddr, sizeof(ipaddr), "192.168.0");
    r = is_valid_ip(ipaddr);
    ck_assert_int_eq(r, 0);

	snprintf(ipaddr, sizeof(ipaddr), "192.168.0.256");
    r = is_valid_ip(ipaddr);
    ck_assert_int_eq(r, 0);

	snprintf(ipaddr, sizeof(ipaddr), "192.168.0,256");
    r = is_valid_ip(ipaddr);
    ck_assert_int_eq(r, 0);

	snprintf(ipaddr, sizeof(ipaddr), "192.168.o.56");
    r = is_valid_ip(ipaddr);
    ck_assert_int_eq(r, 0);

	snprintf(ipaddr, sizeof(ipaddr), "192.168.0.56.1");
    r = is_valid_ip(ipaddr);
    ck_assert_int_eq(r, 0);

}
END_TEST


Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Setting");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_ipaddr_valid);
    tcase_add_test(tc_core, test_hex_to_hexstr);
    tcase_add_test(tc_core, test_hexstr_to_hex);
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
