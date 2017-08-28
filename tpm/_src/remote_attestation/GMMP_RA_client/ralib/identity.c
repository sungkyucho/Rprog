#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <curl/curl.h>

#include <trousers/tss.h>
#include <trousers/trousers.h>

#include "identity.h"
#include "dbg_macros.h"
#include "tpm_util.h"

#define UUID_BYTE 0x5a
/* Size of endorsement key in bytes */
#define	EKSIZE		(2048/8)
/* URL of Privacy CA */
//#define CERTURL		CAURL "pca/cert/pca.crt"
//#define REQURL		CAURL "pca/dosign"
/* Prompt for TPM popup */
#define POPUPSTRING	"TPM owner secret"

#ifndef REALEK

/* Create a fake EK cert for talking to PCA */
/* Not a valid signature, just a holder for the Endorsement Key */

/* Forward declaration, data at end */
static BYTE fakeEKCert[0x41a];

/**
 * @brief Create a fake endorsement key cert using system's actual EK
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param hContext TPM handler 
 * @param hTPM TPM object handler 
 * @param pCertLen EK certificate length 
 * @param pCert pointer to store EK certificate 
*/ 
static TSS_RESULT
makeEKCert(TSS_HCONTEXT hContext, TSS_HTPM hTPM, UINT32 *pCertLen, BYTE **pCert)
{
	TSS_RESULT	result;
	TSS_HKEY	hPubek;
	UINT32		modulusLen;
	BYTE		*modulus;

	//printf("----------HERE<%s:%d>\n", __func__, __LINE__);
	result = Tspi_TPM_GetPubEndorsementKey(hTPM, TRUE, NULL, &hPubek);
	if (result != TSS_SUCCESS)
		return result;

	result = Tspi_GetAttribData (hPubek, TSS_TSPATTRIB_RSAKEY_INFO,
		TSS_TSPATTRIB_KEYINFO_RSA_MODULUS, &modulusLen, &modulus);

	Tspi_Context_CloseObject (hContext, hPubek);

	if (result != TSS_SUCCESS)
		return result;

	if (modulusLen != 256) {
		Tspi_Context_FreeMemory (hContext, modulus);
		return TSS_E_FAIL;
	}

	*pCertLen = sizeof(fakeEKCert);
	*pCert = malloc (*pCertLen);
	memcpy (*pCert, fakeEKCert, *pCertLen);
	memcpy (*pCert + 0xc6, modulus, modulusLen);

	Tspi_Context_FreeMemory (hContext, modulus);

	return TSS_SUCCESS;
}

#endif /* undef REALEK */


/**
 * @brief write given file data to another file
*/ 
void dump_file(FILE *rfp, char *fname)
{
	FILE *ofp = NULL;
	char *buf = NULL;
	int blen = 0;
	long cur_pos = 0;

	if (rfp == NULL) {
		printf("<%s> NULL file pointer\n", __func__);
		return;
	}

	if (fname != NULL) {
		ofp = fopen(fname, "w");
		if (ofp == NULL) {
			printf("<%s> fopen(%s) failed: %m\n", __func__, fname);
		}
	}

	if (ofp == NULL) {
		printf("<%s> dumping on stdout\n", __func__);
		printf("----------------------------\n");
	}

	cur_pos = ftell(rfp); /* mark */
	fseek(rfp, 0, SEEK_END);
	blen = 	ftell(rfp);
	fseek(rfp, 0, SEEK_SET);
	buf = malloc(blen);
	if (buf) {
		if ((int)fread(buf, 1, blen, rfp) != blen) {
			printf("Unable to read in file: %m\n");
		} else {
			fwrite(buf, 1, blen, ofp);
		}
		free(buf);
	}

	fseek(rfp, cur_pos, SEEK_SET); /* restore */

	if (ofp == NULL) {
		printf("----------------------------\n");
	} else {
		fclose(ofp);
	}
}

/**
 * @brief write given buffer data to file
 * @param buf what print to file 
 * @param blen print data length 
 * @param fname target file name to write  
*/ 
void dump_bytes(unsigned char *buf, int blen, char *fname)
{
	FILE *ofp = NULL;
	int wcnt = 0;

	if (buf == NULL) {
		printf("<%s> NULL buffer\n", __func__);
		return;
	}

	if (fname != NULL) {
		ofp = fopen(fname, "w");
		if (ofp == NULL) {
			printf("<%s> fopen(%s) failed: %m\n", __func__, fname);
		}
	}

	if (ofp == NULL) {
		printf("<%s> dumping on stdout\n", __func__);
		printf("----------------------------\n");
	}

	wcnt = fwrite(buf, 1, blen, ofp);
	if (wcnt != blen) {
		printf("<%s> fwrite failed<%d/%d>: %m\n", __func__, wcnt, blen);
	}

	if (ofp == NULL) {
		printf("----------------------------\n");
	} else {
		fclose(ofp);
	}
}


