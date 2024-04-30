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
#include <citro2d.h>

#define NUM_LANGUAGES 6

typedef const struct {
	const CFG_Language language;
	const char* text;
} LanguageString[NUM_LANGUAGES];

extern LanguageString str_loading;
extern LanguageString str_libcurl_error;
extern LanguageString str_libcurl_date_and_time;
extern LanguageString str_httpstatus_error;
extern LanguageString str_3ds_error;
extern LanguageString str_at_home;
extern LanguageString str_goto_train_station;
extern LanguageString str_at_train_station;
extern LanguageString str_goto_plaza;
extern LanguageString str_at_plaza;
extern LanguageString str_goto_mall;
extern LanguageString str_at_mall;
extern LanguageString str_exit;

void stringsInit(void);
const char* _s(LanguageString s);
C2D_Font _font(LanguageString s);
void TextLangParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s);