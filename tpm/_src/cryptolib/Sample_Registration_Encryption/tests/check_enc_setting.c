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

//    save_secret(newSecret);
//    saveSecretToKeyChain(newSecret);
//    saveSecretToTPM(newSecret);

START_TEST(test_sign_settings)
{
    int i;
    const char *cfgfile = "test.cfg";
    struct config_data_t cfg[2];
    char * privpem = "priv.pem";
    char * pubpem = "pub.pem";
    int rc;
    char * cfgstr; size_t cfgsize;
    char * signedCfgBuf; size_t signedBufLen;

    /* add settings entry manually */
    make_initial_settings(&cfg[0]);

    /* cfg-to-string */
    cfgsize = 2048;
    cfgstr = malloc(cfgsize);

    ck_assert_int_eq(cfg[0].nServerPort, 31002);
    ck_assert_int_eq(cfg[0].nErrorLevel, 2);
    ck_assert_str_eq(cfg[0].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[0].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[0].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[0].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[0].pszDeviceMFID, "");

    setTPMOff();

    rc = gen_private_key(privpem);
    ck_assert_int_eq(rc, 0);
    rc = gen_public_key(privpem, pubpem);
    ck_assert_int_eq(rc, 0);

    signedBufLen = 4096; signedCfgBuf = malloc(signedBufLen);
    rc = sign_config(&cfg[0], signedCfgBuf, signedBufLen, privpem);
    ck_assert_int_eq(rc, 0);

    rc = verify_config(signedCfgBuf, signedBufLen, &cfg[1], pubpem);
    ck_assert_int_eq(rc, 0);

    ck_assert_int_eq(cfg[1].nServerPort, 31002);
    ck_assert_int_eq(cfg[1].nErrorLevel, 2);
    ck_assert_str_eq(cfg[1].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[1].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[1].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[1].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[1].pszDeviceMFID, "");

    for (i=0; i<2; ++i) {
	free_config(&cfg[i]);
    }

    unlink(cfgfile);
    unlink(privpem);
    unlink(pubpem);

    free(cfgstr);
    free(signedCfgBuf);
}
END_TEST

/**
 * test target: sign_config(), verify_config()
 */
START_TEST(test_sign_settings_ex)
{
    int i;
    const char *cfgfile = "test.cfg";
    struct config_data_t cfg[2];
    char * privpem = "priv.pem";
    char * pubpem = "pub.pem";
    int rc;
    char * cfgstr; size_t cfgsize;
    char * signedCfgBuf; size_t signedBufLen;
    char backup;

    /* add settings entry manually */
    make_initial_settings(&cfg[0]);

    /* cfg-to-string */
    cfgsize = 2048;
    cfgstr = malloc(cfgsize);

    ck_assert_int_eq(cfg[0].nServerPort, 31002);
    ck_assert_int_eq(cfg[0].nErrorLevel, 2);
    ck_assert_str_eq(cfg[0].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[0].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[0].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[0].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[0].pszDeviceMFID, "");

    setTPMOff();

    rc = gen_private_key(privpem);
    ck_assert_int_eq(rc, 0);
    rc = gen_public_key(privpem, pubpem);
    ck_assert_int_eq(rc, 0);

    signedBufLen = 4096; signedCfgBuf = malloc(signedBufLen);
    rc = sign_config(NULL, signedCfgBuf, signedBufLen, privpem);
    ck_assert_int_ne(rc, 0);
    rc = sign_config(&cfg[0], NULL, signedBufLen, privpem);
    ck_assert_int_ne(rc, 0);
    rc = sign_config(&cfg[0], signedCfgBuf, 0, privpem);
    ck_assert_int_ne(rc, 0);
    rc = sign_config(&cfg[0], signedCfgBuf, signedBufLen, NULL);
    ck_assert_int_ne(rc, 0);
    rc = sign_config(&cfg[0], signedCfgBuf, signedBufLen, privpem);
    ck_assert_int_eq(rc, 0);

    rc = verify_config(NULL, signedBufLen, &cfg[1], pubpem);
    ck_assert_int_ne(rc, 0);
    rc = verify_config(signedCfgBuf, 0, &cfg[1], pubpem);
    ck_assert_int_ne(rc, 0);
    rc = verify_config(signedCfgBuf, signedBufLen, NULL, pubpem);
    ck_assert_int_ne(rc, 0);
    rc = verify_config(signedCfgBuf, signedBufLen, &cfg[1], NULL);
    ck_assert_int_ne(rc, 0);

    backup = signedCfgBuf[20];
    signedCfgBuf[20] = backup + 1;
    rc = verify_config(signedCfgBuf, signedBufLen, &cfg[1], pubpem);
    ck_assert_int_ne(rc, 0);
    signedCfgBuf[20] = backup;
    rc = verify_config(signedCfgBuf, signedBufLen, &cfg[1], pubpem);
    ck_assert_int_eq(rc, 0);

    ck_assert_int_eq(cfg[1].nServerPort, 31002);
    ck_assert_int_eq(cfg[1].nErrorLevel, 2);
    ck_assert_str_eq(cfg[1].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[1].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[1].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[1].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[1].pszDeviceMFID, "");

    for (i=0; i<2; ++i) {
	free_config(&cfg[i]);
    }

    unlink(cfgfile);
    unlink(privpem);
    unlink(pubpem);

    free(cfgstr);
    free(signedCfgBuf);
}
END_TEST