/**
 * @brief Read the CA from privacy CA 
 * @return X509 on success, NULL otherwize
 * @param privacy_ca Privacy CA server address
*/ 
static X509* readPCAcert(char *privacy_ca)
{
	CURL		*hCurl;
	char		url[128];
	FILE		*f_tmp = tmpfile();
	X509		*x509;
	int		result;

	hCurl = curl_easy_init ();
//curl_easy_setopt (hCurl, CURLOPT_VERBOSE, 1);
	sprintf(url, "http://%s/pca/cert/pca.crt", privacy_ca);
	curl_easy_setopt (hCurl, CURLOPT_URL, url);
	curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, (BYTE **)f_tmp);

	if ((result = curl_easy_perform(hCurl))) {
		dbg_printf(DERROR, "Unable to connect to Privacy CA, curl library result code %d\n", result);
 		curl_easy_cleanup(hCurl);
 		curl_global_cleanup();
		fclose(f_tmp);
		return NULL;
	}
	curl_easy_cleanup(hCurl);
 	curl_global_cleanup();

	if(dbg_lv == DDBG)
		dump_file(f_tmp, "pca.crt");

	rewind (f_tmp);
	x509 = PEM_read_X509(f_tmp, NULL, NULL, NULL);
	fclose(f_tmp);

	return x509;
}

/**
 * @brief certification validation check 
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param x509 certification pointer 
 */
TSS_RESULT 
is_x509_valid_period(X509 *x509){
	int r;
	int len=0;
	char bigBuffer[256];
	BIO *out ;
		
	if(x509 == NULL)
		return TSS_E_FAIL;

	out = BIO_new(BIO_s_mem());
	BIO_printf(out, "DateValid.From:");
	ASN1_TIME_print(out, X509_get_notBefore(x509));
	BIO_printf(out, "\r\nDateValid.To:");
	ASN1_TIME_print(out, X509_get_notAfter(x509));
	BIO_printf(out, "\r\n");
	len = BIO_read(out, bigBuffer, 255);
	bigBuffer[len] = '\0';
	BIO_free(out);

	dbg_printf(DINFO, "x509 validity period:%s\n", bigBuffer);
	
	r=X509_cmp_current_time(X509_get_notBefore(x509));
	if (r >= 0){
		dbg_printf(DERROR, "Certificate is not yet valid.\n");
		return TSS_E_FAIL;
	}
	r=X509_cmp_current_time(X509_get_notAfter(x509));
	if (r <= 0){
		dbg_printf(DERROR, "Certificate has expired.\n");
		return TSS_E_FAIL;
	}
	return TSS_SUCCESS;
}

/**
 * @brief Get RSA key from Privacy CA
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param privacy_ca Privacy CA server address
 * @param rsa_n RSA Key  
 * @param size_n RSA Key length 
 */
TSS_RESULT
get_pca_rsa(char *privacy_ca, BYTE **rsa_n, int *size_n)
{
	X509		*x509;
	EVP_PKEY	*pcaKey;
	RSA		*rsa = NULL;
	int		len_n;

	x509 = readPCAcert (privacy_ca);
	if (x509 == NULL) {
		dbg_printf (DERROR, "Error reading PCA key\n");
//		exit (1);
		return TSS_E_FAIL;
	}
	pcaKey = X509_get_pubkey(x509);
	if (pcaKey == NULL) {
		X509_free (x509);
		return TSS_E_FAIL;
	}

	rsa = EVP_PKEY_get1_RSA(pcaKey);
	if (rsa == NULL) {
		dbg_printf (DERROR, "Error reading RSA key from PCA\n");
//		exit (1);
		X509_free (x509);
		EVP_PKEY_free(pcaKey);
		return TSS_E_FAIL;
	}
	EVP_PKEY_free(pcaKey);
	X509_free (x509);

	if ((len_n= BN_bn2bin(rsa->n, *rsa_n)) <= 0) {
		dbg_printf(DERROR, "BN_bn2bin failed\n");
		RSA_free(rsa);
		return TSS_E_FAIL;
	}
	*size_n = len_n;
	RSA_free(rsa);
	return TSS_SUCCESS;
}

/**
 * @brief Set key password
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param hContext TPM handler 
 * @param hIdentKey ident Key handler  
 * @param aikpass  AIK authorization
 */
