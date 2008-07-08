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
 */

#include "PatternPropertiesDialog.h"
#include <hydrogen/hydrogen.h>
#include <hydrogen/Pattern.h>

#include "Skin.h"

#include <QPixmap>
using namespace std;
using namespace H2Core;

PatternPropertiesDialog::PatternPropertiesDialog(QWidget* parent, Pattern *pattern)
 : QDialog(parent) {
	setupUi( this );
	setWindowTitle( trUtf8( "Pattern properties" ) );
//	setIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	this->pattern = pattern;

	patternNameTxt->setText( pattern->get_name() );
	patternNameTxt->selectAll();
}




/**
 * Destructor
 */
PatternPropertiesDialog::~PatternPropertiesDialog() {
}



void PatternPropertiesDialog::on_cancelBtn_clicked() {
	reject();
}



void PatternPropertiesDialog::on_okBtn_clicked() {
	QString pattName = patternNameTxt->text();

	pattern->set_name(pattName);
	accept();
}



/**
 * Do some name check
 */
bool PatternPropertiesDialog::nameCheck( QString pattName )
{
	if (pattName == "") {
		return false;
	}
	PatternList *patternList = Hydrogen::get_instance()->getSong()->get_pattern_list();
	
	for (uint i = 0; i < patternList->get_size(); i++) {
		if ( patternList->get(i)->get_name() == pattName) {
			return false;
		}
	}
	return true;
}


void PatternPropertiesDialog::on_patternNameTxt_textChanged() {

	if ( nameCheck( patternNameTxt->text() )) {
		okBtn->setEnabled(true);
	}
	else {
		okBtn->setEnabled(false);
	}
}


