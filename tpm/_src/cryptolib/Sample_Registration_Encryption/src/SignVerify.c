/** @file
 *  @brief functions related asymmetric cipher functions
 */
#include "CryptoSuite.h"
#include "TPMCryptoSuite.h"

#include <string.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/x509.h>

#include <openssl/engine.h> /* for ENGINE_load_private_key( ) */

static inline void _print_openssl_error(const char *func, int line)
{
    char errbuf[256];
    unsigned long err = ERR_peek_last_error();
    ERR_error_string(err, errbuf);
    fprintf(stderr, "Error: %s():%d %s\n", func, line, errbuf);
}

#define print_openssl_error() _print_openssl_error(__func__, __LINE__)

static void apps_shutdown(void)
{
    ERR_remove_thread_state(NULL);
}

static int _gen_private_key(char * privkeyfile)
{
    RSA *rsa  = NULL;
    BIGNUM *bn = BN_new();
    BIO *out = NULL;
    unsigned long f4 = RSA_F4;
    int num = 2048;
    int r = -1;

    out = BIO_new(BIO_s_file());
    if (NULL == out) {
	goto err;
    }

    if (BIO_write_filename(out, privkeyfile) <= 0) {
	perror("BIO_write_filename");
	r = -2;
	goto err;
    }

    rsa = RSA_new();
    if (!rsa) {
	r = -3;
	goto err;
    }

    if (!BN_set_word(bn, f4)) {
	r = -4;
	goto err;
    }
    if (!RSA_generate_key_ex(rsa, num, bn, NULL)) {
	r = -5;
	goto err;
    }

    if (!PEM_write_bio_RSAPrivateKey(out, rsa, NULL, NULL, 0, NULL, NULL)) {
	r = -6;
	goto err;
    }

    r = 0;
err:
    if (bn)
	BN_free(bn);
    if (rsa)
	RSA_free(rsa);
    if (out)
	BIO_free_all(out);
    apps_shutdown();
    return r;
}

/** \brief generate private key.<br>
 *	   key spec: RSA, 2048, no encryption.<br>
 *         the result is private key file.<br>
 *	   it's same as running "openssl genrsa -out privkey.pem 2048"
 *  \param privkeyfile filename to store private key in.
 * \return 성공이면 0, 실패면 -1
 */
int gen_private_key(char * privkeyfile)
{
    if (is_TPM_available()) {
	fprintf(stderr, "call _gen_private_key_in_TPM()\n");
	return _gen_private_key_in_TPM(privkeyfile);
    }
    return _gen_private_key(privkeyfile);
}

static int is_TPM_PrivKeyFile(char *file)
{
    int r = 0;
    char linebuf[256];
    FILE *fp = fopen(file, "r");
    if (!fp) {
	return -1;
    }

    while(fgets(linebuf, 255, fp)) {
	if (strstr(linebuf, "TSS KEY BLOB")) {
	    r = 1;
	    goto out;
	}
    }
out:
    fclose(fp);
    return r;
}

static EVP_PKEY *load_privkey(char *file)
{
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    if (file == NULL) {
	fprintf(stderr, "no keyfile\n");
	goto out;
    }

    if(is_TPM_PrivKeyFile(file) > 0) {
	return load_private_key_from_TPM(file);
    }

    key = BIO_new(BIO_s_file());
    if (NULL == key) {
	fprintf(stderr, "Error: BIO_new()\n");
	goto out;
    }

    if (BIO_read_filename(key, file) <= 0) {
	fprintf(stderr, "Error: BIO_read_filename()\n");
	goto out;
    }

    pkey = PEM_read_bio_PrivateKey(key, NULL, NULL, NULL);
    if  (NULL == pkey) {
	fprintf(stderr, "Error: PEM_read_bio_PrivateKey\n");
	goto out;
    }
out:
    if (key)
	BIO_free(key);
    return pkey;
}

EVP_PKEY *load_pubkey(char *keyfile)
{
    BIO *key = NULL;
    EVP_PKEY *pkey = NULL;

    key = BIO_new(BIO_s_file());
    if (key == NULL) {
	fprintf(stderr, "Error: BIO_new()\n");
	return NULL;
    }

    if (BIO_read_filename(key, keyfile) <= 0) {
	fprintf(stderr, "Error: BIO_read_filename()\n");
	goto out;
    }

    pkey = PEM_read_bio_PUBKEY(key, NULL, NULL, NULL);
    if (!pkey) {
	fprintf(stderr, "Error: PEM_read_bio_PUBKey()\n");
    }
out:
    if (key)
	BIO_free(key);
    return pkey;
}

