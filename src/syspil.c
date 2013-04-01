/*******************************************************************
* System Platform Independent Layer
* Distributed under GPLv3 license
* Copyright (c) 2005-11 Stanley Huang <stanleyhuangyc@gmail.com>
* All rights reserved.
*******************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include "syspil.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#endif

char *GetTimeString()
{
	static char buf[16];
	time_t tm=time(NULL);
	memcpy(buf,ctime(&tm)+4,15);
	buf[15]=0;
	return buf;
}

int IsDir(const char* pchName)
{
#ifdef WIN32
	DWORD attr=GetFileAttributes(pchName);
	if (attr==INVALID_FILE_ATTRIBUTES) return 0;
	return (attr & FILE_ATTRIBUTE_DIRECTORY)?1:0;
#else
	struct stat stDirInfo;
	if (stat( pchName, &stDirInfo) < 0) return 0;
	return (stDirInfo.st_mode & S_IFDIR)?1:0;
#endif //WIN32
}

int ReadDir(const char* pchDir, char* pchFileNameBuf)
{
#ifdef WIN32
	static HANDLE hFind=NULL;
	WIN32_FIND_DATA finddata;

	if (!pchFileNameBuf) {
		if (hFind) {
			FindClose(hFind);
			hFind=NULL;
		}
		return 0;
	}
	if (pchDir) {
		char *p;
		int len;
		if (hFind) FindClose(hFind);
		len = strlen(pchDir);
		p = malloc(len + 5);
		snprintf(p, len + 5, "%s\\*.*", pchDir);
		hFind=FindFirstFile(p,&finddata);
		free(p);
		if (hFind==INVALID_HANDLE_VALUE) {
			hFind=NULL;
			return -1;
		}
		strcpy(pchFileNameBuf,finddata.cFileName);
		return 0;
	}
	if (!hFind) return -1;
	if (!FindNextFile(hFind,&finddata)) {
		FindClose(hFind);
		hFind=NULL;
		return -1;
	}
	strcpy(pchFileNameBuf,finddata.cFileName);
#else
	static DIR *stDirIn=NULL;
	struct dirent *stFiles;

	if (!pchFileNameBuf) {
		if (stDirIn) {
			closedir(stDirIn);
			stDirIn=NULL;
		}
		return 0;
	}
	if (pchDir) {
		if (!IsDir(pchDir)) return -1;
		if (stDirIn) closedir(stDirIn);
		stDirIn = opendir( pchDir);
	}
	if (!stDirIn) return -1;
	stFiles = readdir(stDirIn);
	if (!stFiles) {
		closedir(stDirIn);
		stDirIn=NULL;
		return -1;
	}
	strcpy(pchFileNameBuf, stFiles->d_name);
#endif
	return 0;
}

int IsFileExist(const char* filename)
{
	struct stat stat_ret;
	if (stat(filename, &stat_ret) != 0) return 0;

	return (stat_ret.st_mode & S_IFREG) != 0;
}

#ifndef WIN32
unsigned int GetTickCount()
{
	struct timeval ts;
	gettimeofday(&ts,0);
	return ts.tv_sec * 1000 + ts.tv_usec / 1000;
}
#endif