TSS_RESULT
set_identkey_policy(TSS_HCONTEXT hContext, TSS_HKEY	*hIdentKey, char *aikpass)
{
	TSS_HPOLICY	hIdentKeyPolicy;
	BYTE secret[] = TSS_WELL_KNOWN_SECRET;
	TSS_RESULT result;

	if (aikpass)
	{
		result = Tspi_Context_CreateObject(hContext, 
						TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE,
						&hIdentKeyPolicy);
		if (result != TSS_SUCCESS) {
//			printf ("Error 0x%x on Tspi_CreateObject for AIK policy\n", result);
			dbg_printf(DERROR, "Tspi_CreateObject for AIK policy: %s\n", Trspi_Error_String(result));
			return result;
		}

		result = Tspi_Policy_AssignToObject(hIdentKeyPolicy, *hIdentKey);
		if (result != TSS_SUCCESS) {
//			printf ("Error 0x%x on Tspi_Policy_AssignToObject for AIK\n", result);
			dbg_printf(DERROR, "Tspi_Policy_AssignToObject for AIK: %s\n", Trspi_Error_String(result));
			return result;
		}

		result = Tspi_Policy_SetSecret(hIdentKeyPolicy, TSS_SECRET_MODE_PLAIN,
						strlen(aikpass), (BYTE *)aikpass);
		if (result != TSS_SUCCESS) {
//			printf ("Error 0x%x on Tspi_Policy_SetSecret for AIK\n", result);
			dbg_printf(DERROR, "Tspi_Policy_SetSecret for AIK: %s\n", Trspi_Error_String(result));
			return result;
		}
	}
	else {
		//result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_POLICY, TSS_POLICY_USAGE, &hIdentKeyPolicy);
		result = Tspi_GetPolicyObject(*hIdentKey, TSS_POLICY_USAGE, &hIdentKeyPolicy);
		if (result != TSS_SUCCESS) {
//			printf ("Error 0x%x on Tspi_CreateObject for AIK policy\n", result);
			dbg_printf(DERROR, "Tspi_CreateObject for AIK policy: %s\n", Trspi_Error_String(result));
			return result;
		}

		result = Tspi_Policy_AssignToObject(hIdentKeyPolicy, *hIdentKey);
		if (result != TSS_SUCCESS) {
			dbg_printf(DERROR, "Tspi_Policy_AssignToObject for AIK: %s\n", Trspi_Error_String(result));
//			printf ("Error 0x%x on Tspi_Policy_AssignToObject for AIK\n", result);
			return result;
		}

		result = Tspi_Policy_SetSecret(hIdentKeyPolicy, TSS_SECRET_MODE_SHA1, 20, secret);
		if (result != TSS_SUCCESS) {
//			printf ("Error 0x%x on Tspi_Policy_SetSecret for AIK\n", result);
			dbg_printf(DERROR, "Tspi_Policy_SetSecret for AIK: %s\n", Trspi_Error_String(result));
			return result;
		}
	}
	return TSS_SUCCESS;
}

/**
 * @brief request Certification to Privacy CA 
 * @return 0 on success, -1 otherwize
 * @param symBuf pointer to store response from Privacy CA
 * @param symBufSize response size  
 * @param privacy_ca privacy CA address 
 * @param rgbTCPAIdentityReq  AIK 
 * @param ulTCPAIdentityReqLength AIK length
 * @param asymLen  
 */
int request_to_pca(BYTE **symBuf, UINT32 *symBufSize, char *privacy_ca, BYTE *rgbTCPAIdentityReq, UINT32 ulTCPAIdentityReqLength, int asymLen)
{
	FILE		*f_tmp;
	char		url[128];

	CURL		*hCurl;
	struct curl_slist *slist=NULL;

	int		result;
	UINT32 bufSize;

	dbg_printf (DINFO, "Sending request to PrivacyCA.com...\n");
	curl_global_init (CURL_GLOBAL_ALL);

	/* Send to server */
	f_tmp = tmpfile();
	hCurl = curl_easy_init ();
//  curl_easy_setopt (hCurl, CURLOPT_VERBOSE, 1);
//	sprintf (url, REQURL, level);
	sprintf (url, "http://%s/pca/dosign.cgi", privacy_ca );
	curl_easy_setopt (hCurl, CURLOPT_URL, url);
	curl_easy_setopt (hCurl, CURLOPT_POSTFIELDS, rgbTCPAIdentityReq);
	curl_easy_setopt (hCurl, CURLOPT_POSTFIELDSIZE, ulTCPAIdentityReqLength);
	curl_easy_setopt (hCurl, CURLOPT_WRITEDATA, (BYTE **)f_tmp);
	slist = curl_slist_append (slist, "Pragma: no-cache");
	slist = curl_slist_append (slist, "Content-Type: application/octet-stream");
	slist = curl_slist_append (slist, "Content-Transfer-Encoding: binary");
	curl_easy_setopt (hCurl, CURLOPT_HTTPHEADER, slist);
	// Return: 0 is ok 
	if ((result = curl_easy_perform(hCurl))) {
//		printf ("Unable to connect to Privacy CA, curl library result code %d\n", result);
		dbg_printf(DERROR, "Unable to connect to Privacy CA, curl library result code %d\n", result);
		curl_slist_free_all(slist);
		curl_easy_cleanup(hCurl);
		curl_global_cleanup();
		fclose(f_tmp);
		return result;
	}
	curl_slist_free_all(slist);
	curl_easy_cleanup(hCurl);
	curl_global_cleanup();

	printf ("Processing response...\n");

	fflush(f_tmp);
	// debug (EDIT)
	if(dbg_lv == DDBG)
		dump_file(f_tmp, "aik.signed");
	// debug
	// NAREMO - HERE
	bufSize = ftell(f_tmp);
	*symBuf = malloc(bufSize);
	rewind(f_tmp);
	if(fread(*symBuf, 1, bufSize, f_tmp) <= 0){
		dbg_printf(DERROR, "Unable to read in file: %m\n");
		fclose(f_tmp);
		free(*symBuf);
		return -1;
	}
	*symBufSize = bufSize;

	printf("symBufSize <%u>\n", bufSize);
	if (bufSize <= asymLen)
	{
		dbg_printf (DERROR, "Bad response from PrivacyCA.com: %s\n", *symBuf);
		fclose(f_tmp);
		free(*symBuf);
		return -1;
	}
	fclose(f_tmp);
	return 0;
}

