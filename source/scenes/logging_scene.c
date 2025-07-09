/**
 * NetPass
 * Copyright (C) 2025 introkun
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

#include "logging_scene.h"

#define N(x) scenes_logging_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_LOGGER_SETTINGS_LEN + STR_LOGGER_SETTINGS_MESSAGE_LEN + STR_LOGGER_LEVEL_LEN + \
	STR_LOGGER_OUTPUT_LEN + STR_BACK_LEN)

#define NUM_ENTRIES 3

typedef struct {
	int cursor;
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_subtitle;
	C2D_Text g_logger_level;
	C2D_Text g_logger_output;
	C2D_Text g_back;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->cursor = 0;

	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_logger_settings);
	TextLangParse(&_data->g_subtitle, _data->g_staticBuf, str_logger_settings_message);
	TextLangParse(&_data->g_logger_level, _data->g_staticBuf, str_logger_level);
	TextLangParse(&_data->g_logger_output, _data->g_staticBuf, str_logger_output);
	TextLangParse(&_data->g_back, _data->g_staticBuf, str_back);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	C2D_DrawText(&_data->g_title, C2D_AlignLeft | C2D_WithColor, 10, 10, 0, 1, 1, clr);
	C2D_DrawText(&_data->g_subtitle, C2D_AlignLeft | C2D_WithColor | C2D_WordWrap, 11, 40, 0, 0.5, 0.5, clr, 369.);

	C2D_DrawText(&_data->g_logger_level, C2D_AlignLeft | C2D_WithColor, 30, 74 + (0 * 14), 0, 0.5, 0.5);
	C2D_DrawText(&_data->g_logger_output, C2D_AlignLeft | C2D_WithColor, 30, 74 + (1 * 14), 0, 0.5, 0.5);

	C2D_DrawText(&_data->g_back, C2D_AlignLeft | C2D_WithColor, 30, 74 + (2*14), 0, 0.5, 0.5, clr);

	int x = 22;
	int y = _data->cursor*14 + 74 + 3;
	C2D_DrawTriangle(x, y, clr, x, y + 10, clr, x + 8, y + 5, clr, 0);
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
		if (_data->cursor < 0) _data->cursor = NUM_ENTRIES - 1;
		if (_data->cursor > NUM_ENTRIES - 1) _data->cursor = 0;
		if (kDown & KEY_A) {
			if (_data->cursor == 0) {
				// TODO: add change of logger level
				return scene_pop;
			}
			if (_data->cursor == 1) {
				// TODO: add change of logger output
				return scene_pop;
			}
			if (_data->cursor == 2) return scene_pop;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getLoggingScene(void) {
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
