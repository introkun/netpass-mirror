#pragma once

#include <3ds.h>
#include <curl/curl.h>
#include "cecd.h"

typedef struct {
	u8 ptr[MAX_MESSAGE_SIZE];
	size_t len;
	int offset;
} CurlReply;

void initCurlReply(CurlReply* r, size_t size);
void deinitCurlReply(CurlReply* r);
Result curlInit(void);
void curlExit(void);
void curlFreeHandler(int offset);
Result httpRequest(char* method, char* url, int size, u8* body, CurlReply** reply);