#include "common.h"

int main(void){
	BYTE		secret[] = TSS_WELL_KNOWN_SECRET;
	TSS_HCONTEXT	hContext;
	TSS_RESULT	result;
	TSS_HTPM	hTPM;
	TSS_HKEY	hSRK;
	TSS_HKEY	hIdentKey;
	TSS_HPCRS	hPcrComposite;
	TSS_HPOLICY	srkUsagePolicy;
	UINT32		versionInfoSize;
	BYTE*		versionInfo;
	TSS_VALIDATION valData;
	BYTE*		data= "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12";
	uint32_t pcrCompositeSize = 0;
	unsigned char *pcrComposite; 
	int i,j,k;
	int pcrSelect[3][3] = {{4, 5, 23}, {4, 6, 23}, {5, 6, 23}};
	char pcrstr[128];

		//Create Context
	result = Tspi_Context_Create(&hContext);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Create ", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Connect Context
	result = Tspi_Context_Connect(hContext, NULL);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_Connect", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Get TPM Object
	result = Tspi_Context_GetTpmObject(hContext, &hTPM);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_GetTpmObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Load Key By UUID
	result = Tspi_Context_LoadKeyByUUID(hContext,
			TSS_PS_TYPE_SYSTEM,
			SRK_UUID, &hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_LoadKeyByUUID", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Get Policy Object
	result = Tspi_GetPolicyObject(hSRK, TSS_POLICY_USAGE, &srkUsagePolicy);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_GetPolicyObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Set Secret
	result = Tspi_Policy_SetSecret(srkUsagePolicy, TSS_SECRET_MODE_SHA1, 20, secret);
	if (result != TSS_SUCCESS) {
		printf ("Error 0x%x on Tspi_Policy_SetSecret for SRK\n", result);
		exit(result);
	}
		//Create object for the hIdentKey
	result = Tspi_Context_CreateObject(hContext,
			TSS_OBJECT_TYPE_RSAKEY,
			TSS_KEY_SIZE_2048 |TSS_KEY_TYPE_SIGNING, &hIdentKey);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Context_CreateObject", result);
		Tspi_Context_Close(hContext);
		exit(result);
	}
		//Create hIdentKey
	result = Tspi_Key_CreateKey(hIdentKey, hSRK, 0);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_CreateKey", result);
		Tspi_Context_CloseObject(hContext, hIdentKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}
	result = Tspi_Key_LoadKey(hIdentKey, hSRK);
	if (result != TSS_SUCCESS) {
		print_error("Tspi_Key_LoadKey", result);
		Tspi_Context_CloseObject(hContext, hIdentKey);
		Tspi_Context_Close(hContext);
		exit(result);
	}


	for(i=0; i<3; i++){

		//Create object for the hPcrComposite Key
		result = Tspi_Context_CreateObject(hContext,
				TSS_OBJECT_TYPE_PCRS, TSS_PCRS_STRUCT_INFO_SHORT,
				&hPcrComposite);
		if (result != TSS_SUCCESS) {
			print_error("Tspi_Context_CreateObject", result);
			Tspi_Context_CloseObject(hContext, hIdentKey);
			Tspi_Context_Close(hContext);
			exit(result);
		}

		for(j=0; j<3; j++){

			snprintf(pcrstr + j*2, sizeof(pcrstr), "%d ",  pcrSelect[i][j]);
			//SelectPcrIndexEx
			result = Tspi_PcrComposite_SelectPcrIndexEx(hPcrComposite, pcrSelect[i][j],
					TSS_PCRS_DIRECTION_RELEASE);
			if (result != TSS_SUCCESS) {
				print_error("Tspi_PcrComposite_SelectPcrIndexEx ", result);
				Tspi_Context_Close(hContext);
				Tspi_Context_CloseObject(hContext, hIdentKey);
				Tspi_Context_CloseObject(hContext, hPcrComposite);
				exit(result);
			}
		}

		valData.ulExternalDataLength = 20;
		valData.rgbExternalData = data;

		//Call TPM Quote2 with fAddVersion = TRUE
		result = Tspi_TPM_Quote2(hTPM, hIdentKey, FALSE, 
				hPcrComposite, &valData ,&versionInfoSize,&versionInfo);
		pcrCompositeSize = valData.ulDataLength;
		pcrComposite = valData.rgbData;
		printf("pcr composite data [%s]\n",pcrstr);
		for(k=32; k<pcrCompositeSize; k++){
			printf("%02X", pcrComposite[k]);
		}
		printf("\n");
		Tspi_Context_CloseObject(hContext, hPcrComposite);
	}

	Tspi_Context_Close(hContext);
	Tspi_Context_CloseObject(hContext, hIdentKey);
	return 0;
}
