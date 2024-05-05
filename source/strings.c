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

C2D_Font _cache_fonts_loaded[4] = {0};
const int fontLoadArr[4] = {CFG_REGION_CHN, CFG_REGION_KOR, CFG_REGION_TWN, CFG_REGION_USA};

C2D_Font __f(int i) {
	if (_cache_fonts_loaded[i]) {
		return _cache_fonts_loaded[i];
	}
	_cache_fonts_loaded[i] = C2D_FontLoadSystem(fontLoadArr[i]);
	return _cache_fonts_loaded[i];
}

C2D_Font _get_local_font(int lang) {
	C2D_Font font;
	if (lang == CFG_LANGUAGE_ZH) {
		font = __f(0);
	} else if (lang == CFG_LANGUAGE_KO) {
		font = __f(1);
	} else if (lang == CFG_LANGUAGE_TW) {
		font = __f(2);
	} else {
		font = __f(3);
	}
	return font;
}

void stringsInit(void) {
	if (config.language == -1) {
		CFGU_GetSystemLanguage(&_language);
	} else {
		_language = config.language;
	}
	printf("Got language %d\n", _language);
	font_default = __f(3);
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
			return _get_local_font(_language);
		}
	}
	return font_default;
}

void TextLangParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s) {
	TextLangSpecificParse(staticText, staticBuf, s, _language);
}

void TextLangSpecificParse(C2D_Text* staticText, C2D_TextBuf staticBuf, LanguageString s, int l) {
	const char* text = 0;
	for (int i = 0; i < NUM_LANGUAGES; i++) {
		if (s[i].language == l && s[i].text) {
			text = s[i].text;
			break;
		}
	}
	C2D_Font font = _get_local_font(l);
	if (!text) {
		text = s[0].text;
		font = font_default;
	}
	C2D_TextFontParse(staticText, font, staticBuf, text);
}