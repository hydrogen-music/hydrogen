###
### must have the following commandline tools:
###
### xml2po  (Debian package: poxml)
### po2xml  (Debian package: poxml)
### xmlto   (Debian package: xmlto)
### xmllint (Debian package: libxml2-utils)
###

MANUAL_MASTER = manual.docbook
TUTORIAL_MASTER = tutorial.docbook

ALL_MASTERS = $(MANUAL_MASTER) $(TUTORIAL_MASTER)

ALL_MANUALS = manual_en.html \
	manual_es.html \
	manual_fr.html \
	manual_it.html \
	manual_nl.html

ALL_TUTORIALS = tutorial_en.html \
	tutorial_fr.html \
	tutorial_it.html

ALL_POT_FILES = manual_en.pot tutorial_en.pot

XMLTO_OPTS = --stringparam section.autolabel=1 \
	--stringparam toc.max.depth=2 \
	--stringparam xref.with.number.and.title=0

all: all_manuals all_tutorials all_pot_files

all_manuals: $(ALL_MANUALS)

all_tutorials: $(ALL_TUTORIALS)

## Explicit build to avoid circular dependency
all_pot_files: $(ALL_MASTERS)
	xml2po -u manual.pot manual.docbook
	xml2po -u tutorial.pot tutorial.docbook

clean:
	-rm -f $(ALL_MANUALS) $(ALL_TUTORIALS) *_{en,es,it,fr,nl}.docbook *.docbook_validated

%.html: %.docbook %.docbook_validated
	xmlto html-nochunks $(XMLTO_OPTS) $<

%.docbook_validated: %.docbook
	xmllint --noout --valid $^
	touch $@

## Special rule for master manual and tutorial
%_en.docbook: %.docbook
	cp -f $^ $@

manual_%.docbook: manual_%.po $(MANUAL_MASTER)
	po2xml $(MANUAL_MASTER) $< > $@

manual_%.po: $(MANUAL_MASTER)
	xml2po -u $@ $^

tutorial_%.docbook: tutorial_%.po $(TUTORIAL_MASTER)
	po2xml $(TUTORIAL_MASTER) $< > $@

tutorial_%.po: $(TUTORIAL_MASTER)
	xml2po -u $@ $^

