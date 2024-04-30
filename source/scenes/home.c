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

#include "home.h"
#include <stdlib.h>
#include "../api.h"
#define N(x) scenes_home_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_home;
	C2D_Text g_entries[4];
	int cursor;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(2000);
	_data->cursor = 0;
	TextLangParse(&_data->g_home, _data->g_staticBuf, str_at_home);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_goto_train_station);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_goto_plaza);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_goto_mall);
	TextLangParse(&_data->g_entries[3], _data->g_staticBuf, str_exit);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_home, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < 4; i++) {
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
		if (_data->cursor > 3) _data->cursor = 3;
		if (kDown & KEY_A) {
			if (_data->cursor == 3) return scene_stop;
			// load location scene
			location = _data->cursor;
			sc->next_scene = getLoadingScene(getLocationScene(location), lambda(void, (void) {
				setLocation(location);
				triggerDownloadInboxes();
			}));
			return scene_switch;
		}
	}
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getHomeScene(void) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->need_free = true;
	return scene;
}