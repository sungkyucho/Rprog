#include <stdlib.h>
#include <unistd.h>
#include "GMMP_lib/GMMP.h"
#include "Setting.h"
#include "CryptoSuite.h"
#include "TPMCryptoSuite.h"

#include <check.h>

/* for stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/* for memset() and memcmp() */
#include <string.h>

#if 0
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
#endif

static void dumpkey(uint8_t *buf, int len)
{
    int i;
    printf("%s(): ", __func__);
    for (i=0; i<len; ++i) {
	printf("%02x ", buf[i]);
	if (i%16 == 15) printf("\n");
	else if (i%8 == 7) printf(" ");
    }
    printf("\n");
}

static void clearKeyFile(char *filename)
{
    FILE *fp;
    if (!filename)
	return;
    chmod(filename, S_IFREG | S_IRUSR | S_IWUSR);
    unlink(filename);
}

/**
 * tests without error handling just for use case
 */
START_TEST(test_setting_gensecret)
{
    int r;
    struct stat statbuf;
    uint8_t newSecret[16];
    uint8_t fileSecret[16];
    uint8_t tmpSecret[16];
    char *keyfile = "newsecret.key.blob";
    char keyfilebuf[256];
    extern void setTPMOff(void);
    extern void setTPMOn(void);

    /* make sure keyfile doesn't exist */
    clearKeyFile(keyfile);

    r = gen_secret_key(newSecret, 16);
    ck_assert_ptr_eq(r, 0);
    r = gen_secret_key(fileSecret, 16);
    ck_assert_ptr_eq(r, 0);
    ck_assert_int_ne(memcmp(newSecret, fileSecret, 16), 0);

    dumpkey(newSecret, 16);

    r = save_secret(keyfile, newSecret, 16);
    ck_assert_int_eq(r, 0);

    r = get_default_secret_file(keyfilebuf, 256);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(keyfilebuf, "secret.key.blob");


    /* filename length > 255 not accepted */
    char longnamekeyfile[300]={0,};
    int i;

    /* non-NULL argument changes secret file location */
    r = set_default_secret_file(keyfile);
    ck_assert_int_eq(r, 0);

    r = get_default_secret_file(keyfilebuf, 256);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(keyfilebuf, "newsecret.key.blob");

    /* NULL sets secret file to default */
    r = set_default_secret_file(NULL);
    ck_assert_int_eq(r, 0);

    r = get_default_secret_file(keyfilebuf, 256);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(keyfilebuf, "secret.key.blob");

    /* without filename, use default file */
    r = save_secret(NULL, newSecret, 16);
    ck_assert_int_eq(r, 0);


    /* key file is regular file with permission 0400 */
    r = stat(keyfile, &statbuf);
    ck_assert_int_eq(r, 0);
    ck_assert_int_ne(statbuf.st_size, 0);
    ck_assert_int_eq(statbuf.st_mode, S_IFREG | S_IRUSR);



    r = load_secret(keyfile, fileSecret, 16);
    ck_assert_int_eq(r, 0);

    r = load_secret(NULL, tmpSecret, 16);
    ck_assert_int_eq(r, 0);

    /* if keyfile if NULL, use defaultKeyFile
     * but in this case default file name is different from keyfile.
     * error should be returned.
     */

    r = set_default_secret_file("key.blob");
    ck_assert_int_eq(r, 0);

    r = memcmp(fileSecret, tmpSecret, 16);
    ck_assert_int_eq(0, r);

    r = load_secret(keyfile, fileSecret, 16);
    ck_assert_int_eq(r, 0);

    ck_assert_int_eq(memcmp(newSecret, fileSecret, 16), 0);
}
END_TEST

/**
 * tests with error handling
 */
