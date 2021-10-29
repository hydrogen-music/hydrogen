/*
 * Hydrogen
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <memory>
#include <vector>
#include <utility>

#include <QGridLayout>

#include "PaletteDialog.h"
#include "../HydrogenApp.h"

#include <core/Preferences.h>

PaletteDialog::PaletteDialog( QWidget* pParent )
	: QDialog( pParent )
	, m_previousStyle( H2Core::UIStyle( H2Core::Preferences::get_instance()->getDefaultUIStyle() ) )
	, m_currentStyle( H2Core::UIStyle( H2Core::Preferences::get_instance()->getDefaultUIStyle() ) )
{
	setWindowTitle( tr( "Palette Dialog" ) );

	m_colorSelections.reserve( 50 );

	addPair( tr( "Accent Color" ), m_currentStyle.m_accentColor );
	addPair( tr( "Widget Color" ), m_currentStyle.m_widgetColor );
	addPair( tr( "Widget Text Color" ), m_currentStyle.m_widgetTextColor );
	addPair( tr( "Window Color" ), m_currentStyle.m_windowColor );
	addPair( tr( "Window Text Color" ), m_currentStyle.m_windowTextColor );
	addPair( tr( "Base Color" ), m_currentStyle.m_baseColor );
	addPair( tr( "Alternate Base Color" ), m_currentStyle.m_alternateBaseColor );
	addPair( tr( "Text Color" ), m_currentStyle.m_textColor );
	addPair( tr( "Button Color" ), m_currentStyle.m_buttonColor );
	addPair( tr( "Button Text Color" ), m_currentStyle.m_buttonTextColor );
	addPair( tr( "Light Color" ), m_currentStyle.m_lightColor );
	addPair( tr( "Mid Light Color" ), m_currentStyle.m_midLightColor );
	addPair( tr( "Mid Color" ), m_currentStyle.m_midColor );
	addPair( tr( "Dark Color" ), m_currentStyle.m_darkColor );
	addPair( tr( "Shadow Text Color" ), m_currentStyle.m_shadowTextColor );
	addPair( tr( "Highlight Color" ), m_currentStyle.m_highlightColor );
	addPair( tr( "Highlighted Text Color" ), m_currentStyle.m_highlightedTextColor );
	addPair( tr( "Background Color" ), m_currentStyle.m_songEditor_backgroundColor );
	addPair( tr( "AlternateRow Color" ), m_currentStyle.m_songEditor_alternateRowColor );
	addPair( tr( "SelectedRow Color" ), m_currentStyle.m_songEditor_selectedRowColor );
	addPair( tr( "Line Color" ), m_currentStyle.m_songEditor_lineColor );
	addPair( tr( "Text Color" ), m_currentStyle.m_songEditor_textColor );
	addPair( tr( "Background Color" ), m_currentStyle.m_patternEditor_backgroundColor );
	addPair( tr( "AlternateRow Color" ), m_currentStyle.m_patternEditor_alternateRowColor );
	addPair( tr( "SelectedRow Color" ), m_currentStyle.m_patternEditor_selectedRowColor );
	addPair( tr( "Text Color" ), m_currentStyle.m_patternEditor_textColor );
	addPair( tr( "Note Color" ), m_currentStyle.m_patternEditor_noteColor );
	addPair( tr( "Noteoff Color" ), m_currentStyle.m_patternEditor_noteoffColor );
	addPair( tr( "Line Color" ), m_currentStyle.m_patternEditor_lineColor );
	addPair( tr( "Line 1 Color" ), m_currentStyle.m_patternEditor_line1Color );
	addPair( tr( "Line 2 Color" ), m_currentStyle.m_patternEditor_line2Color );
	addPair( tr( "Line 3 Color" ), m_currentStyle.m_patternEditor_line3Color );
	addPair( tr( "Line 4 Color" ), m_currentStyle.m_patternEditor_line4Color );
	addPair( tr( "Line 5 Color" ), m_currentStyle.m_patternEditor_line5Color );
	addPair( tr( "Selection Highlight Color" ), m_currentStyle.m_selectionHighlightColor );
	addPair( tr( "Selection Inactive Color" ), m_currentStyle.m_selectionInactiveColor );
	addPair( tr( "Accent Text Color" ), m_currentStyle.m_accentTextColor );
	addPair( tr( "Button Red Color" ), m_currentStyle.m_buttonRedColor );
	addPair( tr( "Button Red Text Color" ), m_currentStyle.m_buttonRedTextColor );
	addPair( tr( "Spin Box Selection Color" ), m_currentStyle.m_spinBoxSelectionColor );
	addPair( tr( "Spin Box Selection Text Color" ), m_currentStyle.m_spinBoxSelectionTextColor );
	addPair( tr( "Automation Color" ), m_currentStyle.m_automationColor );
	addPair( tr( "Automation Circle Color" ), m_currentStyle.m_automationCircleColor );
	addPair( tr( "Tool Tip Base Color" ), m_currentStyle.m_toolTipBaseColor );
	addPair( tr( "Tool Tip Text Color" ), m_currentStyle.m_toolTipTextColor );

	std::vector vWidget{ 0, 36, 1, 2, 37, 38, 39, 40, 41, 42 };
	std::vector vPalette{ 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 34, 35, 43, 44 };
	std::vector vSongEditor{ 17, 18, 19, 20, 21 };
	std::vector vPatternEditor{ 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33 };

	QVBoxLayout* pMainLayout = new QVBoxLayout;

	QHBoxLayout* pColorCategoriesLayout = new QHBoxLayout;
	QGroupBox* pColorCategoriesBox = new QGroupBox;
	pColorCategoriesBox->setLayout( pColorCategoriesLayout );

	QVBoxLayout* pWidgetVBox = new QVBoxLayout;
	QGroupBox* pWidgetVGBox = new QGroupBox;
	pWidgetVGBox->setLayout( pWidgetVBox );
	QGroupBox* pWidgetBox = new QGroupBox( tr( "Widget Colors" ) );;
	QGridLayout* pWidgetGrid = new QGridLayout;
	pWidgetBox->setLayout( pWidgetGrid );
	pWidgetGrid->setRowMinimumHeight( 0, 15 );
	for ( auto ii : vWidget ) {
		pWidgetGrid->addWidget( m_colorSelections[ii].first.get(), ii + 1, 0 );
		pWidgetGrid->addWidget( m_colorSelections[ii].second.get(), ii + 1, 1 );
	}
	pWidgetGrid->setColumnMinimumWidth( 1, 40 );
	pWidgetGrid->setColumnStretch( 1, 0 );
	pWidgetGrid->setColumnStretch( 0, 10 );
	pWidgetVBox->addWidget( pWidgetBox );
	pWidgetVBox->insertStretch( vWidget.size(), -1 );
	pColorCategoriesLayout->addWidget( pWidgetVGBox );

	QVBoxLayout* pPaletteVBox = new QVBoxLayout;
	QGroupBox* pPaletteVGBox = new QGroupBox;
	pPaletteVGBox->setLayout( pPaletteVBox );
	QGroupBox* pPaletteBox = new QGroupBox( tr( "General Colors" ) );
	QGridLayout* pPaletteGrid = new QGridLayout;
	pPaletteBox->setLayout( pPaletteGrid );
	pPaletteGrid->setRowMinimumHeight( 0, 15 );
	for ( auto ii : vPalette ) {
		pPaletteGrid->addWidget( m_colorSelections[ii].first.get(), ii + 1, 0 );
		pPaletteGrid->addWidget( m_colorSelections[ii].second.get(), ii + 1, 1 );
	}
	pPaletteGrid->setColumnMinimumWidth( 1, 40 );
	pPaletteGrid->setColumnStretch( 1, 0 );
	pPaletteGrid->setColumnStretch( 0, 10 );
	pPaletteVBox->addWidget( pPaletteBox );
	pPaletteVBox->insertStretch( vPalette.size(), -1 );
	pColorCategoriesLayout->addWidget( pPaletteVGBox );

	QVBoxLayout* pSongEditorVBox = new QVBoxLayout;
	QGroupBox* pSongEditorVGBox = new QGroupBox;
	pSongEditorVGBox->setLayout( pSongEditorVBox );
	QGroupBox* pSongEditorBox = new QGroupBox( tr( "Colors used in SongEditor" ) );
	QGridLayout* pSongEditorGrid = new QGridLayout;
	pSongEditorBox->setLayout( pSongEditorGrid );
	pSongEditorGrid->setRowMinimumHeight( 0, 15 );
	for ( auto ii : vSongEditor ) {
		pSongEditorGrid->addWidget( m_colorSelections[ii].first.get(), ii + 1, 0 );
		pSongEditorGrid->addWidget( m_colorSelections[ii].second.get(), ii + 1, 1 );
	}
	pSongEditorGrid->setColumnMinimumWidth( 1, 40 );
	pSongEditorGrid->setColumnStretch( 1, 0 );
	pSongEditorGrid->setColumnStretch( 0, 10 );
	pSongEditorVBox->addWidget( pSongEditorBox );
	pSongEditorVBox->insertStretch( vSongEditor.size(), -1 );
	pColorCategoriesLayout->addWidget( pSongEditorVGBox );

	QVBoxLayout* pPatternEditorVBox = new QVBoxLayout;
	QGroupBox* pPatternEditorVGBox = new QGroupBox;
	pPatternEditorVGBox->setLayout( pPatternEditorVBox );
	QGroupBox* pPatternEditorBox = new QGroupBox( tr( "Colors used in PatternEditor" ) );
	QGridLayout* pPatternEditorGrid = new QGridLayout;
	pPatternEditorBox->setLayout( pPatternEditorGrid );
	pPatternEditorGrid->setRowMinimumHeight( 0, 15 );
	for ( auto ii : vPatternEditor ) {
		pPatternEditorGrid->addWidget( m_colorSelections[ii].first.get(), ii + 1, 0 );
		pPatternEditorGrid->addWidget( m_colorSelections[ii].second.get(), ii + 1, 1 );
	}
	pPatternEditorGrid->setColumnMinimumWidth( 1, 40 );
	pPatternEditorGrid->setColumnStretch( 1, 0 );
	pPatternEditorGrid->setColumnStretch( 0, 10 );
	pPatternEditorVBox->addWidget( pPatternEditorBox );
	pPatternEditorVBox->insertStretch( vPatternEditor.size(), -1 );
	pColorCategoriesLayout->addWidget( pPatternEditorVGBox );
	
	connect( this, &PaletteDialog::rejected, this, &PaletteDialog::onRejected );

	QDialogButtonBox* pButtonBox = new QDialogButtonBox( QDialogButtonBox::Ok
														 | QDialogButtonBox::Cancel);

    connect( pButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept );
    connect( pButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject );

	pMainLayout->addWidget( pColorCategoriesBox );
	pMainLayout->addWidget( pButtonBox );
	
	setLayout( pMainLayout );
	setStyleSheet( QString( "\
QGroupBox { \
    background-color: %1; \
    border: 1px solid %2; \
    font-size: 15px; \
    font-weight: bold; \
    color: %3; \
 }")
				   .arg( m_previousStyle.m_songEditor_alternateRowColor.name() )
				   .arg( m_previousStyle.m_windowColor.name() )
				   .arg( m_previousStyle.m_windowTextColor.name() ) );

}
		
PaletteDialog::~PaletteDialog() {
}

void PaletteDialog::addPair( QString sNew, QColor newColor ) {

	auto pLabel = std::make_shared<ClickableLabel>( nullptr, QSize( 0, 0 ), sNew );
	auto pButton = std::make_shared<ColorSelectionButton>( nullptr, newColor, 0 );
	connect( pButton.get(), &ColorSelectionButton::clicked, this, &PaletteDialog::onColorSelectionClicked );
	
	m_colorSelections.push_back( std::pair<std::shared_ptr<ClickableLabel>,
								 std::shared_ptr<ColorSelectionButton>>( pLabel, pButton ) );
}

void PaletteDialog::onColorSelectionClicked() {

	m_currentStyle.m_accentColor = m_colorSelections[ 0 ].second->getColor();
	m_currentStyle.m_widgetColor = m_colorSelections[ 1 ].second->getColor();
	m_currentStyle.m_widgetTextColor = m_colorSelections[ 2 ].second->getColor();
	m_currentStyle.m_windowColor = m_colorSelections[ 3 ].second->getColor();
	m_currentStyle.m_windowTextColor = m_colorSelections[ 4 ].second->getColor();
	m_currentStyle.m_baseColor = m_colorSelections[ 5 ].second->getColor();
	m_currentStyle.m_alternateBaseColor = m_colorSelections[ 6 ].second->getColor();
	m_currentStyle.m_textColor = m_colorSelections[ 7 ].second->getColor();
	m_currentStyle.m_buttonColor = m_colorSelections[ 8 ].second->getColor();
	m_currentStyle.m_buttonTextColor = m_colorSelections[ 9 ].second->getColor();
	m_currentStyle.m_lightColor = m_colorSelections[ 10 ].second->getColor();
	m_currentStyle.m_midLightColor = m_colorSelections[ 11 ].second->getColor();
	m_currentStyle.m_midColor = m_colorSelections[ 12 ].second->getColor();
	m_currentStyle.m_darkColor = m_colorSelections[ 13 ].second->getColor();
	m_currentStyle.m_shadowTextColor = m_colorSelections[ 14 ].second->getColor();
	m_currentStyle.m_highlightColor = m_colorSelections[ 15 ].second->getColor();
	m_currentStyle.m_highlightedTextColor = m_colorSelections[ 16 ].second->getColor();
	m_currentStyle.m_songEditor_backgroundColor = m_colorSelections[ 17 ].second->getColor();
	m_currentStyle.m_songEditor_alternateRowColor = m_colorSelections[ 18 ].second->getColor();
	m_currentStyle.m_songEditor_selectedRowColor = m_colorSelections[ 19 ].second->getColor();
	m_currentStyle.m_songEditor_lineColor = m_colorSelections[ 20 ].second->getColor();
	m_currentStyle.m_songEditor_textColor = m_colorSelections[ 21 ].second->getColor();
	m_currentStyle.m_patternEditor_backgroundColor = m_colorSelections[ 22 ].second->getColor();
	m_currentStyle.m_patternEditor_alternateRowColor = m_colorSelections[ 23 ].second->getColor();
	m_currentStyle.m_patternEditor_selectedRowColor = m_colorSelections[ 24 ].second->getColor();
	m_currentStyle.m_patternEditor_textColor = m_colorSelections[ 25 ].second->getColor();
	m_currentStyle.m_patternEditor_noteColor = m_colorSelections[ 26 ].second->getColor();
	m_currentStyle.m_patternEditor_noteoffColor = m_colorSelections[ 27 ].second->getColor();
	m_currentStyle.m_patternEditor_lineColor = m_colorSelections[ 28 ].second->getColor();
	m_currentStyle.m_patternEditor_line1Color = m_colorSelections[ 29 ].second->getColor();
	m_currentStyle.m_patternEditor_line2Color = m_colorSelections[ 30 ].second->getColor();
	m_currentStyle.m_patternEditor_line3Color = m_colorSelections[ 31 ].second->getColor();
	m_currentStyle.m_patternEditor_line4Color = m_colorSelections[ 32 ].second->getColor();
	m_currentStyle.m_patternEditor_line5Color = m_colorSelections[ 33 ].second->getColor();
	m_currentStyle.m_selectionHighlightColor = m_colorSelections[ 34 ].second->getColor();
	m_currentStyle.m_selectionInactiveColor = m_colorSelections[ 35 ].second->getColor();
	m_currentStyle.m_accentTextColor = m_colorSelections[ 36 ].second->getColor();
	m_currentStyle.m_buttonRedColor = m_colorSelections[ 37 ].second->getColor();
	m_currentStyle.m_buttonRedTextColor = m_colorSelections[ 38 ].second->getColor();
	m_currentStyle.m_spinBoxSelectionColor = m_colorSelections[ 39 ].second->getColor();
	m_currentStyle.m_spinBoxSelectionTextColor = m_colorSelections[ 40 ].second->getColor();
	m_currentStyle.m_automationColor = m_colorSelections[ 41 ].second->getColor();
	m_currentStyle.m_automationCircleColor = m_colorSelections[ 42 ].second->getColor();
	m_currentStyle.m_toolTipBaseColor = m_colorSelections[ 43 ].second->getColor();
	m_currentStyle.m_toolTipTextColor = m_colorSelections[ 44 ].second->getColor();
	
	H2Core::Preferences::get_instance()->setDefaultUIStyle( &m_currentStyle );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Colors );
}

void PaletteDialog::onRejected() {
	H2Core::Preferences::get_instance()->setDefaultUIStyle( &m_previousStyle );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Colors );
}
	
