#include <stdlib.h>
#include <unistd.h>
#include "CryptoSuite.h"

#include <check.h>

/* for memset() and memcmp() */
#include <string.h>

START_TEST(test_base64)
{
    char *sample = "SAMPLE";
    int len = strlen(sample);
    char encoded[64] = {0,};
    size_t encsize = 64;
    char decoded[64] = {0,};
    size_t decsize = 64;
    int r;

    r = encode_base64(encoded, &encsize, sample, len);
    ck_assert_int_eq(r, 0);
    ck_assert_str_ne(encoded, sample);

    r = decode_base64(decoded, &decsize, encoded, encsize);
    ck_assert_int_eq(r, 0);
    ck_assert_int_eq(decsize, len);
    ck_assert_str_eq(decoded, sample);

}
END_TEST

START_TEST(test_base64_ex)
{
    char *sample = "SAMPLE";
    int len = strlen(sample);
    char encoded[64] = {0,};
    size_t encsize = 64;
    char decoded[64] = {0,};
    size_t decsize = 64;
    int r;

    r = encode_base64(NULL, &encsize, sample, len);
    ck_assert_int_ne(r, 0);
    r = encode_base64(encoded, &encsize, NULL, len);
    ck_assert_int_ne(r, 0);
    r = encode_base64(encoded, NULL, sample, len);
    ck_assert_int_ne(r, 0);
    encsize = 0;
    r = encode_base64(encoded, &encsize, sample, len);
    ck_assert_int_ne(r, 0);
    encsize = 1;
    r = encode_base64(encoded, &encsize, sample, len);
    ck_assert_int_ne(r, 0);
    encsize=64; len = 0;
    r = encode_base64(encoded, &encsize, sample, len);
    ck_assert_int_ne(r, 0);

    len=strlen(sample);
    r = encode_base64(encoded, &encsize, sample, len);
    ck_assert_int_eq(r, 0);
    ck_assert_str_ne(encoded, sample);

    r = decode_base64(NULL, &decsize, encoded, encsize);
    ck_assert_int_ne(r, 0);
    r = decode_base64(decoded, &decsize, NULL, encsize);
    ck_assert_int_ne(r, 0);
    r = decode_base64(decoded, NULL, encoded, encsize);
    ck_assert_int_ne(r, 0);
    r = decode_base64(decoded, &decsize, encoded, 0);
    ck_assert_int_ne(r, 0);
    decsize = 0;
    r = decode_base64(decoded, &decsize, encoded, encsize);
    ck_assert_int_ne(r, 0);
    decsize = 3;
    r = decode_base64(decoded, &decsize, encoded, encsize);
    ck_assert_int_ne(r, 0);

    decsize = 64;
    r = decode_base64(decoded, &decsize, encoded, encsize);
    ck_assert_int_eq(r, 0);
    ck_assert_int_eq(decsize, len);
    ck_assert_str_eq(decoded, sample);

}
END_TEST

Suite * setting_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Setting");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_base64);
    tcase_add_test(tc_core, test_base64_ex);
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