/**
 * @brief get Credential from Privacy CA 
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param hContext TPM handler 
 * @param hTPM TPM object handler 
 * @param hSRK TPM SRK handler
 * @param privacy_ca Privacy CA address
 * @param x509 pointer to store certification 
 */
TSS_RESULT get_create_credential(TSS_HCONTEXT *hContext, TSS_HTPM *hTPM, TSS_HKEY *hSRK, char *privacy_ca, X509 **x509)
{
	TSS_HKEY	hPCAKey;
	TSS_HKEY	hIdentKey ;
//	TSS_HPOLICY	hIdentKeyPolicy;
	TSS_UUID	SRK_UUID = TSS_UUID_SRK;
	TSS_UUID    AIK_UUID = BACKUP_KEY_UUID ;
//	TSS_UUID	uuid0;
	TSS_RESULT result;

//	BYTE		secret[] = TSS_WELL_KNOWN_SECRET;
//	X509		*x509;
	BYTE		*rsa_n;//[16384/8];
	int		size_n;
//	FILE		*f_blob;
	char		*aikpass = NULL;
//	char		*outfilename;
//	char		*outblobfilename;
	BYTE		*rgbIdentityLabelData = NULL;
	BYTE		*labelString;
	UINT32		labelLen;
//	BYTE		*popupString;
//	UINT32		popupLen;
	BYTE		*rgbTCPAIdentityReq;
	UINT32		ulTCPAIdentityReqLength;
	UINT32		initFlags = TSS_KEY_TYPE_IDENTITY | TSS_KEY_SIZE_2048 |
					TSS_KEY_VOLATILE | TSS_KEY_NOT_MIGRATABLE;
	BYTE		asymBuf[EKSIZE];
	BYTE		*symBuf;
	BYTE		*toFreeBuf;
	BYTE		*credBuf = NULL;
	BYTE		*tbuf;
	UINT32		asymBufSize;
	UINT32		symBufSize;
	UINT32		credBufSize = 0;
//	BYTE		*blob;
//	UINT32		blobLen;
	//int		keynum;
#ifdef REALEK
	const int	level = 1;
#else
//	const int	level = 0;
	BYTE		*ekCert;
	UINT32		ekCertLen;
#endif

//	memset(&uuid0, UUID_BYTE, sizeof(TSS_UUID));

	dbg_printf (DINFO, "Retrieving PCA certificate...\n");


	if (aikpass)
		initFlags |= TSS_KEY_AUTHORIZATION;
	else /* debug */
		initFlags |= TSS_KEY_NO_AUTHORIZATION; /* CHECK */

	result = Tspi_Context_CreateObject(*hContext,
					   TSS_OBJECT_TYPE_RSAKEY,
					   initFlags, &hIdentKey);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_Context_CreateObject for key\n", result);
		dbg_printf(DERROR, "Tspi_Context_CreateObject for key: %s\n", Trspi_Error_String(result));
		Tspi_Context_Close(*hContext);
		return result;
	}

	if((result = set_identkey_policy(*hContext, &hIdentKey, aikpass))){
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

	//printf("----------HERE<%s:%d>\n", __func__, __LINE__);
	result = Tspi_Context_CreateObject(*hContext,
					   TSS_OBJECT_TYPE_RSAKEY,
					   TSS_KEY_TYPE_LEGACY|TSS_KEY_SIZE_2048,
					   &hPCAKey);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_Context_CreateObject for PCA\n", result);
		dbg_printf(DERROR, "Tspi_Context_CreateObject for PCA: %s\n", Trspi_Error_String(result));
//		exit(result);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

	rsa_n = malloc((16384/8));
	if (rsa_n == NULL){
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

	if((result = get_pca_rsa(privacy_ca, &rsa_n, &size_n))){
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		free(rsa_n);
		return result;
	}

	result = Tspi_SetAttribData (hPCAKey, TSS_TSPATTRIB_RSAKEY_INFO,
		TSS_TSPATTRIB_KEYINFO_RSA_MODULUS, size_n, rsa_n);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_SetAttribData for PCA modulus\n", result);
		dbg_printf(DERROR, "Tspi_SetAttribData for PCA modulus: %s\n", Trspi_Error_String(result));
//		exit(result);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		free(rsa_n);
		return result;
	}
	free(rsa_n);

	result = Tspi_SetAttribUint32(hPCAKey, TSS_TSPATTRIB_KEY_INFO,
			  TSS_TSPATTRIB_KEYINFO_ENCSCHEME, TSS_ES_RSAESPKCSV15);
	if (result != TSS_SUCCESS) {
//		printf ("Error 0x%x on Tspi_SetAttribUint32 for PCA encscheme\n", result);
		dbg_printf(DERROR, "Tspi_SetAttribUint32 for PCA encscheme: %s\n", Trspi_Error_String(result));
//		exit(result);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

#ifndef REALEK
	result = makeEKCert(*hContext, *hTPM, &ekCertLen, &ekCert);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "makeEKCert: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on makeEKCert\n", result);
//		exit(result);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

	result = Tspi_SetAttribData(*hTPM, TSS_TSPATTRIB_TPM_CREDENTIAL,
			TSS_TPMATTRIB_EKCERT, ekCertLen, ekCert);
	free(ekCert);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "SetAttribData for EKCert: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on SetAttribData for EKCert\n", result);
