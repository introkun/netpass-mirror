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
#include "base64.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LOG_DIR "/config/netpass/log/"
#define LOG_INDEX "/config/netpass/log/batch_ids.list"

Result APT_Wrap(u32 in_size, u8* in, u32 nonce_offset, u32 nonce_size, u32 out_size, u8* out) {
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x46,4,2); // 0x001F0084
	cmdbuf[1] = out_size;
	cmdbuf[2] = in_size;
	cmdbuf[3] = nonce_offset;
	cmdbuf[4] = nonce_size;

	cmdbuf[5] = IPC_Desc_Buffer(in_size, IPC_BUFFER_R);
	cmdbuf[6] = (u32)in;
	cmdbuf[7] = IPC_Desc_Buffer(out_size, IPC_BUFFER_W);
	cmdbuf[8] = (u32)out;

	Result res = aptSendCommand(cmdbuf);
	if (R_FAILED(res)) return res;
	res = (Result)cmdbuf[1];

	return res;
}

Result APT_Unwrap(u32 in_size, u8* in, u32 nonce_offset, u32 nonce_size, u32 out_size, u8* out) {
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x47,4,2); // 0x001F0084
	cmdbuf[1] = out_size;
	cmdbuf[2] = in_size;
	cmdbuf[3] = nonce_offset;
	cmdbuf[4] = nonce_size;

	cmdbuf[5] = IPC_Desc_Buffer(in_size, IPC_BUFFER_R);
	cmdbuf[6] = (u32)in;
	cmdbuf[7] = IPC_Desc_Buffer(out_size, IPC_BUFFER_W);
	cmdbuf[8] = (u32)out;

	Result res = aptSendCommand(cmdbuf);
	if (R_FAILED(res)) return res;
	res = (Result)cmdbuf[1];

	return res;
}

void test_stuffs(void) {
	uint8_t data[112] = {
		0x98, 0xF5, 0xB0, 0x39, 0x7C, 0xBB, 0x8A, 0x6C, 0x20, 0xA8, 0x6B, 0xFF, 0xFD, 0x53, 0x71, 0x67, 
		0x2C, 0x7D, 0xDF, 0xB9, 0xFC, 0xC6, 0x5C, 0xAF, 0x81, 0x49, 0x30, 0xAE, 0xC2, 0x46, 0xAE, 0x44, 
		0xF8, 0x64, 0xCC, 0xEF, 0xDF, 0xE7, 0x2C, 0x41, 0x93, 0x54, 0x6E, 0x43, 0xD6, 0x5B, 0xD4, 0x88, 
		0x6C, 0xE6, 0x57, 0xD8, 0xB2, 0xFF, 0xD5, 0x62, 0xF2, 0x17, 0x4F, 0x0E, 0xF2, 0x59, 0xF8, 0x96, 
		0x5F, 0x0F, 0x6F, 0x3A, 0xC0, 0x98, 0x69, 0xDA, 0x30, 0xD4, 0xD6, 0x58, 0xD8, 0xF7, 0xD5, 0x3C, 
		0x93, 0x1E, 0xA5, 0x51, 0x2D, 0x9D, 0xEA, 0x7B, 0xE1, 0x62, 0x38, 0xDF, 0x5A, 0x73, 0x84, 0xC7, 
		0x63, 0xD7, 0x6F, 0xAA, 0xD2, 0xA8, 0x48, 0xF7, 0xDB, 0x23, 0x78, 0x0F, 0x14, 0x51, 0x19, 0xA3, 
	};

	MiiData* out = malloc(0x70);
	memset(out, 0, 0x70);
	Result res = APT_Unwrap(0x70, data, 12, 10, 0x70, (u8*)out);
	printf("Result: %lx\n", res);
	FILE* f = fopen("/test.bin", "wb");
	if (f) {
		fwrite(out, 0x70, 1, f);
		fclose(f);
	} else {
		printf("WTF, no file?\n");
	}
	printf("Mii magic: %d\nMii id: %lx\n", out->magic, out->mii_id);
	free(out);
}

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
		printf("Mii Plaza detected!\n");
		ReportListEntry* e = &list->entries[found_i];
		static const int cfpb_offset = 0x36bc;
		static const int cfpb_size = 0x88;
		if (msg->message_size > msg->total_header_size + cfpb_offset + cfpb_size) {
			printf("Valid size!\n");
			CFPB* cfpb = (CFPB*)((u8*)msg + msg->total_header_size + cfpb_offset);
			if (cfpb->magic == 0x42504643) {
				printf("Valid magic!\n");
				APT_Unwrap(0x60, (u8*)&cfpb->mii_id, 12, 8, sizeof(MiiData), (u8*)&e->mii);
				edited = true;
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