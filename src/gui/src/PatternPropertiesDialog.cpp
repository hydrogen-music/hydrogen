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
#include "Skin.h"
#include "HydrogenApp.h"
#include "UndoActions.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/Preferences.h>


using namespace std;
using namespace H2Core;

PatternPropertiesDialog::PatternPropertiesDialog(QWidget* parent, Pattern *pattern, int nselectedPattern, bool savepattern)
 : QDialog(parent)
{
	setupUi( this );
	setWindowTitle( trUtf8( "Pattern properties" ) );

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
	}else
	{
		SE_modifyPatternPropertiesAction *action = new SE_modifyPatternPropertiesAction(  pattern->get_name() , pattern->get_info(), pattern->get_category(),
												  pattName, pattInfo, pattCategory, __nselectedPattern );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}
	accept();
}

void PatternPropertiesDialog::defaultNameCheck( QString pattName, bool savepattern)
{
	if ( savepattern && !nameCheck(pattName) )
	{
		defaultNameCheck( trUtf8( "%1#2").arg(pattName), savepattern );
	}
	else
	{
		patternNameTxt->setText( trUtf8( "%1").arg(pattName) );
	}
}


bool PatternPropertiesDialog::nameCheck( QString pattName )
{
	if (pattName == "") {
		return false;
	}
	PatternList *patternList = Hydrogen::get_instance()->getSong()->get_pattern_list();
	
	for (uint i = 0; i < patternList->size(); i++) {
		if ( patternList->get(i)->get_name() == pattName) {
			return false;
		}
	}
	return true;
}


void PatternPropertiesDialog::on_categoryComboBox_editTextChanged()
{
	if ( categoryComboBox->currentText() == pattern->get_category() ) {
		okBtn->setEnabled( false );
	}
	else {
		okBtn->setEnabled(true);
	}
}


void PatternPropertiesDialog::on_patternNameTxt_textChanged()
{
	if ( nameCheck( patternNameTxt->text() ) ) {
		okBtn->setEnabled( true );
	}
	else {
		okBtn->setEnabled( false );
	}
}
