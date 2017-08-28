#ifndef _TPM_UTIL_H_
#define _TPM_UTIL_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <tss/tspi.h>
#include <tss/tddl_error.h>
#include <tss/tcs_error.h>

#define TSS_ERROR_CODE(x)	(x & 0xFFF)

#define BACKUP_KEY_UUID {0,0,0,0,0,{0,0,0,0,2,10}}

UINT16 Decode_UINT16(BYTE * in);
UINT32 Decode_UINT32(BYTE * y);

BYTE * Tspi_Native_To_UNICODE(BYTE *string, unsigned *size);
TSS_RESULT Tspi_UnloadBlob_KEY(UINT16 * offset, BYTE * blob, TCPA_KEY * key);
void Tspi_UnloadBlob_TCPA_VERSION(UINT16 * offset, BYTE * blob, TCPA_VERSION * out);
void Tspi_UnloadBlob_UINT16(UINT16 * offset, UINT16 * out, BYTE * blob);
void Tspi_UnloadBlob_UINT32(UINT16 * offset, UINT32 * out, BYTE * blob);
void Tspi_UnloadBlob_KEY_FLAGS(UINT16 * offset, BYTE * blob, TCPA_KEY_FLAGS * flags);
void Tspi_UnloadBlob(UINT16 * offset, UINT32 size, BYTE * container, BYTE * object);
TSS_RESULT Tspi_UnloadBlob_STORE_PUBKEY(UINT16 * offset, BYTE * blob, TCPA_STORE_PUBKEY * store);
TSS_RESULT Tspi_UnloadBlob_KEY_PARMS(UINT16 * offset, BYTE * blob, TCPA_KEY_PARMS * keyParms);

TSS_RESULT load_tpm(TSS_HCONTEXT *hContext, TSS_HTPM *hTPM);
TSS_RESULT connect_load_srk(TSS_HCONTEXT *hContext, TSS_HKEY *hSRK);
#endif
