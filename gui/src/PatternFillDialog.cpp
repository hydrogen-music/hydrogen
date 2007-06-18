/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include <hydrogen/Hydrogen.h>
#include <hydrogen/Song.h>
#include "Skin.h"

#include <qradiobutton.h>
#include <qlineedit.h>
#include <qpixmap.h>
#include <qpushbutton.h>


PatternFillDialog::PatternFillDialog(QWidget* parent, FillRange* pRange)
 : QDialog(parent)
 , Object( "PatternFillDialog" )
{
	setupUi( this );

	setMaximumSize( width(), height() );
	setMinimumSize( width(), height() );
	setWindowTitle( trUtf8( "Fill with selected pattern" ) );
//	setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );
	this->fillRange = pRange;
	this->textChanged();
}




/**
 * Destructor
 */
PatternFillDialog::~PatternFillDialog() {
}



void PatternFillDialog::on_cancelBtn_clicked()
{
	reject();
}



void PatternFillDialog::on_okBtn_clicked()
{
	fillRange->fromVal = fromText->text().toUInt();
	fillRange->toVal =   toText->text().toUInt();
	fillRange->bInsert = fillRB->isChecked();
	accept();
}



void PatternFillDialog::on_fromText_textChanged( const QString& )
{
	textChanged();
}


void PatternFillDialog::on_toText_textChanged( const QString& )
{
	textChanged();
}


/**
 * Do some name check
 */
void PatternFillDialog::textChanged()
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
