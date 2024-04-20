#pragma once

#include "../scene.h"

Scene* getSwitchScene(Scene*(*next_scene)(void));