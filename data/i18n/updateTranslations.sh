#!/bin/sh

echo "Updating translation (*.ts) files"

cd ../../gui/

UI=`find . | grep "\.ui"`
CPP=`find . | grep "\.cpp"`
H=`find . | grep "\.h"`
FILES="$UI $CPP $H"

CMD="$QTDIR/bin/lupdate ${FILES} -ts"

$CMD ../data/i18n/hydrogen.it.ts
$CMD ../data/i18n/hydrogen.es.ts
$CMD ../data/i18n/hydrogen.ru.ts
$CMD ../data/i18n/hydrogen.fr.ts
$CMD ../data/i18n/hydrogen.pt_BR.ts
$CMD ../data/i18n/hydrogen.hu_HU.ts
$CMD ../data/i18n/hydrogen.pl.ts
$CMD ../data/i18n/hydrogen.nl.ts
$CMD ../data/i18n/hydrogen.ja.ts
$CMD ../data/i18n/hydrogen.de.ts
$CMD ../data/i18n/hydrogen.sv.ts

echo "Creating *.qm files"
cd ../data/i18n
$QTDIR/bin/lrelease *.ts


echo "Stats"
./stats.py
