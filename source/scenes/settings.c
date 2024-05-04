#include "switch.h"
#include "../curl-handler.h"
#include <stdlib.h>
#define N(x) scenes_settings_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_entries[3];
	int cursor;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(2000);
	_data->cursor = 0;
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_settings);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_download_data);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_delete_data);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_back);
}
void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < 3; i++) {
		C2D_DrawText(&_data->g_entries[i], C2D_AlignLeft, 30, 10 + (i+1)*25, 0, 1, 1);
	}
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	int x = 10;
	int y = 10 + (_data->cursor + 1)*25 + 5;
	C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 1);
}
void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (_data) {
		_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (_data->cursor < 0) _data->cursor = 0;
		if (_data->cursor > 2) _data->cursor = 2;
		if (kDown & KEY_A) {
			if (_data->cursor == 0) {
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					char url[50];
					snprintf(url, 50, "%s/data", BASE_URL);
					Result res = httpRequest("GET", url, 0, 0, (void*)1, "/netpass_data.txt");
					if (R_FAILED(res)) {
						printf("ERROR downloading all data: %ld\n", res);
						return;
					}
					printf("Successfully downloaded all data!\n");
					printf("File stored at sd:/netpass_data.txt\n");
				}));
				return scene_push;
			}
			if (_data->cursor == 1) {
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					char url[50];
					snprintf(url, 50, "%s/data", BASE_URL);
					Result res = httpRequest("DELETE", url, 0, 0, 0, 0);
					if (R_FAILED(res)) {
						printf("ERROR deleting all data: %ld\n", res);
						return;
					}
					printf("Successfully deleted all data!\n");
				}));
				return scene_push;
			}
			if (_data->cursor == 2) return scene_pop;
		}
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getSettingsScene(void) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->is_popup = false;
	scene->need_free = true;
	return scene;
}