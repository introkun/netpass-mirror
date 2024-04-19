#include "loading.h"
#include <stdlib.h>
typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_loading;
	C2D_Text g_dots;
	float text_x;
	float text_y;
	float text_width;
	Thread thread;
	bool thread_done;
} DataStruct;

DataStruct* data = 0;

void threadFn(Scene* sc) {
	((void(*)(void))(sc->data))();
	data->thread_done = true;
}

void init(Scene* sc) {
	data = malloc(sizeof(DataStruct));
	data->g_staticBuf = C2D_TextBufNew(100);
	C2D_TextParse(&data->g_loading, data->g_staticBuf, "Loading    ");
	C2D_TextParse(&data->g_dots, data->g_staticBuf, "...");
	float height;
	C2D_TextGetDimensions(&data->g_loading, 1, 1, &data->text_width, &height);
	data->text_x = (SCREEN_TOP_WIDTH - data->text_width) / 2;
	data->text_y = (SCREEN_TOP_HEIGHT - height) / 2;

	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	data->thread_done = false;
	data->thread = threadCreate((void(*)(void*))threadFn, sc, 8*1024, prio-1, -2, true);
}

void render(Scene* sc) {
	C2D_DrawText(&data->g_loading, C2D_AlignLeft, data->text_x, data->text_y, 0, 1, 1);
	C2D_DrawText(&data->g_dots, C2D_AlignLeft, data->text_x + data->text_width - 35 + 10*(time(NULL)%2), data->text_y, 0, 1, 1);
}


void exits(Scene* sc) {
	C2D_TextBufDelete(data->g_staticBuf);
	threadJoin(data->thread, U64_MAX);
	threadFree(data->thread);
	free(data);
}

SceneResult process(Scene* sc) {
	if (data->thread_done) return scene_switch;
	return scene_continue;
}

void getLoadingScene(Scene* scene, Scene* next_scene, void(*func)(void)) {
	Scene new = {
		init, render, exits, process,
		.next_scene = next_scene,
		.data = (u32)func
	};
	memcpy(scene, &new, sizeof(Scene));
}