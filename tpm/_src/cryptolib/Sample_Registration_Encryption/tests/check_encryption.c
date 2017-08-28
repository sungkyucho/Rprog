#include <stdlib.h>
#include <unistd.h>
#include "GMMP_lib/GMMP.h"
#include "Setting.h"
#include "CryptoSuite.h"

#include <check.h>

/* for stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* for memset() and memcmp() */
#include <string.h>

static void make_initial_settings(struct config_data_t *pCfg)
{
    pCfg->szServerIP = strdup("61.250.21.211");
    pCfg->nServerPort = 31002;
    pCfg->pszDomainCode = strdup("ThingPlug");
    pCfg->pszGWAuthID = strdup("002655EDE8F1");
    pCfg->pszGWMFID = strdup("mycorporation");
    pCfg->pszDeviceMFID = strdup("");
    pCfg->nErrorLevel = GMMP_ERROR_LEVEL_DEBUG_DATA;
}

START_TEST(test_setting_encryption)
{
    int r;
    char newSecret[16];
    struct config_data_t cfg;
    size_t sLen, eLen, pLen;
    char * settingsEncString;
    char * recoveredSettingString ;
    char * cfgbuf;
    size_t allocsize = 4096;

    /* preparation */
    r = gen_secret_key(newSecret, 16);
    ck_assert_int_eq(r, 0);

    make_initial_settings(&cfg);

    cfgbuf = malloc(allocsize);
    sLen = config_to_string(&cfg, cfgbuf, allocsize);
    settingsEncString = malloc(allocsize);
    recoveredSettingString = malloc(allocsize);
    eLen = allocsize;
    pLen = allocsize;

    /* body */
    eLen = sLen+15;
    r = sym_encrypt(cfgbuf, sLen, settingsEncString, &eLen, newSecret);
    ck_assert_int_ne(r, 0);

    eLen = sLen+16;
    r = sym_encrypt(cfgbuf, sLen, settingsEncString, &eLen, newSecret);
    ck_assert_int_eq(r, 0);

    r = sym_decrypt(settingsEncString, eLen, recoveredSettingString, &pLen, newSecret);
    ck_assert_int_eq(r, 0);
    ck_assert_int_eq(pLen, sLen);

    ck_assert_int_ne(memcmp(cfgbuf, settingsEncString, sLen), 0);
    ck_assert_int_eq(memcmp(cfgbuf, recoveredSettingString, sLen), 0);

    free(cfgbuf);
    free(settingsEncString);
    free(recoveredSettingString);
}
END_TEST


Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Setting");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_setting_encryption);
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
