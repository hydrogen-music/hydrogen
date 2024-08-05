/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 */

#include "PatternPropertiesDialog.h"
#include "HydrogenApp.h"
#include "UndoActions.h"

#include <core/Hydrogen.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Preferences/Preferences.h>

using namespace H2Core;

PatternPropertiesDialog::PatternPropertiesDialog(QWidget* parent, Pattern *pattern, int nselectedPattern, bool savepattern)
 : QDialog(parent)
{
	setupUi( this );
	setWindowTitle( tr( "Pattern properties" ) );

	this->pattern = pattern;

	patternNameTxt->setText( pattern->get_name() );
	patternNameTxt->selectAll();

	patternDescTxt->setText( pattern->get_info() );

	QString category = pattern->get_category();
	__nselectedPattern = nselectedPattern;
	__savepattern = savepattern;	
	
	if ( category == "" ){
		category = "not_categorized";
	}
	categoryComboBox->addItem( category );

	const auto pPref = H2Core::Preferences::get_instance();

	for ( const auto& ssCategory : pPref->m_patternCategories ) {
		if ( categoryComboBox->currentText() != ssCategory ){
			categoryComboBox->addItem( ssCategory );
		}
	}

	defaultNameCheck( pattern->get_name(), savepattern );
	okBtn->setEnabled(true);
}


/**
 * Destructor
 */
PatternPropertiesDialog::~PatternPropertiesDialog()
{
}


void PatternPropertiesDialog::on_cancelBtn_clicked()
{
	reject();
}


void PatternPropertiesDialog::on_okBtn_clicked()
{
	QString sPattName = patternNameTxt->text();
	const QString sPattCategory = categoryComboBox->currentText();
	const QString sPattInfo = patternDescTxt->toPlainText();

	// Ensure the pattern name is unique
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	sPattName = pPatternList->find_unused_pattern_name(sPattName, pattern);

	auto pPref = H2Core::Preferences::get_instance();

	if ( pPref->m_patternCategories.contains( sPattCategory ) ) {
		pPref->m_patternCategories.push_back( sPattCategory );
	}

	if( __savepattern ){
		pattern->set_name( sPattName );
		pattern->set_info( sPattInfo );
		pattern->set_category( sPattCategory );
	} else if ( pattern->get_name() != sPattName || pattern->get_info() != sPattInfo || pattern->get_category() != sPattCategory) {
		SE_modifyPatternPropertiesAction *action = new SE_modifyPatternPropertiesAction(  pattern->get_name() , pattern->get_info(), pattern->get_category(),
												  sPattName, sPattInfo, sPattCategory, __nselectedPattern );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}
	accept();
}

void PatternPropertiesDialog::defaultNameCheck( const QString& pattName, bool savepattern)
{
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	if ( savepattern && !pPatternList->check_name(pattName, pattern) ) {
		patternNameTxt->setText(
			pPatternList->find_unused_pattern_name(pattName, pattern));
	}
	else {
		patternNameTxt->setText(pattName);
	}

}