START_TEST(test_enc_settings)
{
    int i;
    const char *cfgfile = "test.cfg";
    struct config_data_t cfg[2];
    int rc;
    char * cfgstr; size_t cfglen;
    char *secret; size_t secretsize;
    char *encbuf; size_t encsize;

    /* add settings entry manually */
    make_initial_settings(&cfg[0]);

    /* cfg-to-string */
    cfgstr = malloc(2048);
    cfglen = config_to_string(&cfg[0], cfgstr, 2048);

    ck_assert_int_eq(cfg[0].nServerPort, 31002);
    ck_assert_int_eq(cfg[0].nErrorLevel, 2);
    ck_assert_str_eq(cfg[0].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[0].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[0].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[0].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[0].pszDeviceMFID, "");

    secretsize = 16; secret = malloc(secretsize);
    rc = gen_secret_key(secret, secretsize);
    ck_assert_int_eq(rc, 0);

    encsize = 4096; encbuf = malloc(encsize);
    rc = encrypt_config(&cfg[0], secret, encbuf, encsize);
    ck_assert_int_eq(rc, 0);

    rc = decrypt_config(&cfg[1], secret, encbuf, encsize);
    ck_assert_int_eq(rc, 0);


    ck_assert_int_eq(cfg[1].nServerPort, 31002);
    ck_assert_int_eq(cfg[1].nErrorLevel, 2);
    ck_assert_str_eq(cfg[1].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[1].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[1].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[1].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[1].pszDeviceMFID, "");

    for (i=0; i<2; ++i) {
	free_config(&cfg[i]);
    }

    free(encbuf);
    unlink(cfgfile);
}
END_TEST

START_TEST(test_enc_settings_ex)
{
    int i;
    const char *cfgfile = "test.cfg";
    struct config_data_t cfg[2];
    int rc;
    char * cfgstr; size_t cfglen;
    char *secret; size_t secretsize;
    char *encbuf; size_t encsize;

    /* add settings entry manually */
    make_initial_settings(&cfg[0]);

    /* cfg-to-string */
    cfgstr = malloc(2048);
    cfglen = config_to_string(&cfg[0], cfgstr, 2048);

    ck_assert_int_eq(cfg[0].nServerPort, 31002);
    ck_assert_int_eq(cfg[0].nErrorLevel, 2);
    ck_assert_str_eq(cfg[0].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[0].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[0].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[0].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[0].pszDeviceMFID, "");

    secretsize = 16; secret = malloc(secretsize);
    rc = gen_secret_key(secret, secretsize);
    ck_assert_int_eq(rc, 0);

    rc = encrypt_config(NULL, secret, encbuf, encsize);
    ck_assert_int_ne(rc, 0);
    rc = encrypt_config(&cfg[0], NULL, encbuf, encsize);
    ck_assert_int_ne(rc, 0);
    rc = encrypt_config(&cfg[0], secret, NULL, encsize);
    ck_assert_int_ne(rc, 0);
    rc = encrypt_config(&cfg[0], secret, encbuf, 0);
    ck_assert_int_ne(rc, 0);

    encsize = 4096; encbuf = malloc(encsize);
    rc = encrypt_config(&cfg[0], secret, encbuf, 1);
    ck_assert_int_ne(rc, 0);
    rc = encrypt_config(&cfg[0], secret, encbuf, encsize);
    ck_assert_int_eq(rc, 0);

    rc = decrypt_config(NULL, secret, encbuf, encsize);
    ck_assert_int_ne(rc, 0);
    rc = decrypt_config(&cfg[1], NULL, encbuf, encsize);
    ck_assert_int_ne(rc, 0);
    rc = decrypt_config(&cfg[1], secret, NULL, encsize);
    ck_assert_int_ne(rc, 0);
    rc = decrypt_config(&cfg[1], secret, encbuf, 0);
    ck_assert_int_ne(rc, 0);
    rc = decrypt_config(&cfg[1], secret, encbuf, encsize);
    ck_assert_int_eq(rc, 0);

    ck_assert_int_eq(cfg[1].nServerPort, 31002);
    ck_assert_int_eq(cfg[1].nErrorLevel, 2);
    ck_assert_str_eq(cfg[1].szServerIP, "61.250.21.211");
    ck_assert_str_eq(cfg[1].pszDomainCode, "ThingPlug");
    ck_assert_str_eq(cfg[1].pszGWAuthID, "002655EDE8F1");
    ck_assert_str_eq(cfg[1].pszGWMFID, "mycorporation");
    ck_assert_str_eq(cfg[1].pszDeviceMFID, "");

    for (i=0; i<2; ++i) {
	free_config(&cfg[i]);
    }

    free(encbuf);
    unlink(cfgfile);
}
END_TEST

Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Setting");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_sign_settings);
    tcase_add_test(tc_core, test_sign_settings_ex);
    tcase_add_test(tc_core, test_enc_settings);
    tcase_add_test(tc_core, test_enc_settings_ex);
    tcase_set_timeout(tc_core, 0);
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