/** \brief generate public key from given private key<br>
 *         the result is public key file<br>
 *         it's same as running "openssl rsa -in privkey.pem -out pubkey.pem -pubout"
 *  \param privkeyfile filename containing private key
 *  \param pubkeyfile filename to store public key in.
 * \return 성공이면 0, 실패면 -1
 */
int gen_public_key(char * privkeyfile, char *pubkeyfile)
{
    BIO *out = NULL;
    EVP_PKEY *pkey;
    RSA *rsa;
    int r = -1;

    out = BIO_new(BIO_s_file());
    if (NULL == out) {
	fprintf(stderr, "Error: BIO_new");
	goto err;
    }

    pkey = load_privkey(privkeyfile);
    if (NULL == pkey) {
	fprintf(stderr, "Error: load_privkey");
	goto err;
    }

    rsa = EVP_PKEY_get1_RSA(pkey);
    EVP_PKEY_free(pkey);
    if (NULL == rsa) {
	fprintf(stderr, "Error: EVP_PKEY_get1_RSA");
	goto err;
    }

    if (BIO_write_filename(out, pubkeyfile) <= 0) {
	fprintf(stderr, "Error: BIO_write_filename");
	goto err;
    };

    if (PEM_write_bio_RSA_PUBKEY(out, rsa) <= 0) {
	fprintf(stderr, "unable to write key");
	goto err;
    }
    
    r = 0;
err:
    if (rsa)
	RSA_free(rsa);
    if (out)
	BIO_free_all(out);
    return r;
}

/** \brief load private key from file
 *  \param privkeyfile
 *  \return handle of private key structure
 */
void * load_private_key(char * privkeyfile)
{
    return load_privkey(privkeyfile);
    /*
    RSA *rsa;
    EVP_PKEY* pkey =  load_privkey(privkeyfile, NULL);
    rsa = EVP_PKEY_get1_RSA(pkey);
    EVP_PKEY_free(pkey);
    return rsa;
    */
}

void free_private_key(void *p)
{
    EVP_PKEY *pkey = (EVP_PKEY *)p;
    if (pkey)
	EVP_PKEY_free(pkey);
}

/** \brief 비밀키와 메시지를 사용해 이에 대응하는 서명을 생성.<br>
 *         다음 명령을 실행하는 것과 동일한 결과를 생성함.<br>
 *         "openssl rsautl -in conffile -inkey priv.pem -sign -out sig.txt"
 *  \param privkeyfile 비밀키가 저장된 파일.
 *  \param contents 서명을 생성할 대상이 되는 메시지 혹은 컨텐츠
 *  \param len contents의 길이
 *  \param sig 생성한 서명을 저장할 메모리 공간
 *  \param siglen sig의 크기
 * \return 성공이면 0, 실패면 -1
 */
int gen_signature(char *privkeyfile, uint8_t *contents, size_t len, uint8_t *sig, size_t *siglen)
{
    EVP_PKEY *pkey = NULL;
    int pad = RSA_PKCS1_PADDING;
    int keysize;
    int r = 0, slen;
    RSA *rsa; 
    uint8_t digest[SHA512_DIGEST_LENGTH];
    int digestlen = SHA512_DIGEST_LENGTH;

    pkey = load_privkey(privkeyfile);
    if (NULL == pkey) {
	return -1;
    }

    SHA512(contents, len, digest);

    rsa = EVP_PKEY_get1_RSA(pkey);

    keysize = RSA_size(rsa);
    if (*siglen < keysize) {
	fprintf(stderr, "sig buffer length(%ld) should be > %d\n", *siglen, keysize);
    }
    
    slen = RSA_private_encrypt(digestlen, digest, sig, rsa, pad);
    if (slen < 0) {
	fprintf(stderr, "Error: RSA_private_encrypt, %d\n", r);
	r = -1;
	goto out;
    }
    *siglen = slen;
out:
    if (pkey)
	EVP_PKEY_free(pkey);
    return r;
}

/** \brief load public key from pubkeyfile
 *  \param pubkeyfile c string filename containing public key pubkeyfile
 *  \return void type pointer which points RSA structure.
 */
