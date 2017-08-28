#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <trousers/tss.h>
#include <trousers/trousers.h>

#include "tpm_util.h"
#include "dbg_macros.h"

#define EVENT_DATA_SIZE 32 

static void print_hex(BYTE* buf, UINT32 len){
	UINT32 j;
	for (j = 0; j < len; j++) {
		printf("%02x", buf[j] & 0xff);
	}
	printf("\n");
}

static TSS_RESULT connect_load_TPM(TSS_HCONTEXT *hContext, TSS_HKEY *hSRK, TSS_HTPM *hTPM)
{
	TSS_RESULT result;
	TSS_HPOLICY	hSrkPolicy;
//	TSS_HPOLICY	hTPMPolicy;
	TSS_UUID	SRK_UUID = TSS_UUID_SRK;
	BYTE		secret[] = TSS_WELL_KNOWN_SECRET;

	result = Tspi_Context_Create( hContext );
	if ( result != TSS_SUCCESS )
	{
//		print_error( "Tspi_Context_Create", result );
		dbg_printf(DERROR, "Tspi_Context_Create: %s\n", Trspi_Error_String(result));
		return( result );
	}

		// Connect to Context
	result = Tspi_Context_Connect(*hContext, NULL); 
	if ( result != TSS_SUCCESS )
	{
//		print_error( "Tspi_Context_Connect", result );
		dbg_printf(DERROR, "Tspi_Context_Connect: %s\n", Trspi_Error_String(result));
		Tspi_Context_FreeMemory( *hContext, NULL );
		Tspi_Context_Close( *hContext );
		return( result );
	}

		//Load Key by UUID for SRK
	result = Tspi_Context_LoadKeyByUUID(*hContext, TSS_PS_TYPE_SYSTEM,
			SRK_UUID, hSRK);
	if (result != TSS_SUCCESS) {
//		print_error("Tspi_Context_LoadKeyByUUID", result);
		dbg_printf(DERROR, "Tspi_Context_LoadKeyByUUID: %s\n", Trspi_Error_String(result));
		Tspi_Context_Close(*hContext);
		return(result);
	}

	result = Tspi_GetPolicyObject(*hSRK, TSS_POLICY_USAGE, &hSrkPolicy);
	if (result != TSS_SUCCESS) {
		printf ("Error 0x%x on Tspi_GetPolicyObject for SRK\n", result);
		return(result);
	}

	result = Tspi_Policy_SetSecret(hSrkPolicy, TSS_SECRET_MODE_SHA1, 20, secret);
	if (result != TSS_SUCCESS) {
		printf ("Error 0x%x on Tspi_Policy_SetSecret for SRK\n", result);
		return(result);
	}

	/*
	result = Tspi_Context_CreateObject(*hContext, TSS_OBJECT_TYPE_POLICY,
			TSS_POLICY_USAGE, &hTPMPolicy);
	if (result != TSS_SUCCESS) {
		printf ("Error 0x%x on Tspi_CreateObject for TPM policy\n", result);
		exit(result);
	}
	result = Tspi_Policy_AssignToObject(hTPMPolicy, *hTPM);
	if (result != TSS_SUCCESS) {
		printf ("Error 0x%x on Tspi_Policy_AssignToObject for TPM\n", result);
		exit(result);
	}
	result = Tspi_Policy_SetSecret(hTPMPolicy, TSS_SECRET_MODE_SHA1, 20, secret);
	if (result != TSS_SUCCESS) {
		printf ("Error 0x%x on Tspi_Policy_SetSecret for TPM\n", result);
		exit(result);
	}
	*/

		// Retrieve TPM object of context
	result = Tspi_Context_GetTpmObject( *hContext, hTPM );
	if ( result != TSS_SUCCESS )
	{
//		print_error( "Tspi_Context_GetTpmObject", result );
		dbg_printf(DERROR, "Tspi_Context_GetTpmObject: %s\n", Trspi_Error_String(result));
		Tspi_Context_FreeMemory( *hContext, NULL );
		Tspi_Context_Close( *hContext );
		return( result );
	}

	return TSS_SUCCESS;
}

