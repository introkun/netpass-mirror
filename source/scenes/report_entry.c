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

#include "report_entry.h"
#include "../report.h"
#include "../hmac_sha256/sha256.h"
#include <stdlib.h>
#include <malloc.h>
#include <turbojpeg.h>
#define N(x) scenes_report_entry_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_Image pane[4];
} N(ExtraDataLetterbox);

typedef struct {
	C2D_TextBuf g_staticBuf;
	ReportListEntry* entry;
	C2D_Text g_title;
	u32 title_ids[12];
	C2D_Text* g_game_names;
	C2D_Text* g_mii_names;
	ReportMessages* msgs;
	void* extra_data[12];
} N(DataStruct);

char* N(send_msg);
u32 N(send_transfer_id);

SceneResult N(report)(Scene* sc) {
	static const int msgmaxlen = 200;
	N(send_msg) = malloc(msgmaxlen + 1);
	SwkbdResult button;
	{
		char hint_text[50];
		char* mii_name[15];
		memset(mii_name, 0, sizeof(mii_name));
		utf16_to_utf8((u8*)mii_name, _data->entry->mii.mii_name, 15);
		snprintf(hint_text, 50, _s(str_report_user_hint), mii_name);
		SwkbdState swkbd;
		memset(N(send_msg), 0, msgmaxlen + 1);
		swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, msgmaxlen);
		swkbdSetHintText(&swkbd, hint_text);
		swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, _s(str_cancel), false);
		swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, _s(str_submit), true);
		swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN | SWKBD_MULTILINE);
		swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
		button = swkbdInputText(&swkbd, N(send_msg), msgmaxlen + 1);
	}
	if (button == SWKBD_D1_CLICK1) {
		// successfully submitted the input
		N(send_transfer_id) = _data->entry->transfer_id;
		printf("Got report: \"%s\", sending...\n", N(send_msg));
		Scene* scene = getLoadingScene(0, lambda(void, (void) {
			CecMessageHeader msg;
			Result res = reportGetSomeMsgHeader(&msg, N(send_transfer_id));
			if (R_FAILED(res)) {
				printf("ERROR: %lx\n", res);
				goto exit;
			}
			SHA256_HASH hash;
			Sha256Calculate(&msg, 0x28, &hash);
			ReportSendPayload* data = malloc(sizeof(ReportSendPayload));
			if (!data) goto exit;
			
			data->magic = 0x5053524e;
			data->version = 1;
			memcpy(data->message_id, msg.message_id, sizeof(CecMessageId));
			memcpy(&data->hash, &hash, sizeof(SHA256_HASH));
			memcpy(data->msg, N(send_msg), sizeof(data->msg));

			char url[50];
			snprintf(url, 50, "%s/report/new", BASE_URL);
			res = httpRequest("POST", url, sizeof(ReportSendPayload), (u8*)data, 0, 0, 0);
			free(data);
			if (R_FAILED(res)) {
				printf("Error sending report: %ld\n", res);
				goto exit;
			}

			printf("report sent\n");
		exit:
			free(N(send_msg));
		}));
		scene->pop_scene = sc->pop_scene;
		sc->next_scene = scene;
		return scene_switch;
	}
	free(N(send_msg));
	return scene_pop;
}

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	memset(sc->d, 0, sizeof(N(DataStruct)));
	_data->entry = (ReportListEntry*)sc->data;

	_data->msgs = malloc(sizeof(ReportMessages));
	if (!_data->msgs) {
		free(_data);
		sc->d = NULL;
		return;
	}

	if (!loadReportMessages(_data->msgs, _data->entry->transfer_id)) {
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
		sc->d = NULL;
		return;
	}

	_data->g_game_names = malloc(sizeof(C2D_Text) * _data->msgs->count);
	if (!_data->g_game_names) {
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
		sc->d = NULL;
		return;
	}

	_data->g_mii_names = malloc(sizeof(C2D_Text) * _data->msgs->count);
	if (!_data->g_mii_names) {
		free(_data->g_game_names);
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
		sc->d = NULL;
		return;
	}

	_data->g_staticBuf = C2D_TextBufNew(200 * (_data->msgs->count + 1));

	for (int i = 0; i < _data->msgs->count; i++) {
		ReportMessagesEntry* entry = &_data->msgs->entries[i];
		_data->extra_data[i] = 0;
		if (entry->mii) {
			char mii_name[11] = {0};
			utf16_to_utf8((u8*)mii_name, entry->mii->mii_name, 11);
			C2D_TextFontParse(&_data->g_mii_names[i], getFontIndex(entry->mii->mii_options.char_set), _data->g_staticBuf, mii_name);
		}
		if (entry->name) {
			C2D_TextParse(&_data->g_game_names[i], _data->g_staticBuf, entry->name);
		} else {
			char game_name[10];
			snprintf(game_name, 50, "%04lx", entry->title_id);
			C2D_TextParse(&_data->g_game_names[i], _data->g_staticBuf, game_name);
		}
		switch (entry->title_id) {
			case TITLE_LETTER_BOX: {
				if (!entry->data) break;
				_data->extra_data[i] = malloc(sizeof(N(ExtraDataLetterbox)));
				if (!_data->extra_data[i]) break;
				memset(_data->extra_data[i], 0, sizeof(N(ExtraDataLetterbox)));
				N(ExtraDataLetterbox)* ex_data = _data->extra_data[i];
				u8* data_count = entry->data;
				for (int j = 0; j < 4; j++) {
					u32 size = *((u32*)data_count);
					data_count += 4;
					if (size < 5000) { // protective measure
						if (!loadJpeg(&ex_data->pane[j], data_count, size)) {
							ex_data->pane[j].tex = 0;
						}
					}
					data_count += size;
					if (size % 4) data_count += 4 - (size % 4);
				}
			}
		}
	}
	C2D_TextParse(&_data->g_title, _data->g_staticBuf, "Report User");
}

