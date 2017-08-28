#include "CryptoSuite.h"

/* for malloc() */
#include <stdlib.h>
/* for memcpy() */
#include <string.h>

#include <openssl/aes.h>
#include <openssl/evp.h>

static size_t align_16(size_t len)
{
    return (len + 15) & ~0xf;
}

static void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

/** @brief encrypt the contents of buffer with aes128-ctr
 *  @param plainbuf buffer with plaintext
 *  @param plainlen plaintext length
 *  @param cipherbuf buffer for iv + ciphertext
 *  @param cipherlen ciphertext's length
 *         this value must be > plainlen + 16 (iv length)
 *  @param key 128bit(16byte) length key
 *  @return 0 on success, -1 otherwise
 *          on return, cipherbuf has iv + ciphertext.
 *          sym_decrypt() uses the iv value.
 */
int sym_encrypt(char *plainbuf, size_t plainlen,
	    char *cipherbuf, size_t *cipherlen, uint8_t *key)
{
    uint8_t iv[16] = {0,};
    void * ctx = NULL;
    size_t len = 0, lentmp = 0;
    int r;

    if (*cipherlen < plainlen + 16) {
	fprintf(stderr, "invalid cipherlen\n");
	return -1;
    }

    r = get_random(iv, 16);
    if (r < 0) {
	return -1;
    }
    memcpy(cipherbuf, iv, 16);
    ctx = encrypt_init(key, iv);
    lentmp = *cipherlen;
    len = lentmp;
    *cipherlen = 16;
    encrypt_update(ctx, plainbuf, plainlen, &cipherbuf[16], &len);
    *cipherlen += len;

    len = lentmp - *cipherlen;
    encrypt_final(ctx, &cipherbuf[*cipherlen], &len);
    *cipherlen += len;
    return 0;
}

/** \brief 스트림용 암호연산의 초기화 함수. aes128-ctr 사용.
 *  \param key 암호연산에 사용할 대칭키.
 *  \param iv 암호연산에 사용할 초기화벡터.
 *      일반적으로 난수를 사용하고, 이 난수를 해독연산에도 동일하게 사용해야 함.
 *  \return 이후 encrypt_update()와 encrypt_final()에 사용할 컨텍스트 핸들.
 */
void * encrypt_init(uint8_t *key, uint8_t *iv)
{
    int r = 0;
    EVP_CIPHER_CTX *ctx = NULL;
    uint8_t *ivector = iv;
    if (!ivector) {
	ivector = malloc(16);
	r = get_random(iv, 16);
	if (r < 0) {
	    goto out;
	}
    }
    ctx = malloc(sizeof(EVP_CIPHER_CTX));
    EVP_EncryptInit(ctx, EVP_aes_128_ctr(), key, ivector);
out:
    if (ivector && !iv)
	free(ivector);
    return ctx;
}

/** \brief 스트림용 암호연산 실행함수. aes128-ctr 사용.
 *  \param pctx encrypt_init()이 전달한 컨텍스트 핸들.
 *  \param plainbuf 암호화할 원본데이터가 있는 메모리의 포인터
 *  \param plainlen 암호화할 원본데이터의 길이
 *  \param cipherbuf 암호문을 기록할 메모리의 포인터
 *  \param cipherlen cipherbuf의 크기
 */
int encrypt_update(void * pctx, char *plainbuf, size_t plainlen,
	    char *cipherbuf, size_t *cipherlen)
{
    int *len = (int *)cipherlen;
    EVP_CIPHER_CTX *ctx = (EVP_CIPHER_CTX *)pctx;
    EVP_EncryptUpdate(ctx, cipherbuf, len, plainbuf, plainlen);
    return 0;
}

/** \brief 스트림용 암호연산 정리함수. aes128-ctr 사용
 *  \param pctx encrypt_init()이 전달한 컨텍스트 핸들.
 *  \param cipherbuf encrypt_update()가 미처 반환하지 않은 암호문을 기록할 메모리
 *  \param cipherlen cipherbuf의 크기
 */
