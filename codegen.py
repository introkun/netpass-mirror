import os, yaml, json

SRCDIR = "locale"
DESTDIR = "codegen"

basepath = os.path.dirname(os.path.realpath(__file__))
SRCDIR = basepath + "/" + SRCDIR
DESTDIR = basepath + "/" + DESTDIR

language_map = {
	"ja": "jp"
}

def l(s):
	return (language_map[s] if s in language_map else s).upper()

translations = {}

for file in os.listdir(SRCDIR):
	if file.endswith(".yaml"):
		language = file[:-5]
		with open(SRCDIR + "/" + file, "r") as f:
			translations[language] = yaml.safe_load(f)

headerfile = "#pragma once\n\n#include <3ds.h>\n"

outfile = "#include <3ds.h>\n#include \"lang_strings.h\"\n";

lang_i = 0
for lang in translations.keys():
	if l(lang) not in ("JP", "EN", "FR", "DE", "IT", "ES", "ZH", "KO", "NL", "PT", "RU", "TW"):
		lang_i += 1
		headerfile += f"#define CFG_LANGUAGE_{l(lang)} {20+lang_i}\n"
headerfile += f"#define NUM_LANGUAGES {len(translations.keys())}\n"

headerfile += """
typedef const struct {
	const CFG_Language language;
	const char* text;
} LanguageString[NUM_LANGUAGES];

extern const int all_languages[];
"""

outfile += f"const int all_languages[{len(translations.keys())}] = {{"
for lang in translations.keys():
	outfile += f"CFG_LANGUAGE_{l(lang)}, "
outfile += "};\n"

for key in translations["en"].keys():
	outfile += f"LanguageString {key} = {{\n"
	headerfile += f"extern LanguageString {key};\n"
	for lang in translations.keys():
		s = "0"
		if key in translations[lang] and translations[lang][key] != "":
			s = json.dumps(translations[lang][key], ensure_ascii=False)
		outfile += f"\t{{CFG_LANGUAGE_{l(lang)}, {s}}},\n"
	outfile += "};\n";

os.makedirs(DESTDIR, exist_ok=True)
with open(DESTDIR + "/lang_strings.h", "w") as f:
	f.write(headerfile)

with open(DESTDIR + "/lang_strings.c", "w") as f:
	f.write(outfile)
