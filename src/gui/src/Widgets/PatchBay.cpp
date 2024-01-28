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

#include "PatchBay.h"
#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"

#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

PatchBay::PatchBay( QWidget* pParent,
					std::shared_ptr<H2Core::Drumkit> pSourceDrumkit,
					std::shared_ptr<H2Core::Drumkit> pTargetDrumkit,
					Type type )
	: QDialog( pParent )
	, m_pSourceDrumkit( pSourceDrumkit )
	, m_pTargetDrumkit( pTargetDrumkit)
	, m_type( type )
	, m_nFixedRowHeight( 30 )
{
	// We use a local version of the drumkit maps and only save them to the kits
	// if requested by the user.
	m_pSourceDrumkitMap =
		std::make_shared<H2Core::DrumkitMap>(pSourceDrumkit->getDrumkitMap());
	m_pTargetDrumkitMap =
		std::make_shared<H2Core::DrumkitMap>(pTargetDrumkit->getDrumkitMap());

	// General layout structure
	QHBoxLayout* pHBoxLayout = new QHBoxLayout( this );
	setLayout( pHBoxLayout );
	setMinimumWidth( 750 );

	m_pLeftColumn = new QWidget();
	m_pLeftColumnLayout = new QVBoxLayout();
	m_pLeftColumn->setLayout( m_pLeftColumnLayout );
	m_pLeftColumn->setStyleSheet( "background-color: #123; " );
	pHBoxLayout->addWidget( m_pLeftColumn );

	m_pLeftConnections = new QWidget();
	m_pLeftConnections->setFixedWidth( 30 );
	m_pLeftConnections->setStyleSheet( "background-color: #FFF" );
	pHBoxLayout->addWidget( m_pLeftConnections );

	m_pMiddleColumn = new QWidget();
	m_pMiddleColumnLayout = new QVBoxLayout();
	m_pMiddleColumn->setLayout( m_pMiddleColumnLayout );
	pHBoxLayout->addWidget( m_pMiddleColumn );

	m_pRightConnections = new QWidget();
	m_pRightConnections->setFixedWidth( 30 );
	m_pRightConnections->setStyleSheet( "background-color: #FFF" );
	pHBoxLayout->addWidget( m_pRightConnections );

	m_pRightColumn = new QWidget();
	m_pRightColumnLayout = new QVBoxLayout();
	m_pRightColumn->setLayout( m_pRightColumnLayout );
	m_pRightColumn->setStyleSheet( "background-color: #123;" );
	pHBoxLayout->addWidget( m_pRightColumn );

	m_types = QStringList();

	// Fill left and right columns
	for ( const auto& ppInstr : *pSourceDrumkit->getInstruments() ) {
		addLeft( ppInstr );
	}
	m_pLeftColumnLayout->addStretch( 1 );
	for ( const auto& ppInstr : *pTargetDrumkit->getInstruments() ) {
		addRight( ppInstr );
	}
	m_pMiddleColumnLayout->addStretch( 1 );
	m_pRightColumnLayout->addStretch( 1 );

	// Fill middle column.
	const auto sourceTypes = m_pSourceDrumkitMap->getAllTypes();
	for ( const auto& ssType : sourceTypes ) {
		m_types << ssType;
	}
	for ( const auto& ssType : m_pTargetDrumkitMap->getAllTypes() ) {
		if ( auto search = sourceTypes.find( ssType );
			 // Only add items not already present
			 search == sourceTypes.end() ) {
			m_types << ssType;
		}
	}
	for ( const auto& ssType : m_types ) {
		DEBUGLOG( QString( "adding %1" ).arg( ssType ) );
		auto pType = createElement( ssType );
		m_pMiddleColumnLayout->addWidget( pType );
		m_midColumn[ ssType ] = pType;
	}

	// m_pNewTypeButton = new QPushButton( nullptr );
	// m_pNewTypeButton->setIcon( QIcon( Skin::getSvgImagePath() +
	// 								  "/icons/black/plus.svg" ) );
	// m_pNewTypeButton->setCheckable( false );
	m_pNewTypeButton = new Button( nullptr, QSize( 0, 0 ), Button::Type::Push,
								   "plus.svg", "", false );
	m_pNewTypeButton->setFixedHeight( m_nFixedRowHeight );
	m_pNewTypeButton->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
	connect( m_pNewTypeButton, SIGNAL( clicked() ), this, SLOT( newType() ) );
	m_pMiddleColumnLayout->addWidget( m_pNewTypeButton );
}

PatchBay::~PatchBay() {}