void N(render)(Scene* sc) {
	if (!_data) {
		return;
	}
	int ycursor = 10;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, ycursor, 0, 1, 1);
	ycursor += 28;
	for (int i = 0; i < _data->msgs->count; i++) {
		ReportMessagesEntry* entry = &_data->msgs->entries[i];
		C2D_DrawText(&_data->g_game_names[i], C2D_AlignLeft, 20, ycursor, 0, 0.5, 0.5);
		ycursor += 14;
		if (entry->mii) {
			C2D_DrawText(&_data->g_mii_names[i], C2D_AlignLeft, 40, ycursor, 0, 0.5, 0.5);
			ycursor += 14;
		}
		switch (entry->title_id) {
			case TITLE_LETTER_BOX: {
				N(ExtraDataLetterbox)* ex_data = _data->extra_data[i];
				if (ex_data) {
					for (int j = 0; j < 4; j++) {
						if (!ex_data->pane[j].tex) continue;
						C2D_DrawImageAt(ex_data->pane[j], 40 + (82 * j), ycursor, 0, NULL, 1, 1);
					}
					ycursor += 50;
				}
				break;
			}
		}
		ycursor += 3; // bottom padding
	}
}

void N(exit)(Scene* sc) {
	if (_data) {
		for (int i = 0; i < 12; i++) {
			if (!_data->extra_data[i]) continue;
			ReportMessagesEntry* entry = &_data->msgs->entries[i];
			switch (entry->title_id) {
				case TITLE_LETTER_BOX: {
					N(ExtraDataLetterbox)* ex_data = _data->extra_data[i];
					for (int j = 0; j < 4; j++) {
						if (!ex_data->pane[j].tex) continue;
						C2D_ImageDelete(&ex_data->pane[j]);
					}
				}
			}
			free(_data->extra_data[i]);
		}
		C2D_TextBufDelete(_data->g_staticBuf);
		free(_data->g_mii_names);
		free(_data->g_game_names);
		freeReportMessages(_data->msgs);
		free(_data->msgs);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & KEY_A) {
		return N(report)(sc);
	}
	if (kDown & KEY_B) return scene_pop;
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getReportEntryScene(ReportListEntry* entry) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->is_popup = false;
	scene->need_free = true;
	scene->data = (int)entry;
	return scene;
}