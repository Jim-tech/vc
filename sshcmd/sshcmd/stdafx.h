// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define dbgprint(...) \
    do\
    {\
		SYSTEMTIME __sys;\
		GetLocalTime(&__sys);\
		if (NULL != fplog) \
		{\
            fprintf(fplog, "[%04d-%02d-%02d %02d:%02d:%02d][%s][%d][0x%08x]", __sys.wYear, __sys.wMonth, __sys.wDay, \
            					__sys.wHour, __sys.wMinute, __sys.wSecond, __FUNCTION__, __LINE__, GetCurrentThreadId());\
    		fprintf(fplog, __VA_ARGS__);\
    		fprintf(fplog, "\n");\
    		fflush(fplog);\
		}\
		printf("[%04d-%02d-%02d %02d:%02d:%02d][%s][%d][0x%08x]", __sys.wYear, __sys.wMonth, __sys.wDay, \
        					__sys.wHour, __sys.wMinute, __sys.wSecond, __FUNCTION__, __LINE__, GetCurrentThreadId());\
        printf(__VA_ARGS__);\
        printf("\n");\
        fflush(stdout);\
        fflush(stderr);\
     }while(0)

// TODO: reference additional headers your program requires here
