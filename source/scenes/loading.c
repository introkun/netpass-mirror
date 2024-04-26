#include "loading.h"
#include <stdlib.h>
#define N(x) scenes_loading_namespace_##x
#define _data ((N(DataStruct)*)sc->d)

typedef struct {
	C2D_TextBuf g_staticBuf;
	C2D_Text g_loading;
	C2D_Text g_dots;
	C2D_SpriteSheet spr;
	float text_x;
	float text_y;
	float text_width;
	Thread thread;
	bool thread_done;
} N(DataStruct);

void N(threadFn)(Scene* sc) {
	((void(*)(void))(sc->data))();
	_data->thread_done = true;
}

void N(init)(Scene* sc) {
	sc->d = malloc(sizeof(N(DataStruct)));
	if (!_data) return;
	_data->g_staticBuf = C2D_TextBufNew(100);
	//C2D_TextParse(&_data->g_loading, _data->g_staticBuf, "Loading    ");
	TextLangParse(&_data->g_loading, _data->g_staticBuf, str_loading);
	C2D_TextParse(&_data->g_dots, _data->g_staticBuf, "...");
	float height;
	C2D_TextGetDimensions(&_data->g_loading, 1, 1, &_data->text_width, &height);
	_data->text_x = (SCREEN_TOP_WIDTH - _data->text_width) / 2;
	_data->text_y = (SCREEN_TOP_HEIGHT - height) / 2;

	_data->spr = C2D_SpriteSheetLoad("romfs:/gfx/loading.t3x");

	_data->thread_done = false;
	_data->thread = threadCreate((void(*)(void*))N(threadFn), sc, 8*1024, main_thread_prio()-1, -2, false);
}

void N(render)(Scene* sc) {
	if (!_data) return;
	C2D_Image img = C2D_SpriteSheetGetImage(_data->spr, 0);
	C2D_DrawImageAt(img, 0, 0, 0, NULL, 1, 1);
	C2D_DrawText(&_data->g_loading, C2D_AlignLeft, _data->text_x, _data->text_y, 0, 1, 1);
	C2D_DrawText(&_data->g_dots, C2D_AlignLeft, _data->text_x + _data->text_width - 35 + 10*(time(NULL)%2), _data->text_y, 0, 1, 1);
}


void N(exit)(Scene* sc) {
	if (_data) {
		C2D_TextBufDelete(_data->g_staticBuf);
		C2D_SpriteSheetFree(_data->spr);
		threadJoin(_data->thread, U64_MAX);
		threadFree(_data->thread);
		free(_data);
	}
}

SceneResult N(process)(Scene* sc) {
	if (_data && _data->thread_done) return scene_switch;
	return scene_continue;
}

Scene* getLoadingScene(Scene* next_scene, void(*func)(void)) {
	Scene* scene = malloc(sizeof(Scene));
	if (!scene) return NULL;
	scene->init = N(init);
	scene->render = N(render);
	scene->exit = N(exit);
	scene->process = N(process);
	scene->next_scene = next_scene;
	scene->data = (u32)func;
	scene->need_free = true;
	return scene;
}