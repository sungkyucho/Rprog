/**
 * @date 2013/11/21
 * @version 0.0.0.1
 * @file Network.c
 **/

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

#include "Network.h"
#include "../Operation/GMMP_Operation.h"

int g_socket = -1;

void CloseSocket()
{
	if (g_socket <= 0) return;

	close(g_socket);
	g_socket = -1;

	GMMP_Printf(GMMP_ERROR_LEVEL_DEBUG, GMMP_LOG_MARKET_NOT, "Close Socket : IP = %s, Port = %d\n",g_szServerIP, g_nServerPort);

	return;
}

int CheckSocket()
{
	if( g_socket <= 0)
	{
		int nRet = Connect(g_szServerIP, g_nServerPort);
		if(nRet != GMMP_SUCCESS)
		{
			return nRet;
		}
	}

	return 0;
}

int Connect()
{
	int addrlen, rtn;
	struct sockaddr_in addr;

	if(strlen(g_szServerIP) <= 0)
	{
		return SERVER_INFO_NOT_FOUND;
	}

	g_socket = socket(AF_INET, SOCK_STREAM, 0);

	if(g_socket < 0 )
	{
		g_socket = -1;
		return SERVER_SOCKET_ERROR;
	}

	addrlen = sizeof(addr);
	memset( (char *)&addr, 0, addrlen);
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(g_nServerPort);
	addr.sin_addr.s_addr = inet_addr (g_szServerIP);

	rtn = connect (g_socket, (struct sockaddr *)&addr, sizeof(addr));
	if (rtn < 0)
	{
		perror("connect Error");
		CloseSocket ();
		return SERVER_CONNECT_ERROR;
	}

	GMMP_Printf(GMMP_ERROR_LEVEL_DEBUG, GMMP_LOG_MARKET_NOT, "Connect Socket : IP = %s, Port = %d\n",g_szServerIP, g_nServerPort);

	return GMMP_SUCCESS;
}

int WriteTCP(char* pBuf, int nLen)
{
	int nWriteLen = 0;
	int nWrittedLen = 0;

	if (pBuf == NULL || nLen <= 0) return LIB_PARAM_ERROR;

	do
	{
		nWriteLen = write(g_socket, pBuf+nWriteLen, nLen-nWriteLen);
		if(nWriteLen <= 0)
		{
			g_socket = -1;
			return SERVER_DISCONNECT;
		}

		nWrittedLen += nWriteLen;
	}while(nWrittedLen < nLen);

	return GMMP_SUCCESS;
}

void ClearBuffer()
{
	char Buffer[50];
	int nReaded = 0;
	while(true)
	{
		nReaded = read(g_socket, &Buffer, 50);
		if(nReaded < 50)
		{
			break;
		}

		usleep(100);
	}
}

int ReadTCP(char* pBuf, const int nMaxlen)
{
	if (pBuf == NULL || nMaxlen <= 0) return LIB_PARAM_ERROR;

	int nReadedLen = 0;
	int nReadLen = 0;

	memset(pBuf,  0, nMaxlen);

	while(nReadedLen < nMaxlen)
	{
		nReadLen  = read(g_socket, &pBuf[nReadedLen], nMaxlen - nReadedLen);
		if(nReadLen <= 0)
		{
			g_socket = -1;
			return SERVER_DISCONNECT;
		}

		nReadedLen += nReadLen;
	}

	return GMMP_SUCCESS;
}
