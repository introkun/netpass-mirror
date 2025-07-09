/**
 * NetPass
 * Copyright (C) 2024, 2025 Sorunome
                 2024 SunOfLife1
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

#include "config.h"
#include "strings.h"
#include "utils.h"
#include "cecd.h"
#include "boss.h"
#include "logger.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#define PATCHES_COPY_DSTDIR "sdmc:/luma/sysmodules/"
#define PATCHES_COPY_SRCDIR "romfs:/patches/"
#define SPRELAY_TITLE_ID 0x0004013000003400ll
#define SPRELAY_TASK_ID "sprelay"

static const char config_path[] = "sdmc:/config/netpass/netpass.cfg";

Config config = {
	.last_location = -1,
	.language = -1,
	.year = 0,
	.month = 0,
	.day = 0,
	.price = 0,
	.welcome_version = 0,
	.patches_version = 0,
	.bg_music = 1,
	.log_level = LOG_LEVEL_INFO,
	.log_output = LOG_OUTPUT_SCREEN
};

void addIgnoredTitle(u32 title_id) {
	for (size_t i = 0; i < 24; i++) {
		// If title_id is already ignored, just return
		if (config.title_ids_ignored[i] == title_id) return;

		// Add title_id to first empty spot in array
		if (config.title_ids_ignored[i] == 0) {
			config.title_ids_ignored[i] = title_id;
			return;
		}
	}
}

void removeIgnoredTitle(u32 title_id) {
	for (size_t i = 0; i < 24; i++) {
		if (config.title_ids_ignored[i] == title_id) {
			config.title_ids_ignored[i] = 0;
			return;
		}
	}
}

bool isTitleIgnored(u32 title_id) {
	for (size_t i = 0; i < 24; i++) {
		if (config.title_ids_ignored[i] == title_id) return true;
	}
	return false;
}

void load(void) {
	FILE* f = fopen(config_path, "r");
	if (!f) {
		_e(-1);
		logError("Error loading config file");
		return;
	}
	char line[200];
	while (fgets_blk(line, 200, f)) {
		char* separator = strchr(line, '=');
		if (!separator) continue;
		char* key = line;
		*separator = '\0';
		char* value = separator + 1;
		char* eol = strchr(value, '\n');
		if (eol) {
			*eol = '\0';
		}
		if (!key[0] || !value[0]) continue;
		const int key_len = strlen(key);
		const int value_len = strlen(value);
		for (int i = 0; i < key_len; i++) key[i] = toupper(key[i]);
		for (int i = 0; i < value_len; i++) value[i] = toupper(value[i]);
		if (strcmp(key, "LANGUAGE") == 0) {
			if (strcmp(value, "SYSTEM") == 0) {
				config.language = -1;
			} else {
				for (int i = 0; i < NUM_LANGUAGES; i++) {
					if (strcmp(value, all_languages_str[i]) == 0) {
						config.language = all_languages[i];
						break;
					}
				}
			}
		}
		if (strcmp(key, "LAST_LOCATION") == 0) {
			config.last_location = atoi(value);
		}
		if (strcmp(key, "YEAR") == 0) {
			config.year = atoi(value);
		}
		if (strcmp(key, "MONTH") == 0) {
			config.month = atoi(value);
		}
		if (strcmp(key, "DAY") == 0) {
			config.day = atoi(value);
		}
		if (strcmp(key, "PRICE") == 0) {
			config.price = atoi(value);
		}
		if (strcmp(key, "WELCOME_VERSION") == 0) {
			config.welcome_version = atoi(value);
		}
		if (strcmp(key, "PATCHES_VERSION") == 0) {
			config.patches_version = atoi(value);
		}
		if (strcmp(key, "BG_MUSIC") == 0) {
			config.bg_music = strcmp(value, "TRUE") == 0;
		}
		if (strcmp(key, "LOG_LEVEL") == 0) {
			config.log_level = atoi(value);
			loggerSetLevel(config.log_level);
		}
		if (strcmp(key, "LOG_OUTPUT") == 0) {
			config.log_output = atoi(value);
			loggerSetOutput(config.log_output, "log.txt");
		}
		if (strcmp(key, "TITLE_IDS_IGNORED") == 0) {
			// Open mbox_list now to avoid repeatedly doing it later
			Result res = 0;
			CecMboxListHeader mbox_list;
			res = cecdOpenAndRead(0, CEC_PATH_MBOX_LIST, sizeof(CecMboxListHeader), (u8*)&mbox_list);
			if (R_FAILED(res)) continue;
			
			// Read titles ids
			for (size_t i = 0; i < 24; i++) {
				sscanf(&value[9*i], "%lx,", &config.title_ids_ignored[i]);
				
				// Remove title id if not in mbox_list
				if (config.title_ids_ignored[i] == 0) continue;
				bool found = false;
				for (size_t j = 0; j < mbox_list.num_boxes; j++) {
					u32 title_id = strtol((const char*)mbox_list.box_names[j], NULL, 16);
					if (title_id == config.title_ids_ignored[i]) {
						found = true;
						break;
					}
				}
				if (!found) config.title_ids_ignored[i] = 0;
			}
		}
	}
	fclose(f);
}

void configWrite(void) {
	FILE* f = fopen(config_path, "w");
	if (!f) {
		_e(-1);
		logError("Error saving config file");
		return;
	}
	char line[250];
	snprintf(line, 250, "last_location=%d\n", config.last_location);
	fputs_blk(line, f);
	snprintf(line, 250, "year=%d\n", config.year);
	fputs_blk(line, f);
	snprintf(line, 250, "month=%d\n", config.month);
	fputs_blk(line, f);
	snprintf(line, 250, "day=%d\n", config.day);
	fputs_blk(line, f);
	snprintf(line, 250, "price=%ld\n", config.price);
	fputs_blk(line, f);
	snprintf(line, 250, "welcome_version=%d\n", config.welcome_version);
	fputs_blk(line, f);
	snprintf(line, 250, "patches_version=%d\n", config.patches_version);
	fputs_blk(line, f);
	snprintf(line, 250, "bg_music=%s\n", config.bg_music ? "true" : "false");
	fputs_blk(line, f);
	snprintf(line, 250, "log_level=%d\n", config.log_level);
	fputs_blk(line, f);
	snprintf(line, 250, "log_output=%d\n", config.log_output);
	fputs_blk(line, f);
	if (config.language == -1) {
		fputs_blk("language=system\n", f);
	} else {
		for (int i = 0; i < NUM_LANGUAGES; i++) {
			if (config.language == all_languages[i]) {
				snprintf(line, 250, "language=%s\n", all_languages_str[i]);
				fputs_blk(line, f);
				break;
			}
		}
	}
	snprintf(line, 250, "title_ids_ignored=");
	for (size_t i = 0; i < 24; i++) {
		snprintf(line + 18 + (9*i), 250 - (18 + (9*i)), "%08lx,", config.title_ids_ignored[i]);
	}
	snprintf(line + 18 + (9*24), 250 - (18 + (9*24)), "\n");
	fputs_blk(line, f);
	
	fclose(f);
}

void configInit(void) {
	mkdir_p((char*)config_path);
	load();
}

bool clearPatches(void) {
	DIR* d = opendir(PATCHES_COPY_SRCDIR);
	if (!d) {
		logError("ERROR: src dir not found\n");
		return false;
	}
	struct dirent* p;
	char srcpath[100];
	char dstpath[100];
	bool deleted_file = false;
	while ((p = readdir(d))) {
		snprintf(srcpath, 100, "%s%s", PATCHES_COPY_SRCDIR, p->d_name);
		snprintf(dstpath, 100, "%s%s", PATCHES_COPY_DSTDIR, p->d_name);
		struct stat statbuf;
		if (!stat(srcpath, &statbuf) && !S_ISDIR(statbuf.st_mode)) {
			// we skip the ssl patch
			if (strcmp(p->d_name, "0004013000002F02.ips") == 0) continue;
			// we skip files that don't exist
			if (access(dstpath, F_OK) != 0) continue;
			// ok we actually have a file, delete it
			logInfo("%s...", p->d_name);
			remove(dstpath);
			logInfo("Done\n");
			deleted_file = true;
		}
	}
	closedir(d);
	logInfo("Updating patches version in config...");
	config.patches_version = _PATCHES_VERSION_;
	configWrite();
	logInfo("Done\nClearing sysmodules done\n");
	return deleted_file;
}

bool writePatches(void) {
	mkdir_p(PATCHES_COPY_DSTDIR);
	logInfo("Copying sysmodules...\n");
	DIR* d = opendir(PATCHES_COPY_SRCDIR);
	if (!d) {
		logError("ERROR: src dir not found\n");
		return false;
	}
	void* buffer = malloc(0x4000);
	if (!buffer) {
		logError("ERROR: malloc\n");
		return false;
	}
	struct dirent* p;
	char srcpath[100];
	char dstpath[100];
	while ((p = readdir(d))) {
		snprintf(srcpath, 100, "%s%s", PATCHES_COPY_SRCDIR, p->d_name);
		snprintf(dstpath, 100, "%s%s", PATCHES_COPY_DSTDIR, p->d_name);
		struct stat statbuf;
		if (!stat(srcpath, &statbuf) && !S_ISDIR(statbuf.st_mode)) {
			// ok we actually have a file, copy it
			logInfo("%s...", p->d_name);
			FILE* src = fopen(srcpath, "rb");
			FILE* dst = fopen(dstpath, "wb+");
			if (!src || !dst) {
				if (src) fclose(src);
				if (dst) fclose(dst);
				logError("ERROR: open\n");
				continue;
			}
			size_t len = fread(buffer, 1, 0x4000, src);
			if (!len) {
				fclose(src);
				fclose(dst);
				logError("ERROR: read\n");
				continue;
			}
			if (!fwrite(buffer, len, 1, dst)) {
				fclose(src);
				fclose(dst);
				logError("ERROR: write\n");
				continue;
			}
			fclose(src);
			fclose(dst);

			logInfo("Done\n");
		}
	}
	closedir(d);
	logInfo("Updating patches version in config...");
	config.patches_version = _PATCHES_VERSION_;
	configWrite();
	logInfo("Done\nCopying sysmodules done\n");
	free(buffer);
	return true;
}

void clearBossCacheAndReboot(void) {
	logInfo("Clearing spr cache...");
	bossInit(SPRELAY_TITLE_ID, false);
	bossUnregisterTask(SPRELAY_TASK_ID, 0);
	bossUnregisterTask(SPRELAY_TASK_ID, 0);
	logInfo("Done\n");
	nsInit();
	NS_RebootSystem();
	logInfo("Rebooting system...\n");
}
