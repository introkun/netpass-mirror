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

#include "switch.h"
#include "../report.h"
#include <stdlib.h>
#define N(x) scenes_report_list_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	ReportList* list;
	C2D_Text g_entries[MAX_REPORT_ENTRIES_LEN];
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;

	_data->list = malloc(sizeof(ReportList));

	if (!_data->list || !loadReportList(_data->list)) {
		free(_data);
		sc->d = NULL;
		return;
	}
	_data->g_staticBuf = C2D_TextBufNew(40 * MAX_REPORT_ENTRIES_LEN);

	for (int i = 0; i < _data->list->cur_size; i++) {
		ReportListEntry* entry = &_data->list->entries[i];
		char mii_name[11] = {0};
		utf16_to_utf8((u8*)mii_name, (const u16*)&entry->mii.mii_name, 11);
		char render_entry[50];
		snprintf(render_entry, 50, "%s  %04lu-%02d-%02d %02d:%02d:%02d", mii_name,
			entry->received.year, entry->received.month, entry->received.day,
			entry->received.hour, entry->received.minute, entry->received.second);
		C2D_TextFontParse(&_data->g_entries[i], getFontIndex(entry->mii.mii_options.char_set), _data->g_staticBuf, render_entry);
	}
}

void N(render)(Scene* sc) {
	if (!_data) {
		return;
	}
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	for (int i = 0; i < _data->list->cur_size; i++) {
		int ix = _data->list->cur_size - i - 1;
		C2D_DrawText(&_data->g_entries[ix], C2D_AlignLeft | C2D_WithColor, 30, 35 + i*14, 0, 0.5, 0.5, clr);
	}
}

void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data->list);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (!_data) return scene_pop;
	
	if (kDown & KEY_B) return scene_pop;
	return scene_continue;
}

Scene* getReportListScene(void) {
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