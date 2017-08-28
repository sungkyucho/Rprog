#include "CryptoSuite.h"
#include "TPMCryptoSuite.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>

#define print_error(a, b) \
	fprintf(stderr, "%s:%d %s result: 0x%x (%s)\n", __FILE__, __LINE__, \
			a, b, Trspi_Error_String(b))
#define TPM_OFF_FORCED 1
#define TPM_ON_FORCED 2
static int tpm_off = 0;

/**
 * just for testing purpose
 */
void setTPMOff(void) { tpm_off = TPM_OFF_FORCED; }
void setTPMOn(void) { tpm_off = TPM_ON_FORCED; }

TSS_RESULT contextCreateAndConnect(TSS_HCONTEXT * a_hContext)
{
    TSS_RESULT result = Tspi_Context_Create(a_hContext);
    if (result) {
	print_error("Tspi_Context_Create", result);
	return result;
    }
    result = Tspi_Context_Connect(*a_hContext, NULL);
    if (result) {
	print_error("Tspi_Context_Connect", result);
	return result;
    }

    return result;
}

TSS_RESULT contextClose(TSS_HCONTEXT a_hContext)
{

    TSS_RESULT result = Tspi_Context_FreeMemory(a_hContext, NULL);
    if (result)
	print_error("Tspi_Context_FreeMemory", result);

    result = Tspi_Context_Close(a_hContext);
    if (result)
	print_error("Tspi_Context_Close", result);

    return result;
}

int is_TPM_available(void)
{
    struct stat statbuf;

    if (tpm_off == TPM_OFF_FORCED)
	return 0;
    else if (tpm_off == TPM_ON_FORCED)
	return 1;

    int r = stat("/dev/tpm0", &statbuf);
    if (r < 0) {
	return 0;
    }
    if (S_ISCHR(statbuf.st_mode)) {
	return 1;
    }
    return 0;
}

/** \brief generate random number using TPM
 *  \param buf buffer to store generated random number
 *  \param buflen size of buf
 */
int get_random_from_TPM(uint8_t *buf, size_t buflen)
{
    TSS_HCONTEXT hContext = 0;
    TSS_HTPM hTpm;
    TSS_RESULT result;
    BYTE *p = NULL;

    if (contextCreateAndConnect(&hContext) != TSS_SUCCESS) {
	hContext = 0;
	goto out;
    }
    if ((result = Tspi_Context_GetTpmObject(hContext, &hTpm)) != TSS_SUCCESS) {
	print_error("Tspi_Context_GetTpmObject", result);
	goto out;
    }
    if ((result = Tspi_TPM_GetRandom(hTpm, buflen, &p)) != TSS_SUCCESS) {
	print_error("Tspi_TPM_GetRandom", result);
	goto out;
    }
    memcpy(buf, p, buflen);
out:
    if (p)
	Tspi_Context_FreeMemory(hContext, p);
    if (hContext)
	contextClose(hContext);
}

/** \brief generate private key via TPM.
 *	   key spec: RSA, 2048, no encryption.
 *         the result is private key file.
 *  \param privkeyfile filename to store private key in.
 *  \return 0 on success, -1 otherwise
 */
