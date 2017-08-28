#include <tss/tspi.h> 
  
/*
typedef struct {
	BYTE tpmVersion[4]; 
	BYTE tpmCmd[4];
	BYTE pcr_digest[20];
	BYTE nonce[20];
} pcrComposite_data;

typedef struct {
	unsigned long pcrCompositeSize; //Example field
	unsigned long signedPCRSize;
	unsigned long keyPubSize; // no longer used?
	unsigned long credLen;  // no longer used?
	pcrComposite_data * pcrComposite;
	unsigned char * signedPCR;
	unsigned char * keyPub; // no longer used?
	unsigned char * cred;  //no longer used?
	BYTE sizeOfSeclect; //SRTM is 2 , DRTM is 3
	BYTE pcrSelect[2];
	int selectpcrSize;
} remote_verifier_data;
*/

TSS_RESULT remote_attestation(BYTE * nonce_from_client, int sizeofclientnonce,
		BYTE **receive_msg, int * receive_msg_size, char *, BYTE*, const char *);
//		remote_verifier_data** remote_data, int * remote_data_size, char *, BYTE*);


