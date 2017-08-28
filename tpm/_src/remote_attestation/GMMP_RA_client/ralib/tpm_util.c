#include <stdio.h>
#include <iconv.h>
#include <stdlib.h>
#include <errno.h>
#include <langinfo.h>
#include "tpm_util.h"
#include "dbg_macros.h"

#define MAX_BUF	4096

/*
 * @brief Create TPM handler
 *        set TPM SRK 
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param hContext TPM handler 
 * @param hSRK TPM SRK handler
 */
TSS_RESULT
connect_load_srk(TSS_HCONTEXT *hContext, TSS_HKEY *hSRK)
{
	TSS_RESULT result;
	TSS_HPOLICY	hSrkPolicy;
	BYTE secret[] = TSS_WELL_KNOWN_SECRET;
	TSS_UUID	SRK_UUID = TSS_UUID_SRK;

	result = Tspi_Context_Create(hContext);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_Context_Create\n", result);
		dbg_printf(DERROR, "Tspi_Context_Create: %s\n", Trspi_Error_String(result));
//		exit(result);
		return result;
	}
	result = Tspi_Context_Connect(*hContext, NULL);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_Context_Connect\n", result);
		dbg_printf(DERROR, "Tspi_Context_Connect: %s\n", Trspi_Error_String(result));
//		exit(result);
		Tspi_Context_Close( *hContext );
		return result;
	}
	result = Tspi_Context_LoadKeyByUUID(*hContext,
			TSS_PS_TYPE_SYSTEM, SRK_UUID, hSRK);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_Context_LoadKeyByUUID for SRK\n", result);
		dbg_printf(DERROR, "Tspi_Context_LoadKeyByUUID : %s\n", Trspi_Error_String(result));
//		exit(result);
		Tspi_Context_Close(*hContext);
		return result;
	}
	result = Tspi_GetPolicyObject(*hSRK, TSS_POLICY_USAGE, &hSrkPolicy);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_GetPolicyObject for SRK\n", result);
		dbg_printf(DERROR, "Tspi_GetPolicyObject for SRK: %s\n", Trspi_Error_String(result));
//		exit(result);
		Tspi_Context_Close(*hContext);
		return result;
	}
	result = Tspi_Policy_SetSecret(hSrkPolicy, TSS_SECRET_MODE_SHA1, 20, secret);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_Policy_SetSecret for SRK: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on Tspi_Policy_SetSecret for SRK\n", result);
//		exit(result);
		Tspi_Context_Close(*hContext);
	}
	return result;
}

/*
 * @brief set TPM object and set secret
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param hContext TPM handler 
 * @param hTPM TPM object handler 
 */
TSS_RESULT
load_tpm(TSS_HCONTEXT *hContext, TSS_HTPM *hTPM)
{
	TSS_RESULT result;
	BYTE secret[] = TSS_WELL_KNOWN_SECRET;
	TSS_HPOLICY	hTPMPolicy;

	result = Tspi_Context_GetTpmObject (*hContext, hTPM);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_Context_GetTpmObject\n", result);
		dbg_printf(DERROR, "Tspi_Context_GetTpmObject : %s\n", Trspi_Error_String(result));
		return result;
	}
	result = Tspi_Context_CreateObject(*hContext, TSS_OBJECT_TYPE_POLICY,
			TSS_POLICY_USAGE, &hTPMPolicy);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_CreateObject for TPM policy: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on Tspi_CreateObject for TPM policy\n", result);
		return result;
	}
	result = Tspi_Policy_AssignToObject(hTPMPolicy, *hTPM);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_Policy_AssignToObject for TPM: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on Tspi_Policy_AssignToObject for TPM\n", result);
		return result;
	}
	result = Tspi_Policy_SetSecret(hTPMPolicy, TSS_SECRET_MODE_SHA1, 20, secret);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_Policy_SetSecret for TPM: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on Tspi_Policy_SetSecret for TPM\n", result);
	}
	return result;
}