int _gen_private_key_in_TPM(char * privkeyfile)
{
	TSS_HCONTEXT	hContext;
	TSS_FLAG	initFlags = TSS_KEY_TYPE_LEGACY | TSS_KEY_VOLATILE;
        TSS_HKEY        hKey;
        TSS_HKEY        hSRK;
        TSS_RESULT      result;
        TSS_HPOLICY     srkUsagePolicy, keyUsagePolicy, keyMigrationPolicy;
        BYTE            *blob;
        UINT32          blob_size; //, srk_authusage;
        BIO             *outb;
        ASN1_OCTET_STRING *blob_str;
        unsigned char   *blob_asn1 = NULL;
        int             asn1_len;
        char            c, *openssl_key = NULL;
        int             option_index, auth = 0, popup = 0, wrap = 0, zero=0;
        UINT32          enc_scheme = TSS_ES_RSAESPKCSV15;
        UINT32          sig_scheme = TSS_SS_RSASSAPKCS1V15_DER;
        UINT32          key_size = 2048;
        RSA             *rsa;
	TSS_UUID SRK_UUID = TSS_UUID_SRK;
	
	//Create Context
	if (contextCreateAndConnect(&hContext) != TSS_SUCCESS) {
		exit(result);
	}

	//Create Object
	if ((result = Tspi_Context_CreateObject(hContext,
						TSS_OBJECT_TYPE_RSAKEY,
						initFlags, &hKey))) {
		print_error("Tspi_Context_CreateObject", result);
		contextClose(hContext);
		exit(result);
	}

	if ((result = Tspi_SetAttribUint32(hKey, TSS_TSPATTRIB_KEY_INFO,
					   TSS_TSPATTRIB_KEYINFO_SIGSCHEME,
					   sig_scheme))) {
		print_error("Tspi_SetAttribUint32", result);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(result);
	}

	if ((result = Tspi_SetAttribUint32(hKey, TSS_TSPATTRIB_KEY_INFO,
					   TSS_TSPATTRIB_KEYINFO_ENCSCHEME,
					   enc_scheme))) {
		print_error("Tspi_SetAttribUint32", result);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(result);
	}

	//Load Key By UUID
	if ((result = Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM,
						 SRK_UUID, &hSRK))) {
		print_error("Tspi_Context_LoadKeyByUUID", result);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(result);
	}


	/*
	if ((result = Tspi_GetAttribUint32(hSRK, TSS_TSPATTRIB_KEY_INFO,
					   TSS_TSPATTRIB_KEYINFO_AUTHUSAGE,
					   &srk_authusage))) {
		print_error("Tspi_GetAttribUint32", result);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(result);
	}
    */

	if ((result = Tspi_Context_CreateObject(hContext,
						TSS_OBJECT_TYPE_POLICY,
						TSS_POLICY_MIGRATION,
						&keyMigrationPolicy))) {
		print_error("Tspi_Context_CreateObject", result);
		contextClose(hContext);
		exit(result);
	}

	if ((result = Tspi_Policy_SetSecret(keyMigrationPolicy,
					    TSS_SECRET_MODE_NONE,
					    0, NULL))) {
		print_error("Tspi_Policy_SetSecret", result);
		contextClose(hContext);
		exit(result);
	}

	if ((result = Tspi_Policy_AssignToObject(keyMigrationPolicy, hKey))) {
		print_error("Tspi_Policy_AssignToObject", result);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(result);
	}

	if ((result = Tspi_Key_CreateKey(hKey, hSRK, 0))) {
		print_error("Tspi_Key_CreateKey", result);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(result);
	}

	if ((result = Tspi_GetAttribData(hKey, TSS_TSPATTRIB_KEY_BLOB,
					 TSS_TSPATTRIB_KEYBLOB_BLOB,
					 &blob_size, &blob))) {
		print_error("Tspi_GetAttribData", result);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(result);
	}

	if ((outb = BIO_new_file(privkeyfile, "w")) == NULL) {
		fprintf(stderr, "Error opening file for write: %s\n", privkeyfile);
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(-1);
	}
	blob_str = ASN1_OCTET_STRING_new();
	if (!blob_str) {
		fprintf(stderr, "Error allocating ASN1_OCTET_STRING\n");
		Tspi_Context_CloseObject(hContext, hKey);
		contextClose(hContext);
		exit(-1);
	}

	ASN1_STRING_set(blob_str, blob, blob_size);
	asn1_len = i2d_ASN1_OCTET_STRING(blob_str, &blob_asn1);
	PEM_write_bio(outb, "TSS KEY BLOB", "", blob_asn1, asn1_len);

	BIO_free(outb);
	contextClose(hContext);

	printf("Success.\n");
 
	return 0;
}

static int ENGINE_misc(ENGINE *e)
{
    int r = 0;
    char SRK_password[] = TSS_WELL_KNOWN_SECRET;
    if (!ENGINE_init(e)) {
	fprintf(stderr, "Error: ENGINE_init\n");
	return -1;
    }

    if (!ENGINE_set_default_RSA(e) || !ENGINE_set_default_RAND(e)) {
	fprintf(stderr, "Erorr: ENGINE_misc\n");
	return -1;
    }

    r = ENGINE_ctrl_cmd(e, "SECRET_MODE", TSS_SECRET_MODE_SHA1, NULL, NULL, 0);
    if (0 == r) {
	fprintf(stderr, "Error: ENGINE_ctrl, SECRET_MODE\n");
	return -1;
    }

    r = ENGINE_ctrl_cmd(e, "PIN", 0, SRK_password, NULL, 0);
    if (0 == r) {
	fprintf(stderr, "Error %d: ENGINE_ctrl, PIN\n", r);
	return -1;
    }
    return 0;
}

