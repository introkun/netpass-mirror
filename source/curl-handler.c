#include "curl-handler.h"
#include "cecd.h"
#include <malloc.h>
#include <string.h>
#define MAX_CONNECTIONS 20

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
static u32 *SOC_buffer = NULL;

static CURLM* curl_multi_handle;
static Thread curl_multi_thread;
static bool running = false;
static u8 mac[6];

#define CURL_HANDLE_STATUS_FREE 0
#define CURL_HANDLE_STATUS_RUNNING 1
#define CURL_HANDLE_STATUS_DONE 2
#define CURL_HANDLE_STATUS_RESET 3
struct CurlHandle {
	CURL* handle;
	CURLcode result;
	int status;
};

static struct CurlHandle handles[MAX_CONNECTIONS];

void curl_multi_loop(void* p) {
	running = true;
	int openHandles = 0;
	do {
		CURLMcode mc = curl_multi_perform(curl_multi_handle, &openHandles);
		if (mc != CURLM_OK) {
			printf("curl multi fail: %u\n", mc);
		}
		CURLMsg* msg;
		int msgsLeft;
		while ((msg = curl_multi_info_read(curl_multi_handle, &msgsLeft))) {
			if (msg->msg == CURLMSG_DONE) {
				for (int i = 0; i < MAX_CONNECTIONS; i++) {
					if (handles[i].handle == msg->easy_handle) {
						handles[i].result = msg->data.result;
						handles[i].status = CURL_HANDLE_STATUS_DONE;
						break;
					}
				}
			}
			svcSleepThread(0);
		}
		if (!openHandles) {
			svcSleepThread((u64)1000000 * 100);
		} else {
			svcSleepThread(1000000);
		}
		for (int i = 0; i < MAX_CONNECTIONS; i++) {
			if (handles[i].status == CURL_HANDLE_STATUS_RESET) {
				handles[i].handle = 0;
				handles[i].result = 0;
				handles[i].status = CURL_HANDLE_STATUS_FREE;
			}
		}
	} while (running);
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

void initCurlReply(CurlReply* r, size_t size) {
	r->len = 0;
	if (!r->ptr) {
		r->size = size;
		r->ptr = malloc(size);
	}
}

void deinitCurlReply(CurlReply* r) {
	if (r->ptr) {
		free(r->ptr);
	}
	r->ptr = NULL;
	r->len = 0;
	r->size = 0;
}

Result curlInit(void) {
	Result res;
	// ok, we have to init this first
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
	if (!SOC_buffer) return -1;
	res = socInit(SOC_buffer, SOC_BUFFERSIZE);
	if (R_FAILED(res)) return res;
	curl_global_init(CURL_GLOBAL_ALL);
	curl_multi_handle = curl_multi_init();

	res = getMac(mac);
	if (R_FAILED(res)) return res;

	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	curl_multi_thread = threadCreate(curl_multi_loop, NULL, 8*1024, prio-1, -2, true);

	return res;
}

void curlExit(void) {
	running = false;
	threadJoin(curl_multi_thread, U64_MAX);
	threadFree(curl_multi_thread);

	curl_global_cleanup();
	socExit();
}

void curl_handle_cleanup(CURL* curl) {
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if (handles[i].handle == curl) {
			handles[i].status = CURL_HANDLE_STATUS_RESET;
			break;
		}
	}
	curl_easy_cleanup(curl);
}

size_t curlWrite(void *data, size_t size, size_t nmemb, void* ptr) {
	CurlReply* r = (CurlReply*)ptr;
	if (!r) {
		return size*nmemb; // let's just pretend we did all correct
	}
	if (!r->ptr) {
		initCurlReply(r, 0xFF);
	}
	size_t new_len = r->len + size*nmemb;
	if (new_len > r->size) {
		if (new_len > MAX_MESSAGE_SIZE) return 0;
		r->size += new_len;
		u8* newptr = realloc(r->ptr, r->size);
		if (!newptr) {
			return 0; // out of memory
		}
		r->ptr = newptr;
	}
	memcpy(r->ptr + r->len, data, size*nmemb);
	r->ptr[new_len] = '\0';
	r->len = new_len;
	return size*nmemb;
}

Result httpRequestSetup(CURL* curl, char* method, char* url, int size, u8* body, CurlReply* reply) {
	Result res = 0;

	int curl_handle_slot = 0;
	bool found_handle_slot = false;
	for (; curl_handle_slot < MAX_CONNECTIONS; curl_handle_slot++) {
		if (handles[curl_handle_slot].status == CURL_HANDLE_STATUS_FREE) {
			found_handle_slot = true;
			handles[curl_handle_slot].status = CURL_HANDLE_STATUS_RUNNING;
			handles[curl_handle_slot].handle = curl;
			break;
		}
	}
	if (!found_handle_slot) {
		// TODO: dunno, wait or something?
		return -1;
	}

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
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "3ds");
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50);
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
	curl_easy_setopt(curl, CURLOPT_SERVER_RESPONSE_TIMEOUT, 5);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
	curl_easy_setopt(curl, CURLOPT_CAINFO, "romfs:/certs.pem");

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, reply);

	curl_multi_add_handle(curl_multi_handle, curl);
	return res;
}

Result httpRequestFinish(CURL* curl) {
	Result res = 0;
	int curl_handle_slot = 0;
	bool found_handle_slot = false;
	for (; curl_handle_slot < MAX_CONNECTIONS; curl_handle_slot++) {
		if (handles[curl_handle_slot].handle == curl) {
			found_handle_slot = true;
			break;
		}
	}
	if (!found_handle_slot) return -1;

	while (handles[curl_handle_slot].status != CURL_HANDLE_STATUS_DONE) {
		svcSleepThread((u64)1000000 * 1);
	}

	res = handles[curl_handle_slot].result;
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
	res = http_code;
cleanup:
	curl_handle_cleanup(curl);
	return res;
}

Result httpRequest(char* method, char* url, int size, u8* body, CurlReply* reply) {
	Result res = 0;
	CURL* curl = curl_easy_init();
	if (!curl) return -1;
	res = httpRequestSetup(curl, method, url, size, body, reply);
	if (R_FAILED(res)) {
		curl_handle_cleanup(curl);
		return res;
	}
	res = httpRequestFinish(curl);
	if (R_FAILED(res)) return res;

	return res;
}