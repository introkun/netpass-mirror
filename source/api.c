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
 *<
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "api.h"
#include "cecd.h"
#include "utils.h"
#include "config.h"
#include "report.h"
#include <stdlib.h>
#include <string.h>

int location = -1;
FS_Archive sharedextdata_b = 0;
NetpassTitleData title_data;

Result initTitleData(void) {
	Result res = 0;
	CecMboxListHeader mbox_list;
	res = cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(CecMboxListHeader), (u8*)&mbox_list);
	if (R_FAILED(res)) return res;
	u16 title_name_utf16[65];
	for (int i = 0; i < mbox_list.num_boxes; i++) {
		u32 title_id = strtol((const char*)mbox_list.box_names[i], NULL, 16);

		memset(title_name_utf16, 0, sizeof(title_name_utf16));
		res = cecdOpenAndRead(title_id, CECMESSAGE_BOX_TITLE, sizeof(title_name_utf16)-2, (u8*)title_name_utf16);
		if (R_FAILED(res)) return res;

		memset(title_data.titles[i].name, 0, sizeof(title_data.titles[i].name));
		// SAFETY: utf16_to_utf8 does not write a null terminator, so we memset above
		utf16_to_utf8((u8*)title_data.titles[i].name, title_name_utf16, sizeof(title_data.titles[i].name)-1);

		char* ptr = title_data.titles[i].name;
		while (*ptr) {
			if (*ptr == '\n') *ptr = ' ';
			ptr++;
		}
		title_data.titles[i].title_id = title_id;
	}
	title_data.num_titles = mbox_list.num_boxes;
	return res;
}

NetpassTitleData* getTitleData(void) {
	return &title_data;
}

int numUsedTitles(void) {
	int num = 0;
	for (int i = 0; i < title_data.num_titles; i++) {
		if (!isTitleIgnored(title_data.titles[i].title_id)) num++;
	}
	return num;
}

void clearIgnoredTitles(CecMboxListHeader* mbox_list) {
	size_t pos = 0;
	for (size_t i = 0; i < mbox_list->num_boxes; i++) {
		u32 title_id = strtol((const char*)mbox_list->box_names[i], NULL, 16);
		if (!isTitleIgnored(title_id)) {
			if (pos != i) memcpy(mbox_list->box_names[pos], mbox_list->box_names[i], 16);
			pos++;
		}
	}
	memset(mbox_list->box_names[pos], 0, 16 * (mbox_list->num_boxes - pos));
	mbox_list->num_boxes = pos;
}


typedef struct SlotInfo {
	SlotMetadata metadata[12];
	void* slots[12];
} SlotInfo;

typedef struct TitleExtraInfo {
	u32 title_id;
	char* title_name;
	char* hmac_key;
} TitleExtraInfo;

Result uploadSlot(TitleExtraInfo* extra, SlotMetadata* metadata) {
	Result res = 0;
	char url[50];
	if (!metadata->title_id) {
		return -1; // something went wrong
	}
	if (metadata->size == 0 || metadata->send_method == 1) {
		// recv only, delete outbox
		snprintf(url, 50, "%s/outbox/%08lx", BASE_URL, metadata->title_id);
		res = httpRequest("DELETE", url, 0, 0, 0, 0, 0);
		return res;
	}

	// read extra metadata to send
	u8* slot = malloc(metadata->size);
	if (!slot) {
		return -2;
	}

	// now it is time to *actually* fetch the slot
	res = cecdSprGetSlot(metadata->title_id, metadata->size, slot);
	if (R_FAILED(res)) {
		free(slot);
		return res;
	}

	// now upload the slot
	snprintf(url, 50, "%s/outbox/slot", BASE_URL);
	res = httpRequest("POST", url, metadata->size, slot, 0, extra->title_name, extra->hmac_key);
	free(slot);
	return res;
}