TSS_RESULT
Tspi_UnloadBlob_KEY(UINT16 * offset, BYTE * blob, TCPA_KEY * key)
{
	TSS_RESULT result;

	Tspi_UnloadBlob_TCPA_VERSION(offset, blob, &key->ver);
	Tspi_UnloadBlob_UINT16(offset, &key->keyUsage, blob);
	Tspi_UnloadBlob_KEY_FLAGS(offset, blob, &key->keyFlags);
	key->authDataUsage = blob[(*offset)++];
	if ((result = Tspi_UnloadBlob_KEY_PARMS(offset, (BYTE *) blob, &key->algorithmParms)))
		return result;
	Tspi_UnloadBlob_UINT32(offset, &key->PCRInfoSize, blob);

	if (key->PCRInfoSize > 0) {
		key->PCRInfo = malloc(key->PCRInfoSize);
		if (key->PCRInfo == NULL) {
			fprintf(stderr, "malloc of %d bytes failed.", key->PCRInfoSize);
			return TSS_E_OUTOFMEMORY;
		}
		Tspi_UnloadBlob(offset, key->PCRInfoSize, blob, key->PCRInfo);
	} else {
		key->PCRInfo = NULL;
	}

	if ((result = Tspi_UnloadBlob_STORE_PUBKEY(offset, blob, &key->pubKey)))
		return result;
	Tspi_UnloadBlob_UINT32(offset, &key->encSize, blob);

	if (key->encSize > 0) {
		key->encData = malloc(key->encSize);
		if (key->encData == NULL) {
			fprintf(stderr, "malloc of %d bytes failed.", key->encSize);
			return TSS_E_OUTOFMEMORY;
		}
		Tspi_UnloadBlob(offset, key->encSize, blob, key->encData);
	} else {
		key->encData = NULL;
	}

	return result;
}

void
Tspi_UnloadBlob_TCPA_VERSION(UINT16 * offset, BYTE * blob, TCPA_VERSION * out)
{
	out->major = blob[(*offset)++];
	out->minor = blob[(*offset)++];
	out->revMajor = blob[(*offset)++];
	out->revMinor = blob[(*offset)++];
}

UINT16 Decode_UINT16(BYTE * in)
{
	UINT16 x = 0;
	x = (in[1] & 0xFF);
	x |= (in[0] << 8);
	return x;
}

UINT32 Decode_UINT32(BYTE * y)
{
	UINT32 x = 0;
	x = y[0];
	x = ((x << 8) | (y[1] & 0xFF));
	x = ((x << 8) | (y[2] & 0xFF));
	x = ((x << 8) | (y[3] & 0xFF));

	return x;
}

void
Tspi_UnloadBlob_UINT16(UINT16 * offset, UINT16 * out, BYTE * blob)
{
	*out = Decode_UINT16(&blob[*offset]);
	*offset += sizeof(UINT16);
}
void
Tspi_UnloadBlob_UINT32(UINT16 * offset, UINT32 * out, BYTE * blob)
{
	*out = Decode_UINT32(&blob[*offset]);
	*offset += sizeof(UINT32);
}
void
Tspi_UnloadBlob_KEY_FLAGS(UINT16 * offset, BYTE * blob, TCPA_KEY_FLAGS * flags)
{
        UINT32 tempFlag = 0;
        memset(flags, 0x00, sizeof(TCPA_KEY_FLAGS));

        Tspi_UnloadBlob_UINT32(offset, &tempFlag, blob);

        if (tempFlag & redirection)
                *flags |= redirection;
        if (tempFlag & migratable)
                *flags |= migratable;
        if (tempFlag & volatileKey)
                *flags |= volatileKey;
}

