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

#include "strings.h"
#include "config.h"

static u8 _language;

C2D_Font font_default;
C2D_Font font_local;

void stringsInit(void) {
	if (config.language == -1) {
		CFGU_GetSystemLanguage(&_language);
	} else {
		_language = config.language;
	}
	printf("Got language %d\n", _language);
	font_default = C2D_FontLoadSystem(CFG_REGION_USA);
	if (_language == CFG_LANGUAGE_ZH) {
		font_local = C2D_FontLoadSystem(CFG_REGION_CHN);
	} else if (_language == CFG_LANGUAGE_KO) {
		font_local = C2D_FontLoadSystem(CFG_REGION_KOR);
	} else if (_language == CFG_LANGUAGE_TW) {
		font_local = C2D_FontLoadSystem(CFG_REGION_TWN);
	} else {
		font_local = font_default;
	}
}

const char* _s(LanguageString s) {
	return string_in_language(s, _language);
}

const char* string_in_language(LanguageString s, int lang) {
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == lang && s[i].text) {
			return s[i].text;
		}
	}
	return s[0].text;
}

C2D_Font _font(LanguageString s) {
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == _language && s[i].text) {
			return font_local;
		}
	}
	return font_default;
}

void TextLangParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s) {
	const char* text = 0;
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == _language && s[i].text) {
			text = s[i].text;
			break;
		}
	}
	C2D_Font font = font_local;
	if (!text) {
		text = s[0].text;
		font = font_default;
	}
	C2D_TextFontParse(staticText, font, staticBuf, text);
}
