#include "connection_error.h"
#include <stdlib.h>
#include <curl/curl.h>
#define N(x) scenes_error_namespace_##x

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_subtext;
} N(DataStruct);

N(DataStruct)* N(data) = 0;

void N(init)(Scene* sc) {
	N(data) = malloc(sizeof(N(DataStruct)));
	if (!N(data)) return;
	N(data)->g_staticBuf = C2D_TextBufNew(1000);
	Result res = (Result)sc->data;
	char str[200];
	const char* subtext = "";
	if (res > -100) {
		// libcurl error code
		int errcode = -res;
		const char* errmsg = curl_easy_strerror(errcode);
		snprintf(str, 200, "libCURL error (%d): %s", errcode, errmsg);
		if (errcode == 60) {
			subtext = "Make sure that your systems date and time are set correctly!";
		}
	} else if (res > -600) {
		// http status code
		int status_code = -res;
		snprintf(str, 200, "HTTP status code %d", status_code);
	} else {
		// 3ds error code
		snprintf(str, 200, "3DS error code %08lx", (u32)res);
	}
	C2D_TextParse(&N(data)->g_title, N(data)->g_staticBuf, str);
	C2D_TextParse(&N(data)->g_subtext, N(data)->g_staticBuf, subtext);
}

void N(render)(Scene* sc) {
	if (!N(data)) return;
	C2D_DrawText(&N(data)->g_title, C2D_AlignLeft, 10, 10, 0, 0.5, 0.5);
	C2D_DrawText(&N(data)->g_subtext, C2D_AlignLeft, 10, 35, 0, 0.5, 0.5);
}

void N(exit)(Scene* sc) {
	if (N(data)) {
		C2D_TextBufDelete(N(data)->g_staticBuf);
		free(N(data));
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getConnectionErrorScene(Result res) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->data = (u32)res;
	scene->need_free = true;
	return scene;
}