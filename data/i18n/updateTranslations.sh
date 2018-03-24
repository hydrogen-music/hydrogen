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



$CMD ../../data/i18n/hydrogen.it.ts
$CMD ../../data/i18n/hydrogen.es.ts
$CMD ../../data/i18n/hydrogen.ru.ts
$CMD ../../data/i18n/hydrogen.fr.ts
$CMD ../../data/i18n/hydrogen.pt_BR.ts
$CMD ../../data/i18n/hydrogen.hu_HU.ts
$CMD ../../data/i18n/hydrogen.pl.ts
$CMD ../../data/i18n/hydrogen.nl.ts
$CMD ../../data/i18n/hydrogen.ja.ts
$CMD ../../data/i18n/hydrogen.de.ts
$CMD ../../data/i18n/hydrogen.sv.ts
$CMD ../../data/i18n/hydrogen.hr.ts
$CMD ../../data/i18n/hydrogen.cs.ts
$CMD ../../data/i18n/hydrogen.el.ts
$CMD ../../data/i18n/hydrogen.gl.ts
$CMD ../../data/i18n/hydrogen.sr.ts
$CMD ../../data/i18n/hydrogen.ca.ts
$CMD ../../data/i18n/hydrogen.uk.ts

echo "Creating *.qm files"
cd ../../data/i18n
$LRELEASE *.ts


echo "Stats"
./stats.py
