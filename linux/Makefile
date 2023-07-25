###
### must have the following commandline tools:
###
### itstool  (Debian package: itstool)
### msgfmt   (Debian package: gettext)
### msgmerge (Debian package: gettext)
###

OUT_FILES := org.hydrogenmusic.Hydrogen.metainfo.xml
PO_DIR := po
MO_FILES := $(patsubst %.po,%.mo,$(wildcard $(PO_DIR)/*.po))

ITSTOOL := itstool -i ./$(PO_DIR)/appstream-metainfo.its
MSGMERGE := msgmerge
MSGFMT := msgfmt

all: $(OUT_FILES)
.PHONY: all

clean:
	-rm -f $(MO_FILES)

$(OUT_FILES): %.xml: %.xml.in $(MO_FILES)
	$(ITSTOOL) -j $@.in -o $@ $(MO_FILES)

$(PO_DIR)/appstream.pot: *.xml.in $(PO_DIR)/appstream-metainfo.its
	$(ITSTOOL) -o ./$(PO_DIR)/appstream.pot $^

$(PO_DIR)/*.po: $(PO_DIR)/appstream.pot
	$(MSGMERGE) -N -U $@ ./$(PO_DIR)/appstream.pot --backup=simple --no-wrap --verbose

$(PO_DIR)/%.mo: $(PO_DIR)/%.po
	$(MSGFMT) --check -o $@ -L $* $<

