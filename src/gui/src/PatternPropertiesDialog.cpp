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

	Preferences *pPref = H2Core::Preferences::get_instance();

	std::list<QString>::const_iterator cur_patternCategories;
	
	if ( pPref->m_patternCategories.size() == 0 ) {
		pPref->m_patternCategories.push_back( "not_categorized" );
	}

	//categoryComboBox->clear();

	for( cur_patternCategories = pPref->m_patternCategories.begin(); cur_patternCategories != pPref->m_patternCategories.end(); ++cur_patternCategories )
	{
		if ( categoryComboBox->currentText() != *cur_patternCategories ){
			categoryComboBox->addItem( *cur_patternCategories );
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
	QString pattName = patternNameTxt->text();
	QString pattCategory = categoryComboBox->currentText();
	QString pattInfo = patternDescTxt->toPlainText();

	// Ensure the pattern name is unique
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	pattName = pPatternList->find_unused_pattern_name(pattName, pattern);

	Preferences *pPref = H2Core::Preferences::get_instance();
	std::list<QString>::const_iterator cur_testpatternCategories;

	bool test = true;
	for( cur_testpatternCategories = pPref->m_patternCategories.begin(); cur_testpatternCategories != pPref->m_patternCategories.end(); ++cur_testpatternCategories )
	{
		if ( categoryComboBox->currentText() == *cur_testpatternCategories ){
			test = false;
		}
	}

	if (test == true ) {
		pPref->m_patternCategories.push_back( pattCategory );
	}

	if( __savepattern ){
		pattern->set_name( pattName );
		pattern->set_info( pattInfo );
		pattern->set_category( pattCategory );
	} else if ( pattern->get_name() != pattName || pattern->get_info() != pattInfo || pattern->get_category() != pattCategory) {
		SE_modifyPatternPropertiesAction *action = new SE_modifyPatternPropertiesAction(  pattern->get_name() , pattern->get_info(), pattern->get_category(),
												  pattName, pattInfo, pattCategory, __nselectedPattern );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}
	accept();
}

void PatternPropertiesDialog::defaultNameCheck( QString pattName, bool savepattern)
{
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	if ( savepattern && !pPatternList->check_name(pattName, pattern) ) {
		pattName = pPatternList->find_unused_pattern_name(pattName, pattern);
	}

	patternNameTxt->setText(pattName);
}
