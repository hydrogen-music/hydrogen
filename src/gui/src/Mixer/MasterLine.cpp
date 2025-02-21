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

#include "MasterLine.h"

#include "../InstrumentEditor/InstrumentEditor.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../CommonStrings.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/Fader.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LED.h"
#include "../Widgets/WidgetWithInput.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/MidiAction.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

using namespace H2Core;

MasterLine::MasterLine( QWidget* pParent )
 : PixmapWidget( pParent )
 , m_fMaxPeak( 0 )
 , m_nPeakTimer( 0 )
{
	setMinimumSize( MasterLine::nWidth, MasterLine::nHeight );
	setMaximumSize( MasterLine::nWidth, MasterLine::nHeight );
	resize( MasterLine::nWidth, MasterLine::nHeight );

	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );

	// Background image
	setPixmap( "/mixerPanel/masterMixerline_background.png" );

	const auto pPref = Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	const float fFalloff =
		Preferences::get_instance()->getTheme().m_interface.m_fMixerFalloffSpeed;
	m_nFalloffSpeed = static_cast<int>( std::floor( fFalloff * 20 - 2 ) );

	m_pMasterFader = new Fader(
		this, Fader::Type::Master, tr( "Master volume" ), false, false, 0.0, 1.5 );
	m_pMasterFader->move( 24, 75 );
	connect( m_pMasterFader, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( faderChanged( WidgetWithInput* ) ) );

	auto pAction = std::make_shared<Action>("MASTER_VOLUME_ABSOLUTE");
	m_pMasterFader->setAction( pAction );

	m_pPeakLCD = new LCDDisplay( this, QSize( 38, 18 ), false, false );
	m_pPeakLCD->move( 22, 51 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setToolTip( tr( "Peak" ) );

	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Window, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );

	m_pHumanizeVelocityRotary = new Rotary(
		this, Rotary::Type::Normal, tr( "Humanize velocity" ), false );
	m_pHumanizeVelocityRotary->move( 66, 88 );
	connect( m_pHumanizeVelocityRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	m_pHumanizeTimeRotary = new Rotary(
		this, Rotary::Type::Normal, tr( "Humanize time" ), false );
	m_pHumanizeTimeRotary->move( 66, 125 );
	connect( m_pHumanizeTimeRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	m_pSwingRotary = new Rotary(
		this,  Rotary::Type::Normal, tr( "16th-note Swing" ), false );
	m_pSwingRotary->move( 66, 162 );
	connect( m_pSwingRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	// Mute btn
	m_pMuteBtn = new Button( this, QSize( 42, 17 ), Button::Type::Toggle, "",
							 pCommonStrings->getBigMuteButton(), true );
	m_pMuteBtn->setObjectName( "MixerMasterMuteButton" );
	m_pMuteBtn->move( 20, 31 );
	connect( m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteClicked() ) );
	pAction = std::make_shared<Action>("MUTE_TOGGLE");
	m_pMuteBtn->setAction( pAction );

	m_pMasterLbl = new ClickableLabel(
		this, QSize( 55, 15 ), pCommonStrings->getMasterLabel(),
		ClickableLabel::Color::Dark );
	m_pMasterLbl->move( 14, 8 );
	m_pHumanizeLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getHumanizeLabel(),
		ClickableLabel::Color::Dark );
	m_pHumanizeLbl->move( 62, 79 );
	m_pSwingLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getNotePropertyVelocity(),
		ClickableLabel::Color::Dark );
	m_pSwingLbl->move( 62, 116 );
	m_pTimingLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getTimingLabel(),
		ClickableLabel::Color::Dark );
	m_pTimingLbl->move( 62, 153 );
	m_pVelocityLbl = new ClickableLabel(
		this, QSize( 51, 9 ), pCommonStrings->getSwingLabel(),
		ClickableLabel::Color::Dark );
	m_pVelocityLbl->move( 62, 190 );
}

MasterLine::~MasterLine() {
}

void MasterLine::muteClicked() {
	CoreActionController::setMasterIsMuted( m_pMuteBtn->isChecked() );
}

void MasterLine::faderChanged( WidgetWithInput *pRef )
{
	assert( pRef );
	
	Fader* pFader = static_cast<Fader*>( pRef );
	m_pMasterFader->setValue( pFader->getValue() );

	emit volumeChanged(this);

	Hydrogen::get_instance()->setIsModified( true );

	double value = (double) pFader->getValue();
	( HydrogenApp::get_instance() )->
		showStatusBarMessage( tr( "Set master volume [%1]" )
							  .arg( value, 0, 'f', 2 ),
							  QString( "%1:faderChanged" )
							  .arg( class_name() ) );
}

