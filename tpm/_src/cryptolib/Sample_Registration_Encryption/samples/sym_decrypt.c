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
    printf("Usage:\n\t%s secretkeyfile encryptedfile decryptedfile\n\n", cmd);
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
    char encfile[256];
    char decfile[256];
    uint8_t secret[16];
    uint8_t *dec, *enc;
    size_t declen, enclen;
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
    strncpy(encfile, argv[2], 255);

    if (strlen(argv[3]) > 256 - 1) {
	fprintf(stderr, "Error: file name too long: '%s'\n", argv[3]);
	return -1;
    }
    strncpy(decfile, argv[3], 255);

    r = load_secret(keyfile, secret, 16);
    if (r < 0) {
	fprintf(stderr, "Error: load_secret()\n");
	return -1;
    }

    enc = NULL;
    r = read_whole_file(encfile, &enc, &enclen);
    if (r < 0) {
	fprintf(stderr, "Error: read_whole_file()\n");
	return -1;
    }

    dec = (uint8_t *)malloc(enclen);
    declen = enclen;
    r = sym_decrypt(enc, enclen, dec, &declen, secret);
    if (r < 0) {
	fprintf(stderr, "Error: sym_decrypt()\n");
	return -1;
    }

    write_to_file(decfile, dec, declen);

    free(enc);
    free(dec);

    printf("check %s\n\n", decfile);

    return 0;
}
