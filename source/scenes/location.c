#include "switch.h"
#include <stdlib.h>
#define N(x) scenes_location_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_location;
	C2D_Text g_entries[1];
	int cursor;
} N(DataStruct);

LanguageString* N(locations)[3] = {
	&str_at_train_station,
	&str_at_plaza,
	&str_at_mall,
};

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(2000);
	_data->cursor = 0;
	TextLangParse(&_data->g_location, _data->g_staticBuf, *N(locations)[sc->data]);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_exit);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_location, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < 1; i++) {
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
		if (_data->cursor > 0) _data->cursor = 0;
		if (kDown & KEY_A) {
			return scene_stop;
		}
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getLocationScene(int location) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->data = location;
	scene->need_free = true;
	return scene;
}