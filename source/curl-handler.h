#pragma once

#include <3ds.h>
#include <curl/curl.h>

typedef struct {
	u8 *ptr;
	size_t len;
	size_t size;
} CurlReply;

void initCurlReply(CurlReply* r, size_t size);
void deinitCurlReply(CurlReply* r);
Result curlInit(void);
void curlExit(void);
void curl_handle_cleanup(CURL* curl);
Result httpRequestSetup(CURL* curl, char* method, char* url, int size, u8* body, CurlReply* reply);
Result httpRequestFinish(CURL* curl);
Result httpRequest(char* method, char* url, int size, u8* body, CurlReply* reply);