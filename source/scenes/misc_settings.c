/**
 * NetPass
 * Copyright (C) 2025 Sorunome
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

#include "misc_settings.h"
#include "about.h"
#define N(x) scenes_misc_settings_namespace_##x
#define _data ((N(DataStruct)*)sc->d)
#define TEXT_BUF_LEN (STR_SETTINGS_LEN + STR_DOWNLOAD_DATA_LEN + STR_DELETE_DATA_LEN + STR_UPDATE_PATCHES_LEN + STR_BACK_LEN)

#define NUM_ENTRIES 5

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_entries[NUM_ENTRIES];
	int cursor;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(TEXT_BUF_LEN);
	_data->cursor  = 0;
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_settings);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_settings_about);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_download_data);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_delete_data);
	TextLangParse(&_data->g_entries[3], _data->g_staticBuf, str_update_patches);
	TextLangParse(&_data->g_entries[4], _data->g_staticBuf, str_back);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < NUM_ENTRIES; i++) {
		C2D_DrawText(&_data->g_entries[i], C2D_AlignLeft, 30, 10 + (i+1)*25, 0, 1, 1);
	}
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	int x = 10;
	int y = 10 + (_data->cursor + 1)*25 + 5;
	C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 1);
	u32 blue = C2D_Color32(0x2B, 0xCF, 0xFF, 0xFF);
	u32 pink = C2D_Color32(0xF5, 0xAB, 0xB9, 0xFF);
	u32 white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	C2D_DrawRectSolid(400 - 90, 240 - 60, 0, 90, 12, blue);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 12, 0, 90, 12, pink);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 24, 0, 90, 12, white);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 36, 0, 90, 12, pink);
	C2D_DrawRectSolid(400 - 90, 240 - 60 + 48, 0, 90, 12, blue);

	C2D_DrawRectSolid(400 - 180, 240 - 60, 0, 90, 10, C2D_Color32(0xE5, 0x00, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 50, 0, 90, 10, C2D_Color32(0xFF, 0x8D, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 40, 0, 90, 10, C2D_Color32(0xFF, 0xEE, 0x00, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 30, 0, 90, 10, C2D_Color32(0x02, 0x81, 0x21, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 20, 0, 90, 10, C2D_Color32(0x00, 0x4C, 0xFF, 0xFF));
	C2D_DrawRectSolid(400 - 180, 240 - 10, 0, 90, 10, C2D_Color32(0x77, 0x00, 0x88, 0xFF));
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
				// About
				sc->next_scene = getAboutScene();
				return scene_push;
			}
			if (_data->cursor == 1) {
				// download personal data
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					char url[50];
					snprintf(url, 50, "%s/data", BASE_URL);
					Result res = httpRequest("GET", url, 0, 0, (void*)1, "/netpass_data.txt", 0);
					if (R_FAILED(res)) {
						printf("ERROR downloading all data: %ld\n", res);
						return;
					}
					printf("Successfully downloaded all data!\n");
					printf("File stored at sdmc:/netpass_data.txt\n");
				}));
				return scene_push;
			}
			if (_data->cursor == 2) {
				// delete personal data
				sc->next_scene = getLoadingScene(0, lambda(void, (void) {
					char url[50];
					snprintf(url, 50, "%s/data", BASE_URL);
					Result res = httpRequest("DELETE", url, 0, 0, 0, 0, 0);
					if (R_FAILED(res)) {
						printf("ERROR deleting all data: %ld\n", res);
						return;
					}
					printf("Successfully sent request to delete all data! This can take up to 15 days.\n");
				}));
				return scene_push;
			}
			if (_data->cursor == 3) {
				// update patches
				sc->next_scene = getUpdatePatchesScene(NULL);
				return scene_push;
			}
			if (_data->cursor == 4) return scene_pop;
		}
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getMiscSettingsScene(void) {
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
