###
### must have the following commandline tools:
###
### xml2po  (Debian package: poxml)
### po2xml  (Debian package: poxml)
### xmlto   (Debian package: xmlto)
###

ALL_MANUALS= manual_en.html \
	manual_es.html \
	manual_fr.html \
	manual_it.html \
	manual_nl.html

ALL_TUTORIALS = tutorial_en.html \
	tutorial_fr.html \
	tutorial_it.html

ALL_POT_FILES = manual_en.pot tutorial_en.pot

all: all_manuals all_tutorials all_pot_files

all_manuals: $(ALL_MANUALS)

all_tutorials: $(ALL_TUTORIALS)

## Explicit build to avoid circular dependency
all_pot_files: $(ALL_POT_FILES)
	xml2po -u manual_en.pot manual_en.docbook
	xml2po -u tutorial_en.pot tutorial_en.docbook

clean:
	-rm -f $(ALL_MANUALS) $(ALL_TUTORIALS)

%.html: %.docbook
	xmlto html-nochunks $^

%.docbook: %.po
	po2xml manual_en.docbook $^ > $@

manual_%.po: manual_en.docbook
	xml2po -u $@ $^

tutorial_%.po: tutorial_en.docbook
	xml2po -u $@ $^

