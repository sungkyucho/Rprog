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

int main(int argc, char *argv[])
{
    char keyfile[256];
    char msgfile[256];
    char encfile[256];
    uint8_t secret[16];
    uint8_t iv[16];
    uint8_t msg[16], enc[16];
    size_t msglen = 16, enclen = 16;
    void *ctx;
    FILE *fpmsg, *fpenc;
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

    r = get_random(iv, 16);
    if (r < 0) {
	fprintf(stderr, "Error: get_random()\n");
	return -1;
    }

    fpmsg = fopen(msgfile, "r");
    if (!fpmsg) {
	perror(msgfile);
	return -1;
    }

    fpenc = fopen(encfile, "w");
    if (!fpenc) {
	perror(encfile);
	return -1;
    }

    ctx = encrypt_init(secret, iv);

    fwrite(iv, 1, 16, fpenc);

    do {
	msglen = fread(msg, 1, 16, fpmsg);
	enclen = msglen;
	r = encrypt_update(ctx, msg, msglen, enc, &enclen);
	if (r < 0) {
	    fprintf(stderr, "Error: encrypt_update()\n");
	    exit(-1);
	}
	fwrite(enc, 1, enclen, fpenc);
    } while(msglen == 16);

    enclen = 16;
    r = encrypt_final(ctx, enc, &enclen);
    if (r < 0) {
	fprintf(stderr, "Error: encrypt_final()\n");
	exit(-1);
    }

    if (enclen > 0) {
	fwrite(enc, 1, enclen, fpenc);
    }

    fclose(fpmsg);
    fclose(fpenc);

    printf("check %s\n\n", encfile);

    return 0;
}
