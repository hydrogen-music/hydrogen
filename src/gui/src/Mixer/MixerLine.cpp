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

#include "MixerLine.h"

#include <stdio.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/MidiAction.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../Widgets/Button.h"
#include "../Widgets/Fader.h"
#include "../Widgets/InstrumentNameWidget.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LED.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/WidgetWithInput.h"

using namespace H2Core;

MixerLine::MixerLine(QWidget* parent, int nInstr)
	: PixmapWidget( parent )
	, m_fMaxPeak( 0.0 )
	, m_nActivity( 0 )
	, m_bIsSelected( false )
	, m_nPeakTimer( 0 )
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	std::shared_ptr<Action> pAction;

	resize( MixerLine::nWidth, MixerLine::nHeight );
	setFixedSize( MixerLine::nWidth, MixerLine::nHeight );

	setPixmap( "/mixerPanel/mixerline_background.png" );

	// Play sample button
	m_pPlaySampleBtn = new Button(
		this, QSize( 20, 15 ), Button::Type::Push, "play.svg", "", false,
		QSize( 7, 7 ), tr( "Play sample" ) );
	m_pPlaySampleBtn->move( 6, 1 );
	m_pPlaySampleBtn->setObjectName( "PlaySampleButton" );
	connect(m_pPlaySampleBtn, &Button::clicked,
			[&]() { emit noteOnClicked(this); });
	connect(m_pPlaySampleBtn, &Button::rightClicked,
			[&]() { emit noteOffClicked(this); });

	// Trigger sample LED
	m_pTriggerSampleLED = new LED( this, QSize( 5, 13 ) );
	m_pTriggerSampleLED->move( 26, 2 );
	m_pTriggerSampleLED->setObjectName( "TriggerSampleLED" );
	
	// LED indicating that this particular mixerline is selected
	m_pSelectionLED = new LED( this, QSize( 11, 9 ) );
	m_pSelectionLED->move( 39, 2 );
	m_pSelectionLED->setObjectName( "SelectionLED" );

	// Mute button
	m_pMuteBtn = new Button(
		this, QSize( 22, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getSmallMuteButton(), true, QSize(), tr( "Mute" ) );
	m_pMuteBtn->move( 5, 16 );
	m_pMuteBtn->setObjectName( "MixerMuteButton" );
	connect(m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteBtnClicked() ));
	pAction = std::make_shared<Action>("STRIP_MUTE_TOGGLE");
	pAction->setParameter1( QString::number(nInstr ));
	m_pMuteBtn->setAction(pAction);

	// Solo button
	m_pSoloBtn = new Button(
		this, QSize( 22, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getSmallSoloButton(), false, QSize(), tr( "Solo" ) );
	m_pSoloBtn->move( 28, 16 );
	m_pSoloBtn->setObjectName( "MixerSoloButton" );
	connect(m_pSoloBtn, SIGNAL( clicked() ), this, SLOT( soloBtnClicked() ));
	pAction = std::make_shared<Action>("STRIP_SOLO_TOGGLE");
	pAction->setParameter1( QString::number(nInstr ));
	m_pSoloBtn->setAction(pAction);

	// pan rotary
	m_pPanRotary = new Rotary(
		this, Rotary::Type::Center, pCommonStrings->getNotePropertyPan(), false,
		PAN_MIN, PAN_MAX );
	m_pPanRotary->setObjectName( "PanRotary" );
	m_pPanRotary->move( 6, 32 );
	pAction = std::make_shared<Action>("PAN_ABSOLUTE");
	pAction->setParameter1( QString::number(nInstr ));
	pAction->setValue( QString::number( 0 ));
	m_pPanRotary->setAction(pAction);
	connect( m_pPanRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( panChanged( WidgetWithInput* ) ) );

	// FX send
	int nnRow = 0;
	for ( int ii = 0; ii < MAX_FX; ii++ ) {
		auto pRotary = new Rotary(
			this, Rotary::Type::Small, tr( "FX %1 send" ).arg( ii + 1 ), false );
		pRotary->setObjectName( "FXRotary" );
		pAction = std::make_shared<Action>( "EFFECT_LEVEL_ABSOLUTE" );
		pAction->setParameter1( QString::number( nInstr ) );
		pAction->setParameter2( QString::number( ii ) );
		pRotary->setAction( pAction );
		if ( (ii % 2) == 0 ) {
			pRotary->move( 9, 63 + (20 * nnRow) );
		}
		else {
			pRotary->move( 30, 63 + (20 * nnRow) );
			nnRow++;
		}
		connect( pRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
				 this, SLOT( knobChanged( WidgetWithInput* ) ) );
		m_fxRotaries.push_back( pRotary );
	}

	const float fFalloff =
		Preferences::get_instance()->getTheme().m_interface.m_fMixerFalloffSpeed;
	m_nFalloffSpeed = static_cast<int>( std::floor( fFalloff * 20 - 2 ) );

	// instrument name widget
	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 6, 128 );
	connect( m_pNameWidget, SIGNAL( doubleClicked() ), this, SLOT( nameClicked() ) );
	connect( m_pNameWidget, SIGNAL( clicked() ), this, SLOT( nameSelected() ) );

	// m_pFader
	m_pFader = new Fader( this, Fader::Type::Normal, tr( "Volume" ), false,
						  false, 0.0, 1.5 );
	m_pFader->move( 23, 128 );
	connect( m_pFader, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( faderChanged( WidgetWithInput* ) ) );
	pAction = std::make_shared<Action>("STRIP_VOLUME_ABSOLUTE");
	pAction->setParameter1( QString::number(nInstr) );
	m_pFader->setAction( pAction );


	m_pPeakLCD = new LCDDisplay( this, QSize( 41, 19 ), false, false );
	m_pPeakLCD->move( 8, 105 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setToolTip( tr( "Peak" ) );
	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Window, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );
}