//		exit(result);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		return result;
	}
#endif

	//rgbIdentityLabelData = labelString;
	labelString = (BYTE *)strdup("securityplatform"); 
	if( NULL == labelString ){
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		return result;
	}
	labelLen = strlen((char *)labelString) + 1;
	rgbIdentityLabelData = (BYTE *)Tspi_Native_To_UNICODE(labelString, &labelLen);
	if (rgbIdentityLabelData == NULL) {
//		printf("Trspi_Native_To_UNICODE failed\n");
		dbg_printf(DERROR, "Trspi_Native_To_UNICODE : %s\n", Trspi_Error_String(result));
		free(labelString);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		return result;
	}
	free(labelString);

#if 0
	{
	/* Work around a bug in Trousers 0.3.1 - remove this block when fixed */
	/* Force POPUP to activate, it is being ignored */
		BYTE *dummyblob1; UINT32 dummylen1;
		printf("----------HERE<%s:%d>\n", __func__, __LINE__);
		if (Tspi_TPM_OwnerGetSRKPubKey(*hTPM, &dummylen1, &dummyblob1)
				== TSS_SUCCESS) {
			Tspi_Context_FreeMemory (*hContext, dummyblob1);
		}
		printf("----------HERE<%s:%d>\n", __func__, __LINE__);
	}
#endif

	dbg_printf (DINFO, "Generating identity key...\n");
	result = Tspi_TPM_CollateIdentityRequest(*hTPM, *hSRK, 
				hPCAKey, labelLen, rgbIdentityLabelData, 
				hIdentKey, TSS_ALG_AES, &ulTCPAIdentityReqLength,
				&rgbTCPAIdentityReq);
	if (result != TSS_SUCCESS){
		dbg_printf(DERROR, "Tspi_TPM_CollateIdentityRequest: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on Tspi_TPM_CollateIdentityRequest\n", result);
//		exit(result);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_CloseObject(*hContext, hPCAKey);
		Tspi_Context_Close(*hContext);
		free(rgbIdentityLabelData);
		return result;
	}
	//TODO: uncomment below code after update trousers library! 
	//Tspi_FreeAttribData(*hTPM); 
	free(rgbIdentityLabelData);
	Tspi_Context_CloseObject(*hContext, hPCAKey);

	// debug
