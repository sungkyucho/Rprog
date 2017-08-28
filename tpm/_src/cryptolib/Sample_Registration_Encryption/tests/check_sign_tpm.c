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

/* for fopen(), fread() */
#include <stdio.h>

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

static int readall(char *filename, char *buf, int len)
{
    FILE *fp;
    int r;
    fp = fopen(filename, "r");
    if (!fp) {
	perror("fopen");
	return -1;
    }
    r = fread(buf, sizeof(char), len, fp);
    if (r <= 0) {
	perror("fread");
	goto out;
    }
out:
    fclose(fp);
    return r;
}

#if 0
static hexdump(uint8_t *buf, size_t len)
{
    int i;
    for (i=0; i<len; ++i) {
	if (i % 16 == 0) fprintf(stderr, "%03x: ", i);
	fprintf(stderr, "%02x ", buf[i]);
	if (i % 16 == 15) fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}
#endif

START_TEST(test_setting_sign_verify)
{
    struct config_data_t cfg;
    char * cfgstr;
    size_t allocsize = 4096;
    size_t sLen;
    make_initial_settings(&cfg);
    cfgstr = malloc(allocsize);
    sLen = config_to_string(&cfg, cfgstr, allocsize);

    int r;
    uint8_t sig[1024] = {0,}; size_t siglen=1024;
    void *privkey;
    void *pubkey;
    char *privfile = "priv.pem";
    char *pubfile = "pub.pem";

    setTPMOn();

    r = gen_private_key(privfile);
    ck_assert_int_eq(r, 0);

    struct stat statbuf;
    r = stat(privfile, &statbuf);
    ck_assert_int_eq(r, 0);

    char keybuf[2048];
    r = readall(privfile, keybuf, 2048);
    ck_assert_int_ge(r, 0);
    ck_assert_ptr_ne(strstr(keybuf, "-----BEGIN TSS KEY BLOB-----"), NULL);
    ck_assert_ptr_ne(strstr(keybuf, "-----END TSS KEY BLOB-----"), NULL);

    r = gen_public_key(privfile, pubfile);
    ck_assert_int_eq(r, 0);

    r = readall(pubfile, keybuf, 2048);
    ck_assert_int_ge(r, 0);
    ck_assert_ptr_ne(strstr(keybuf, "-----BEGIN PUBLIC KEY-----"), NULL);
    ck_assert_ptr_ne(strstr(keybuf, "-----END PUBLIC KEY-----"), NULL);

    privkey = load_private_key(privfile);
    ck_assert_ptr_ne(privkey, NULL);

    r = gen_signature(privfile, cfgstr, sLen, sig, &siglen);
    ck_assert_int_eq(r, 0);
    ck_assert_int_eq(siglen, 256);
    //fprintf(stderr, "siglen %d\n", siglen);
    //hexdump(sig, siglen);

    pubkey = load_public_key(pubfile);
    ck_assert_ptr_ne(pubkey, NULL);

    r = verify_signature(pubfile, cfgstr, sLen, sig, siglen);
    ck_assert_int_eq(r, 0);

    free_public_key(pubkey);
    free_private_key(privkey);
    free(cfgstr);

    unlink(pubfile);
    unlink(privfile);
}
END_TEST

Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("SignVerify");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_setting_sign_verify);
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