void * load_private_key_from_TPM(char * privkeyfile)
{
    ENGINE *e;
    int r = 0;
    EVP_PKEY *pkey = NULL;

    ENGINE_load_builtin_engines();
    e = ENGINE_by_id("tpm");
    if (!e) {
	fprintf(stderr, "Error: ENGINE_by_id\n");
	goto err;
    }

    if ((r = ENGINE_misc(e)) != 0) {
	fprintf(stderr, "Error: ENGINE_misc\n");
	goto err;
    }

    pkey = ENGINE_load_private_key(e, privkeyfile, NULL, NULL);
    if (NULL == pkey) {
	fprintf(stderr, "Error: ENGINE_load_private_key\n");
	goto err;
    }

err:
    if (e) {
	ENGINE_free(e);
	ENGINE_finish(e);
    }
    return pkey;
}

static int loadKeyAndSetSecretPolicy(TSS_HCONTEXT hContext, TSS_HKEY *hKey)
{
    TSS_RESULT r;
    TSS_HPOLICY hPolicy;
    TSS_UUID SRK_UUID = TSS_UUID_SRK;
    BYTE wellKnown[TCPA_SHA1_160_HASH_LEN] = TSS_WELL_KNOWN_SECRET;
    if ((r=Tspi_Context_LoadKeyByUUID(hContext, TSS_PS_TYPE_SYSTEM,
				    SRK_UUID, hKey)) != TSS_SUCCESS) {
	print_error("Tspi_Context_LoadKeyByUUID", r);
	return r;
    }
    /* Don't create a new policy for the SRK's secret, just use the context's
     * default policy */
    if ((r=Tspi_GetPolicyObject(*hKey, TSS_POLICY_USAGE,
				    &hPolicy)) != TSS_SUCCESS){
	print_error("Tspi_GetPolicyObject", r);
	return r;
    }

    if ((r=Tspi_Policy_SetSecret(hPolicy, TSS_SECRET_MODE_SHA1,
			    sizeof(wellKnown), wellKnown)) != TSS_SUCCESS) {
	print_error("Tspi_Policy_SetSecret", r);
	return r;
    }
    return 0;
}

int assignSealSecretToObject(TSS_HCONTEXT hContext, TSS_HOBJECT hObj)
{
    TSS_RESULT r;
    TSS_HPOLICY hPolicy;
    char * SEAL_PASS = "sealpass";
    if ((r=Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_POLICY,
				TSS_POLICY_USAGE, &hPolicy)) != TSS_SUCCESS) {
	print_error("Tspi_Context_CreateObject", r);
	return r;
    }
    if ((r=Tspi_Policy_SetSecret(hPolicy, TSS_SECRET_MODE_PLAIN,
				strlen(SEAL_PASS), SEAL_PASS)) != TSS_SUCCESS) {
	print_error("Tspi_Policy_SetSecret", r);
	return r;
    }
    if ((r=Tspi_Policy_AssignToObject(hPolicy, hObj)) != TSS_SUCCESS) {
	print_error("Tspi_Policy_AssignToObject", r);
	return r;
    }
    return 0;
}

#define TPMSEAL_HDR_STRING "-----BEGIN TSS-----\n"
#define TPMSEAL_FTR_STRING "-----END TSS-----\n"
#define TPMSEAL_TSS_STRING "-----TSS KEY-----\n"
#define TPMSEAL_EVP_STRING "-----ENC KEY-----\n"
#define TPMSEAL_ENC_STRING "-----ENC DAT-----\n"

#define TPMSEAL_KEYTYPE_SYM "Symmetric Key: "
#define TPMSEAL_CIPHER_AES256CBC "AES-128-CTR\n"

#define TPMSEAL_IV "IBM SEALIBM SEAL"

