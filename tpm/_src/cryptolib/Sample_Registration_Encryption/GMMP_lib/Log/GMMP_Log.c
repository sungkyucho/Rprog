/**
 * @date 2013/11/21
 * @version 0.0.0.1
 * @file GMMP_Log.c
 **/

#include "GMMP_Log.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int  g_LogFd = -1;


int GMMP_Log_Init(const char* pszFileName, int nDayPart)
{
	if(pszFileName == NULL || strlen(pszFileName) <= 0)
	{
		return LIB_PARAM_ERROR;
	}

	int openFlags;
	mode_t filePerms;

	if(g_LogFd != -1)
	{
		GMMP_Log_Close();
	}

/* HC */
	openFlags = O_CREAT | O_WRONLY | O_APPEND | O_NONBLOCK;
	filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; /*rw-r-r*/

//	openFlags = 1 | 2 | 3 | 4;

	//filePerms = 1 | 2 | 3 | 4; /*rw-r-r*/

	g_LogFd = open((const char*)pszFileName, openFlags, filePerms);
	if(g_LogFd == -1)
	{
		return GMMP_LOG_ERROR_OPEN;
	}

	return GMMP_SUCCESS;
}

void GMMP_Log_Close()
{
	if(g_LogFd > 0)
	{
		close(g_LogFd);
		g_LogFd = -1;
	}
}

int GMMP_Log_Write(char* pMessage, int nLen)
{
	if(g_LogFd < 0)
	{
		return  GMMP_LOG_ERROR_OPEN;
	}

	if(nLen > LEN_LOG_DATA)
	{
		return GMMP_LOG_LONG_STRING;
	}

	if( write(g_LogFd, pMessage, nLen) == -1)
	{
		return GMMP_LOG_ERROR_WRITE;
	}

	return GMMP_SUCCESS;
}

int _GetDateTime(char* pBuffer, int nLen)
{
	char szTime[LEN_TIME_TIME*2];
	char szDate[LEN_TIME_DATE];

	memset(szTime, 0, sizeof(szTime) );
	memset(szDate, 0, sizeof(szDate) );

	if(pBuffer == NULL || nLen < LEN_TIME_TIME*2)
	{
		return GMMP_LOG_LONG_STRING;
	}

	_GetDate(szDate, sizeof(szDate) );
	_GetTime(szTime, sizeof(szTime) );

	sprintf(pBuffer, "%s_%s", szDate, szTime);

	return GMMP_SUCCESS;
}

void _GetDate(char* pBuffer, int nLen)
{
	if(pBuffer == NULL || nLen < 1)
	{
		return;
	}

	time_t  t = time(NULL);

	strftime(pBuffer, nLen, "%Y%m%d", localtime(&t) );

	return;
}

void _GetTime(char* pBuffer, int nLen)
{
	if(pBuffer == NULL || nLen < 1)
	{
		return;
	}

	struct timeval t = {0, 0};
	struct tm *tm;

	gettimeofday(&t, NULL);
	tm = localtime(&t.tv_sec);

	sprintf(pBuffer, "%02d:%02d:%02d,%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, (int)t.tv_usec/1000);

	return;
}