Result downloadSlot(int i, SlotInfo* slotinfo) {
	Result res = 0;
	SlotMetadata* metadata = &slotinfo->metadata[i];
	if (metadata->send_method == 2) {
		// send-only, nothing to do
		metadata->size = 0;
		return res;
	}
	char url[100];
	snprintf(url, 100, "%s/inbox/%lx/slot", BASE_URL, metadata->title_id);
	CurlReply* reply;
	res = httpRequest("GET", url, 0, 0, &reply, 0, 0);
	if (R_FAILED(res)) goto fail;
	u32 http_code = res;
	if (http_code == 204) {
		metadata->size = 0;
		curlFreeHandler(reply->offset);
		return res;
	}
	if (http_code != 200) {
		res = -1;
		goto fail;
	}
	if (reply->len < sizeof(CecSlotHeader)) {
		metadata->size = 0;
		curlFreeHandler(reply->offset);
		return res;
	}
	CecMessageHeader* msg = (CecMessageHeader*)(reply->ptr + sizeof(CecSlotHeader));
	CecSlotHeader* slot = (CecSlotHeader*)reply->ptr;
	metadata->send_method = msg->send_method;
	metadata->size = slot->size;
	slotinfo->slots[i] = malloc(slot->size);
	if (!slotinfo->slots[i]) {
		res = -1;
		goto fail;
	}

	memcpy(slotinfo->slots[i], reply->ptr, slot->size);

	curlFreeHandler(reply->offset);
	return res;
fail:
	metadata->size = 0;
	curlFreeHandler(reply->offset);
	return res;
}

