#include "home.h"
#include <stdlib.h>
#include "../api.h"
#define N(x) scenes_home_namespace_##x

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_home;
	C2D_Text g_entries[4];
	int cursor;
} N(DataStruct);

N(DataStruct)* N(data) = 0;
int N(new_location) = 0;

void N(init)(Scene* sc) {
	N(data) = malloc(sizeof(N(DataStruct)));
	if (!N(data)) return;
	N(data)->g_staticBuf = C2D_TextBufNew(2000);
	N(data)->cursor = 0;
	TextLangParse(&N(data)->g_home, N(data)->g_staticBuf, str_at_home);
	TextLangParse(&N(data)->g_entries[0], N(data)->g_staticBuf, str_goto_train_station);
	TextLangParse(&N(data)->g_entries[1], N(data)->g_staticBuf, str_goto_plaza);
	TextLangParse(&N(data)->g_entries[2], N(data)->g_staticBuf, str_goto_mall);
	TextLangParse(&N(data)->g_entries[3], N(data)->g_staticBuf, str_exit);
}

void N(render)(Scene* sc) {
	if (!N(data)) return;
	C2D_DrawText(&N(data)->g_home, C2D_AlignLeft, 10, 10, 0, 1, 1);
	for (int i = 0; i < 4; i++) {
		C2D_DrawText(&N(data)->g_entries[i], C2D_AlignLeft, 30, 10 + (i+1)*25, 0, 1, 1);
	}
	u32 clr = C2D_Color32(0, 0, 0, 0xff);
	int x = 10;
	int y = 10 + (N(data)->cursor + 1)*25 + 5;
	C2D_DrawTriangle(x, y, clr, x, y + 18, clr, x + 15, y + 9, clr, 1);
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
	if (N(data)) {
	N(data)->cursor += ((kDown & KEY_DOWN || kDown & KEY_CPAD_DOWN) && 1) - ((kDown & KEY_UP || kDown & KEY_CPAD_UP) && 1);
		if (N(data)->cursor < 0) N(data)->cursor = 0;
		if (N(data)->cursor > 3) N(data)->cursor = 3;
		if (kDown & KEY_A) {
			if (N(data)->cursor == 3) return scene_stop;
			// load location scene
			N(new_location) = N(data)->cursor;
			sc->next_scene = getLoadingScene(getLocationScene(N(new_location)), lambda(void, (void) {
				setLocation(N(new_location));
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