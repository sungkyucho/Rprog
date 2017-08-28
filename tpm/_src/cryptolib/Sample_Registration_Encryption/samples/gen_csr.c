#include <stdio.h>
#include <string.h>

#include "CryptoSuite.h"

static void usage(char *cmd)
{
    printf("Usage:\n\t%s privkeyfile csrfile\n\n", cmd);
}

int main(int argc, char *argv[])
{
    char privfile[256];
    char reqfile[256];
    int r;

    if (argc < 3) {
	usage(argv[0]);
	return -1;
    }

    if (strlen(argv[1]) > 256 - 1) {
	fprintf(stderr, "Error: file name too long - '%s'\n", argv[1]);
	return -1;
    }
    strncpy(privfile, argv[1], 255);

    if (strlen(argv[2]) > 256 - 1) {
	fprintf(stderr, "Error: flie name too long - '%s'\n", argv[2]);
	return -1;
    }
    strncpy(reqfile, argv[2], 255);

    r = gen_x509_request(privfile, reqfile);
    if (r < 0) {
	fprintf(stderr, "Error: gen_x509_request\n");
	return -1;
    }
    return 0;
}
