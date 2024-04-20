#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include <malloc.h>

#include <3ds.h>
#include <citro2d.h>
#include "cecd.h"
#include "curl-handler.h"
#include "scene.h"

#define VERSION "v0.1.0"

//#define BASE_URL "https://streetpass.sorunome.de"
#define BASE_URL "http://10.6.42.119:8080"

#define lambda(return_type, function_body) \
({ \
	return_type __fn__ function_body \
		__fn__; \
})

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
			res = cecdReadMessage(title_id, true, outbox.messages[j].message_size, msg, outbox.messages[j].message_id);
			if (R_FAILED(res)) {
				free(msg);
				continue;
			}
			char url[50];
			sprintf(url, "%s/outbox/upload", BASE_URL);
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
	for (int i = 0; i < mbox_list.num_boxes; i++) {
		printf("Checking inbox %d/%ld", i+1, mbox_list.num_boxes);
		char url[100];
		sprintf(url, "%s/inbox/%s/pop", BASE_URL, mbox_list.box_names[i]);
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
	sprintf(url, "%s/location/current", BASE_URL);
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

static int location;

int main() {
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
	printf("Starting StreetPass %s\n", VERSION);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	romfsInit();
	curlInit();
	srand(time(NULL));

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	cecdInit();
/*
	Result upload_messages = uploadOutboxes();
	if (R_FAILED(upload_messages)) {
		printf("Failed to upload StreetPass data: %ld\n", upload_messages);
	} else {
		printf("Uploaded %ld StreetPass message(s)\n", upload_messages);
	}*/
/*
	Result download_messages = downloadInboxes();
	if (R_FAILED(download_messages)) {
		printf("Failed to download StreetPass data: %ld\n", download_messages);
	} else {
		printf("Downloaded %ld new StreetPass message(s)\n", download_messages);
	}*/

	Scene* scene = getLoadingScene(getSwitchScene(lambda(Scene*, (void) {
		if (location == -1) {
			return getHomeScene(); // load home
		}
		return 0;
	})), lambda(void, (void) {
		uploadOutboxes();
		Result res = getLocation();
		if (R_FAILED(res) && res != -1) {
			printf("ERROR failed to get location: %ld\n", res);
			location = -1;
		} else {
			location = res;
			printf("Got location: %d\n", location);
		}
	}));

	scene->init(scene);

	while (aptMainLoop()) {
		Scene* new_scene = processScene(scene);
		if (!new_scene) break;
		if (new_scene != scene) {
			scene = new_scene;
			continue;
		}
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, C2D_Color32(0x68, 0xB0, 0xD8, 0xFF));
		C2D_SceneBegin(top);
		scene->render(scene);
		C3D_FrameEnd(0);
	}
	/*
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & KEY_START) break;
	*/
	C2D_Fini();
	C3D_Fini();
	curlExit();
	romfsExit();
	gfxExit();
	return 0;
}
