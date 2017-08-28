#include "CryptoSuite.h"
#include "TPMCryptoSuite.h"

/* for random() */
#include <stdlib.h>

/* for open(), stat() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for read(), stat() */
#include <unistd.h>

/* for perror(), fwrite(), fopen() */
#include <stdio.h>

/* for strncpy() */
#include <string.h>

/* for chmod() */
#include <sys/stat.h>

/* for getifaddrs() */
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <ifaddrs.h>

/* for SHA() */
#include <openssl/sha.h>

#include <errno.h>

/** \brief generate random number
 *  \param buf buffer to store generated random number
 *  \param buflen size of buf
 */
int get_random(uint8_t *buf, size_t buflen)
{
    ssize_t r = 0, rdlen = 0;
    if (buflen <= 0 || NULL == buf) {
	return -1;
    }

    if (is_TPM_available()) {
	return get_random_from_TPM(buf, buflen);
    }

    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
	return -1;
    }
    while(rdlen < buflen) {
	r = read(fd, &buf[rdlen], buflen-rdlen);
	if (r < 0) {
	    perror("read");
	    goto out;
	}
	rdlen += r;
    }

    r = 0;
out:
    close(fd);
    return r;
}

void rand_seed(void)
{
    static int initialized = 0;
    int r = 0;
    uint8_t buf[32];
    if (initialized)
	return;
    initialized = 1;
    r = get_random(buf, 32);
    RAND_seed(buf, 32);
}

/**
 * @brief generate random number to be used as secret key
 * @param buf buffer to store generated random number
 * @param buflen length of buf
 * @return 0 on Success, < 0 on error
 */
int gen_secret_key(uint8_t *buf, size_t buflen)
{
    return get_random(buf, buflen);
}

#define KEYFILE_DEFAULT "secret.key.blob"
#define MAX_KEYFILENAME 255
#define KEYFILENAMEBUFLEN 256
static char defaultKeyFile[KEYFILENAMEBUFLEN] = KEYFILE_DEFAULT;

static int makeFileWritable(char *filename)
{
    int r;
    struct stat statbuf;

    r = stat(filename, &statbuf);
    if (r < 0) {
	if (errno == ENOENT) {
	    return 0;
	}
	return -1;
    }

    r = chmod(filename, statbuf.st_mode | S_IWUSR);
    if (r < 0) {
	perror("chmod");
	return -1;
    }
    return 0;

}

static void get_systemmac(uint8_t *buf, int buflen)
{
    struct ifaddrs *addrs,*tmp;

    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp)
    {
	struct sockaddr_ll *sll;
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET &&
	    (strncmp(tmp->ifa_name, "wlan", 4) == 0 || strncmp(tmp->ifa_name, "eth", 3) == 0)) {
	    sll = (struct sockaddr_ll *)tmp->ifa_addr;
	    memcpy(buf, sll->sll_addr,
		buflen < sll->sll_halen ? buflen : sll->sll_halen);
	    break;
	}  

	tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
}

/*
 * assumption: get_systemmac() returns always the same interface's address.
 */
static int make_system_secret(uint8_t *systemid, size_t len)
{
    uint8_t mac[8] = {0,};
    if (len < SHA512_DIGEST_LENGTH) {
	perror("make_system_secret(): small buffer");
	return -1;
    }
    get_systemmac(mac, sizeof(mac));
    SHA512(mac, sizeof(mac), systemid);
    return 0;
}

static int seal_with_systeminfo(char *filename, uint8_t *secret, size_t buflen)
{
    FILE *fp;
    char *fname = filename;
    int r = 0;
    uint8_t systemid[SHA512_DIGEST_LENGTH] = {0,};
    uint8_t cipherbuf[256];
    size_t cipherlen = sizeof(cipherbuf);

    fp = fopen(fname, "w");
    if (!fp) {
	perror("fopen");
	return -1;
    }

    make_system_secret(systemid, sizeof(systemid));

    r = sym_encrypt(secret, buflen, cipherbuf, &cipherlen, systemid);
    if (r < 0) {
	perror("sym_encrypt");
	goto out;
    }

    r = fwrite(cipherbuf, sizeof(uint8_t), cipherlen, fp);
    if (r < 0) {
	perror("fwrite");
	goto out;
    } else if (r < buflen) {
	fprintf(stderr, "only parts of bytes written: (%d/%ld)?\n", r, buflen);
	r = -1;
	goto out;
    }
    fclose(fp);
    fp = NULL;
out:
    if (fp)
	fclose(fp);
    return r;
}

