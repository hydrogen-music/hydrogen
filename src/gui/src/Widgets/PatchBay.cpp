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

#include "PatchBay.h"

#include "Button.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Types/Patch.h"

PatchBay::PatchBay( QWidget* pParent,
					H2Core::PatternList* pPatternList,
					std::shared_ptr<H2Core::Drumkit> pDrumkit )
	: QDialog( pParent )
	, m_pPatternList( pPatternList )
	, m_pDrumkit( pDrumkit)
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// General layout structure
	setMinimumWidth( 750 );

	m_pMainLayout = new QVBoxLayout();

	QHBoxLayout* pButtonLayout = new QHBoxLayout();
	pButtonLayout->setMargin( 0 );
	Button* pApplyButton =
		new Button( this, QSize( 120, 24 ), Button::Type::Push, "",
					pCommonStrings->getButtonApply() );
	connect( pApplyButton, &QPushButton::clicked,
			 this, &PatchBay::applyButtonClicked );
	Button* pCancelButton =
		new Button( this, QSize( 120, 24 ), Button::Type::Push, "",
					pCommonStrings->getButtonCancel() );
	connect( pCancelButton, &QPushButton::clicked, this, &QDialog::reject );

	// stretches on both sides center the buttons
	pButtonLayout->addStretch();
	pButtonLayout->addWidget( pApplyButton );
	pButtonLayout->addWidget( pCancelButton );
	pButtonLayout->addStretch();

	QWidget* pButtonBox = new QWidget();
	pButtonBox->setLayout( pButtonLayout );

	m_pMainLayout->insertWidget( 1, pButtonBox );
	setLayout( m_pMainLayout );

	setup();
}

PatchBay::~PatchBay() {}

void PatchBay::setup() {
	if ( m_pPatternList == nullptr ) {
		ERRORLOG( "Invalid pattern list" );
		return;
	}
	if ( m_pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit" );
		return;
	}

	m_patternTypesBoxes.clear();
	m_drumkitInstrumentBoxes.clear();
	m_instrumentLabels.clear();

	for ( const auto& ppInstrument : *m_pDrumkit->getInstruments() ) {
		if ( ppInstrument == nullptr ) {
			continue;
		}

		QString sLabel = ppInstrument->get_name();
		if ( ! ppInstrument->getType().isEmpty() ) {
			sLabel.append( QString( " (%1)" ).arg( ppInstrument->getType() ) );
		}
		m_instrumentLabels.push_back(
			std::make_pair( ppInstrument->get_id(), sLabel ) );
	}
	const auto drumkitTypes = m_pDrumkit->getAllTypes();

	const auto patternTypes = m_pPatternList->getAllTypes();
	if ( patternTypes.size() == 0 ) {
		ERRORLOG( "Provided pattern list does not contain instrument types" );
		return;
	}

	QGridLayout* pGridLayout = new QGridLayout();

	// Containing the right hand side - which instrument type all notes with a
	// specific type will be remapped to.
	auto createComboBox = [=]() {
		LCDCombo* pCombo = new LCDCombo( this );
		pCombo->addItem( "" );
		for ( const auto& [_, ssLabel ] : m_instrumentLabels ) {
			pCombo->addItem( ssLabel );
		}

		return pCombo;
	};

	int nnRow = 0;
	for ( const auto& ssPatternType : patternTypes ) {
		LCDDisplay* pDisplay =
			new LCDDisplay( this, QSize( 0, 0 ), false, false );
		pDisplay->setText( ssPatternType );

		LCDCombo* pCombo = createComboBox();

		// On startup the combo box will either show the same type in case it is
		// also present within the drumkit or will be empty.
		if ( auto search = drumkitTypes.find( ssPatternType );
			 search != drumkitTypes.end() ) {
			for ( const auto& ppInstrument : *m_pDrumkit->getInstruments() ) {
				if ( ppInstrument != nullptr &&
					 ppInstrument->getType() == ssPatternType ) {

					// Offset of one because we add an empty element at the top.
					int nnIndex = 1;
					for ( const auto [ nnId, _ ] : m_instrumentLabels ) {
						if ( nnId == ppInstrument->get_id() ) {
							pCombo->setCurrentIndex( nnIndex );
							break;
						}
						++nnIndex;
					}
					break;
				}
			}
		}

		pGridLayout->addWidget( pDisplay, nnRow, 0 );
		pGridLayout->addWidget( pCombo, nnRow, 1 );

		m_patternTypesBoxes.push_back( pDisplay );
		m_drumkitInstrumentBoxes.push_back( pCombo );

		++nnRow;
	}

	QWidget* pGridBox = new QWidget();
	pGridBox->setLayout( pGridLayout );

	m_pMainLayout->insertWidget( 0, pGridBox );
}

void PatchBay::applyButtonClicked() {
	Patch patch;

	for ( int ii = 0; ii < m_patternTypesBoxes.size(); ++ii ) {
		const QString ssPatternType = m_patternTypesBoxes[ ii ]->text();
		patch.addMapping(
			ssPatternType,
			m_instrumentLabels[
				m_drumkitInstrumentBoxes[ ii ]->currentIndex() + 1 ].first,
			m_pPatternList->getAllNotesOfType( ssPatternType ) );
	}

	DEBUGLOG( patch.toQString() );

	accept();
}