int seal_secret(char *filename, uint8_t *secret, size_t buflen)
{
    TSS_HCONTEXT hContext = 0;
    TSS_RESULT r;
    TSS_HPCRS hPcrs = 0;
    TSS_HKEY hKey = 0;
    TSS_HKEY hSRK = 0;
    TSS_HPOLICY hSRKPolicy;
    TSS_HPOLICY hPolicy;
    TSS_HENCDATA hEncData;
    BYTE *encKey, *sealKey;
    UINT32 encLen, sealKeyLen;
    BIO *bdata = NULL, *b64 = NULL;
    TSS_FLAG initFlags = TSS_KEY_TYPE_STORAGE | TSS_KEY_SIZE_2048 |
			 TSS_KEY_VOLATILE | TSS_KEY_AUTHORIZATION |
			 TSS_KEY_NOT_MIGRATABLE;

    if (contextCreateAndConnect(&hContext) != TSS_SUCCESS) {
	hContext = 0;
	goto out;
    }
    if (loadKeyAndSetSecretPolicy(hContext, &hSRK) != TSS_SUCCESS) {
	goto out;
    }
    if ((r = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_RSAKEY,
					    initFlags, &hKey)) != TSS_SUCCESS) {
	print_error("Tspi_Context_CreateObject", r);
	goto out;
    }
    if (assignSealSecretToObject(hContext, hKey) != TSS_SUCCESS) {
	goto out;
    }
    if ((r = Tspi_Key_CreateKey(hKey, hSRK, 0)) != TSS_SUCCESS) {
	print_error("Tspi_Key_CreateKey", r);
	goto out;
    }
    if ((r = Tspi_Key_LoadKey(hKey, hSRK)) != TSS_SUCCESS) {
	print_error("Tspi_Key_LoadKey", r);
	goto out;
    }
    if ((r = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_ENCDATA,
				TSS_ENCDATA_SEAL, &hEncData)) != TSS_SUCCESS) {
	print_error("Tspi_Context_CreateObject", r);
	goto out;
    }
    if (assignSealSecretToObject(hContext, hEncData) != TSS_SUCCESS) {
	goto out;
    }
    if ((r = Tspi_Data_Seal(hEncData, hKey, buflen, secret, hPcrs)) != TSS_SUCCESS) {
	print_error("Tspi_Data_Seal", r);
	goto out;
    }
    if ((r = Tspi_GetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
				TSS_TSPATTRIB_ENCDATABLOB_BLOB,
				&encLen, &encKey)) != TSS_SUCCESS) {
	print_error("Tspi_GetAttribData", r);
	goto out;
    }
    if ((r = Tspi_GetAttribData(hKey, TSS_TSPATTRIB_KEY_BLOB,
				TSS_TSPATTRIB_KEYBLOB_BLOB,
				&sealKeyLen, &sealKey)) != TSS_SUCCESS) {
	print_error("Tspi_GetAttribData", r);
	goto out;
    }


    /* Create a BIO to perform base64 encoding */
    if ((b64 = BIO_new(BIO_f_base64())) == NULL) {
        fprintf(stderr, "Unable to open base64 BIO\n");
        goto out;
    }

    /* Create a BIO for the output file */
    if ((bdata = BIO_new(BIO_s_file())) == NULL) {
        fprintf(stderr, "Unable to open output BIO\n");
        goto out;
    }

    /* Assign the output file to the BIO */
    if (BIO_write_filename(bdata, filename) <= 0) {
        fprintf(stderr, "Unable to open output file: %s\n",
             filename);
        goto out;
    }

    /* Output the sealed data header string */
    BIO_puts(bdata, TPMSEAL_HDR_STRING);

    /* Sealing key used on the TPM */
    BIO_puts(bdata, TPMSEAL_TSS_STRING);
    bdata = BIO_push(b64, bdata);
    BIO_write(bdata, sealKey, sealKeyLen);
    if (BIO_flush(bdata) != 1) {
        fprintf(stderr, "Unable to flush output\n");
        goto out;
    }
    bdata = BIO_pop(b64);

    /* Sealed EVP Symmetric Key */
    BIO_puts(bdata, TPMSEAL_EVP_STRING);
    BIO_puts(bdata, TPMSEAL_KEYTYPE_SYM);
    BIO_puts(bdata, TPMSEAL_CIPHER_AES256CBC);
    bdata = BIO_push(b64, bdata);
    BIO_write(bdata, encKey, encLen);
    if (BIO_flush(bdata) != 1) {
        fprintf(stderr, "Unable to flush output\n");
        goto out;
    }
    bdata = BIO_pop(b64);

