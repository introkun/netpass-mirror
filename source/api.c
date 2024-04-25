#include "api.h"
#include "cecd.h"
#include <stdlib.h>

Result uploadOutboxes(void) {
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
			if (!msg) {
				printf("ERROR: failed to allocate message\n");
				return -1;
			}
			res = cecdReadMessage(title_id, true, outbox.messages[j].message_size, msg, outbox.messages[j].message_id);
			if (R_FAILED(res)) {
				free(msg);
				continue;
			}
			char url[50];
			snprintf(url, 50, "%s/outbox/upload", BASE_URL);
			res = httpRequest("POST", url, outbox.messages[j].message_size, msg, 0);
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

Result downloadInboxes(void) {
	Result res = 0;
	Result messages = 0;
	CecMboxListHeader mbox_list;
	res = cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(CecMboxListHeader), (u8*)&mbox_list);
	if (R_FAILED(res)) return -1;
	CurlReply reply;
	reply.ptr = 0;
	for (int i = 0; i < mbox_list.num_boxes; i++) {
		printf("Checking inbox %d/%ld", i+1, mbox_list.num_boxes);
		char url[100];
		snprintf(url, 100, "%s/inbox/%s/pop", BASE_URL, mbox_list.box_names[i]);
		u32 http_code;
		do {
			printf(".");
			initCurlReply(&reply, MAX_MESSAGE_SIZE);
			res = httpRequest("GET", url, 0, 0, &reply);
			if (R_FAILED(res)) break;

			http_code = res;
			if (http_code == 200) {
				res = addStreetpassMessage(reply.ptr);
				if (!R_FAILED(res)) {
					messages++;
				}
			}
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

Result getLocation(void) {
	Result res;
	CurlReply reply;
	initCurlReply(&reply, 4);
	char url[80];
	snprintf(url, 80, "%s/location/current", BASE_URL);
	res = httpRequest("GET", url, 0, 0, &reply);
	if (R_FAILED(res)) goto cleanup;
	int http_code = res;
	if (http_code == 200) {
		res = *(u32*)(reply.ptr);
	} else {
		res = -1;
	}
cleanup:
	deinitCurlReply(&reply);
	return res;
}

Result setLocation(int location) {
	Result res;
	char url[80];
	snprintf(url, 80, "%s/location/%d/enter", BASE_URL, location);
	res = httpRequest("PUT", url, 0, 0, 0);
	if (R_FAILED(res)) {
		printf("ERROR: Failed to enter location %d: %ld\n", location, res);
		return res;
	}
	printf("Entered location %d!\n", location);
	return res;
}

static int dl_inbox_status = 1;
static bool dl_loop_running = true;
Thread bg_loop_thread = 0;
void triggerDownloadInboxes(void) {
	while (dl_inbox_status != 0) svcSleepThread((u64)1000000 * 100);
	dl_inbox_status = 1;
}

void bgLoop(void* p) {
	do {
		dl_inbox_status = 2;
		downloadInboxes();
		dl_inbox_status = 0;
		for(int i = 0; i < 10*60*5; i++) {
			svcSleepThread((u64)1000000 * 100);
			if (dl_inbox_status == 1 || !dl_loop_running) break;
		}
	} while(dl_loop_running);
}

void bgLoopInit(void) {
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	bg_loop_thread = threadCreate(bgLoop, NULL, 8*1024, prio-1, -2, false);
}

void bgLoopExit(void) {
	dl_loop_running = false;
	if (bg_loop_thread) {
		threadFree(bg_loop_thread);
	}
}