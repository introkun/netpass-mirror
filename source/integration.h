/**
 * NetPass
 * Copyright (C) 2025 Sorunome
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

typedef struct {
	u32 magic; // 0x4C49504E "NPIL"
	u32 version;
	u32 size;
	u32 count;
} IntegrationListHeader;

typedef struct {
	u32 id;
	u32 type;
	char name[0x40];
	u16 source_ident;
	bool enabled;
	char padding;
} IntegrationListEntry;

typedef struct {
	IntegrationListHeader header;
	IntegrationListEntry entries[];
} IntegrationList;

void integrationExit(void);
IntegrationList* get_integration_list(void);
Result toggle_integration(u32 id);