#if 0
    /* Encrypted Data */
    BIO_puts(bdata, TPMSEAL_ENC_STRING);
    bdata = BIO_push(b64, bdata);

    EVP_CIPHER_CTX ctx;
    EVP_EncryptInit(&ctx, EVP_aes_256_cbc(), randKey, (unsigned char *)TPMSEAL_IV);

    while ((lineLen = BIO_read(bin, line, sizeof(line))) > 0) {
        EVP_EncryptUpdate(&ctx, encData, &encDataLen,
                  line, lineLen);
        BIO_write(bdata, encData, encDataLen);
    }

    EVP_EncryptFinal(&ctx, encData, &encDataLen);
    BIO_write(bdata, encData, encDataLen);
    if (BIO_flush(bdata) != 1) {
        fprintf(stderr, "Unable to flush output\n");
        goto out;
    }
    bdata = BIO_pop(b64);

#endif
    BIO_puts( bdata, TPMSEAL_FTR_STRING);

out:
    if (bdata)
	BIO_free(bdata);
    if (b64)
	BIO_free(b64);
    if (hContext)
	contextClose(hContext);
    return r;
}

#define TSSKEY_DEFAULT_SIZE 768
#define EVPKEY_DEFAULT_SIZE 512
static int readSecretFromFile(char *filename, BYTE **evpKey, int *evpSize,
					      BYTE **tssKey, int *tssSize)
{
    BIO *bdata = NULL, *b64 = NULL, *bmem = NULL;
    char data[256];
    int r, len, bioRc;
    BYTE *rcPtr;
    int evpKeyDataSize = 0;
    int tssKeyDataSize = 0;
    BYTE *evpKeyData = NULL;
    BYTE *tssKeyData = NULL;
    int evpLen = 0;
    int tssLen = 0;

    /* Create an input file BIO */
    if((bdata = BIO_new_file(filename, "r")) == NULL ) {
	fprintf(stderr, "Error : BIO_new_file\n");
	r = -1;
	goto out;
    }

    /* Test file header for TSS */
    BIO_gets(bdata, data, sizeof(data));
    if (strncmp(data, TPMSEAL_HDR_STRING, strlen(TPMSEAL_HDR_STRING)) != 0) {
	fprintf(stderr, "Error : NOT TSS File\n");
	goto out;
    }

    /* Looking for TSS Key Header */
    BIO_gets(bdata, data, sizeof(data));
    if (strncmp(data, TPMSEAL_TSS_STRING, strlen(TPMSEAL_TSS_STRING)) != 0) {
	fprintf(stderr, "Error : NOT TSS KEY File\n");
	goto out;
    }

    /* Create a memory BIO to hold the base64 TSS key */
    if ((bmem = BIO_new(BIO_s_mem())) == NULL) {
	fprintf(stderr, "Error : BIO_new(BIO_S_mem())\n");
	r = -1;
	goto out;
    }
    BIO_set_mem_eof_return(bmem, 0);

    /* Read the base64 TSS key into the memory BIO */
    while ((len = BIO_gets(bdata, data, sizeof(data))) > 0) {
	/* Look for EVP Key Header (end of key) */
	if (strncmp(data, TPMSEAL_EVP_STRING,
			strlen(TPMSEAL_EVP_STRING)) == 0)
	    break;

	if (BIO_write(bmem, data, len) <= 0) {
	    r = -1;
	    fprintf(stderr, "Error: BIO_write()\n");
	    goto out;
	}
    }
    if (strncmp(data, TPMSEAL_EVP_STRING,
		    strlen(TPMSEAL_EVP_STRING)) != 0 ) {
	r = -1;
	fprintf(stderr, "Error: Wrong EVP TAG\n");
	goto out;
    }

    /* Create a base64 BIO to decode the TSS key */
    if ((b64 = BIO_new(BIO_f_base64())) == NULL) {
	r = -1;
	fprintf(stderr, "Error: BIO_new\n");
	goto out;
    }

    /* Decode the TSS key */
    bmem = BIO_push( b64, bmem );
    while ((len = BIO_read(bmem, data, sizeof(data))) > 0) {
	if ((tssLen + len) > tssKeyDataSize) {
	    tssKeyDataSize += TSSKEY_DEFAULT_SIZE;
	    rcPtr = realloc( tssKeyData, tssKeyDataSize);
	    if ( rcPtr == NULL ) {
		r = -1;
		fprintf(stderr, "Error: realloc\n");
		goto out;
	    }
	    tssKeyData = rcPtr;
	}
	memcpy(tssKeyData + tssLen, data, len);
	tssLen += len;
    }
    bmem = BIO_pop(b64);
    BIO_free(b64);
    b64 = NULL;
    bioRc = BIO_reset(bmem);
    if (bioRc != 1) {
	r = -1;
	fprintf(stderr, "Error: BIO_reset\n");
	goto out;
    }

    /* Check for EVP Key Type Header */
    BIO_gets(bdata, data, sizeof(data));
    if (strncmp(data, TPMSEAL_KEYTYPE_SYM,
		    strlen(TPMSEAL_KEYTYPE_SYM)) != 0 ) {
	r = -1;
	fprintf(stderr, "Error: BIO_gets\n");
	goto out;
    }

    /* Make sure it's a supported cipher
       (currently only AES 256 CBC) */
    if (strncmp(data + strlen(TPMSEAL_KEYTYPE_SYM),
		    TPMSEAL_CIPHER_AES256CBC,
		    strlen(TPMSEAL_CIPHER_AES256CBC)) != 0) {
	r = -1;
	fprintf(stderr, "Error: WRONG KEY TYPE\n");
	goto out;
    }

    /* Read the base64 Symmetric key into the memory BIO */
    while ((len = BIO_gets(bdata, data, sizeof(data))) > 0) {
	/* Look for Encrypted Data Header (end of key) */
	if (strncmp(data, TPMSEAL_FTR_STRING, strlen(TPMSEAL_FTR_STRING)) == 0)
	    break;

	if (BIO_write(bmem, data, len) <= 0) {
	    r = -1;
	    fprintf(stderr, "Error: BIO_write\n");
	    goto out;
	}
    }
    if (strncmp(data, TPMSEAL_FTR_STRING,
		    strlen(TPMSEAL_FTR_STRING)) != 0 ) {
	r = -1;
	fprintf(stderr, "Error: WRONG FTR TAG\n");
	goto out;
    }

    /* Create a base64 BIO to decode the Symmetric key */
    if ((b64 = BIO_new(BIO_f_base64())) == NULL) {
	r = -1;
	fprintf(stderr, "Error: BIO_new\n");
	goto out;
    }

    /* Decode the Symmetric key */
    bmem = BIO_push( b64, bmem );
    while ((len = BIO_read(bmem, data, sizeof(data))) > 0) {
	if ((evpLen + len) > evpKeyDataSize) {
	    evpKeyDataSize += EVPKEY_DEFAULT_SIZE;
	    rcPtr = realloc( evpKeyData, evpKeyDataSize);
	    if ( rcPtr == NULL ) {
		r = -1;
		fprintf(stderr, "Error: realloc\n");
		goto out;
	    }
	    evpKeyData = rcPtr;
	}
	memcpy(evpKeyData + evpLen, data, len);
	evpLen += len;
    }
    bmem = BIO_pop(b64);
    BIO_free(b64);
    b64 = NULL;
    bioRc = BIO_reset(bmem);
    if (bioRc != 1) {
	r = -1;
	fprintf(stderr, "Error: BIO_reset\n");
	goto out;
    }

#if 0
    /* Read the base64 encrypted data into the memory BIO */
    while ((len = BIO_gets(bdata, data, sizeof(data))) > 0) {
	/* Look for TSS Footer (end of data) */
	if (strncmp(data, TPMSEAL_FTR_STRING,
			strlen(TPMSEAL_FTR_STRING)) == 0)
	    break;

	if (BIO_write(bmem, data, len) <= 0) {
	    r = -1;
	    fprintf(stderr, "Error: BIO_write\n");
	    goto out;
	}
    }
    if (strncmp(data, TPMSEAL_FTR_STRING,
		    strlen(TPMSEAL_FTR_STRING)) != 0 ) {
	r = -1;
	fprintf(stderr, "Error: WRONG TSS FOOTER: %s\n", data);
	goto out;
    }
#endif

out:
    *evpSize = evpLen;
    *evpKey = evpKeyData;
    *tssSize = tssLen;
    *tssKey = tssKeyData;

    if ( bdata )
	BIO_free(bdata);
    if ( b64 )
	BIO_free(b64);
    if ( bmem ) {
	bioRc = BIO_set_close(bmem, BIO_CLOSE);
	BIO_free(bmem);
    }

    if (bioRc != 1) {
	r = -1;
    }
    return 0;
}

