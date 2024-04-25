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
	C2D_Font str_font;
	C2D_Font subtext_font = _font(str_libcurl_error);
	if (res > -100) {
		// libcurl error code
		int errcode = -res;
		const char* errmsg = curl_easy_strerror(errcode);
		snprintf(str, 200, _s(str_libcurl_error), errcode, errmsg);
		str_font = _font(str_libcurl_error);
		if (errcode == 60) {
			subtext = _s(str_libcurl_date_and_time);
			subtext_font = _font(str_libcurl_date_and_time);
		}
	} else if (res > -600) {
		// http status code
		int status_code = -res;
		snprintf(str, 200, _s(str_httpstatus_error), status_code);
		str_font = _font(str_httpstatus_error);
	} else {
		// 3ds error code
		snprintf(str, 200, _s(str_3ds_error), (u32)res);
		str_font = _font(str_3ds_error);
	}
	C2D_TextFontParse(&N(data)->g_title, str_font, N(data)->g_staticBuf, str);
	C2D_TextFontParse(&N(data)->g_subtext, subtext_font, N(data)->g_staticBuf, subtext);
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