#if 0
	{
		int xx;
		if ((rgbTCPAIdentityReq == NULL) || (ulTCPAIdentityReqLength < 1)) {
			printf("Invalid rgbTCPAIdentityReq<%p> OR ulTCPAIdentityReqLength<%d>\n", rgbTCPAIdentityReq, ulTCPAIdentityReqLength);
//			exit(4);
			Tspi_Context_Close(*hContext);
			return TSS_E_FAIL;
		}

		printf("-- POST to PrivacyCA <%d byte%s> -- Begin --\n", ulTCPAIdentityReqLength, ulTCPAIdentityReqLength > 1 ? "s" : "");
		for (xx = 0; xx < (int)ulTCPAIdentityReqLength; xx++) {
			printf("[%02x]", rgbTCPAIdentityReq[xx]);
			if ((xx % 20) == 19)
				printf("\n");
		}
		if ((xx % 20) != 19)
			printf("\n");
		printf("-- POST to PrivacyCA -- End --\n");

		// dump to file
		xx = open("/tmp/aik.req", O_CREAT|O_WRONLY|O_TRUNC, 0644);
		if (xx < 0) {
			printf("open(aik.req) failed: %m");
		} else {
			if(write(xx, rgbTCPAIdentityReq, ulTCPAIdentityReqLength) <= 0){
				printf("write failed\n");
			}
			close(xx);
		}
	}
#endif

	// Sending request to PrivacyCA
	// Return 0 is success 
	asymBufSize = sizeof(asymBuf); // 2048/8 bytes (const) -- RSA key
	if(0 != request_to_pca(&symBuf, &symBufSize, privacy_ca, rgbTCPAIdentityReq, ulTCPAIdentityReqLength, asymBufSize)){
		Tspi_Context_FreeMemory(*hContext, rgbTCPAIdentityReq);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return TSS_E_FAIL;
	}
	Tspi_Context_FreeMemory(*hContext, rgbTCPAIdentityReq);

	toFreeBuf = symBuf;
	//decrypt symBuf(AIK Cert.) by asymBuf(key)
	memcpy(asymBuf, symBuf, asymBufSize);
	symBufSize -= asymBufSize;
	symBuf += asymBufSize;
	printf("symBuf <%p> symBufSize <%u>\n", symBuf, symBufSize);
	/* debug */
	if(dbg_lv == DDBG){
		dump_bytes(asymBuf, asymBufSize, "rst.asym");
		dump_bytes(symBuf, symBufSize, "rst.sym");
	}

	result = Tspi_Key_LoadKey(hIdentKey, *hSRK);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_Key_LoadKey for AIK: %s\n", Trspi_Error_String(result));
//		printf("Error 0x%x on Tspi_Key_LoadKey for AIK\n", result);
//		exit(result);
		free(toFreeBuf);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

	// debug
	{
		UINT32 credSize = 0;
		UINT64 offset = 0;

		Trspi_UnloadBlob_UINT32(&offset, &credSize, symBuf);
		printf("offset<%ju> credSize<%u>\n", offset, credSize);
	}
	// debug

	result = Tspi_TPM_ActivateIdentity(*hTPM, hIdentKey, asymBufSize, asymBuf,
						symBufSize, symBuf,
						&credBufSize, &credBuf);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_TPM_ActivateIdentity: %s\n", Trspi_Error_String(result));
//		printf ("Error 0x%x on Tspi_TPM_ActivateIdentity\n", result);
//		exit(result);
		free(toFreeBuf);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return result;
	}
	free(toFreeBuf);

	result = Tspi_Context_UnregisterKey(*hContext,
				TSS_PS_TYPE_SYSTEM, AIK_UUID, &hIdentKey);
//	if(TSS_ERROR_CODE(result) != TSS_E_PS_KEY_NOTFOUND) 
	if(TSS_ERROR_CODE(result) == TSS_E_INVALID_HANDLE ) {
		dbg_printf(DERROR, "Tspi_Context_UnregisterKey: %s\n", Trspi_Error_String(result));
//		print_error("Tspi_Context_UnregisterKey", result);
		Tspi_Context_FreeMemory(*hContext, credBuf);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

	result = Tspi_Context_RegisterKey(*hContext, hIdentKey,
					TSS_PS_TYPE_SYSTEM, AIK_UUID , TSS_PS_TYPE_SYSTEM, SRK_UUID);
	if (result != TSS_SUCCESS && TSS_ERROR_CODE(result) != TSS_E_KEY_ALREADY_REGISTERED) {
//		print_error("Tspi_Context_RegisterKey", result);
		dbg_printf(DERROR, "Tspi_Context_RegisterKey: %s\n", Trspi_Error_String(result));
		Tspi_Context_FreeMemory(*hContext, credBuf);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return result;
	}

#if 0
	result = Tspi_GetAttribData(hIdentKey, TSS_TSPATTRIB_KEY_BLOB,
		TSS_TSPATTRIB_KEYBLOB_BLOB, &blobLen, &blob);
	if (result != TSS_SUCCESS) {
		printf ("Error 0x%x on Tspi_GetAttribData for key blob\n", result);
//		exit(result);
		Tspi_Context_FreeMemory(*hContext, credBuf);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return result;
	}
	/* debug */printf("-- blob<%p> blobLen<%d>\n", blob, blobLen);
	/*
	fwrite(blob, 1, blobLen, f_blob);
	fclose(f_blob);
	*/
	Tspi_Context_FreeMemory(*hContext, blob);
#endif

	/* Output credential in PEM format */
	tbuf = credBuf;
	*x509 = d2i_X509(NULL, (const BYTE **)&tbuf, credBufSize);
	if (*x509 == NULL) {
		dbg_printf (DERROR, "Unable to parse returned credential\n");
//		exit(1);
		Tspi_Context_FreeMemory(*hContext, credBuf);
		Tspi_Context_CloseObject(*hContext, hIdentKey);
		Tspi_Context_Close(*hContext);
		return TSS_E_FAIL;
	}
#if 0
	/*
	if (tbuf-credBuf != (int)credBufSize) {
		printf ("Note, not all data from privacy ca was parsed correctly\n");
	}

	*/

//	X509_free(x509);
#endif

	Tspi_Context_FreeMemory(*hContext, credBuf);
	Tspi_Context_CloseObject(*hContext, hIdentKey);

	dbg_printf(DINFO, "Get AIK Cert Success!\n");
//	printf("************************* context:[%p] *************\n", *hContext);
//	printf("************************* TPM:[%p] *************\n", *hTPM);
//	printf("************************* SRK:[%p] *************\n", *hSRK);
	return TSS_SUCCESS;
}


