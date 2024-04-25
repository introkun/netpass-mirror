#pragma once

#include <3ds.h>
#include <citro2d.h>
#include "api.h"
#include "strings.h"

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240

typedef enum {scene_stop, scene_continue, scene_switch} SceneResult;

typedef struct Scene Scene;

struct Scene {
	void (*init)(Scene*);
	void (*render)(Scene*);
	void (*exit)(Scene*);
	SceneResult (*process)(Scene*);
	Scene* next_scene;
	u32 data;

	bool need_free;
};

Scene* processScene(Scene* scene);

#include "scenes/loading.h"
#include "scenes/switch.h"
#include "scenes/home.h"
#include "scenes/location.h"
#include "scenes/connection_error.h"