int encrypt_final(void * pctx, char *cipherbuf, size_t *cipherlen)
{
    int *len = (int *)cipherlen;
    EVP_CIPHER_CTX *ctx = (EVP_CIPHER_CTX *)pctx;
    EVP_EncryptFinal(ctx, cipherbuf, len);
    free(ctx);
    return 0;
}

/** \brief 스트림용 해독연산의 초기화 함수. aes128-ctr 사용.
 *  \param key 해독연산에 사용할 대칭키.
 *  \param iv 해독연산에 사용할 초기화벡터.
 *            암호연산에 사용한 iv와 동일한 값을 넣어야 해독할 수 있음.
 *  \return 이후 decrypt_update()와 decrypt_final()에 사용할 컨텍스트 핸들.
 */
void * decrypt_init(uint8_t *key, uint8_t *iv)
{
    int r = 0;
    EVP_CIPHER_CTX *ctx = NULL;
    ctx = malloc(sizeof(EVP_CIPHER_CTX));
    EVP_EncryptInit(ctx, EVP_aes_128_ctr(), key, iv);
out:
    return ctx;
}

/** \brief 스트림용 해독연산 실행함수. aes128-ctr 사용.
 *  \param pctx decrypt_init()이 전달한 컨텍스트 핸들.
 *  \param cipherbuf 해독할 원본데이터가 있는 메모리의 포인터
 *  \param cipherlen 해독할 원본데이터의 길이
 *  \param plainbuf 해독 결과를 기록할 메모리의 포인터
 *  \param plainlen plainbuf의 크기
 */
int decrypt_update(void * pctx, char *cipherbuf, size_t cipherlen,
	    char *plainbuf, size_t *plainlen)
{
    int *len = (int *)plainlen;
    EVP_CIPHER_CTX *ctx = (EVP_CIPHER_CTX *)pctx;
    EVP_DecryptUpdate(ctx, plainbuf, len, cipherbuf, cipherlen);
    return 0;
}

/** \brief 스트림용 해독연산 정리함수. aes128-ctr 사용
 *  \param pctx decrypt_init()이 전달한 컨텍스트 핸들.
 *  \param plainbuf encrypt_update()가 미처 반환하지 않은 해독결과를 기록할 메모리
 *  \param plainlen plainbuf의 크기
 */
int decrypt_final(void * pctx, char *plainbuf, size_t *plainlen)
{
    int *len = (int *)plainlen;
    EVP_CIPHER_CTX *ctx = (EVP_CIPHER_CTX *)pctx;
    EVP_DecryptFinal(ctx, plainbuf, len);
    free(ctx);
    return 0;
}

/** @brief decrypt the contents of buffer with aes128-ctr
 *  @param cipherbuf buffer with iv + ciphertext
 *  @param cipherlen ciphertext length
 *  @param plainbuf buffer for plaintext
 *  @param plainlen plaintext's length
 *  @param key 128bit(16byte) length key
 *  @return 0 on success, -1 otherwise
 */
int sym_decrypt(char *cipherbuf, size_t cipherlen,
	    char *plainbuf, size_t *plainlen, uint8_t *key)
{
    uint8_t iv[16] = {0,};
    EVP_CIPHER_CTX *ctx;
    size_t len, lentmp;

    memcpy(iv, cipherbuf, 16);
    ctx = decrypt_init(key, iv);
    lentmp = *plainlen;
    len = lentmp;
    *plainlen = 0;
    decrypt_update(ctx, &cipherbuf[16], cipherlen-16, plainbuf, &len);
    *plainlen += len;

    len = lentmp - *plainlen;
    decrypt_final(ctx, &plainbuf[*plainlen], &len);
    *plainlen += len;
    return 0;
}

/* vi: set ai sw=4 ts=8 sts=4: */
