#include <limits.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "remote_attestation.h"
#include "dbg_macros.h"
#include "identity.h"
#include "tpm_util.h"
#include "ralib.h"
//#include "ra_com_mini.h"

#define UUID_BYTE 0x5a
#define VERSION_FALSE 0x00
#define VERSION_TRUE 0x01

BYTE sizeOfSeclect[2] = "\x00\x03";
X509		*x509=NULL;

/*
 * @brief certification validation check 
 *        refresh certification when it expired 
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param hContext TPM handler 
 * @param hTPM TPM object handler 
 * @param hSRK TPM SRK handler
 * @param caaddr Privacy CA address 
 * @param x509 certification pointer 
 * @param fname certification stored file path and name 
*/ 
static TSS_RESULT x509_valid_check_n_renew(TSS_HCONTEXT *hContext, TSS_HTPM *hTPM, TSS_HKEY *hSRK, char *caaddr, X509 **x509, const char *fname){
	TSS_RESULT result;
	FILE *f_out;

	// certification expired! renew it!
	if(TSS_SUCCESS != is_x509_valid_period(*x509)){
		dbg_printf(DINFO,"Invalid Certification\n");
		if(*x509) {
			X509_free(*x509); 
			*x509 = NULL;
		}

		result = get_create_credential(hContext, hTPM, hSRK, caaddr, x509);
		if(result != TSS_SUCCESS){
			dbg_printf(DERROR, "get_create_credential :%s", Trspi_Error_String(result));
			if(*x509) {
				X509_free(*x509);
				*x509 = NULL;
			}
			Tspi_Context_Close(*hContext);
			return result;
		}
		else{
			f_out = fopen(fname, "w");
			if(f_out){
				PEM_write_X509(f_out, *x509);
				fclose(f_out);
			}
			else{
				dbg_printf(DERROR, "Failed to open file[%s]", fname);
				return TSS_E_FAIL;
			}
		}
	}
	return TSS_SUCCESS;
}