START_TEST(test_setting_gensecret_ex)
{
    int r;
    struct stat statbuf;
    uint8_t newSecret[16];
    uint8_t fileSecret[16];
    uint8_t tmpSecret[16];
    char *keyfile = "newsecret.key.blob";
    char keyfilebuf[256];
    extern void setTPMOff(void);
    extern void setTPMOn(void);

    /* make sure keyfile doesn't exist */
    clearKeyFile(keyfile);

    setTPMOff();
    r = gen_secret_key(NULL, 0);
    /* with length 0, don't allocate buffer */
    ck_assert_int_ne(r, 0);

    r = gen_secret_key(NULL, 16);
    ck_assert_ptr_ne(r, 0);
    
    r = gen_secret_key(newSecret, 0);
    /* with length 0, don't allocate buffer */
    ck_assert_ptr_ne(r, 0);

    /* with buffer, fill it with random number */
    r = gen_secret_key(newSecret, 16);
    ck_assert_ptr_eq(r, 0);
    /* (statistically) always unique values */
    r = gen_secret_key(fileSecret, 16);
    ck_assert_ptr_eq(r, 0);
    ck_assert_int_ne(memcmp(newSecret, fileSecret, 16), 0);

    setTPMOn();
    r = gen_secret_key(newSecret, 16);
    ck_assert_ptr_eq(r, 0);
    r = gen_secret_key(fileSecret, 16);
    ck_assert_ptr_eq(r, 0);
    ck_assert_int_ne(memcmp(newSecret, fileSecret, 16), 0);

    dumpkey(newSecret, 16);


    r = save_secret(keyfile, NULL, 16);
    ck_assert_int_ne(r, 0);

    r = save_secret(keyfile, newSecret, 0);
    ck_assert_int_ne(r, 0);

    r = save_secret(keyfile, newSecret, 16);
    ck_assert_int_eq(r, 0);


    /* set_default_secret_file() changes the name of file for secret */
    r = get_default_secret_file(NULL, 256);
    ck_assert_int_ne(r, 0);

    r = get_default_secret_file(keyfilebuf, 0);
    ck_assert_int_ne(r, 0);

    r = get_default_secret_file(keyfilebuf, 256);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(keyfilebuf, "secret.key.blob");


    /* filename length > 255 not accepted */
    char longnamekeyfile[300]={0,};
    int i;
    memset(longnamekeyfile, 'a', 299);
    r = set_default_secret_file(longnamekeyfile);
    ck_assert_int_ne(r, 0);
    /* non-NULL argument changes secret file location */
    r = set_default_secret_file(keyfile);
    ck_assert_int_eq(r, 0);

    r = get_default_secret_file(keyfilebuf, 256);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(keyfilebuf, "newsecret.key.blob");

    /* NULL sets secret file to default */
    r = set_default_secret_file(NULL);
    ck_assert_int_eq(r, 0);

    r = get_default_secret_file(keyfilebuf, 256);
    ck_assert_int_eq(r, 0);
    ck_assert_str_eq(keyfilebuf, "secret.key.blob");

    /* without filename, use default file */
    r = save_secret(NULL, newSecret, 16);
    ck_assert_int_eq(r, 0);


    /* key file is regular file with permission 0400 */
    r = stat(keyfile, &statbuf);
    ck_assert_int_eq(r, 0);
    ck_assert_int_ne(statbuf.st_size, 0);
    ck_assert_int_eq(statbuf.st_mode, S_IFREG | S_IRUSR);


    r = load_secret(keyfile, NULL, 16);
    ck_assert_int_ne(r, 0);

    r = load_secret(keyfile, fileSecret, 0);
    ck_assert_int_ne(r, 0);

    r = load_secret(keyfile, fileSecret, 16);
    ck_assert_int_eq(r, 0);

    r = load_secret(NULL, tmpSecret, 16);
    ck_assert_int_eq(r, 0);

    /* if keyfile if NULL, use defaultKeyFile
     * but in this case default file name is different from keyfile.
     * error should be returned.
     */

    r = set_default_secret_file("key.blob");
    ck_assert_int_eq(r, 0);

    r = load_secret(NULL, tmpSecret, 16);
    ck_assert_int_ne(r, 0);

    r = memcmp(fileSecret, tmpSecret, 16);
    ck_assert_int_eq(0, r);

    char * noexistingfile = "nofile";
    r = load_secret(noexistingfile, fileSecret, 0);
    ck_assert_int_ne(r, 0);

    r = load_secret(keyfile, fileSecret, 16);
    ck_assert_int_eq(r, 0);

    ck_assert_int_eq(memcmp(newSecret, fileSecret, 16), 0);
}
END_TEST


Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("secretkey");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_setting_gensecret);
//    tcase_add_test(tc_core, test_setting_gensecret_ex);
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
