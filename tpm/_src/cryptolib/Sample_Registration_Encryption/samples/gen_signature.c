#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "CryptoSuite.h"

static void usage(char *cmd)
{
    printf("Usage\n\t%s privatekeyfile messagefile signaturefile\n\n", cmd);
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
    char privkeyfile[256];
    char msgfile[256];
    char sigfile[256];
    char sig[256];
    size_t siglen = 256;
    uint8_t *msg;
    size_t msglen = 0;
    int i;
    int r;

    if (argc < 4) {
	usage(argv[0]);
	return -1;
    }

    for (i=1; i<4; ++i) {
	if (strlen(argv[i]) > 256 - 1) {
	    fprintf(stderr, "Error: too long file name %s\n", argv[1]);
	    return -1;
	}
    }
    strncpy(privkeyfile, argv[1], 255);
    strncpy(msgfile, argv[2], 255);
    strncpy(sigfile, argv[3], 255);

    r = read_whole_file(msgfile, &msg, &msglen);
    if (r < 0) {
	fprintf(stderr, "Error: read_whole_file()\n");
	return -1;
    }

    r = gen_signature(privkeyfile, msg, msglen, sig, &siglen);
    if (r < 0) {
	fprintf(stderr, "Error: gen_signature()\n");
	return -1;
    }

    write_to_file(sigfile, sig, siglen);

    free(msg);
    printf("Check %s\n\n", sigfile);

    return 0;
}