/*
 * @brief Do Remote Attestation 
 *        fill the response message to send attester  
 * @return TSS_SUCCESS on success, TSS_E_FAIL otherwize
 * @param nonce nonce from attester 
 * @param nonce_len nonce length 
 * @param receive_msg  response message to send attester
 * @param receive_msg_size response message size  
 * @param caaddr Privacy CA address 
 * @param pcr_list PCR index to TPM Quote
 * @param x509_path certification stored path 
*/ 
TSS_RESULT remote_attestation(BYTE * nonce, 
		int nonce_len, BYTE **receive_msg, 
		int * receive_msg_size, char *caaddr, BYTE* pcr_list, const char *x509_path) {

	TSS_HCONTEXT		hContext;
	TSS_HTPM		hTPM;
	TSS_HKEY		hSRK, hIdentKey ; 
	TSS_RESULT		result;
//	TSS_UUID	uuid0;
	TSS_HPCRS	hPcrComposite;
	TSS_VALIDATION validationData;
	TSS_UUID        AIK_UUID = BACKUP_KEY_UUID ;

	TCPA_KEY keyContainer;

	BYTE *keyData;
	UINT32 keyDataSize;
	BYTE *getnonce;
	UINT16 getnonce_size ;
	BYTE		*pcrData;
	UINT32		pcrSize;
	UINT16		offset;

	int i = 0, j = 0;
	uint32_t pcrCompositeSize = 0;
	unsigned char *pcrComposite; 
	uint32_t signedPCRSize = 0;
	unsigned char* signedPCR; 
	uint32_t keyPubSize;

	unsigned char keyPub[256];
	int pcrsizeOfSeclect = sizeOfSeclect[1]&0x03;
	TPM_PCRVALUE pcrValue[24];
	int nr_select_PCR = 0;
	int selectpcrSize = 0;

	FILE *f_out;
//	const char fname[] = "x509.pem";
//	BYTE  *pcr_tpm;  
	
//	memset(&uuid0, UUID_BYTE, sizeof(TSS_UUID));

	dbg_printf(DINFO,"Beginning remote_attestation()\n");

	if ((result = connect_load_srk(&hContext, &hSRK)))
		return result;

	if ((result = load_tpm(&hContext, &hTPM))){
		Tspi_Context_Close(hContext);
		return result;
	}

	if(x509){
		if(( result = x509_valid_check_n_renew(&hContext, &hTPM, &hSRK, caaddr, &x509, x509_path))){
			Tspi_Context_Close(hContext);
			return result;
		}
	}
	else{ 
//		printf("x509 path:%s\n", x509_path);
		f_out = fopen(x509_path, "r");
		if(f_out){
			x509 = PEM_read_X509(f_out, NULL, NULL, NULL);
			fclose(f_out);
		}
		else{
			dbg_printf(DERROR, "Failed to open file[%s]", x509_path);
		}

		if(( result = x509_valid_check_n_renew(&hContext, &hTPM, &hSRK, caaddr, &x509, x509_path))){
			Tspi_Context_Close(hContext);
			return result;
		}
	}
	

	{ // Get PCR Data
	result = Tspi_Context_LoadKeyByUUID(hContext,
						TSS_PS_TYPE_SYSTEM,
						AIK_UUID, &hIdentKey);
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_Context_LoadKeyByUUID %s", Trspi_Error_String(result));
		result = Tspi_Context_LoadKeyByUUID(hContext,
						TSS_PS_TYPE_SYSTEM, AIK_UUID, &hIdentKey);
		if (result != TSS_SUCCESS) {
			dbg_printf(DERROR, "Tspi_Context_LoadKeyByUUID 2 times. %s", Trspi_Error_String(result));
			Tspi_Context_Close(hContext);
			return result;
		}
	}
	
	getnonce_size = (UINT16)nonce_len;
	getnonce = nonce;
	if (dbg_lv == DDBG) {
		print_hex((char *)getnonce,getnonce_size);
	}

#ifdef TPM1_1  // TPM 1.1 
	result = Tspi_Context_CreateObject(hContext,
			TSS_OBJECT_TYPE_PCRS, 0, &hPcrComposite);
#else // TPM 1.2
	result = Tspi_Context_CreateObject(hContext,
			TSS_OBJECT_TYPE_PCRS, TSS_PCRS_STRUCT_INFO_SHORT, &hPcrComposite);
#endif 
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_Context_CreateObject %s", Trspi_Error_String(result));
		Tspi_Context_CloseObject(hContext, hIdentKey);
//		Tspi_Context_FreeMemory(hContext, NULL);
		Tspi_Context_Close(hContext);
		return result;
	}

	for (i = 0; i < pcrsizeOfSeclect* 8; i++) {
		if (pcr_list[i >> 3] & (1 << (i & 7))) {

			dbg_printf(DINFO,"*******[PCR Index:%d]********\n", i); 
#ifdef TPM1_1  // TPM 1.1 
			result = Tspi_PcrComposite_SelectPcrIndex(hPcrComposite, i);
#else // TPM 1.2
			result = Tspi_PcrComposite_SelectPcrIndexEx(hPcrComposite,i, TSS_PCRS_DIRECTION_RELEASE);
#endif
			if (result != TSS_SUCCESS) {
				dbg_printf(DERROR, "Tspi_PcrComposite_SelectPcrIndex %s", Trspi_Error_String(result));
				goto clean_exit;
			}
		}
	}
	
	for (i = 0, j = 0; i < pcrsizeOfSeclect* 8; i++) {
		if (pcr_list[i >> 3] & (1 << (i & 7))) {
			result = Tspi_TPM_PcrRead(hTPM, i, &pcrSize, &pcrData);
			if (result != TSS_SUCCESS) {
				dbg_printf(DERROR, "Tspi_TPM_PcrRead %s", Trspi_Error_String(result));
				goto clean_exit;
			}
			memcpy(&pcrValue[j].digest, pcrData, sizeof(TPM_PCRVALUE));
			if (dbg_lv == DDBG) {
			  printf("##The content of the pcr%d Value is:##\n", i);
			  print_hex((char *)pcrValue[j].digest, pcrSize);
			  printf("##The content of the pcr%d Value is over##\n", i);
			}
			Tspi_Context_FreeMemory(hContext, pcrData);
			j++;
		}
	}


	nr_select_PCR = j;
	selectpcrSize = sizeof(TPM_PCRVALUE) * nr_select_PCR;
	dbg_printf(DINFO,"selectpcrSize: %d  nr_select_PCR: %d\n",selectpcrSize,nr_select_PCR);

	validationData.ulExternalDataLength=getnonce_size;
	validationData.rgbExternalData = malloc(validationData.ulExternalDataLength);
	memcpy(validationData.rgbExternalData, getnonce, validationData.ulExternalDataLength);

#ifdef TPM1_1  // TPM 1.1 
    result = Tspi_TPM_Quote(hTPM, hIdentKey, hPcrComposite, &validationData);
#else // TPM 1.2 
	BYTE *versionInfo;
	UINT32 versionInfoSize;
	result = Tspi_TPM_Quote2(hTPM, hIdentKey, VERSION_FALSE, hPcrComposite, &validationData, &versionInfoSize, &versionInfo);