MixerLine::~MixerLine() {
}

void MixerLine::updateMixerLine()
{
	if ( m_nPeakTimer > m_nFalloffSpeed ) {
		if ( m_fMaxPeak > 0.05f ) {
			m_fMaxPeak = m_fMaxPeak - 0.05f;
		}
		else {
			m_fMaxPeak = 0.0f;
			m_nPeakTimer = 0;
		}
		m_pPeakLCD->setText( QString( "%1" ).arg( m_fMaxPeak, 0, 'f', 2 ) );
		if ( m_fMaxPeak > 1.0 ) {
			m_pPeakLCD->setUseRedFont( true );
		}
		else {
			m_pPeakLCD->setUseRedFont( false );
		}
	}
	m_nPeakTimer++;
}

void MixerLine::muteBtnClicked() {
	Hydrogen::get_instance()->setIsModified( true );
	emit muteBtnClicked(this);
}

void MixerLine::soloBtnClicked() {
	Hydrogen::get_instance()->setIsModified( true );
	emit soloBtnClicked(this);
}

void MixerLine::faderChanged( WidgetWithInput *pRef ) {

	assert( pRef );
	
	Hydrogen::get_instance()->setIsModified( true );
	emit volumeChanged(this);

	WidgetWithInput* pFader = static_cast<Fader*>( pRef );
	
	double value = (double) pFader->getValue();

	QString sMessage = tr( "Set volume [%1] of instrument" )
		.arg( value, 0, 'f', 2 );
	sMessage.append( QString( " [%1]" )
					 .arg( m_pNameWidget->text() ) );
	QString sCaller = QString( "%1:faderChanged:%2" )
		.arg( class_name() ).arg( m_pNameWidget->text() );
	
	( HydrogenApp::get_instance() )->
		showStatusBarMessage( sMessage, sCaller );
}

bool MixerLine::isMuteClicked() const {
	return ( ( m_pMuteBtn->isChecked() && ! m_pMuteBtn->isDown() ) ||
			 ( ! m_pMuteBtn->isChecked() && m_pMuteBtn->isDown() ) );
}

void MixerLine::setMuteClicked(bool isClicked) {
	if ( ! m_pMuteBtn->isDown() ) {
		m_pMuteBtn->setChecked(isClicked);
	}
}

bool MixerLine::isSoloClicked() const {
	return ( ( m_pSoloBtn->isChecked() && ! m_pSoloBtn->isDown() ) || ( ! m_pSoloBtn->isChecked() && m_pSoloBtn->isDown() ) );
}

