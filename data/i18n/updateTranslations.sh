#!/bin/sh

echo "Updating translation (*.ts) files"

cd ../../src/

UI=`find | grep "\.ui"`
CPP=`find | grep "\.cpp"`
H=`find | grep "\.h"`
FILES="$UI $CPP $H"

CMD="lupdate ${FILES} -ts"

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

echo "Creating *.qm files"
cd ../data/i18n
lrelease *.ts

