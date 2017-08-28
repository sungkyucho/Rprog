#ifndef _VERIFY_DATA_H_
#define _VERIFY_DATA_H_
#include <tss/tspi.h> // BYTE

int verify_pcr_nonce(unsigned char * pcrComposite, BYTE* nonce);
int verify_pcr_digest(unsigned char * pcrComposite, unsigned char * pcrAnswer);
int verify_pcr_composite(unsigned char * pcrComposite, int pcrCompositeSize, unsigned char *keyPub, int keyPubSize, unsigned char * signedPCR, int signedPCRSize);
#endif // _VERIFY_DATA_H_
