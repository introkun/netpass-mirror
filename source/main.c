/**
 * NetPass
 * Copyright (C) 2024 Sorunome
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <3ds.h>
#include <citro2d.h>
#include <stdlib.h>
#include "scene.h"
#include "api.h"
#include "cecd.h"
#include "curl-handler.h"
#include "config.h"
#include "boss.h"

int main() {
	gfxInitDefault();
	cfguInit();
	amInit();
	nsInit();
	aptInit();
	frdInit();
	consoleInit(GFX_BOTTOM, NULL);
	printf("Starting NetPass v%d.%d.%d\n", _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	romfsInit();
	init_main_thread_prio();

	cecdInit();
	curlInit();
	srand(time(NULL));

	configInit(); // must be after cecdInit()
	stringsInit(); // must be after configInit()
	// mount sharedextdata_b so that we can read it later, for e.g. playcoins
	{
		u32 extdata_lowpathdata[3];
		memset(extdata_lowpathdata, 0, 0xc);
		extdata_lowpathdata[0] = MEDIATYPE_NAND;
		extdata_lowpathdata[1] = 0xf000000b;
		FS_Path extdata_path = {
			type: PATH_BINARY,
			size: 0xC,
			data: (u8*)extdata_lowpathdata,
		};
		archiveMount(ARCHIVE_SHARED_EXTDATA, extdata_path, "sharedextdata_b");
		FSUSER_OpenArchive(&sharedextdata_b, ARCHIVE_SHARED_EXTDATA, extdata_path);
	}

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	Scene* scene = getLoadingScene(getSwitchScene(lambda(Scene*, (void) {
		if (R_FAILED(location) && location != -1) {
			// something not working
			return getConnectionErrorScene(location);
		}

		if (0) {
			#define SPRELAY_TITLE_ID 0x0004013000003400ll
			#define SPRELAY_TASK_ID "sprelay"

			bossInit(SPRELAY_TITLE_ID, false);
			u32 bufsize = sizeof(BossHTTPHeader)*3;
			void* buf = malloc(bufsize);
			char* url = buf;
			char* task_ids = buf;
			BossHTTPHeaders http_headers = buf;
			Result res = 0;

			u8 status;
			u32 out1;
			u8 out2;
			//aptMainLoop();
			bossGetTaskIdList();
			bossGetTaskState(SPRELAY_TASK_ID, 0, &status, &out1, &out2);
			printf("status: %d\n", status);

			res = bossCancelTask(SPRELAY_TASK_ID);
			printf("cancel task: %lx\n", res);

			bossGetTaskState(SPRELAY_TASK_ID, 0, &status, &out1, &out2);
			printf("status: %d\n", status);

			u16 num_tasks;
			bossReceiveProperty(BOSSPROPERTY_TOTALTASKS, &num_tasks, sizeof(num_tasks));
			printf("num_tasks: %d\n", num_tasks);
			bossReceiveProperty(BOSSPROPERTY_TASKIDS, task_ids, bufsize);
			for (int i = 0; i < num_tasks; i++) {
				printf("task_id: %s\n", task_ids + i*8);
			}

			// dump the current properties

			/*FILE* f = fopen("/sprelay.dat", "wb");
			const int sizes[] = {1, 1, 4, 4, 4, 1, 1, 0x200, 4, 1, 0x100, 0x200, 4, 0x360, 4, 0xC, 1, 1, 1, 4, 4, 0x40, 4, 1, 1, 1, 4, 4};
			for (int i = 0; i < 0x1C; i++) {
				bossReceiveProperty(i, buf, sizes[i]);
				fwrite(buf, sizes[i], 1, f);
			}
			fclose(f);*/

			res = bossUnregisterTask(SPRELAY_TASK_ID, 0);
			printf("unregister task: %lx\n", res);
/*
			// add the task to register the sprelay thingy to netpass
			// u32 test_NsDataId = 0x57524248;//This can be anything.
			bossContext ctx;
			u32 test_NsDataId = 0x57524248;//This can be anything.
			bossReinit(0);

			printf("==================\n");

			bossUnregisterTask(SPRELAY_TASK_REGISTER, 0);
			bossDeleteNsData(test_NsDataId);
			res = bossSetStorageInfo(test_NsDataId, 0, 1);
			printf("Set storage info: %lx\n", res);
			bossSetupContextDefault(&ctx, 60, BASE_URL "/spr-register");
			http_headers = ctx.property_xd;

			strcpy(http_headers[0].name, "3ds-mac");
			getMacStr(http_headers[0].value);
			strcpy(http_headers[1].name, "3ds-nid");
			getNetpassId(http_headers[1].value, sizeof(http_headers[1].value));
			strcpy(http_headers[2].name, "3ds-netpass-version");
			snprintf(http_headers[2].value, sizeof(http_headers[2].value), "v%d.%d.%d", _VERSION_MAJOR_, _VERSION_MINOR_, _VERSION_MICRO_);
			
			res = bossSendContextConfig(&ctx);
			printf("Send Context: %lx\n", res);

			res = bossRegisterTask(SPRELAY_TASK_REGISTER, 0, 0);
			printf("Register task: %lx\n", res);

			res = bossStartTaskImmediate(SPRELAY_TASK_REGISTER);
			printf("Start task: %lx\n", res);

			printf("Waiting for task to finish... \n");

			while(1) {
				res = bossGetTaskState(SPRELAY_TASK_ID, 0, &status, &out1, &out2);
				if (R_FAILED(res)) {
					printf("Error: %lx", res);
					break;
				}
				if (status!=BOSSTASKSTATUS_STARTED) break;
				printf(".");
				svcSleepThread(1000000000ll); // 1s
			}
			printf("\n");
			printf("status: %d\n", status);

			// notification title: 0x32
			// notification body: 0x72
			// url offset: 0x21c
			// http headers offset: 0x41c
*/
			free(buf);
		}

		bgLoopInit();
		if (location == -1) {
			return getHomeScene(); // load home
		}
		return getLocationScene(location);
	})), lambda(void, (void) {
		Result res;
		// first, we gotta wait for having internet
		char url[50];
		snprintf(url, 50, "%s/ping", BASE_URL);
	check_internet:
		res = httpRequest("GET", url, 0, 0, 0, 0, 0);
		if (R_FAILED(res)) {
			if (res == -CURLE_COULDNT_RESOLVE_HOST) {
				svcSleepThread((u64)1000000 * 100);
				goto check_internet;
			}
			location = res;
			return;
		}
		uploadOutboxes();
		res = getLocation();
		if (R_FAILED(res) && res != -1) {
			printf("ERROR failed to get location: %ld\n", res);
			location = -1;
		} else {
			location = res;
			if (location == -1) {
				printf("Got location home\n");
			} else {
				printf("Got location: %d\n", location);
			}
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
		C2D_TargetClear(top, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
		C2D_SceneBegin(top);
		if (scene->is_popup) {
			scene->pop_scene->render(scene->pop_scene);
			C2D_Flush();
		}
		scene->render(scene);
		C3D_FrameEnd(0);
		svcSleepThread(1);
	}
	printf("Exiting...\n");
	bgLoopExit();
	C2D_Fini();
	C3D_Fini();
	//curlExit();
	romfsExit();
	aptExit();
	nsExit();
	amExit();
	cfguExit();
	gfxExit();
	return 0;
}
