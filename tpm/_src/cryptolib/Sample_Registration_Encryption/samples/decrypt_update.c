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

int main(int argc, char *argv[])
{
    char keyfile[256];
    char encfile[256];
    char decfile[256];
    uint8_t secret[16], iv[16];
    uint8_t dec[16], enc[16];
    size_t declen = 16, enclen = 16;
    FILE *fpenc, *fpdec;
    void *ctx;
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

    fpenc = fopen(encfile, "r");
    if (!fpenc) {
	perror(encfile);
	return -1;
    }

    r = fread(iv, 1, 16, fpenc);
    if (r < 16) {
	fprintf(stderr, "Error: not enough iv\n");
	return -1;
    }

    fpdec = fopen(decfile, "w");
    if (!fpdec) {
	perror(decfile);
	return -1;
    }

    ctx = decrypt_init(secret, iv);

    do {
	enclen = fread(enc, 1, 16, fpenc);
	declen = enclen;
	decrypt_update(ctx, enc, enclen, dec, &declen);
	fwrite(dec, 1, declen, fpdec);
    } while(enclen == 16);

    declen = 16;
    decrypt_final(ctx, dec, &declen);
    if (declen > 0) {
	fwrite(dec, 1, declen, fpdec);
    }

    fclose(fpenc);
    fclose(fpdec);

    printf("check %s\n\n", decfile);

    return 0;
}
