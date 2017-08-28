#ifndef	_DEBUG_TPM_H_
#define	_DEBUG_TPM_H_

#include <stdio.h>
#include <trousers/tss.h>

//void show_TPM_IDENTITY_REQ(TPM_IDENTITY_REQ *pReq, FILE *lfp);
//void show_TPM_SYMMETRIC_KEY(TPM_SYMMETRIC_KEY *pSkey, FILE *lfp);
//void show_TPM_IDENTITY_PROOF(TPM_IDENTITY_PROOF *proof, FILE *lfp);
void show_TPM_IDENTITY_REQ(TPM_IDENTITY_REQ *pReq, int level);
void show_TPM_SYMMETRIC_KEY(TPM_SYMMETRIC_KEY *pSkey, int level);
void show_TPM_IDENTITY_PROOF(TPM_IDENTITY_PROOF *proof, int level);

#endif /* _DEBUG_TPM_H_ */
