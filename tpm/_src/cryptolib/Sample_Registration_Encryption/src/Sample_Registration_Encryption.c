
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#include "../GMMP_lib/GMMP.h"
#include "Setting.h"

struct config_data_t cfg;
const char *cfgfile = "gmmp.conf";

int Recv(GMMPHeader* pstGMMPHeader, void* pBody)
{
	U8 cResultCode;
	U8 cMessageType = pstGMMPHeader->ucMessageType;

	if(cMessageType == OPERATION_GW_REG_RSP)
	{
		stGwRegistrationRspHdr* pstRspHdr =(stGwRegistrationRspHdr*) pBody;

		cResultCode = pstRspHdr->ucResultCode;
		if(cResultCode != 0x00)
		{
			GMMP_Printf(GMMP_ERROR_LEVEL_ERROR, GMMP_LOG_MARKET_ERR, "%x-%s\n", pstRspHdr->ucResultCode, GetStringtoOMPErrorCode(pstRspHdr->ucResultCode) );

			return 1;
		}

		if(strlen((char*)pstGMMPHeader->usAuthKey) <= 0 || strlen((char*)pstRspHdr->usGWID) <= 0)
		{
			return 1;
		}

		SetAuthKey((char*)pstGMMPHeader->usAuthKey);
		SetGWID((char*)pstRspHdr->usGWID);

		//동작 구현
	}
	else if(cMessageType == OPERATION_DEVICE_REG_RSP)
	{
		stDeviceRegistrationRspHdr* pstRspHdr =(stDeviceRegistrationRspHdr*) pBody;

		cResultCode = pstRspHdr->ucResultCode;
		if(cResultCode != 0x00)
		{
			GMMP_Printf(GMMP_ERROR_LEVEL_ERROR, GMMP_LOG_MARKET_ERR, "%x-%s\n", pstRspHdr->ucResultCode, GetStringtoOMPErrorCode(pstRspHdr->ucResultCode) );

			return 1;
		}

		if(strlen((char*)pstRspHdr->usDeviceID) <= 0)
		{
			return 1;
		}

		//동작 구현
	}
	else if(cMessageType == OPERATION_ENCRYPTION_INFO_RSP)
	{
		stEncryptionInfoRspHdr* pstRspHdr =(stEncryptionInfoRspHdr*) pBody;

		if(pstRspHdr->ucEncryptionFlag == 0x01) //암호화 사용
		{
			//pstRspHdr->ucEncryptionAlgorithm;
			const char* pEncryptionKey = "12345"; //암호화 알고리즘에 맞는 키를 생성한 값

			GO_EncKey((char*)pstRspHdr->usGWID, (char*)pstRspHdr->usDeviceID, pEncryptionKey);
		}
		else
		{
			SetEncryption(false);

		}
				//구현
	}
	else if(cMessageType == OPERATION_ENCRYPTION_KEY_RSP)
	{
		stEncryptionKeyRspHdr* pstRspHdr =(stEncryptionKeyRspHdr*) pBody;

		cResultCode = pstRspHdr->ucResultCode;
		if(cResultCode != 0x00) //암호화 키 설정 완료
		{
			SetEncryption(false);
			GMMP_Printf(GMMP_ERROR_LEVEL_ERROR, GMMP_LOG_MARKET_ERR, "%x-%s\n", pstRspHdr->ucResultCode, GetStringtoOMPErrorCode(pstRspHdr->ucResultCode) );

			return 1;
		}

	}
	return 0;
}

int GW_Reg()
{
	int nRet = GMMP_SUCCESS;

	nRet = GO_Reg(NULL, cfg.pszGWMFID, true);

	if(nRet < 0)
	{
		printf("GW GO_Reg Error : %d\n", nRet);

		return 1;
	}

	return 0;
}

int Device_Reg()
{
	int nRet = GMMP_SUCCESS;

	nRet = GO_Reg(GetGWID(), cfg.pszDeviceMFID, true);

	if(nRet < 0)
	{
		printf("GW GO_Reg Error : %d\n", nRet);

		return 1;
	}

	return 0;
}

int Init()
{
	if(Initializaion(cfg.szServerIP, cfg.nServerPort, cfg.pszDomainCode , cfg.pszGWAuthID, GMMP_ON_LOG, cfg.nErrorLevel, GMMP_NETWORK_ALYWAYS_OFF, "Log") != 0)
	{
		printf("Server Connect Error\n");

		return 1;
	}

	SetCallFunction( (void*)Recv);

	return 0;
}

int main()
{
	int rc = 0;
	if (read_config(&cfg, cfgfile) < 0) {
	    printf("read_config Error\n");
	    rc = -1;
	    goto out;
	}

	if(Init() != 0)
	{
		printf("Init Error");
		rc = -1;
		goto out;
	}

	if(GW_Reg() != 0)
	{
		printf("GW_Reg Error");
		rc = -1;
		goto out;
	}

//	if(Device_Reg() != 0)
//	{
//		printf("Device_Reg Error");
//		return -1;
//	}

	free_config(&cfg);
out:
	return rc;

}

/* vim: ai:ts=4 sw=4
*/
