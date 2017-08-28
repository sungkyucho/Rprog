#ifndef __CRYPTOSUITE_H__
#define __CRYPTOSUITE_H__

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

int get_random(uint8_t *buf, size_t buflen);

int gen_secret_key(uint8_t *buf, size_t buflen);
int save_secret(char *filename, uint8_t *secret, size_t buflen);
int load_secret(char *filename, uint8_t *secret, size_t buflen);

int sym_encrypt(char *plainbuf, size_t plainlen,
	    char *cipherbuf, size_t *cipherlen, uint8_t *key);
int sym_decrypt(char *cipherbuf, size_t cipherlen,
	    char *plainbuf, size_t *plainlen, uint8_t *key);

void * encrypt_init(uint8_t *key, uint8_t *iv);
int encrypt_update(void * ctx, char *plainbuf, size_t plainlen,
	    char *cipherbuf, size_t *cipherlen);
int encrypt_final(void * ctx, char *cipherbuf, size_t *cipherlen);

void * decrypt_init(uint8_t *key, uint8_t *iv);
int decrypt_update(void * ctx, char *cipherbuf, size_t cipherlen,
 char *plainbuf, size_t *plainlen);
int decrypt_final(void * ctx, char *plainbuf, size_t *plainlen);

int gen_private_key(char * privkeyfile);
int gen_public_key(char * privkeyfile, char *pubkeyfile);

int gen_signature(char *privkeyfile, uint8_t *contents, size_t len, uint8_t *sig, size_t *siglen);
int verify_signature(char *pubkeyfile, uint8_t *contents, size_t len, uint8_t *sig, size_t siglen);
int public_encrypt(char *pubkeyfile, uint8_t *plain, size_t plainlen, uint8_t *cipher, size_t *cipherlen);
int private_decrypt(char *privkeyfile, uint8_t *cipher, size_t cipherlen, uint8_t *plain, size_t *plainlen);

int gen_x509_request(char *privkeyfile, char *reqfile);

int encode_base64(char *target, size_t *tsize, char *source, size_t ssize);
int decode_base64(char *target, size_t *tsize, char *source, size_t ssize);

#endif
/* vi: set ai sw=4 ts=8 sts=4: */