TSS_RESULT Tspi_UnloadBlob_KEY_PARMS(UINT16 * offset, BYTE * blob, TCPA_KEY_PARMS * keyParms)
{
	Tspi_UnloadBlob_UINT32(offset, &keyParms->algorithmID, blob);
	Tspi_UnloadBlob_UINT16(offset, &keyParms->encScheme, blob);
	Tspi_UnloadBlob_UINT16(offset, &keyParms->sigScheme, blob);
	Tspi_UnloadBlob_UINT32(offset, &keyParms->parmSize, blob);

	if (keyParms->parmSize > 0) {
		keyParms->parms = malloc(keyParms->parmSize);
		if (keyParms->parms == NULL) {
			fprintf(stderr, "malloc of %d bytes failed.", keyParms->parmSize);
			return TSS_E_OUTOFMEMORY;
		}
		Tspi_UnloadBlob(offset, keyParms->parmSize, blob, keyParms->parms);
	} else {
		keyParms->parms = NULL;
	}

	return TSS_SUCCESS;
}
void Tspi_UnloadBlob(UINT16 * offset, UINT32 size, BYTE * container, BYTE * object)
{
	if (size == 0)
		return;
	memcpy(object, &container[(*offset)], size);
	(*offset) += (UINT16) size;
}

TSS_RESULT
Tspi_UnloadBlob_STORE_PUBKEY(UINT16 * offset, BYTE * blob, TCPA_STORE_PUBKEY * store)
{
	Tspi_UnloadBlob_UINT32(offset, &store->keyLength, blob);

	if (store->keyLength > 0) {
		store->key = malloc(store->keyLength);
		if (store->key == NULL) {
			fprintf(stderr, "malloc of %d bytes failed.", store->keyLength);
			return TSS_E_OUTOFMEMORY;
		}
		Tspi_UnloadBlob(offset, store->keyLength, blob, store->key);
	} else {
		store->key = NULL;
	}

	return TSS_SUCCESS;
}

static int
hacky_strlen(char *codeset, BYTE *string)
{
	BYTE *ptr = string;
	int len = 0;

	if (strncmp("UTF-16", codeset, 6) == 0) {
		while (!(ptr[0] == '\0' && ptr[1] == '\0')) {
			len += 2;
			ptr += 2;
		}
	} else if (strncmp("UTF-32", codeset, 6) == 0) {
		while (!(ptr[0] == '\0' && ptr[1] == '\0' &&
			 ptr[2] == '\0' && ptr[3] == '\0')) {
			len += 4;
			ptr += 4;
		}
	} else {
		/* default to 8bit chars */
		while (*ptr++ != '\0') {
			len++;
		}
	}

	return len;
}

static inline int
char_width(char *codeset)
{
	if (strncmp("UTF-16", codeset, 6) == 0) {
		return 2;
	} else if (strncmp("UTF-32", codeset, 6) == 0) {
		return 4;
	}

	return 1;
}

BYTE *
Tspi_Native_To_UNICODE(BYTE *string, unsigned *size)
{
	char *ptr, *ret, *outbuf, tmpbuf[MAX_BUF] = { 0, };
	unsigned len = 0, tmplen;
	iconv_t cd = 0;
	size_t rc, outbytesleft, inbytesleft;

	if (string == NULL)
		goto alloc_string;

//	if ((cd = iconv_open("UTF-16LE", nl_langinfo(CODESET))) == (iconv_t)-1) {
	if ((cd = iconv_open("UTF-8", nl_langinfo(CODESET))) == (iconv_t)-1) {
		fprintf(stderr, "iconv_open: %s", strerror(errno));
		return NULL;
	}

	if ((tmplen = hacky_strlen(nl_langinfo(CODESET), string)) == 0) {
		fprintf(stderr, "hacky_strlen returned 0");
		goto alloc_string;
	}

	do {
		len++;
		outbytesleft = len;
		inbytesleft = tmplen;
		outbuf = tmpbuf;
		ptr = (char *)string;
		errno = 0;

		rc = iconv(cd, &ptr, &inbytesleft, &outbuf, &outbytesleft);
	} while (rc == (size_t)-1 && errno == E2BIG);

	if (len > MAX_BUF) {
		fprintf(stderr, "string too long.");
		iconv_close(cd);
		return NULL;
	}

alloc_string:
	/* add terminating bytes of the correct width */
	len += char_width("UTF-16");
	if ((ret = calloc(1, len)) == NULL) {
		fprintf(stderr, "malloc of %u bytes failed.", len);
		iconv_close(cd);
		return NULL;
	}

	memcpy(ret, &tmpbuf, len);
	if (size)
		*size = len;

	if (cd)
		iconv_close(cd);

	return (BYTE *)ret;

}

