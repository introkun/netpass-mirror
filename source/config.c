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

#include "config.h"
#include "strings.h"
#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char config_path[] = "/config/netpass/netpass.cfg";

Config config = {
	.last_location = -1,
	.language = -1,
};

void load(void) {
	FILE* f = fopen(config_path, "r");
	if (!f) return;
	char line[200];
	while (fgets(line, 200, f)) {
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
		int key_len = strlen(key);
		int value_len = strlen(value);
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
	}
	fclose(f);
}

void configWrite(void) {
	FILE* f = fopen(config_path, "w");
	char line[200];
	snprintf(line, 200, "last_location=%d\n", config.last_location);
	fputs(line, f);
	if (config.language == -1) {
		fputs("language=system\n", f);
	} else {
		for (int i = 0; i < NUM_LANGUAGES; i++) {
			if (config.language == all_languages[i]) {
				snprintf(line, 200, "language=%s\n", all_languages_str[i]);
				fputs(line, f);
				break;
			}
		}
	}
	
	fclose(f);
}

void configInit(void) {
	mkdir_p((char*)config_path);
	load();
}