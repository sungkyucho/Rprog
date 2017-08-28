#include <string.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include "CryptoSuite.h"

/* allocated memory? */
/**
 * \brief Base64 인코딩 수행.
 * \param target 인코딩된 내용을 기록할 메모리의 주소
 * \param *tsize target의 크기. 대략 ssize 의 4/3 정도 길이가 됨.
 * \param source 원문이 저장된 메모리의 주소
 * \param ssize source의 길이
 * \return 성공하면 0, 실패하면 -1
 */
int encode_base64(char *target, size_t *tsize, char *source, size_t ssize)
{
    int r = 0;
    BIO *bio, *b64;
    BUF_MEM *buffer;
    if ( !target || !tsize || !source || !*tsize || !ssize ) {
	fprintf(stderr, "%s() : Something missing\n", __func__);
	return -1;
    }

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

//    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, source, ssize);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer);
    if (*tsize < buffer->length) {
	fprintf(stderr, "%s(): insufficient buffer\n", __func__);
	r = -1;
	goto out;
    }
    *tsize = buffer->length;
    memcpy(target, buffer->data, *tsize);
//    BIO_set_close(bio, BIO_NOCLOSE);
out:
    BIO_free_all(bio);

    return r;
}

static size_t guessDecodedLength(char *source, size_t len)
{
    int padding = 0;

    if (source[len-1] == '=')
	padding ++;
    if (source[len-2] == '=')
	padding ++;

    return (len * 3)/4 - padding;
}

/**
 * \brief Base64 디코딩 수행.
 * \param target 디코딩된 내용을 기록할 메모리의 주소
 * \param *tsize target의 크기. 대략 ssize 의 3/4 정도 길이가 됨.
 * \param source 원문이 저장된 메모리의 주소
 * \param ssize source의 길이
 * \return 성공하면 0, 실패하면 -1
 */
 /* allocated memory? */
int decode_base64(char *target, size_t *tsize, char *source, size_t ssize)
{
    int r = 0;
    char *s = NULL;
    BIO *bio, *b64;

    if (!target || !tsize || !source || !ssize || !*tsize) {
	fprintf(stderr, "decode_base64(): Something NULL\n");
	return -1;
    }

    s = malloc(sizeof(char) * ssize + 1);
    memcpy(s, source, ssize);
    s[ssize] = 0;

    if (*tsize < guessDecodedLength(s, ssize)) {
	r = -1;
	fprintf(stderr, "decode_base64(): Target buffer too small. %ld < %ld\n", *tsize, guessDecodedLength(s, ssize));
	goto out;
    }
    bio = BIO_new_mem_buf(s, -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
//    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    *tsize = BIO_read(bio, target, ssize);
    BIO_free_all(bio);
out:
    if (s) 
	free(s);
    return r;
}
/* vi: set ai sw=4 ts=8 sts=4: */