/**
 * @brief 대칭키를 파일에 저장. TPM이 있으면 이를 이용해 Seal 연산을 한 다음
 *        저장하고 TPM이 없으면 시스템의 정보를 이용해 키를 암호화 하여 저장함.<br>
 *        파일의 권한은 사용자에게만 읽기 허용.
 * @return 0 on success, -1 otherwize
 * @param filename filename to store secret key
 * @param secret buffer with secret
 * @param buflen the length of buffer, secret
 */
int save_secret(char *filename, uint8_t *secret, size_t buflen)
{
    char *fname = filename;
    int r;
    if (NULL == secret || buflen <= 0) {
	return -1;
    }

    if (!fname) {
	fname = defaultKeyFile;
    }

    if (makeFileWritable(fname) < 0) {
	perror("makeFileWritable");
	return -1;
    }

    if (is_TPM_available()) {
	r = seal_secret(fname, secret, buflen);
	if (r < 0) {
	    fprintf(stderr, "Error: seal_secret\n");
	    goto out;
	}
    } else {
	r = seal_with_systeminfo(fname, secret, buflen);
	if (r < 0) {
	    fprintf(stderr, "Error: seal_with_systeminfo\n");
	    goto out;
	}
    }

    r = chmod(fname, S_IFREG | S_IRUSR);
    if (r < 0) {
	perror("fchmod");
	goto out;
    }
    r = 0;
out:
    return r;
}

static int unseal_with_systeminfo(char *filename, uint8_t *secret, size_t buflen)
{
    int r = 0;
    char *fname = filename;
    FILE *fp;
    uint8_t systemid[SHA512_DIGEST_LENGTH] = {0,};
    uint8_t cipherbuf[256];
    size_t cipherlen = sizeof(cipherbuf);

    fp = fopen(fname, "r");
    if (!fp) {
	perror("fopen");
	return -1;
    }

    r = fread(cipherbuf, sizeof(uint8_t), cipherlen, fp);
    if (r < 0) {
	perror("fread");
	goto out;
    }
    fclose(fp);
    fp = NULL;

    make_system_secret(systemid, sizeof(systemid));

    r = sym_decrypt(cipherbuf, cipherlen, secret, &buflen, systemid);
    if (r < 0) {
	perror("sym_decrypt");
	goto out;
    }

out:
    if (fp) {
	fclose(fp);
    }
    return 0;
}

/**
 * @brief 파일에서 대칭키를 읽어들임. TPM의 존재 여부에 따라 unseal 혹은 
 *        시스템의 정보를 대칭키로 삼아 해독실행.
 * @param filename filename to read secret key from
 * @param secret buffer to store secret key.
 * @param buflen buffer length
 * @return 0 on success, -1 otherwize
 */
int load_secret(char *filename, uint8_t *secret, size_t buflen)
{
    char *fname = filename;
    struct stat statbuf;
    int r = 0;

    if (!secret || 0 == buflen) {
	return -1;
    }

    if (!fname) {
	fname = defaultKeyFile;
    }

    if (stat(fname, &statbuf) < 0) {
	if (errno == ENOENT) {
	    return 0;
	}
	return -1;
    }

    if (is_TPM_available() && statbuf.st_size > 16) {
	return unseal_secret(fname, secret, buflen);
    }

    return unseal_with_systeminfo(fname, secret, buflen);
}

/**
 * @brief This function sets the default file name for key storage.
 * @param filename file location of key storage
 * @return on success 1, if not -1
 */
int set_default_secret_file(char *filename)
{
    if (!filename) {
	strncpy(defaultKeyFile, KEYFILE_DEFAULT, MAX_KEYFILENAME);
    } else {
	if (strlen(filename) > MAX_KEYFILENAME)
	    return -1;
	strncpy(defaultKeyFile, filename, MAX_KEYFILENAME);
	defaultKeyFile[KEYFILENAMEBUFLEN] = 0;
    }
    return 0;
}

/**
 * @brief return default location of secret file
 * @param buf buffer to store file name
 * @param len sizeof of buf
 * @return 0 on success, -1 otherwize
 */
int get_default_secret_file(char *buf, size_t len)
{
    if (!buf || 0 == len) {
	return -1;
    }
    strncpy(buf, defaultKeyFile, len);
    return  0;
}

/* vi: set ai sw=4 ts=8 sts=4: */