Result doSlotExchange(void) {
	Result res = 0;
	TitleExtraInfo title_extra_info[12];
	memset(&title_extra_info, 0, sizeof(TitleExtraInfo)*12);
	SlotInfo slotinfo;
	memset(&slotinfo, 0, sizeof(SlotInfo));
	char* error_origin = "none";
	// first we fetch the mboxlist, extend it and upload it
	{
		CecMboxListHeaderWithCapacities mbox_list;
		res = cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(mbox_list.header), (u8*)&mbox_list.header);
		error_origin = "reading mbox list";
		if (R_FAILED(res)) goto fail;
		clearIgnoredTitles(&mbox_list.header);
		// now fill in the capacities
		for (size_t i = 0; i < mbox_list.header.num_boxes; i++) {
			u32 title_id = strtol((const char*)mbox_list.header.box_names[i], NULL, 16);
			CecBoxInfoHeader boxinfo;
			res = cecdOpenAndRead(title_id, CEC_PATH_INBOX_INFO, sizeof(boxinfo), (u8*)&boxinfo);
			if (R_FAILED(res)) goto fail;
			mbox_list.capacities[i] = boxinfo.max_num_messages - boxinfo.num_messages;
		}
		
		char url[50];
		snprintf(url, 50, "%s/outbox/mboxlist_ext", BASE_URL);
		res = httpRequest("POST", url, sizeof(mbox_list), (u8*)&mbox_list, 0, 0, 0);
		error_origin = "sending mboxlist ext";
		if (R_FAILED(res)) goto fail;
	}

	// now we populate the extra data to upload, before we go into cecd state
	{
		CecMboxListHeaderWithCapacities mbox_list;
		res = cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(mbox_list.header), (u8*)&mbox_list.header);
		if (R_FAILED(res)) goto fail;
		clearIgnoredTitles(&mbox_list.header);
		u8* buf = malloc(MAX(200, sizeof(CecMBoxInfoHeader)));
		if (!buf) {
			res = -1;
			error_origin = "malloc for mboxinfo";
			goto fail;
		}
		// now fetch the data
		for (size_t i = 0; i < mbox_list.header.num_boxes; i++) {
			u32 title_id = strtol((const char*)mbox_list.header.box_names[i], NULL, 16);
			title_extra_info[i].title_id = title_id;
			memset(buf, 0, 200);

			// first title name
			res = cecdOpenAndRead(title_id, CECMESSAGE_BOX_TITLE, 198, buf);
			if (R_FAILED(res)) {
				error_origin = "Reading mbox title";
				free(buf);
				goto fail;
			}
			title_extra_info[i].title_name = b64encode(buf, 200);

			//second hmac key
			res = cecdOpenAndRead(title_id, CEC_PATH_MBOX_INFO, sizeof(CecMBoxInfoHeader), buf);
			if (R_FAILED(res)) {
				free(buf);
				error_origin = "Reading mboxlist";
				goto fail;
			}
			title_extra_info[i].hmac_key = b64encode(((CecMBoxInfoHeader*)buf)->hmac_key, 32);
		}
		free(buf);
	}

	// get cecd into the spr state
	error_origin = "Getting cecd into spr state";
	res = waitForCecdState(false, CEC_COMMAND_OVER_BOSS, CEC_STATE_ABBREV_INACTIVE);
	if (R_FAILED(res)) goto fail;

	// now we init spr stuffs
	res = cecdSprCreate();
	error_origin = "cecd spr create";
	if (R_FAILED(res)) goto fail;
	res = cecdSprInitialise();
	error_origin = "cecd spr init";
	if (R_FAILED(res)) goto fail;

	// Fetch the metadata

	u32 slots_total;
	error_origin = "cecd spr get slots metadata";
	res = cecdSprGetSlotsMetadata(sizeof(SlotMetadata)*12, slotinfo.metadata, &slots_total);
	if (R_FAILED(res)) goto fail;
	printf("Uploading outboxes (%ld/%d)", slots_total, numUsedTitles());

	// Upload all slots
	for (int i = 0; i < slots_total; i++) {
		TitleExtraInfo* extra = 0;
		for (int j = 0; j < 12; j++) {
			if (slotinfo.metadata[i].title_id == title_extra_info[j].title_id) {
				extra = &title_extra_info[j];
				break;
			}
		}
		if (!extra) {
			continue; // the slot was disabled
		}
		Result res2 = uploadSlot(extra, &slotinfo.metadata[i]);
		if (R_FAILED(res2)) {
			printf("-");
		} else {
			printf("=");
		}
		error_origin = "upload slot";
		res = cecdSprSetTitleSent(slotinfo.metadata[i].title_id, !R_FAILED(res2));
		if (res2 == -400) { // we still want to continue if it was http 400
			res2 = 0;
		}
		if (R_FAILED(res) || R_FAILED(res = res2)) goto fail;
	}
	// we are done sending things
	res = cecdSprFinaliseSend();
	error_origin = "finalise send";
	if (R_FAILED(res)) goto fail;
	printf(" Done\nDownloading inboxes (%ld/%d)", slots_total, numUsedTitles());

	// time to start download!
	res = cecdSprStartRecv();
	error_origin = "start recv";
	if (R_FAILED(res)) goto fail;

	// download all slots
	for (int i = 0; i < slots_total; i++) {
		// make sure the slot isn't disabled
		bool found = false;
		for (int j = 0; j < 12; j++) {
			if (slotinfo.metadata[i].title_id == title_extra_info[j].title_id) {
				found = true;
				break;
			}
		}
		if (!found) {
			continue; // the slot was disabled
		}
		res = downloadSlot(i, &slotinfo);
		error_origin = "download slot";
		if (R_FAILED(res)) goto fail;
	}

	// notify cecd of the slots
	res = cecdSprAddSlotsMetadata(sizeof(SlotMetadata)*slots_total, (u8*)slotinfo.metadata);
	error_origin = "add slots metadata";
	if (R_FAILED(res)) goto fail;

	// add all slots
	error_origin = "add slots";
	int slot_new_data_num = 0;
	for (int i = 0; i < slots_total; i++) {
		// make sure the slot isn't disabled
		bool found = false;
		for (int j = 0; j < 12; j++) {
			if (slotinfo.metadata[i].title_id == title_extra_info[j].title_id) {
				found = true;
				break;
			}
		}
		if (!found) {
			continue; // the slot was disabled
		}
		if (slotinfo.metadata[i].size == 0 || slotinfo.slots[i] == 0) {
			printf("=");
			continue;
		}
		slot_new_data_num++;
		res = cecdSprAddSlot(slotinfo.metadata[i].title_id, ((CecSlotHeader*)(slotinfo.slots[i]))->size, slotinfo.slots[i]);
		saveSlotInLog(slotinfo.slots[i]);
		if (R_FAILED(res)) {
			printf("-");
			goto fail;
		} else {
			printf("=");
		}
	}

	res = cecdSprFinaliseRecv();
	error_origin = "cecd spr finalise recv";
	if (R_FAILED(res)) goto fail;
	res = cecdSprDone(true);
	error_origin = "cecd spr done";
	if (R_FAILED(res)) goto fail;

	printf(" Done (%d)\n", slot_new_data_num);

	goto cleanup;