float MasterLine::getVolume() const {
	return m_pMasterFader->getValue();
}

void MasterLine::setVolume( float fValue, H2Core::Event::Trigger trigger ) {
	m_pMasterFader->setValue( fValue, false, trigger );
}

float MasterLine::getHumanizeTime() const {
	return m_pHumanizeTimeRotary->getValue();
}

void MasterLine::setHumanizeTime( float fValue,
									   H2Core::Event::Trigger trigger ) {
	m_pHumanizeTimeRotary->setValue( fValue, false, trigger );
}

float MasterLine::getHumanizeVelocity() const {
	return m_pHumanizeVelocityRotary->getValue();
}

void MasterLine::setHumanizeVelocity( float fValue,
										   H2Core::Event::Trigger trigger ) {
	m_pHumanizeVelocityRotary->setValue( fValue, false, trigger );
}

float MasterLine::getSwing() const {
	return m_pSwingRotary->getValue();
}

void MasterLine::setSwing( float fValue, H2Core::Event::Trigger trigger ) {
	m_pSwingRotary->setValue( fValue, false, trigger );
}

void MasterLine::setPeak_L( float fPeak )
{
	if ( fPeak != getPeak_L() ) {
		m_pMasterFader->setPeak_L(fPeak);
		if (fPeak > m_fMaxPeak) {
			if ( fPeak < 0.1f ) {
				fPeak = 0.0f;
			}
			m_pPeakLCD->setText( QString( "%1" ).arg( m_fMaxPeak, 0, 'f', 2 ) );
			if ( fPeak > 1.0 ) {
				m_pPeakLCD->setUseRedFont( true );
			}
			else {
				m_pPeakLCD->setUseRedFont( false );
			}
			m_fMaxPeak = fPeak;
			m_nPeakTimer = 0;
		}
	}
}

float MasterLine::getPeak_L() const {
	return m_pMasterFader->getPeak_L();
}

void MasterLine::setPeak_R( float fPeak ) {
	if ( fPeak != getPeak_R() ) {
		m_pMasterFader->setPeak_R(fPeak);
		if (fPeak > m_fMaxPeak) {
			if ( fPeak < 0.1f ) {
				fPeak = 0.0f;
			}
			m_pPeakLCD->setText( QString( "%1" ).arg( fPeak, 0, 'f', 2 ) );
			if ( fPeak > 1.0 ) {
				m_pPeakLCD->setUseRedFont( true );
			}
			else {
				m_pPeakLCD->setUseRedFont( false );
			}
			m_fMaxPeak = fPeak;
			m_nPeakTimer = 0;
		}
	}
}

float MasterLine::getPeak_R() const {
	return m_pMasterFader->getPeak_R();
}

void MasterLine::updateMixerLine() {
	if ( m_nPeakTimer > m_nFalloffSpeed ) {
		if ( m_fMaxPeak  > 0.05f ) {
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

bool MasterLine::isMuteChecked() const {
	return m_pMuteBtn->isChecked();
}

void MasterLine::setMuteChecked( bool bIsChecked ) {
	m_pMuteBtn->setChecked( bIsChecked );
}

void MasterLine::rotaryChanged( WidgetWithInput *pRef )
{
	assert( pRef );
	
	Rotary* pRotary = static_cast<Rotary*>( pRef );
	
	QString sMsg;
	QString sCaller = QString( "%1:rotaryChanged" ).arg( class_name() );
	double fVal = (double) pRotary->getValue();

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	if ( pRotary == m_pHumanizeTimeRotary ) {
		pHydrogen->getSong()->setHumanizeTimeValue( fVal );
		sMsg = tr( "Set humanize time param [%1]" ).arg( fVal, 0, 'f', 2 ); //not too long for display
		sCaller.append( ":humanizeTime" );
	}
	else if ( pRotary == m_pHumanizeVelocityRotary ) {
		pHydrogen->getSong()->setHumanizeVelocityValue( fVal );
		sMsg = tr( "Set humanize vel. param [%1]" ).arg( fVal, 0, 'f', 2 ); //not too long for display
		sCaller.append( ":humanizeVelocity" );
	}
	else if ( pRotary == m_pSwingRotary ) {
		pHydrogen->getSong()->setSwingFactor( fVal );
		sMsg = tr( "Set swing factor [%1]").arg( fVal, 0, 'f', 2 );
		sCaller.append( ":humanizeSwing" );
	}
	else {
		ERRORLOG( "[knobChanged] Unhandled knob" );
	}

	pHydrogen->getAudioEngine()->unlock();

	( HydrogenApp::get_instance() )->showStatusBarMessage( sMsg, sCaller );
}
