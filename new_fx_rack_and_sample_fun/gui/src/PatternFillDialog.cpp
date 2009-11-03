/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */

#include "PatternFillDialog.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/Song.h>
#include <hydrogen/Pattern.h>

#include "Skin.h"


PatternFillDialog::PatternFillDialog(QWidget* parent, FillRange* pRange)
 : QDialog(parent)
 , Object( "PatternFillDialog" )
{
	setupUi( this );

	setFixedSize( width(), height() );
	setWindowTitle( trUtf8( "Fill with selected pattern" ) );

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
