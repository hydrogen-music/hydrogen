#!/bin/sh

echo "Updating translation (*.ts) files"

cd ../../src/gui



if [ "$QTDIR" ]; then
	LUPDATE="$QTDIR/bin/lupdate"
	LRELEASE="$QTDIR/bin/lrelease"
else
	LUPDATE=$(which lupdate)
	LRELEASE=$(which lrelease)
fi;

UI=`find . | grep "\.ui$"`
CPP=`find . | grep "\.cpp$"`
H=`find . | grep "\.h$"`
FILES="$UI $CPP $H"

CMD="$LUPDATE -noobsolete ${FILES} -ts"



$CMD ../../data/i18n/hydrogen_it.ts
$CMD ../../data/i18n/hydrogen_es.ts
$CMD ../../data/i18n/hydrogen_ru.ts
$CMD ../../data/i18n/hydrogen_fr.ts
$CMD ../../data/i18n/hydrogen_pt_BR.ts
$CMD ../../data/i18n/hydrogen_hu_HU.ts
$CMD ../../data/i18n/hydrogen_pl.ts
$CMD ../../data/i18n/hydrogen_nl.ts
$CMD ../../data/i18n/hydrogen_ja.ts
$CMD ../../data/i18n/hydrogen_de.ts
$CMD ../../data/i18n/hydrogen_sv.ts
$CMD ../../data/i18n/hydrogen_hr.ts
$CMD ../../data/i18n/hydrogen_cs.ts
$CMD ../../data/i18n/hydrogen_el.ts
$CMD ../../data/i18n/hydrogen_gl.ts
$CMD ../../data/i18n/hydrogen_sr.ts
$CMD ../../data/i18n/hydrogen_ca.ts
$CMD ../../data/i18n/hydrogen_uk.ts

echo "Creating *.qm files"
cd ../../data/i18n
$LRELEASE *.ts


echo "Stats"
./stats.py