void * load_public_key(char *pubkeyfile)
{
    RSA *rsa;
    EVP_PKEY *pkey = load_pubkey(pubkeyfile);
    if (!pkey) {
	fprintf(stderr, "Error: load_pubkey()\n");
	return NULL;
    }

    rsa = EVP_PKEY_get1_RSA(pkey);
    EVP_PKEY_free(pkey);
    if (!rsa) {
	fprintf(stderr, "Error: RSA Public Key\n");
    }

    return (void *)rsa;
}

void free_public_key(void *p)
{
    RSA *rsa = (RSA *)p;
    if (rsa) {
	RSA_free(rsa);
    }
}

/** \brief 주어진 공개키와, 컨텐츠 혹은 메시지에 대해 서명의 일치여부를 검증.<br>
 *         다음 명령을 실행하는 효과와 동일함.<br>
 *         "openssl rsautl -in sig.txt -pubin -inkey pub.pem -verify -out sig1.txt"
 *  \param pubkeyfile 공개키 파일
 *  \param contents 검증 대상이 되는 컨텐츠가 있는 메모리의 포인터
 *  \param len 검증 대상 컨텐츠의 길이
 *  \param sig 검증 대상 서명
 *  \param siglen 서명의 길이.
 * \return 성공이면 0, 실패면 -1
 */
int verify_signature(char *pubkeyfile, uint8_t *contents, size_t len, uint8_t *sig, size_t siglen)
{
    RSA *rsa = NULL;
    int pad = RSA_PKCS1_PADDING;
    uint8_t *recovdigest = NULL;
    ssize_t recovlen = 0;
    uint8_t digest[SHA512_DIGEST_LENGTH];
    int digestlen = SHA512_DIGEST_LENGTH;
    int r = 0;

    rsa = (RSA *)load_public_key(pubkeyfile);
    if (NULL == rsa) {
	r = -1;
	goto out;
    }
    recovdigest = (uint8_t *)malloc(digestlen);

    recovlen = RSA_public_decrypt(siglen, sig, recovdigest, rsa, pad);
    if (recovlen < 0) {
	fprintf(stderr, "Error: RSA_public_decrypt, %d\n", recovlen);
	r = -1;
	goto out;
    }

    SHA512(contents, len, digest);
    if (recovlen == digestlen && 0 == memcmp(digest, recovdigest, recovlen)) {
	r = 0;
    } else {
	r = -1;
	goto out;
    }
    memset(recovdigest, 0, recovlen);
out:
    if (recovdigest) 
	free(recovdigest);
    if (rsa)
	RSA_free(rsa);
    return r;
}

/**
 * \brief 주어진 메시지를 공개키로 암호화.
 * \param pubkeyfile 공개키가 저장된 파일.
 * \param plain 암호화할 메시지.
 * \param plainlen 암호화할 메시지의 길이. 245보다 작아야 함.
 * \param cipher 암호문을 저장할 메모리의 주소.
 * \param cipherlen cipher의 크기. RSA2048의 경우 암호문이 256바이트이므로 이보다 커야 함.
 * \return 성공이면 0, 실패면 -1
 */
int public_encrypt(char *pubkeyfile, uint8_t *plain, size_t plainlen, uint8_t *cipher, size_t *cipherlen)
{
    int r = 0;
    RSA *rsa = NULL;
    int pad = RSA_PKCS1_PADDING;
    int rsasize;
    rsa = (RSA *)load_public_key(pubkeyfile);
    if (NULL == rsa) {
	r = -1;
	goto out;
    }

    rsasize = RSA_size(rsa);
    if (plainlen >= rsasize - 11) {
	fprintf(stderr, "too large plaintext: %ld, must be < %d\n",
	    plainlen, rsasize-11);
	r = -1;
	goto out;
    }

    if (*cipherlen < rsasize) {
	fprintf(stderr, "too small memory buffer cipher: %ld, must be >= %d\n",
	    *cipherlen, rsasize);
	r = -1;
	goto out;
    }

    rand_seed();

    r = RSA_public_encrypt(plainlen, plain, cipher, rsa, pad);
    if (r < 0) {
	print_openssl_error();
	goto out;
    } else {
	*cipherlen = r;
    }
out:
    if (rsa)
	RSA_free(rsa);
    return r;
}

/**
 * \brief 공개키로 만든 암호문을 이에 대응하는 비밀키로 해독.
 * \param privkeyfile 비밀키 파일.
 * \param cipher 암호문이 있는 메모리의 주소.
 * \param cipherlen 암호문의 길이. RSA2048의 경우 256바이트라야 함.
 * \param plain 해독 결과를 기록할 메모리의 주소
 * \param plainlen plain의 크기
 * \return 성공이면 0, 실패면 -1
 */
