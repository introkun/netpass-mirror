#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <malloc.h>

#include <3ds.h>
#include "cecd.h"
#include "base64.h"

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
static u32 *SOC_buffer = NULL;

#define BASE_URL "https://streetpass.sorunome.de"
//#define BASE_URL "http://10.6.42.119:8080"

struct curlReply {
  u8 *ptr;
  size_t len;
  size_t size;
};

void initCurlReply(struct curlReply* r, size_t size) {
	if (!r->ptr) {
		r->len = 0;
		r->size = size;
		r->ptr = malloc(size);
	}
}
void deinitCurlReply(struct curlReply* r) {
	if (r->ptr) {
		free(r->ptr);
	}
	r->ptr = NULL;
	r->len = 0;
	r->size = 0;
}

size_t curlWrite(void *data, size_t size, size_t nmemb, void* ptr) {
	struct curlReply* r = (struct curlReply*)ptr;
	if (!r) {
		return size*nmemb; // let's just pretend we did all correct
	}
	initCurlReply(r, 0xFF);
	size_t new_len = r->len + size*nmemb;
	if (new_len > r->size) {
		if (new_len > MAX_MESSAGE_SIZE) return 0;
		r->size += new_len;
		u8* newptr = realloc(r->ptr, r->size);
		if (!newptr) {
			deinitCurlReply(r);
			return 0; // out of memory
		}
		r->ptr = newptr;
	}
	memcpy(r->ptr + r->len, data, size*nmemb);
	r->ptr[new_len] = '\0';
	r->len = new_len;
	return size*nmemb;
}

Result networkInit() {
	Result res;
	// ok, we have to init this first
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	if (!SOC_buffer) return -1;
	res = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (R_FAILED(res)) return res;
	curl_global_init(CURL_GLOBAL_ALL);
	return res;
}

Result httpRequest(CURL* curl, char* method, char* url, u8 mac[6], int size, u8* body, struct curlReply* reply) {
	Result res = 0;
	bool need_curl_cleanup = false;
	if (!curl) {
		curl = curl_easy_init();
		need_curl_cleanup = true;
	}
	if (!curl) return -1;

	struct curl_slist* headers = NULL;

	// add mac header
	char header_mac[25];
	char* header_mac_i = header_mac + sprintf(header_mac, "3ds-mac: ");
	for (int i = 0; i < 6; i++) {
		header_mac_i += sprintf(header_mac_i, "%02X", mac[i]);
	}
	headers = curl_slist_append(headers, header_mac);

	if (body) {
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
		headers = curl_slist_append(headers, "Content-Type: application/binary");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, size);
	}

	// set some options
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "3ds");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, reply);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		res = -res;
		goto cleanup;
	}
	long http_code = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (!(http_code >= 200 && http_code < 300)) {
		res = -http_code;
		goto cleanup;
	}
cleanup:
	if (need_curl_cleanup) {
		curl_easy_cleanup(curl);
	}
	return res;
}

Result getMac(u8 mac[6]) {
	Result res = 0;
	Handle handle;
	res = srvGetServiceHandle(&handle, "nwm::SOC");
	if (R_FAILED(res)) return res;

	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(8, 1, 0);
	cmdbuf[1] = 6;

	u32 saved_threadstorage[2];

	u32 *staticbufs = getThreadStaticBuffers();
	saved_threadstorage[0] = staticbufs[0];
	saved_threadstorage[1] = staticbufs[1];
	staticbufs[0] = IPC_Desc_StaticBuffer(6, 0);
	staticbufs[1] = (u32)mac;
	res = svcSendSyncRequest(handle);
	staticbufs[0] = saved_threadstorage[0];
	staticbufs[1] = saved_threadstorage[1];
	if (R_FAILED(res)) return res;
	res = (Result)cmdbuf[1];
	memcpy(mac, (u8*)cmdbuf[3], 6);
	return res;
}

Result uploadOutboxes(u8 mac[6]) {
	Result res = 0;
	Result messages = 0;
	CecMboxListHeader mbox_list;
	res = cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(CecMboxListHeader), (u8*)&mbox_list);
	if (R_FAILED(res)) return -1;
	for (int i = 0; i < mbox_list.num_boxes; i++) {
		printf("Uploading outbox %d/%ld...", i+1, mbox_list.num_boxes);
		int title_id = strtol((const char*)mbox_list.box_names[i], NULL, 16);
		CecBoxInfoFull outbox;
		res = cecdOpenAndRead(title_id, CEC_PATH_OUTBOX_INFO, sizeof(CecBoxInfoFull), (u8*)&outbox);
		if (R_FAILED(res)) continue;
		for (int j = 0; j < outbox.header.num_messages; j++) {
			u8* msg = malloc(outbox.messages[j].message_size);
			res = cecdReadMessage(title_id, true, outbox.messages[j].message_size, msg, outbox.messages[j].message_id);
			if (R_FAILED(res)) {
				printf("%ld", res);
				free(msg);
				continue;
			}
			char url[50];
			sprintf(url, "%s/outbox/upload", BASE_URL);
			res = httpRequest(0, "POST", url, mac, outbox.messages[j].message_size, msg, 0);
			if (R_FAILED(res)) {
				free(msg);
				continue;
			}
			messages++;
			free(msg);
		}
		if (R_FAILED(res)) {
			printf("Failed %ld\n", res);
		} else {
			printf("Done\n");
		}
	}
	return messages;
}

Result downloadInboxes(u8 mac[6]) {
	Result res = 0;
	Result messages = 0;
	CecMboxListHeader mbox_list;
	res = cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(CecMboxListHeader), (u8*)&mbox_list);
	if (R_FAILED(res)) return -1;
	struct curlReply reply;
	for (int i = 0; i < mbox_list.num_boxes; i++) {
		printf("Checking inbox %d/%ld", i+1, mbox_list.num_boxes);
		char url[100];
		sprintf(url, "%s/inbox/%s/pop", BASE_URL, mbox_list.box_names[i]);
		long http_code = 0;
		do {
			printf(".");
			initCurlReply(&reply, MAX_MESSAGE_SIZE);
			CURL* curl = curl_easy_init();
			res = httpRequest(curl, "GET", url, mac, 0, 0, &reply);
			if (R_FAILED(res)) {
				curl_easy_cleanup(curl);
				break;
			}

			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code == 200) {
				res = addStreetpassMessage(reply.ptr);
				if (!R_FAILED(res)) {
					messages++;
				}
			}
			curl_easy_cleanup(curl);
		} while (http_code == 200);
		if (R_FAILED(res)) {
			printf("Failed %ld\n", res);
		} else {
			printf("Done\n");
		}
	}
	deinitCurlReply(&reply);
	return messages;
}

int main() {
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	networkInit();
	srand(time(NULL));

	cecdInit();
	u8 mac[6] = {0, 0, 0, 0, 0, 0};
	getMac(mac);

	Result upload_messages = uploadOutboxes(mac);
	if (R_FAILED(upload_messages)) {
		printf("Failed to upload StreetPass data: %ld\n", upload_messages);
	} else {
		printf("Uploaded %ld StreetPass message(s)\n", upload_messages);
	}

	Result download_messages = downloadInboxes(mac);
	if (R_FAILED(download_messages)) {
		printf("Failed to download StreetPass data: %ld\n", download_messages);
	} else {
		printf("Downloaded %ld new StreetPass message(s)\n", download_messages);
	}

	printf("Press start to exit");

	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
	}
	gfxExit();
	curl_global_cleanup();
	return 0;
}
