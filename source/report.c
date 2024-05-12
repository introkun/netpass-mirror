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

#include "report.h"
#include "config.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_DIR "/config/netpass/log/"
#define LOG_INDEX "/config/netpass/log/batch_ids.list"

bool loadReportList(ReportList* reports) {
	FILE* f = fopen(LOG_INDEX, "rb");
	if (!f) return false;

	fread(reports, sizeof(ReportList), 1, f);
	fclose(f);
	if (reports->max_size > MAX_REPORT_ENTRIES_LEN) return false;
	return true;
}

void saveMsgInLog(CecMessageHeader* msg) {
	ReportList* list = malloc(sizeof(ReportList));
	if (!list) return;
	mkdir_p(LOG_DIR);
	FILE* f = fopen(LOG_INDEX, "rb");
	if (!f) {
		// ok, file is empty, we have to create it
		f = fopen(LOG_INDEX, "wb");
		if (!f) goto error;
		memset(list, 0, sizeof(ReportList));
		list->max_size = MAX_REPORT_ENTRIES_LEN;
		fwrite(list, sizeof(ReportList), 1, f);
		fclose(f);
		f = fopen(LOG_INDEX, "rb");
		if (!f) goto error;
	}
	fread(list, sizeof(ReportList), 1, f);
	fclose(f);

	int found_i = -1;
	// find if the batch already exists
	for (int i = 0; i < list->cur_size; i++) {
		if (list->entries[i].batch_id == msg->batch_id) {
			found_i = i;
			break;
		}
	}
	char* b64name = b64encode(msg->message_id, 8);
	char filename[100];
	snprintf(filename, 100, "%s%lu/_%s", LOG_DIR, msg->batch_id, b64name);
	free(b64name);
	bool edited = false;
	if (found_i < 0) {
		// we have to add a new entry!
		if (list->max_size == list->cur_size) {
			// uho, all is full, gotta pop the first one of the list
			u32 rm_batch = list->entries[0].batch_id;
			char rm_dirname[100];
			snprintf(rm_dirname, 100, "%s/%ld", LOG_DIR, rm_batch);
			rmdir_r(rm_dirname);
			list->cur_size--;
			memmove(list->entries, list->entries + sizeof(ReportListEntry), list->cur_size * sizeof(ReportListEntry));
		}
		ReportListEntry* e = &list->entries[list->cur_size];
		e->batch_id = msg->batch_id;
		memcpy(&e->received, &msg->received, sizeof(CecTimestamp));
		found_i = list->cur_size;
		list->cur_size++;
		edited = true;
	}
	if (msg->title_id == 0x20800) {
		// mii plaza
		ReportListEntry* e = &list->entries[found_i];
		static const int cfpb_offset = 0x36bc;
		static const int cfpb_size = 0x88;
		if (msg->message_size > msg->total_header_size + cfpb_offset + cfpb_size) {
			CFPB* cfpb = (CFPB*)((u8*)msg + msg->total_header_size + cfpb_offset);
			if (cfpb->magic == 0x42504643) {
				Result r = decryptMii(&cfpb->nonce, &e->mii);
				if (!R_FAILED(r)) edited = true;
			}
		}
	}

	if (edited) {
		f = fopen(LOG_INDEX, "wb");
		if (!f) goto error;
		fwrite(list, sizeof(ReportList), 1, f);
		fclose(f);
	}
	mkdir_p(filename);
	f = fopen(filename, "wb");
	if (!f) goto error;
	fwrite(msg, msg->message_size, 1, f);
	fclose(f);

error:
	free(list);
}