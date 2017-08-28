#include <stdlib.h>
#include <unistd.h>
#include "GMMP_lib/GMMP.h"
#include "Setting.h"

#include <check.h>

START_TEST(test_setting_create)
{
    const char *cfgfile = "test.cfg";
    struct config_data_t cfg;
    int rc;
    /* add settings entry manually */
    cfg.szServerIP = strdup("61.250.21.211");
    cfg.nServerPort = 31002;
    cfg.pszDomainCode = strdup("ThingPlug");
    cfg.pszGWAuthID = strdup("002655EDE8F1");
    cfg.pszGWMFID = strdup("mycorporation");
    cfg.pszDeviceMFID = strdup("");
    cfg.nErrorLevel = GMMP_ERROR_LEVEL_DEBUG_DATA;

    /* write settings in file */
    rc = write_config(NULL, NULL);
    ck_assert_int_eq(rc, -1);
    rc = write_config(&cfg, NULL);
    ck_assert_int_eq(rc, -1);
    rc = write_config(&cfg, cfgfile);
    free_config(&cfg);

    /* re-init buffer for setting */
    memset(&cfg, 0, sizeof(cfg));

    /* read settings from file */
    read_config(&cfg, cfgfile);
    ck_assert_int_eq(cfg.nServerPort, 31002);
    ck_assert_int_eq(cfg.nErrorLevel, 2);
    ck_assert_str_eq(cfg.szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg.pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg.pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg.pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg.pszDeviceMFID, "");
    free_config(&cfg);

    unlink(cfgfile);
}
END_TEST

Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Setting");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_setting_create);
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
