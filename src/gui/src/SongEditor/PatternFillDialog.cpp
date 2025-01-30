/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 *
 */

#include "PatternFillDialog.h"

#include <core/Hydrogen.h>
#include <core/Basics/Song.h>
#include <core/Basics/Pattern.h>

PatternFillDialog::PatternFillDialog(QWidget* parent, FillRange* pRange)
 : QDialog(parent)
 {
	setupUi( this );

	setWindowTitle( tr( "Fill with selected pattern" ) );
        adjustSize();
        setFixedSize( width(), height() );

	__fill_range = pRange;
	__text_changed();
}



PatternFillDialog::~PatternFillDialog()
{
}



void PatternFillDialog::on_cancelBtn_clicked()
{
	reject();
}



void PatternFillDialog::on_okBtn_clicked()
{
	__fill_range->fromVal = fromText->text().toUInt();
	__fill_range->toVal = toText->text().toUInt();
	__fill_range->bInsert = fillRB->isChecked();
	accept();
}



void PatternFillDialog::on_fromText_textChanged( const QString& )
{
	__text_changed();
}


void PatternFillDialog::on_toText_textChanged( const QString& )
{
	__text_changed();
}



void PatternFillDialog::__text_changed()
{
	int fromVal, toVal;

	if ( ( fromVal = fromText->text().toUInt() ) &&
		 ( toVal = toText->text().toUInt() )     &&
		 ( toVal > fromVal ) ) {

		okBtn->setEnabled(true);
	}
	else {
		okBtn->setEnabled(false);
	}
}
