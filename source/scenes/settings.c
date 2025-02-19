#include "switch.h"
#include "../curl-handler.h"
#include "../boss.h"
#include "../utils.h"
#include <stdlib.h>
#include <malloc.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#define N(x) scenes_settings_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_title;
	C2D_Text g_entries[8];
	C2D_Text g_languages[NUM_LANGUAGES + 1];
	int cursor;
	int selected_language;
	float lang_width;
} N(DataStruct);

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(500  + 15*NUM_LANGUAGES);
	_data->cursor = 0;
	TextLangParse(&_data->g_title, _data->g_staticBuf, str_settings);
	TextLangParse(&_data->g_entries[0], _data->g_staticBuf, str_toggle_titles);
	TextLangParse(&_data->g_entries[1], _data->g_staticBuf, str_report_user);
	TextLangParse(&_data->g_entries[2], _data->g_staticBuf, str_language_pick);
	TextLangParse(&_data->g_entries[3], _data->g_staticBuf, str_download_data);
	TextLangParse(&_data->g_entries[4], _data->g_staticBuf, str_delete_data);
	TextLangParse(&_data->g_entries[5], _data->g_staticBuf, str_write_patches);
	TextLangParse(&_data->g_entries[6], _data->g_staticBuf, str_clear_spr_cache);
	TextLangParse(&_data->g_entries[7], _data->g_staticBuf, str_back);
	TextLangParse(&_data->g_languages[0], _data->g_staticBuf, str_system_language);
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		TextLangSpecificParse(&_data->g_languages[i+1], _data->g_staticBuf, str_language, all_languages[i]);
	}
	_data->selected_language = -1;
	if (config.language != -1) {
		for (int i = 0; i < NUM_LANGUAGES; i++) {
			if (all_languages[i] == config.language) {
				_data->selected_language = i;
			}
		}
	}
	get_text_dimensions(&_data->g_entries[2], 1, 1, &_data->lang_width, 0);
}
void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_DrawText(&_data->g_title, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < 8; i++) {
		C2D_DrawText(&_data->g_entries[i], C2D_AlignLeft, 30, 10 + (i+1)*25, 0, 1, 1);
	}
	C2D_DrawText(&_data->g_languages[_data->selected_language + 1], C2D_AlignLeft, 35 + _data->lang_width, 35 + 50, 0, 1, 1);
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
		if (_data->cursor < 0) _data->cursor = 7;
		if (_data->cursor > 7) _data->cursor = 0;
		if (_data->cursor == 2) {
			int old_lang = _data->selected_language;
			_data->selected_language += ((kDown & KEY_RIGHT || kDown & KEY_CPAD_RIGHT) && 1) - ((kDown & KEY_LEFT || kDown & KEY_CPAD_LEFT) && 1);
			if (_data->selected_language < -1) _data->selected_language = -1;
			if (_data->selected_language > NUM_LANGUAGES-1) _data->selected_language = NUM_LANGUAGES-1;
			if (old_lang != _data->selected_language) {
				config.language = _data->selected_language == -1 ? -1 : all_languages[_data->selected_language];
				configWrite();
			}
		}
		if (kDown & KEY_A) {
			if (_data->cursor == 0) {
				// toggle titles
				sc->next_scene = getToggleTitlesScene();
				return scene_push;
			}
			if (_data->cursor == 1) {
				// report users
				sc->next_scene = getReportListScene();
				return scene_push;
			}
			if (_data->cursor == 3) {
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
			if (_data->cursor == 4) {
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
			if (_data->cursor == 5) {
				// copy patches
				#define COPY_DSTDIR "/luma/sysmodules/"
				#define COPY_SRCDIR "romfs:/patches/"
				mkdir_p(COPY_DSTDIR);
				printf("Copying sysmodules...\n");
				DIR* d = opendir(COPY_SRCDIR);
				if (!d) {
					printf("ERROR: src dir not found\n");
					return scene_continue;
				}
				void* buffer = malloc(0x4000);
				if (!buffer) {
					printf("ERROR: malloc\n");
					return scene_continue;
				}
				struct dirent* p;
				char srcpath[100];
				char dstpath[100];
				while ((p = readdir(d))) {
					snprintf(srcpath, 100, "%s%s", COPY_SRCDIR, p->d_name);
					snprintf(dstpath, 100, "%s%s", COPY_DSTDIR, p->d_name);
					struct stat statbuf;
					if (!stat(srcpath, &statbuf) && !S_ISDIR(statbuf.st_mode)) {
						// ok we actually have a file, copy it
						printf("%s...", p->d_name);
						FILE* src = fopen(srcpath, "rb");
						FILE* dst = fopen(dstpath, "wb+");
						if (!src || !dst) {
							if (src) fclose(src);
							if (dst) fclose(dst);
							printf("ERROR: open\n");
							continue;
						}
						size_t len = fread(buffer, 1, 0x4000, src);
						if (!len) {
							fclose(src);
							fclose(dst);
							printf("ERROR: read\n");
							continue;
						}
						if (!fwrite(buffer, len, 1, dst)) {
							fclose(src);
							fclose(dst);
							printf("ERROR: write\n");
							continue;
						}
						fclose(src);
						fclose(dst);

						printf("Done\n");
					}
				}
				printf("Copying sysmodules done\n");
				free(buffer);
				return scene_continue;
			}
			if (_data->cursor == 6) {
				// clear spr cache
				#define SPRELAY_TITLE_ID 0x0004013000003400ll
				#define SPRELAY_TASK_ID "sprelay"
				printf("Clearing spr cache...");
				bossInit(SPRELAY_TITLE_ID, false);
				bossUnregisterTask(SPRELAY_TASK_ID, 0);
				bossUnregisterTask(SPRELAY_TASK_ID, 0);
				printf("Done\n");
				nsInit();
				NS_RebootSystem();
				printf("Rebooting system...");
				return scene_continue;
			}
			if (_data->cursor == 7) return scene_pop;
		}
	}
	if (kDown & KEY_B) return scene_pop;
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