#ifndef REALEK

static BYTE fakeEKCert[0x41a] = {
/* 00000000 */ 0x30, 0x82, 0x04, 0x16, 0x30, 0x82, 0x02, 0xfe,
		0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0x40, /* |0...0..........@| */
/* 00000010 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, /* |...............0| */
/* 00000020 */ 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
		0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x3e, /* |...*.H........0>| */
/* 00000030 */ 0x31, 0x3c, 0x30, 0x3a, 0x06, 0x03, 0x55, 0x04,
		0x03, 0x13, 0x33, 0x49, 0x6e, 0x73, 0x65, 0x63, /* |1<0:..U...3Insec| */
/* 00000040 */ 0x75, 0x72, 0x65, 0x20, 0x44, 0x65, 0x6d, 0x6f,
		0x2f, 0x54, 0x65, 0x73, 0x74, 0x20, 0x45, 0x6e, /* |ure Demo/Test En| */
/* 00000050 */ 0x64, 0x6f, 0x72, 0x73, 0x65, 0x6d, 0x65, 0x6e,
		0x74, 0x20, 0x4b, 0x65, 0x79, 0x20, 0x52, 0x6f, /* |dorsement Key Ro| */
/* 00000060 */ 0x6f, 0x74, 0x20, 0x43, 0x65, 0x72, 0x74, 0x69,
		0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x30, 0x1e, /* |ot Certificate0.| */
/* 00000070 */ 0x17, 0x0d, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31,
		0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x17, /* |..010101000000Z.| */
/* 00000080 */ 0x0d, 0x34, 0x39, 0x31, 0x32, 0x33, 0x31, 0x32,
		0x33, 0x35, 0x39, 0x35, 0x39, 0x5a, 0x30, 0x00, /* |.491231235959Z0.| */
/* 00000090 */ 0x30, 0x82, 0x01, 0x37, 0x30, 0x22, 0x06, 0x09,
		0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, /* |0..70"..*.H.....| */
/* 000000a0 */ 0x07, 0x30, 0x15, 0xa2, 0x13, 0x30, 0x11, 0x06,
		0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, /* |.0...0...*.H....| */
/* 000000b0 */ 0x01, 0x09, 0x04, 0x04, 0x54, 0x43, 0x50, 0x41,
		0x03, 0x82, 0x01, 0x0f, 0x00, 0x30, 0x82, 0x01, /* |....TCPA.....0..| */
/* 000000c0 */ 0x0a, 0x02, 0x82, 0x01, 0x01, 0x00, 0x80, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000000d0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000000e0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000000f0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000100 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000110 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000120 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000130 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000140 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000150 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000160 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000170 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000180 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000190 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000001a0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000001b0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000001c0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
		0x01, 0x00, 0x01, 0xa3, 0x82, 0x01, 0x37, 0x30, /* |..............70| */
/* 000001d0 */ 0x82, 0x01, 0x33, 0x30, 0x37, 0x06, 0x03, 0x55,
		0x1d, 0x09, 0x04, 0x30, 0x30, 0x2e, 0x30, 0x16, /* |..307..U...00.0.| */
/* 000001e0 */ 0x06, 0x05, 0x67, 0x81, 0x05, 0x02, 0x10, 0x31,
		0x0d, 0x30, 0x0b, 0x0c, 0x03, 0x31, 0x2e, 0x31, /* |..g....1.0...1.1| */
/* 000001f0 */ 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x30, 0x14,
		0x06, 0x05, 0x67, 0x81, 0x05, 0x02, 0x12, 0x31, /* |......0...g....1| */
/* 00000200 */ 0x0b, 0x30, 0x09, 0x80, 0x01, 0x00, 0x81, 0x01,
		0x00, 0x82, 0x01, 0x02, 0x30, 0x50, 0x06, 0x03, /* |.0..........0P..| */
/* 00000210 */ 0x55, 0x1d, 0x11, 0x01, 0x01, 0xff, 0x04, 0x46,
		0x30, 0x44, 0xa4, 0x42, 0x30, 0x40, 0x31, 0x16, /* |U......F0D.B0@1.| */
/* 00000220 */ 0x30, 0x14, 0x06, 0x05, 0x67, 0x81, 0x05, 0x02,
		0x01, 0x0c, 0x0b, 0x69, 0x64, 0x3a, 0x30, 0x30, /* |0...g......id:00| */
/* 00000230 */ 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x12,
		0x30, 0x10, 0x06, 0x05, 0x67, 0x81, 0x05, 0x02, /* |0000001.0...g...| */
/* 00000240 */ 0x02, 0x0c, 0x07, 0x55, 0x6e, 0x6b, 0x6e, 0x6f,
		0x77, 0x6e, 0x31, 0x12, 0x30, 0x10, 0x06, 0x05, /* |...Unknown1.0...| */
/* 00000250 */ 0x67, 0x81, 0x05, 0x02, 0x03, 0x0c, 0x07, 0x69,
		0x64, 0x3a, 0x30, 0x30, 0x30, 0x30, 0x30, 0x0c, /* |g......id:00000.| */
/* 00000260 */ 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
		0x04, 0x02, 0x30, 0x00, 0x30, 0x75, 0x06, 0x03, /* |..U.......0.0u..| */
/* 00000270 */ 0x55, 0x1d, 0x20, 0x01, 0x01, 0xff, 0x04, 0x6b,
		0x30, 0x69, 0x30, 0x67, 0x06, 0x04, 0x55, 0x1d, /* |U. ....k0i0g..U.| */
/* 00000280 */ 0x20, 0x00, 0x30, 0x5f, 0x30, 0x25, 0x06, 0x08,
		0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x02, 0x01, /* | .0_0%..+.......| */
/* 00000290 */ 0x16, 0x19, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
		0x2f, 0x77, 0x77, 0x77, 0x2e, 0x70, 0x72, 0x69, /* |..http://www.pri| */
/* 000002a0 */ 0x76, 0x61, 0x63, 0x79, 0x63, 0x61, 0x2e, 0x63,
		0x6f, 0x6d, 0x2f, 0x30, 0x36, 0x06, 0x08, 0x2b, /* |vacyca.com/06..+| */
/* 000002b0 */ 0x06, 0x01, 0x05, 0x05, 0x07, 0x02, 0x02, 0x30,
		0x2a, 0x0c, 0x28, 0x54, 0x43, 0x50, 0x41, 0x20, /* |.......0*.(TCPA | */
/* 000002c0 */ 0x54, 0x72, 0x75, 0x73, 0x74, 0x65, 0x64, 0x20,
		0x50, 0x6c, 0x61, 0x74, 0x66, 0x6f, 0x72, 0x6d, /* |Trusted Platform| */
/* 000002d0 */ 0x20, 0x4d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0x20,
		0x45, 0x6e, 0x64, 0x6f, 0x72, 0x73, 0x65, 0x6d, /* | Module Endorsem| */
/* 000002e0 */ 0x65, 0x6e, 0x74, 0x30, 0x21, 0x06, 0x03, 0x55,
		0x1d, 0x23, 0x04, 0x1a, 0x30, 0x18, 0x80, 0x16, /* |ent0!..U.#..0...| */
/* 000002f0 */ 0x04, 0x14, 0x34, 0xa8, 0x8c, 0x24, 0x7a, 0x97,
		0xf8, 0xcc, 0xc7, 0x56, 0x6d, 0xfb, 0x44, 0xa8, /* |..4..$z....Vm.D.| */
/* 00000300 */ 0xd4, 0x41, 0xaa, 0x5f, 0x4f, 0x1d, 0x30, 0x0d,
		0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, /* |.A._O.0...*.H...| */
/* 00000310 */ 0x01, 0x01, 0x05, 0x05, 0x00, 0x03, 0x82, 0x01,
		0x01, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000320 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000330 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000340 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000350 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000360 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000370 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000380 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000390 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000003a0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000003b0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000003c0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000003d0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000003e0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 000003f0 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000400 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* |................| */
/* 00000410 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01                                      /* |..........|       */
};

#endif /* undef REALEK */
