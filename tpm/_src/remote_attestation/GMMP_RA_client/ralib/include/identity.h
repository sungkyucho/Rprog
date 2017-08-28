#ifndef _IDENTITY_H_
#define _IDENTITY_H_

TSS_RESULT get_pca_rsa(char *privacy_ca, BYTE *rsa_n[2048], int *);
TSS_RESULT set_identkey_policy(TSS_HCONTEXT hContext, TSS_HKEY *hIdentKey, char *aikpass);
int request_to_pca(BYTE **symBuf, UINT32 *, char *, BYTE *, UINT32, int);
TSS_RESULT get_create_credential(TSS_HCONTEXT *hContext, TSS_HTPM *hTPM, TSS_HKEY *hSRK, char *, X509 **);

TSS_RESULT is_x509_valid_period(X509 *x509);
#endif
