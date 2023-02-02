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

find ../../data/i18n/ -name "*.ts" -type f -exec $CMD {} \;

echo "Creating *.qm files"
cd ../../data/i18n
$LRELEASE *.ts


echo "Stats"
./stats.py
