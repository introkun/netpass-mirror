#include "scene.h"

Scene* processScene(Scene* scene) {
	SceneResult res = scene->process(scene);
	switch (res) {
	case scene_continue:
		return scene;
	case scene_stop:
		scene->exit(scene);
		return 0;
	case scene_switch:
		Scene* new_scene = scene->next_scene;
		scene->exit(scene);
		new_scene->init(new_scene);
		return new_scene;
	}
	return scene;
}