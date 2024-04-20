#include "home.h"
#include <stdlib.h>
#define N(x) scenes_home_namespace_##x

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_home;
	C2D_Text g_exit;
} N(DataStruct);

N(DataStruct)* N(data) = 0;

void N(init)(Scene* sc) {
	N(data) = malloc(sizeof(N(DataStruct)));
	N(data)->g_staticBuf = C2D_TextBufNew(2000);
	C2D_TextParse(&N(data)->g_home, N(data)->g_staticBuf, "You are currently at home.");
	C2D_TextParse(&N(data)->g_exit, N(data)->g_staticBuf, "Press start to exit");
}

void N(render)(Scene* sc) {
	C2D_DrawText(&N(data)->g_home, C2D_AlignLeft, 10, 10, 0, 1, 1);
	C2D_DrawText(&N(data)->g_exit, C2D_AlignLeft, 10, 35, 0, 1, 1);
}

void N(exit)(Scene* sc) {
	C2D_TextBufDelete(N(data)->g_staticBuf);
	free(N(data));
}

SceneResult N(process)(Scene* sc) {
	hidScanInput();
	u32 kDown = hidKeysDown();
	if (kDown & KEY_START) return scene_stop;
	return scene_continue;
}

Scene* getHomeScene(void) {
	Scene* scene = malloc(sizeof(Scene));
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->need_free = true;
	return scene;
}