int pcr_extend_without_reset(uint16_t numPcrs, char *target)
{

	TSS_HCONTEXT	hContext;
	TSS_HKEY	hSRK;
	TSS_HTPM	hTPM;
	BYTE		*pcrValue;
	UINT32		ulPcrValueLength ;
	BYTE		*NewPcrValue;
	TSS_RESULT	result;
	BYTE		event_data[EVENT_DATA_SIZE];

    FILE*     pFile    = NULL;
	int inlen;

	TSS_PCR_EVENT event;
	memset(&event, 0, sizeof(TSS_PCR_EVENT));
	event.rgbEvent = event_data;
	event.ulEventLength = EVENT_DATA_SIZE;
	memset(&event_data, 0x00, EVENT_DATA_SIZE);

	//Create Context
	connect_load_TPM(&hContext, &hSRK, &hTPM);

	if(!(pFile = fopen(target, "rb"))){ 
		fprintf(stderr, "cannot open file[%s].\n", target); 
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close(hContext);
		return 0;
	} 
	fseek(pFile, 0, SEEK_END); 
	inlen = ftell(pFile); 
	rewind(pFile); 
	pcrValue= (BYTE *)malloc(sizeof(BYTE)*inlen); 
	fread(pcrValue, 1, inlen, pFile); 
	fclose(pFile);   

	printf("len:%d, pcrindex:%d\n", inlen, numPcrs);
	result = Tspi_TPM_PcrExtend(hTPM, numPcrs, inlen, pcrValue, &event,
			&ulPcrValueLength, &NewPcrValue);
	if (result != TSS_SUCCESS) {
//		print_error("Tspi_TPM_PcrExtend", result);
		dbg_printf(DERROR, "Tspi_TPM_PcrExtend: %s\n", Trspi_Error_String(result));
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close(hContext);
		free(pcrValue);
		return 0;
	}
	else{
		printf("PCR Extend[%02u]: \n", numPcrs);
		print_hex(NewPcrValue, ulPcrValueLength);
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close(hContext);
		free(pcrValue);
		return 1;
	}
}

int pcr_reset_extend(uint16_t numPcrs, char * target)
{

	TSS_HCONTEXT	hContext;
	TSS_HKEY	hSRK;
	TSS_HTPM	hTPM;
	BYTE		*pcrValue;
	UINT32		ulPcrValueLength;
	BYTE		*NewPcrValue;
	TSS_RESULT	result;
	BYTE		event_data[EVENT_DATA_SIZE];
	TSS_HPCRS	hPcrs;

    FILE*     pFile    = NULL;
	int inlen;

	TSS_PCR_EVENT event;
	memset(&event, 0, sizeof(TSS_PCR_EVENT));
	event.rgbEvent = event_data;
	event.ulEventLength = EVENT_DATA_SIZE;
	memset(&event_data, 0x00, EVENT_DATA_SIZE);

	//Create Context
	connect_load_TPM(&hContext, &hSRK, &hTPM);

	//Call PcrExtend
	result = Tspi_Context_CreateObject(hContext, TSS_OBJECT_TYPE_PCRS, 0, &hPcrs);
	if (result != TSS_SUCCESS) {
//		print_error("Tspi_Context_CreateObject", result);
		dbg_printf(DERROR, "Tspi_Context_CreateObject: %s\n", Trspi_Error_String(result));
		Tspi_Context_Close(hContext);
		return 0;
	}

	result = Tspi_PcrComposite_SelectPcrIndex(hPcrs, numPcrs);
	if (result != TSS_SUCCESS) {
//		print_error("Tspi_PcrComposite_SelectPcrIndex", result);
		dbg_printf(DERROR, "Tspi_PcrComposite_SelectPcrIndex: %s\n", Trspi_Error_String(result));
		Tspi_Context_Close(hContext);
		return 0;
	}

	//Call PcrReset
	result = Tspi_TPM_PcrReset(hTPM, hPcrs);
	if (result != TSS_SUCCESS) {
//		print_error("Tspi_TPM_PcrReset", result);
		dbg_printf(DERROR, "Tspi_TPM_PcrReset: %s\n", Trspi_Error_String(result));
		Tspi_Context_Close(hContext);
		return 0;
	}
	printf("PCR Reset[%02u] Success\n", numPcrs);

	if(!(pFile = fopen(target, "rb"))){ 
		fprintf(stderr, "cannot open file[%s].\n", target); 
		Tspi_Context_FreeMemory( hContext, NULL );
		Tspi_Context_Close( hContext );
		return 0;
	} 
	fseek(pFile, 0, SEEK_END); 
	inlen = ftell(pFile); 
	rewind(pFile); 
	pcrValue= (BYTE *)malloc(sizeof(BYTE)*inlen); 
	fread(pcrValue, 1, inlen, pFile); 
	fclose(pFile);   

	result = Tspi_TPM_PcrExtend(hTPM, numPcrs, inlen, pcrValue, &event,
			&ulPcrValueLength, &NewPcrValue);
	if (result != TSS_SUCCESS) {
//		print_error("Tspi_TPM_PcrExtend", result);
		dbg_printf(DERROR, "Tspi_TPM_PcrExtend: %s\n", Trspi_Error_String(result));
		Tspi_Context_Close(hContext);
		free(pcrValue);
		return 0;
	}
	else{
		printf("PCR Extend[%02u]: \n", numPcrs);
		print_hex(NewPcrValue, ulPcrValueLength);
		Tspi_Context_Close(hContext);
		free(pcrValue);
		return 1;
	}
}
