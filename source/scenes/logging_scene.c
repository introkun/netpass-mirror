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

#include "../logger.h"

#define N(x) scenes_logging_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN ( \
	STR_LOGGER_SETTINGS_LEN + \
	STR_LOGGER_SETTINGS_MESSAGE_LEN + \
	STR_LOGGER_LEVEL_TITLE_LEN + \
	STR_LOGGER_LEVEL_DEBUG_LEN + \
	STR_LOGGER_LEVEL_INFO_LEN + \
	STR_LOGGER_LEVEL_WARN_LEN + \
	STR_LOGGER_LEVEL_ERROR_LEN + \
	STR_LOGGER_LEVEL_NONE_LEN + \
	STR_LOGGER_OUTPUT_TITLE_LEN + \
	STR_LOGGER_OUTPUT_SCREEN_LEN + \
	STR_LOGGER_OUTPUT_FILE_LEN + \
	STR_LOGGER_OUTPUT_BOTH_LEN + \
	STR_LOGGER_OUTPUT_NONE_LEN + \
	STR_BACK_LEN \
)

typedef enum {
	MENU_ITEM_LOGGER_LEVEL = 0,
	MENU_ITEM_LOGGER_OUTPUT,
	MENU_ITEM_BACK,
	MENU_ITEM_MAX
} MenuItem;

const LanguageString* logger_level_strings[LOG_LEVEL_MAX] = {
	&str_logger_level_none,
	&str_logger_level_debug,
	&str_logger_level_info,
	&str_logger_level_warn,
	&str_logger_level_error
};

const LanguageString* logger_output_strings[LOG_OUTPUT_MAX] = {
	&str_logger_output_none,
	&str_logger_output_screen,
	&str_logger_output_file,
	&str_logger_output_both,
};

typedef struct {
	int cursor;
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_subtitle;
	C2D_Text g_logger_level_title;
	C2D_Text g_logger_levels[LOG_LEVEL_MAX];
	C2D_Text g_logger_output_title;
	C2D_Text g_logger_outputs[LOG_OUTPUT_MAX];
	C2D_Text g_back;
	int selected_logger_level;
	int selected_logger_output;
	float log_level_title_width;
	float log_level_width;
	float log_output_title_width;
	float log_output_width;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->cursor = 0;

	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_logger_settings);
	TextLangParse(&_data->g_subtitle, _data->g_staticBuf, str_logger_settings_message);
	TextLangParse(&_data->g_logger_level_title, _data->g_staticBuf, str_logger_level_title);
	for (int i = LOG_LEVEL_NONE; i < LOG_LEVEL_MAX; ++i) {
		TextLangParse(&_data->g_logger_levels[i], _data->g_staticBuf, *(logger_level_strings[i]));
	}
	for (int i = LOG_OUTPUT_NONE; i < LOG_OUTPUT_MAX; ++i) {
		TextLangParse(&_data->g_logger_outputs[i], _data->g_staticBuf, *(logger_output_strings[i]));
	}
	TextLangParse(&_data->g_logger_output_title, _data->g_staticBuf, str_logger_output_title);
	TextLangParse(&_data->g_back, _data->g_staticBuf, str_back);

	_data->selected_logger_level = config.log_level;
	_data->selected_logger_output = config.log_output;
	get_text_dimensions(&_data->g_logger_level_title, 0.5, 0.5, &_data->log_level_title_width, 0);
	get_text_dimensions(&_data->g_logger_levels[LOG_LEVEL_WARN], 0.5, 0.5, &_data->log_level_width, 0);
	get_text_dimensions(&_data->g_logger_output_title, 0.5, 0.5, &_data->log_output_title_width, 0);
	get_text_dimensions(&_data->g_logger_outputs[LOG_OUTPUT_SCREEN], 0.5, 0.5, &_data->log_output_width, 0);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	C2D_DrawText(&_data->g_title, C2D_AlignLeft | C2D_WithColor, 10, 10, 0, 1, 1, clr);
	C2D_DrawText(&_data->g_subtitle, C2D_AlignLeft | C2D_WithColor | C2D_WordWrap, 11, 40, 0, 0.5, 0.5, clr, 369.);

	C2D_DrawText(&_data->g_logger_level_title, C2D_AlignLeft | C2D_WithColor, 30, 74 + (0 * 14), 0, 0.5, 0.5);
	C2D_DrawText(&_data->g_logger_levels[_data->selected_logger_level], C2D_AlignLeft, _data->log_level_title_width + 60, 74 + (0 * 14), 0, 0.5, 0.5);
	C2D_DrawText(&_data->g_logger_output_title, C2D_AlignLeft | C2D_WithColor, 30, 74 + (1 * 14), 0, 0.5, 0.5);
	C2D_DrawText(&_data->g_logger_outputs[_data->selected_logger_output], C2D_AlignLeft, _data->log_level_title_width + 60, 74 + (1 * 14), 0, 0.5, 0.5);

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
	const u32 kDown = hidKeysDown();
	if (_data) {
		_data->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (_data->cursor < 0) {
			_data->cursor = MENU_ITEM_MAX - 1;
		} else if (_data->cursor > MENU_ITEM_MAX - 1) {
			_data->cursor = 0;
		}

		if (_data->cursor == MENU_ITEM_LOGGER_LEVEL) {
			const int old_level = _data->selected_logger_level;
			_data->selected_logger_level += ((kDown & KEY_RIGHT || kDown & KEY_CPAD_RIGHT) && 1) - ((kDown & KEY_LEFT || kDown & KEY_CPAD_LEFT) && 1);
			if (_data->selected_logger_level < LOG_LEVEL_NONE) _data->selected_logger_level = LOG_LEVEL_NONE;
			if (_data->selected_logger_level >= LOG_LEVEL_MAX) _data->selected_logger_level = LOG_LEVEL_MAX - 1;
			if (old_level != _data->selected_logger_level) {
				config.log_level = _data->selected_logger_level;
				configWrite();
				loggerSetLevel(_data->selected_logger_level);
				logDebug("Set logger level: %d\n", _data->selected_logger_level);
			}
		} else if (_data->cursor == MENU_ITEM_LOGGER_OUTPUT) {
			const int old_output = _data->selected_logger_output;
			_data->selected_logger_output += ((kDown & KEY_RIGHT || kDown & KEY_CPAD_RIGHT) && 1) - ((kDown & KEY_LEFT || kDown & KEY_CPAD_LEFT) && 1);
			if (_data->selected_logger_output < LOG_OUTPUT_NONE) _data->selected_logger_output = LOG_OUTPUT_NONE;
			if (_data->selected_logger_output >= LOG_OUTPUT_MAX) _data->selected_logger_output = LOG_OUTPUT_MAX - 1;
			if (old_output != _data->selected_logger_output) {
				config.log_output = _data->selected_logger_output;
				configWrite();
				loggerSetOutput(_data->selected_logger_output, NULL);
				logDebug("Set logger output: %d\n", _data->selected_logger_output);
			}
		}

		if (kDown & KEY_A) {
			if (_data->cursor == MENU_ITEM_BACK) return scene_pop;
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
