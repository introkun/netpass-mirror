#include "scene.h"
#include <malloc.h>

Scene* processScene(Scene* scene) {
	SceneResult res = scene->process(scene);
	switch (res) {
	case scene_continue:
		return scene;
	case scene_stop:
		scene->exit(scene);
		if (scene->need_free) {
			free(scene);
		}
		return 0;
	case scene_switch:
		Scene* new_scene = scene->next_scene;
		if (!new_scene) {
			printf("ERROR: Could not create scene!!");
			return NULL;
		}
		scene->exit(scene);
		if (scene->need_free) {
			free(scene);
		}
		new_scene->init(new_scene);
		return new_scene;
	}
	return scene;
}