#ifndef __TPMCRYPTOSUITE_H__
#define __TPMCRYPTOSUITE_H__

int set_default_secret_file(char *filename);
int get_default_secret_file(char *buf, size_t len);

int is_TPM_available(void);
int _gen_private_key_in_TPM(char * privkeyfile);
int get_random_from_TPM(uint8_t *buf, size_t buflen);
void * load_private_key_from_TPM(char * privkeyfile);
int seal_secret(char *filename, uint8_t *secret, size_t buflen);
int unseal_secret(char *filename, uint8_t *secret, size_t buflen);

void * load_private_key(char * privkeyfile);
void free_private_key(void *p);
void * load_public_key(char *pubkeyfile);
void free_public_key(void *p);

void rand_seed(void);

#endif /* __TPMCRYPTOSUITE_H__ */
