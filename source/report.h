/**
 * NetPass
 * Copyright (C) 2024 Sorunome
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
#include "cecd.h"

#define MAX_REPORT_ENTRIES_LEN 128

typedef struct {
	MiiData mii;
	u32 batch_id;
	CecTimestamp received;
} ReportListEntry;


typedef struct {
	int max_size;
	int cur_size;
	ReportListEntry entries[MAX_REPORT_ENTRIES_LEN];
} ReportList;

void saveMsgInLog(CecMessageHeader* msg);
bool loadReportList(ReportList* reports);