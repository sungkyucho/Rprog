#include <stdio.h>
#include <string.h>

#include "CryptoSuite.h"

void usage(const char *exename)
{
    printf("Usage:\n\t%s filename\n\t\tgenerate filename_priv.pem and filename_pub.pem\n\n", exename);
}

int main(int argc, char *argv[])
{
    char privpem[256];
    char pubpem[256];
    char *filename;
    int r;
    if (argc < 2) {
	usage(argv[0]);
	return -1;
    }

    filename = argv[1];
    if (strlen(filename) > 256 - 1 - 9) {
	fprintf(stderr, "Too long filename\n");
	return -1;
    }

    sprintf(privpem, "%s_priv.pem", filename);
    sprintf(pubpem, "%s_pub.pem", filename);

    r = gen_private_key(privpem);
    if (r < 0) {
	fprintf(stderr, "Error: %s\n", privpem);
	return -1;
    }

    r = gen_public_key(privpem, pubpem);
    if (r < 0) {
	fprintf(stderr, "Error: %s\n", pubpem);
	return -1;
    }

    printf("check: %s and %s\n", privpem, pubpem);
    return 0;
}
