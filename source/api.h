/**
 * NetPass
 * Copyright (C) 2024-2025 Sorunome
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <3ds.h>
#include "curl-handler.h"

#define BASE_URL "https://api.netpass.cafe"
//#define BASE_URL "https://devapi.netpass.cafe"

#define RULES_URL "http://netpass.cafe/rules.html"
#define PRIVACY_URL "http://netpass.cafe/privacy.html"

#define lambda(return_type, function_body) \
({ \
	return_type __fn__ function_body \
		__fn__; \
})

typedef struct {
	u32 title_id;
	char name[65];
} TitleDataEntry;

typedef struct {
	int num_titles;
	TitleDataEntry titles[12];
} NetpassTitleData;

Result initTitleData(void);
NetpassTitleData* getTitleData(void);
int numUsedTitles(void);
void clearIgnoredTitles(CecMboxListHeader* mbox_list);

Result doSlotExchange(void);
Result getLocation(void);
Result setLocation(int location);

void bgLoopInit(void);
void bgLoopExit(void);
void triggerDownloadInboxes(void);

s32 main_thread_prio(void);
void init_main_thread_prio(void);

extern int location;
extern FS_Archive sharedextdata_b;
