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