fail:
	cecdSprDone(false);
	_e(res);
	printf("ERROR (%s): %08lx\n", error_origin, res);
cleanup:
	for (int i = 0; i < 12; i++) {
		if (slotinfo.slots[i]) {
			free(slotinfo.slots[i]);
			slotinfo.slots[i] = 0;
		}
		if (title_extra_info[i].title_name) {
			free(title_extra_info[i].title_name);
			title_extra_info[i].title_name = 0;
		}
		if (title_extra_info[i].hmac_key) {
			free(title_extra_info[i].hmac_key);
			title_extra_info[i].hmac_key = 0;
		}
	}
	// get cecd into the normal state
	res = waitForCecdState(true, CEC_COMMAND_STOP, CEC_STATE_ABBREV_IDLE);
	return res;
}

Result getLocation(void) {
	Result res;
	CurlReply* reply;
	char url[80];
	snprintf(url, 80, "%s/location/current", BASE_URL);
	res = httpRequest("GET", url, 0, 0, &reply, 0, 0);
	if (R_FAILED(res)) goto cleanup;
	int http_code = res;
	if (http_code == 200) {
		res = *(u32*)(reply->ptr);
	} else {
		res = -1;
	}
cleanup:
	curlFreeHandler(reply->offset);
	return res;
}

Result setLocation(int location) {
	Result res;
	if (config.last_location == location) return -1;
	char url[80];
	snprintf(url, 80, "%s/location/%d/enter", BASE_URL, location);
	res = httpRequest("PUT", url, 0, 0, 0, 0, 0);
	if (R_FAILED(res)) {
		printf("ERROR: Failed to enter location %d: %ld\n", location, res);
		return res;
	}
	config.last_location = location;
	configWrite();
	printf("Entered location %d!\n", location);
	return res;
}

static s32 main_thread_prio_s = 0;
s32 main_thread_prio(void) {
	return main_thread_prio_s;
}
void init_main_thread_prio(void) {
	svcGetThreadPriority(&main_thread_prio_s, CUR_THREAD_HANDLE);
}

static volatile int dl_inbox_status = 1;
static bool dl_loop_running = true;
Thread bg_loop_thread = 0;
void triggerDownloadInboxes(void) {
	while (dl_inbox_status != 0) svcSleepThread((u64)1000000 * 100);
	dl_inbox_status = 1;
}

void bgLoop(void* p) {
	do {
		dl_inbox_status = 2;
		Result res = doSlotExchange();
		_e(res);
		dl_inbox_status = 0;
		for(int i = 0; i < 10*60*5; i++) {
			svcSleepThread((u64)1000000 * 100);
			if (dl_inbox_status == 1 || !dl_loop_running) break;
		}
	} while(dl_loop_running);
}

void bgLoopInit(void) {
	bg_loop_thread = threadCreate(bgLoop, NULL, 8*1024, main_thread_prio()+1, -2, false);
}

void bgLoopExit(void) {
	dl_loop_running = false;
	if (bg_loop_thread) {
		threadFree(bg_loop_thread);
	}
}