LCDDisplay* PatchBay::createElement( const QString& sLabel ) const {
	auto pDisplay = new LCDDisplay( nullptr, QSize( 0, 0 ), false, false );
	pDisplay->setText( sLabel );
	pDisplay->setFixedHeight( m_nFixedRowHeight );
	pDisplay->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

	return pDisplay;
}

void PatchBay::addLeft( std::shared_ptr<H2Core::Instrument> pInstrument ) {
	DEBUGLOG("");
	if ( pInstrument == nullptr ) {
		ERRORLOG( "NULL instrument" );
		return;
	}

	auto pLabel = createElement( pInstrument->get_name() );
	m_pLeftColumnLayout->addWidget( pLabel );
	m_leftColumn[ pInstrument->get_id() ] = pLabel;
}

void PatchBay::addRight( std::shared_ptr<H2Core::Instrument> pInstrument ) {
	if ( pInstrument == nullptr ) {
		ERRORLOG( "NULL instrument" );
		return;
	}

	auto pLabel = createElement( pInstrument->get_name() );
	m_pRightColumnLayout->addWidget( pLabel );
	m_rightColumn[ pInstrument->get_id() ] = pLabel;
}

void PatchBay::drawConnections( QPainter &p ) {
	DEBUGLOG("");

	QPen pen( "#0FF" );
	pen.setWidth( 3 );
	p.setPen( pen );
	p.setBrush( Qt::NoBrush );

	p.drawLine( m_pLeftColumn->width() + 10,
				m_nFixedRowHeight,
				m_pLeftColumn->width() + m_pLeftConnections->width(),
				m_nFixedRowHeight * 3 );
}

void PatchBay::paintEvent( QPaintEvent* pEv ) {
	QDialog::paintEvent( pEv );

	QPainter painter( this );
	drawConnections( painter );
}

void PatchBay::mouseMoveEvent( QMouseEvent *ev ) {
}

void PatchBay::mousePressEvent( QMouseEvent *ev ) {
}
void PatchBay::newType() {
	DEBUGLOG("");
	auto pNewTypeDialog = new NewTypeDialog( this );
	if ( pNewTypeDialog->exec() == QDialog::Accepted ) {

		if ( ! pNewTypeDialog->getType().isEmpty() ) {
			auto pType = createElement( pNewTypeDialog->getType() );
			// Remove the add button
			m_pMiddleColumnLayout->removeWidget( m_pNewTypeButton );

			m_pMiddleColumnLayout->addWidget( pType );
			m_midColumn[ pNewTypeDialog->getType() ] = pType;

			// Readd the add button to ensure it is placed at the bottom.
			m_pMiddleColumnLayout->addWidget( m_pNewTypeButton );
		}
	}
}

NewTypeDialog::NewTypeDialog( QWidget* pParent ) : QDialog( pParent ) {
	DEBUGLOG("");

	setWindowTitle( tr( "Add a new instrument type" ) );
	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Create interactive widgets.
	m_pCombo = new LCDCombo( this, QSize( 0, 0 ), false );
	m_pCombo->setEditable( true );

	Button* pOkButton = new Button( nullptr, QSize( 80, 24 ),
									Button::Type::Push, "",
									pCommonStrings->getButtonOk() );
	pOkButton->setDefault( true );
	connect( pOkButton, &QPushButton::clicked, this, &QDialog::accept );
	auto pCancelButton = new Button( nullptr, QSize( 80, 24 ),
									 Button::Type::Push, "",
									 pCommonStrings->getButtonCancel() );
	connect( pCancelButton, &QPushButton::clicked, this, &QDialog::reject );

	// Create layout
	auto pVBoxLayout = new QVBoxLayout( nullptr );
	setLayout( pVBoxLayout );
	pVBoxLayout->addWidget( m_pCombo );

	auto pActionButtons = new QWidget( nullptr );
	pVBoxLayout->addWidget( pActionButtons );

	auto pHBoxLayout = new QHBoxLayout( nullptr );
	pActionButtons->setLayout( pHBoxLayout );
	pHBoxLayout->addWidget( pCancelButton );
	pHBoxLayout->addWidget( pOkButton );

	for ( const auto& ssType : H2Core::Hydrogen::get_instance()->
			  getSoundLibraryDatabase()->getAllTypes() ) {
		m_pCombo->addItem( ssType );
	}
}

NewTypeDialog::~NewTypeDialog(){}

H2Core::DrumkitMap::Type NewTypeDialog::getType() const {
	return m_pCombo->currentText();
}

void NewTypeDialog::keyPressEvent( QKeyEvent* ev ) {
	int nKey = ev->key();
	if ( nKey == Qt::Key_Enter || nKey == Qt::Key_Return ) {
		accept();
	}
	else if ( nKey == Qt::Key_Escape ) {
		reject();
	}
}