int unseal_secret(char *filename, uint8_t *secret, size_t buflen)
{
    int r;
    TSS_HCONTEXT hContext = 0;
    TSS_HENCDATA hEncData;
    TSS_HKEY hSRK, hKey;
    TSS_HPOLICY hSRKPolicy, hPolicy;
    BYTE *evpKeyData = NULL;
    BYTE *tssKeyData = NULL;
    int evpLen = 0, tssLen = 0;
    BYTE *symKey;
    UINT32 symKeyLen;

    if ((r = readSecretFromFile(filename,
			&evpKeyData, &evpLen, &tssKeyData, &tssLen)) < 0) {
	goto out;
    }

    /* Unseal */
    if (contextCreateAndConnect(&hContext) != TSS_SUCCESS) {
	hContext = 0;
	goto out;
    }
    if (loadKeyAndSetSecretPolicy(hContext, &hSRK) != TSS_SUCCESS) {
	goto out;
    }

    if ((r=Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_ENCDATA,
				TSS_ENCDATA_SEAL, &hEncData)) != TSS_SUCCESS) {
	print_error("Tspi_Context_CreateObject", r);
	goto tss_out;
    }
    if (assignSealSecretToObject(hContext, hEncData) != TSS_SUCCESS) {
	goto tss_out;
    }
    if ((r=Tspi_SetAttribData(hEncData, TSS_TSPATTRIB_ENCDATA_BLOB,
			    TSS_TSPATTRIB_ENCDATABLOB_BLOB,
			    evpLen, evpKeyData)) != TSS_SUCCESS) {
	print_error("Tspi_SetAttribData", r);
	goto tss_out;
    }

    /* Failure point if trying to unseal data on a differnt TPM */
    if ((r=Tspi_Context_LoadKeyByBlob(hContext, hSRK, tssLen,
				    tssKeyData, &hKey)) != TSS_SUCCESS) {
	print_error("Tspi_Context_LoadKeyByBlob", r);
	goto tss_out;
    }
    if (assignSealSecretToObject(hContext, hKey) != TSS_SUCCESS) {
	goto tss_out;
    }

    if ((r=Tspi_Data_Unseal(hEncData, hKey, &symKeyLen,
				    &symKey)) != TSS_SUCCESS) {
	print_error("Tspi_Data_Unseal", r);
	goto tss_out;
    }
    memcpy(secret, symKey, buflen);

