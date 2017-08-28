#ifndef	_RA_LIB_H_
#define	_RA_LIB_H_

#include "GMMP.h"

#define	RA_OK			0x00
#define	RA_BYPASS		0x01
#define	RA_NO_WORKER	0x02 // nobody listen RA request
#define	RA_BAD_CONFIG	0x03 // invalid or no config
#define	RA_BAD_PACKET	0x04 // invalid GMMP packet
#define	RA_UNKNOWN		0xFF

#define NONCE_LEN 20
#define PCR_LIST_LEN 3
#define MAC_LEN 17

int GMMP_Do_RA(GMMPHeader *pstGMMPHeader, stControlReqHdr *pstReqHdr);

int init_RA(char *pca_addr, char *x509_path, int udc_no);
void stop_RA(void);
void print_hex(char *hexstr, int hlen);
void print_hexstr(unsigned char *hexstr, int hlen);

#endif // _RA_LIB_H_