int private_decrypt(char *privkeyfile, uint8_t *cipher, size_t cipherlen, uint8_t *plain, size_t *plainlen)
{
    int r = -1;
    EVP_PKEY *pkey = NULL;
    int pad = RSA_PKCS1_PADDING;
    RSA *rsa;
    int rsasize = 0;

    pkey = load_privkey(privkeyfile);
    if (NULL == pkey) {
	return -1;
    }
    rsa = EVP_PKEY_get1_RSA(pkey);
    rsasize = RSA_size(rsa);

    if (*plainlen < rsasize) {
	fprintf(stderr, "Error: too small buffer plain: %ld must be >= %d\n",
	    *plainlen, rsasize);
	r = -1;
	goto out;
    }

    r = RSA_private_decrypt(cipherlen, cipher, plain, rsa, pad);
    if (r < 0) {
	print_openssl_error();
	goto out;
    } else {
	*plainlen = r;
    }
out:
    if (pkey)
	EVP_PKEY_free(pkey);
    return r;
}

static int fill_x509_req(X509_REQ *req)
{
    int r = 0;
    X509_NAME *x509_name;
    const char *country = "KR";
    const char *province = "Seoul";
    const char *city = "Jung-GU";
    const char *organization = "SKT";
    const char *common = "localhost";

    x509_name = X509_REQ_get_subject_name(req);

    r = X509_NAME_add_entry_by_txt(x509_name, "C", MBSTRING_ASC, (const unsigned char *)country, -1, -1, 0);
    if (r != 1) {
	r = -1;
	goto out;
    }

    r = X509_NAME_add_entry_by_txt(x509_name, "ST", MBSTRING_ASC, (const unsigned char *)province, -1, -1, 0);
    if (r != 1) {
	r = -1;
	goto out;
    }

    r = X509_NAME_add_entry_by_txt(x509_name, "L", MBSTRING_ASC, (const unsigned char *)city, -1, -1, 0);
    if (r != 1) {
	r = -1;
	goto out;
    }

    r = X509_NAME_add_entry_by_txt(x509_name, "O", MBSTRING_ASC, (const unsigned char *)organization, -1, -1, 0);
    if (r != 1) {
	r = -1;
	goto out;
    }

    r = X509_NAME_add_entry_by_txt(x509_name, "CN", MBSTRING_ASC, (const unsigned char *)common, -1, -1, 0);
    if (r != 1) {
	r = -1;
	goto out;
    }

out:
    return r;
}

/**
 * \brief CA에 제출할 CSR(Certificate Signing Request)를 생성.
 * \param privkeyfile CSR을 생성하는데 필요한 비밀키 파일
 * \param reqfile 생성할 CSR파일의 이름.
 * \return 성공이면 0, 실패면 -1
 */
int gen_x509_request(char *privkeyfile, char *reqfile)
{
    int r = -1;
    EVP_PKEY *pkey = NULL;
    X509_REQ *req = NULL;
    X509_NAME *x509_name = NULL;
    BIO *bio_req = NULL;

    pkey = load_privkey(privkeyfile);
    if (NULL == pkey) {
	return -1;
    }

    req = X509_REQ_new();
    r = X509_REQ_set_version(req, 1);
    if (r != 1) {
	r = -1;
	goto out;
    }

    r = fill_x509_req(req);
    if (r < 0) {
	goto out;
    }

    r = X509_REQ_set_pubkey(req, pkey);
    if (r < 0) {
	fprintf(stderr, "Error: X509_REQ_set_pubkey()\n");
	goto out;
    }

    r = X509_REQ_sign(req, pkey, EVP_sha256());
    if (r <= 0) {
	fprintf(stderr, "Error: X509_REQ_sign()\n");
	goto out;
    }

    bio_req = BIO_new_file(reqfile, "w");
    r = PEM_write_bio_X509_REQ(bio_req, req);
    if (r <= 0) {
	fprintf(stderr, "Error: PEM_write_bio_X509_REQ()\n");
	goto out;
    }

out:
    if (bio_req)
	BIO_free_all(bio_req);
    if (pkey)
	EVP_PKEY_free(pkey);
    if (req)
	X509_REQ_free(req);
    return r;
}

/* vi: set ai sw=4 ts=8 sts=4: */
