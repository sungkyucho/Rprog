#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "CryptoSuite.h"

static void usage(char *cmd)
{
    printf("Usage:\n\t%s secretkeyfile messagefile encryptedfile\n\n", cmd);
}

static int file_not_exist(char *file)
{
    struct stat statbuf;
    int r;
    r = stat(file, &statbuf);
    if (r < 0 && errno == ENOENT) {
	return 1;
    }
    return 0;
}

static void write_to_file(char *encfile, uint8_t *enc, int enclen)
{
    FILE *fp;
    fp = fopen(encfile, "w");
    if (!fp) {
	perror(encfile);
	return;
    }

    fwrite(enc, 1, enclen, fp);
    fclose(fp);
}

int read_whole_file(char *msgfile, uint8_t **msg, size_t *msglen)
{
    FILE *fp;
    struct stat statbuf;
    int r;

    r = stat(msgfile, &statbuf);
    if (r < 0) {
	perror(msgfile);
	return -1;
    }

    *msglen = statbuf.st_size;
    *msg = (uint8_t *)malloc(*msglen);

    fp = fopen(msgfile, "r");
    if (!fp) {
	perror(msgfile);
	return -1;
    }

    r = fread(*msg, 1, *msglen, fp);
    if (r != *msglen) {
	perror("fread");
	return -1;
    }
    fclose(fp);

    return 0;
}

int main(int argc, char *argv[])
{
    char keyfile[256];
    char msgfile[256];
    char encfile[256];
    uint8_t secret[16];
    uint8_t *msg, *enc;
    size_t msglen, enclen;
    int r;
    if (argc < 4) {
	usage(argv[0]);
	return -1;
    }

    /* check arguments */
    if (strlen(argv[1]) > 256 - 1) {
	fprintf(stderr, "Error: file name too long: '%s'\n", argv[1]);
	return -1;
    }
    strncpy(keyfile, argv[1], 255);

    if (strlen(argv[2]) > 256 - 1) {
	fprintf(stderr, "Error: file name too long: '%s'\n", argv[2]);
	return -1;
    }
    strncpy(msgfile, argv[2], 255);

    if (strlen(argv[3]) > 256 - 1) {
	fprintf(stderr, "Error: file name too long: '%s'\n", argv[3]);
	return -1;
    }
    strncpy(encfile, argv[3], 255);

    /* fill secret key buffer */
    if (file_not_exist(keyfile)) {
	r = gen_secret_key(secret, 16);
	if (r < 0) {
	    fprintf(stderr, "Error: gen_secret_key()\n");
	    return -1;
	}
	
	r = save_secret(keyfile, secret, 16);
	if (r < 0) {
	    fprintf(stderr, "Error: save_secret()\n");
	    return -1;
	}
    } else {	/* key file exists */
	r = load_secret(keyfile, secret, 16);
	if (r < 0) {
	    fprintf(stderr, "Error: load_secret()\n");
	    return -1;
	}
    }

    msg = NULL;
    r = read_whole_file(msgfile, &msg, &msglen);
    if (r < 0) {
	fprintf(stderr, "Error: read_whole_file()\n");
	return -1;
    }

    enclen = msglen + 16;
    enc = (uint8_t *)malloc(enclen);
    r = sym_encrypt(msg, msglen, enc, &enclen, secret);
    if (r < 0) {
	fprintf(stderr, "Error: sym_encrypt()\n");
	return -1;
    }

    write_to_file(encfile, enc, enclen);

    free(enc);
    free(msg);

    printf("check %s\n\n", encfile);

    return 0;
}
