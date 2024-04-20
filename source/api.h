#pragma once

#include <3ds.h>
#include "curl-handler.h"

#define VERSION "v0.1.1"

#define BASE_URL "https://streetpass.sorunome.de"
//#define BASE_URL "http://10.6.42.119:8080"

#define lambda(return_type, function_body) \
({ \
	return_type __fn__ function_body \
		__fn__; \
})

Result uploadOutboxes(void);
Result downloadInboxes(void);
Result getLocation(void);
Result setLocation(int location);

void bgLoopInit(void);
void bgLoopExit(void);
void triggerDownloadInboxes(void);