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
#include "hmac_sha256/sha256.h"

typedef struct {
	u32 magic; // 0x5053524e "NRSP"
	int version; // 1
	CecMessageId message_id;
	SHA256_HASH hash;
	char msg[201];
} ReportSendPayload;

typedef struct {
	MiiData mii;
	u32 transfer_id;
	CecTimestamp received;
} ReportListEntry;

typedef struct {
	u32 magic; // 0x454C524e "NRLE"
	int version; // 1
	size_t max_size;
	size_t cur_size;
} ReportListHeader;

typedef struct {
	ReportListHeader header;
	ReportListEntry entries[];
} ReportList;

typedef struct {
	u32 total_size;
	u32 jpeg_size;
	u8 pad[0x60];
	u8* jpegs;
} ReportMessagesEntryLetterBox;

typedef struct {
	char* name;
	u32 title_id;
	MiiData* mii;
	void* data;
} ReportMessagesEntry;

typedef struct {
	int count;
	ReportMessagesEntry entries[12];
} ReportMessages;

void saveSlotInLog(CecSlotHeader* slot);
void saveMsgInLog(CecMessageHeader* msg);
ReportList* loadReportList(void);
bool loadReportMessages(ReportMessages* msgs, u32 transfer_id);
void freeReportMessages(ReportMessages* msgs);
Result reportGetSomeMsgHeader(CecMessageHeader* msg, u32 transfer_id);

void reportInit(void);