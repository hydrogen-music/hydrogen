#!/bin/sh

echo "Updating translation (*.ts) files"

if [ "$QTDIR" ]; then
	LUPDATE="$QTDIR/bin/lupdate"
	LRELEASE="$QTDIR/bin/lrelease"
else
	LUPDATE=$(which lupdate)
	LRELEASE=$(which lrelease)
fi;

UI_GUI=`find ../../src/gui | grep "\.ui$"`
CPP_GUI=`find ../../src/gui | grep "\.cpp$"`
CPP_CORE=`find ../../src/core | grep "\.cpp$"`
H_GUI=`find ../../src/gui | grep "\.h$"`
H_CORE=`find ../../src/core | grep "\.h$"`
FILES="$UI_GUI $CPP_GUI $CPP_CORE $H_GUI $H_CORE"

CMD="$LUPDATE -noobsolete ${FILES} -ts"

find . -name "*.ts" -type f -exec $CMD {} \;

echo "Creating *.qm files"
$LRELEASE *.ts


echo "Stats"
./stats.py
