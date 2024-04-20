#include <3ds.h>
#include <citro2d.h>
#include "scene.h"
#include "api.h"
#include "cecd.h"

static int location;

int main() {
	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
	printf("Starting StreetPass %s\n", VERSION);
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	romfsInit();
	curlInit();
	//bgLoopInit();
	//srand(time(NULL));

	C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	cecdInit();

	Scene* scene = getLoadingScene(getSwitchScene(lambda(Scene*, (void) {
		if (location == -1) {
			return getHomeScene(); // load home
		}
		return getLocationScene(location);
	})), lambda(void, (void) {
		uploadOutboxes();
		downloadInboxes();
		Result res = getLocation();
		if (R_FAILED(res) && res != -1) {
			printf("ERROR failed to get location: %ld\n", res);
			location = -1;
		} else {
			location = res;
			if (location == -1) {
				printf("Got location home\n");
			} else {
				printf("Got location: %d\n", location);
			}
		}
	}));

	scene->init(scene);

	while (aptMainLoop()) {
		Scene* new_scene = processScene(scene);
		if (!new_scene) break;
		if (new_scene != scene) {
			scene = new_scene;
			continue;
		}
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TargetClear(top, C2D_Color32(0x68, 0xB0, 0xD8, 0xFF));
		C2D_SceneBegin(top);
		scene->render(scene);
		C3D_FrameEnd(0);
	}
	printf("Exiting...\n");
	//bgLoopExit();
	C2D_Fini();
	C3D_Fini();
	//curlExit();
	romfsExit();
	gfxExit();
	return 0;
}