void MixerLine::setSoloClicked(bool isClicked) {
	if ( ! m_pSoloBtn->isDown() ) {
		m_pSoloBtn->setChecked(isClicked);
	}
}

float MixerLine::getVolume() const {
	return m_pFader->getValue();
}

void MixerLine::setVolume( float value, H2Core::Event::Trigger trigger ) {
	m_pFader->setValue( value, false, trigger );
}

void MixerLine::setPeak_L( float peak ) {
	if (peak != getPeak_L() ) {
		m_pFader->setPeak_L( peak );
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			m_pPeakLCD->setText( QString( "%1" ).arg( peak, 0, 'f', 2 ) );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setUseRedFont( true );
			}
			else {
				m_pPeakLCD->setUseRedFont( false );
			}
			m_fMaxPeak = peak;
			m_nPeakTimer = 0;
		}
	}
}

float MixerLine::getPeak_L() const {
	return m_pFader->getPeak_L();
}

void MixerLine::setPeak_R( float peak ) {
	if (peak != getPeak_R() ) {
		m_pFader->setPeak_R( peak );
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			m_pPeakLCD->setText( QString( "%1" ).arg( peak, 0, 'f', 2 ) );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setUseRedFont( true );
			}
			else {
				m_pPeakLCD->setUseRedFont( false );
			}
			m_fMaxPeak = peak;
			m_nPeakTimer = 0;
		}
	}
}

float MixerLine::getPeak_R() const {
	return m_pFader->getPeak_R();
}

void MixerLine::setName( const QString& sName) {
	m_pNameWidget->setText( sName );
}

const QString& MixerLine::getName() const {
	return m_pNameWidget->text();
}

void MixerLine::nameClicked() {
	emit instrumentNameClicked(this);
}

void MixerLine::nameSelected() {
	emit instrumentNameSelected(this);
}

void MixerLine::panChanged(WidgetWithInput *ref)
{
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	emit panChanged( this );
	/** Do not update tooltip nor print status message in the old fashion panL and panL style
	 *	since inconsistent with new pan implementation. The resultant pan depends also on note pan.
	 *	The rotary widget valuetip is enough to read the value.
	 */
}

float MixerLine::getPan() const {
	return m_pPanRotary->getValue();
}

void MixerLine::setPan( float fValue, H2Core::Event::Trigger trigger )
{
	m_pPanRotary->setValue( fValue, false, trigger );
	/** Do not update tooltip in the old fashion panL and panL style
	 * since inconsistent with new pan implementation. The resultant pan depends also on note pan.
	 * The rotary widget valuetip is enough to read the value.
	 */
}

void MixerLine::setPlayClicked( bool clicked ) {
	m_pTriggerSampleLED->setActivated( clicked );
}

void MixerLine::knobChanged( WidgetWithInput* pRef)
{
	assert( pRef );
	Rotary* pRotary = static_cast<Rotary*>( pRef );
	
	for ( uint i = 0; i < MAX_FX; i++ ) {
		if ( m_fxRotaries[i] == pRotary ) {
			emit knobChanged( this, i );
			break;
		}
	}
}

void MixerLine::setFXLevel( int nFX, float fValue,
							H2Core::Event::Trigger trigger ) {
	if ( nFX >= MAX_FX ) {
		ERRORLOG( QString("[setFXLevel] nFX >= MAX_FX (nFX=%1)").arg(nFX) );
		return;
	}
	m_fxRotaries[ nFX ]->setValue( fValue, false, trigger );
}

float MixerLine::getFXLevel( int nFX ) const {
	if ( nFX >= MAX_FX ) {
		ERRORLOG( QString("[setFXLevel] nFX >= MAX_FX (nFX=%1)").arg(nFX) );
		return 0.0f;
	}
	return m_fxRotaries[ nFX ]->getValue();
}

void MixerLine::setSelected( bool bIsSelected ) {
	if ( m_bIsSelected == bIsSelected ) {
		return;
	}

	m_bIsSelected = bIsSelected;
	m_pSelectionLED->setActivated( bIsSelected );
}