#endif
	if (result != TSS_SUCCESS) {
		dbg_printf(DERROR, "Tspi_TPM_Quote %s", Trspi_Error_String(result));
		goto clean_exit;
	}
	dbg_printf(DINFO, "Tspi_TPM_Quote Success\n");
	free(validationData.rgbExternalData);

	//*************************************//
	//******  get the publickey of AIK ****//
	//*************************************//
	if ((result = Tspi_GetAttribData(hIdentKey, TSS_TSPATTRIB_KEY_BLOB,
			       TSS_TSPATTRIB_KEYBLOB_BLOB, &keyDataSize, &keyData))) {
		dbg_printf(DERROR, "Tspi_GetAttribData %s", Trspi_Error_String(result));
		goto clean_exit;
	}
	if(dbg_lv == DDBG){
		printf("==== The content of AIK public key      ====\n");
		print_hex((char *)keyData, keyDataSize);
		printf("==== The content of AIK public key done ====\n");
	}

	offset = 0;
	memset(&keyContainer, 0x00, sizeof(TCPA_KEY));

	if ((result = Tspi_UnloadBlob_KEY(&offset, keyData, &keyContainer))){
		dbg_printf(DERROR, "UnloadBlob_TSS_KEY %s", Trspi_Error_String(result));
		goto clean_exit;
	}
	Tspi_Context_FreeMemory(hContext, keyData);

	keyPubSize = (uint32_t)keyContainer.pubKey.keyLength;
	memcpy(keyPub, keyContainer.pubKey.key, keyPubSize);
	if(keyContainer.algorithmParms.parms) free(keyContainer.algorithmParms.parms);
	if(keyContainer.PCRInfo) free(keyContainer.PCRInfo);
	if(keyContainer.encData) free(keyContainer.encData);
	if(keyContainer.pubKey.key) free(keyContainer.pubKey.key);


	pcrCompositeSize = validationData.ulDataLength;
	pcrComposite = validationData.rgbData;

	signedPCRSize = validationData.ulValidationDataLength;
	signedPCR = validationData.rgbValidationData;

#if 0
	pcr_tpm = malloc(selectpcrSize+1024);
	dbg_printf(DINFO,"pcr_tpmSize: %d\n",selectpcrSize);
	for (i = 0; i < nr_select_PCR ; i++) 
		memcpy(pcr_tpm + (i * sizeof(TPM_PCRVALUE)), &pcrValue[i].digest, sizeof(TPM_PCRVALUE));

	/*
	for (i = 0, j = 0; i < pcrsizeOfSeclect* 8; i++) {
		if (pcr_list[i >> 3] & (1 << (i & 7))) {
			j++;
			memcpy(pcr_tpm + (j-1) * sizeof(TPM_PCRVALUE), &pcrValue[i].digest,sizeof(TPM_PCRVALUE));
		}
	}
	*/

	offset = 0;
	int size = 2 + (sizeOfSeclect[1]&0x3) + 4 + 1024; 

	printf("requested malloc size for pcrSelect = %d\n",(size)); 
	pcrSelect = malloc(size); 
	memcpy(pcrSelect, sizeOfSeclect, 2);
	offset += 2;

	for(i=0;i<((sizeOfSeclect[1])&0x3);i++){ 
		pcrSelect[offset++]=pcr_list[i];
	}
	
	for(i=0;i<4;i++){ 
	  pcrSelect[offset++] = (selectpcrSize>>(8*(3-i)))&0xFF; 
	}

	printf("offset = %d\n", offset);
	for(i = 0;i<selectpcrSize;i++){
		pcrSelect[i + offset]=pcr_tpm[i]; 
	}
	
	// TODO why set to all 0? 
	/*
	for(i = 0;i<20;i++){
		pcrSelect[i + offset]=0;
	}
	*/

	if(dbg_lv >= DINFO){
		printf("pcr_tpmSize = %d\n pcr_tpm:\n", selectpcrSize);
		print_hex((char *)pcr_tpm,selectpcrSize);
	}
#endif
	

	*receive_msg_size = 4*sizeof(pcrCompositeSize) + pcrCompositeSize + signedPCRSize + keyPubSize;

	*receive_msg= malloc(*receive_msg_size);
	dbg_printf(DINFO,"receive_msg_size is %d\n", *receive_msg_size);

//	printf("set value : %d/%d/%d\n", pcrCompositeSize, signedPCRSize, keyPubSize );
	*((uint32_t *)(*receive_msg)) = pcrCompositeSize;
	*((uint32_t *)(*receive_msg) + 1) = signedPCRSize;
	*((uint32_t *)(*receive_msg) + 2) = keyPubSize;

	memcpy((*receive_msg) + 3*sizeof(pcrCompositeSize), pcrComposite, pcrCompositeSize);

	memcpy((*receive_msg) + 3*sizeof(pcrCompositeSize) + pcrCompositeSize, signedPCR, signedPCRSize);

	memcpy((*receive_msg) + 3*sizeof(pcrCompositeSize) + pcrCompositeSize + signedPCRSize, keyPub, keyPubSize);
	} // Get PCR Data

	dbg_printf(DINFO,"Cleaning up  <<<end_test>>>\n");
	dbg_printf(DINFO,"receive_msg_size: %d\n", *receive_msg_size);
	dbg_printf(DINFO,"Ending remote_attestation()\n");

clean_exit:
	Tspi_Context_CloseObject(hContext, hPcrComposite);
	Tspi_Context_CloseObject(hContext, hIdentKey);
//	Tspi_Context_FreeMemory(hContext, NULL);
	Tspi_Context_Close(hContext);

	return result;
}