#if 0
    /* Malloc a block of storage to hold the decrypted data
       Using the size of the mem BIO is more than enough
       (plus an extra cipher block size) */
    res_data = malloc(BIO_pending(bmem) + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    if ( res_data == NULL ) {
	r = -1;
	fprintf(stderr, "Error: malloc\n");
	goto tss_out;
    }

    /* Decode and decrypt the encrypted data */
    EVP_CIPHER_CTX ctx;
    EVP_DecryptInit(&ctx, EVP_aes_256_cbc(), symKey, (unsigned char *)TPMSEAL_IV);

    /* Create a base64 BIO to decode the encrypted data */
    if ((b64 = BIO_new(BIO_f_base64())) == NULL) {
	r = -1;
	fprintf(stderr, "Error: BIO_new\n");
	goto tss_out;
    }

    bmem = BIO_push( b64, bmem );
    while ((len = BIO_read(bmem, data, sizeof(data))) > 0) {
	    EVP_DecryptUpdate(&ctx, res_data+res_size,
				    &len, (unsigned char *)data, len);
	    res_size += len;
    }
    EVP_DecryptFinal(&ctx, res_data+res_size, &len);
    res_size += len;
    bmem = BIO_pop(b64);
    BIO_free(b64);
    b64 = NULL;
    /* a BIO_reset failure shouldn't have an affect at this point */
    bioRc = BIO_reset(bmem);
    if (bioRc != 1) {
	r = -1;
	fprintf(stderr, "Error: BIO_reset\n");
	goto out;
    }
#endif

tss_out:
    Tspi_Context_Close(hContext);
out:

    if ( evpKeyData )
	free(evpKeyData);
    if ( tssKeyData )
	free(tssKeyData);

#if 0
    if ( r == 0 ) {
	*tss_data = res_data;
	*tss_size = res_size;
    } else
	free(res_data);
#endif

    return r;
}
/* vi: set ai sw=4 ts=8 sts=4: */
