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

#include <stdio.h>

#include <QPainter>

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

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/MidiAction.h>
using namespace H2Core;

#include "MixerLine.h"

using namespace H2Core;

MixerLine::MixerLine(QWidget* parent, int nInstr)
 : PixmapWidget( parent )
{

	m_fMaxPeak = 0.0;
	m_nActivity = 0;
	m_bIsSelected = false;
	m_nPeakTimer = 0;

	std::shared_ptr<Action> pAction;

	resize( nWidth, nHeight );
	setFixedSize( nWidth, nHeight );

	setPixmap( "/mixerPanel/mixerline_background.png" );

	// Play sample button
	m_pPlaySampleBtn = new Button( this, QSize( 20, 15 ), Button::Type::Push, "play.svg", "", false, QSize( 7, 7 ), tr( "Play sample" ) );
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

	m_pMuteBtn = new Button( this, QSize( 22, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getSmallMuteButton(), true, QSize(), tr( "Mute" ) );
	m_pMuteBtn->move( 5, 16 );
	m_pMuteBtn->setObjectName( "MixerMuteButton" );
	connect(m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteBtnClicked() ));
	pAction = std::make_shared<Action>("STRIP_MUTE_TOGGLE");
	pAction->setParameter1( QString::number(nInstr ));
	m_pMuteBtn->setAction(pAction);

	// Solo button
	m_pSoloBtn = new Button( this, QSize( 22, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getSmallSoloButton(), false, QSize(), tr( "Solo" ) );
	m_pSoloBtn->move( 28, 16 );
	m_pSoloBtn->setObjectName( "MixerSoloButton" );
	connect(m_pSoloBtn, SIGNAL( clicked() ), this, SLOT( soloBtnClicked() ));
	pAction = std::make_shared<Action>("STRIP_SOLO_TOGGLE");
	pAction->setParameter1( QString::number(nInstr ));
	m_pSoloBtn->setAction(pAction);

	// pan rotary
	m_pPanRotary = new Rotary( this, Rotary::Type::Center, tr( "Pan" ), false, -1.0, 1.0 );
	m_pPanRotary->setObjectName( "PanRotary" );
	m_pPanRotary->move( 6, 32 );
	connect( m_pPanRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( panChanged( WidgetWithInput* ) ) );
	pAction = std::make_shared<Action>("PAN_ABSOLUTE");
	pAction->setParameter1( QString::number(nInstr ));
	pAction->setValue( QString::number( 0 ));
	m_pPanRotary->setAction(pAction);

	// FX send
	uint y = 0;
	for ( uint i = 0; i < MAX_FX; i++ ) {
		m_pFxRotary[i] = new Rotary( this, Rotary::Type::Small, tr( "FX %1 send" ).arg( i + 1 ), false );
		m_pFxRotary[i]->setObjectName( "FXRotary" );
		pAction = std::make_shared<Action>( "EFFECT_LEVEL_ABSOLUTE" );
		pAction->setParameter1( QString::number( nInstr ) );
		pAction->setParameter2( QString::number( i ) );
		m_pFxRotary[i]->setAction( pAction );
		if ( (i % 2) == 0 ) {
			m_pFxRotary[i]->move( 9, 63 + (20 * y) );
		}
		else {
			m_pFxRotary[i]->move( 30, 63 + (20 * y) );
			y++;
		}
		connect( m_pFxRotary[i], SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( knobChanged( WidgetWithInput* ) ) );
	}

	Preferences *pPref = Preferences::get_instance();

	float fFalloffTemp = pPref->getMixerFalloffSpeed();
	fFalloffTemp = (fFalloffTemp * 20) - 2;
	m_nFalloff = (int)fFalloffTemp;

	QPixmap textBackground;
	bool ok = textBackground.load( Skin::getImagePath() + "/mixerPanel/mixerline_text_background.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	// instrument name widget
	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 6, 128 );
	connect( m_pNameWidget, SIGNAL( doubleClicked () ), this, SLOT( nameClicked() ) );
	connect( m_pNameWidget, SIGNAL( clicked () ), this, SLOT( nameSelected() ) );

	// m_pFader
	m_pFader = new Fader( this, Fader::Type::Normal, tr( "Volume" ), false, false, 0.0, 1.5 );
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
	if ( m_nPeakTimer > m_nFalloff ) {
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

bool MixerLine::isMuteClicked() {
	return ( ( m_pMuteBtn->isChecked() && ! m_pMuteBtn->isDown() ) ||
			 ( ! m_pMuteBtn->isChecked() && m_pMuteBtn->isDown() ) );
}

void MixerLine::setMuteClicked(bool isClicked) {
	if ( ! m_pMuteBtn->isDown() ) {
		m_pMuteBtn->setChecked(isClicked);
	}
}

bool MixerLine::isSoloClicked() {
	return ( ( m_pSoloBtn->isChecked() && ! m_pSoloBtn->isDown() ) || ( ! m_pSoloBtn->isChecked() && m_pSoloBtn->isDown() ) );
}

void MixerLine::setSoloClicked(bool isClicked) {
	if ( ! m_pSoloBtn->isDown() ) {
		m_pSoloBtn->setChecked(isClicked);
	}
}

float MixerLine::getVolume()
{
	return m_pFader->getValue();
}

void MixerLine::setVolume( float value ) {
	m_pFader->setValue( value );
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

float MixerLine::getPeak_L() {
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

float MixerLine::getPeak_R() {
	return m_pFader->getPeak_R();
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

float MixerLine::getPan()
{
	return m_pPanRotary->getValue();
}

void MixerLine::setPan(float fValue)
{
	if ( fValue != m_pPanRotary->getValue() ) {
		m_pPanRotary->setValue( fValue );
		/** Do not update tooltip in the old fashion panL and panL style
		 * since inconsistent with new pan implementation. The resultant pan depends also on note pan.
		 * The rotary widget valuetip is enough to read the value.
		 */
	}
}

void MixerLine::setPlayClicked( bool clicked ) {
	m_pTriggerSampleLED->setActivated( clicked );
}

void MixerLine::knobChanged( WidgetWithInput* pRef)
{
	assert( pRef );
	Rotary* pRotary = static_cast<Rotary*>( pRef );
	
	for ( uint i = 0; i < MAX_FX; i++ ) {
		if ( m_pFxRotary[i] == pRotary ) {
			emit knobChanged( this, i );
			break;
		}
	}
}

void MixerLine::setFXLevel( uint nFX, float fValue )
{
	if (nFX >= MAX_FX) {
		ERRORLOG( QString("[setFXLevel] nFX >= MAX_FX (nFX=%1)").arg(nFX) );
		return;
	}
	m_pFxRotary[nFX]->setValue( fValue );
}

float MixerLine::getFXLevel(uint nFX)
{
	if (nFX >= MAX_FX) {
		ERRORLOG( QString("[setFXLevel] nFX >= MAX_FX (nFX=%1)").arg(nFX) );
		return 0.0f;
	}
	return m_pFxRotary[nFX]->getValue();
}

void MixerLine::setSelected( bool bIsSelected )
{
	if ( m_bIsSelected == bIsSelected ) {
		return;
	}

	m_bIsSelected = bIsSelected;
	m_pSelectionLED->setActivated( bIsSelected );
}

// ::::::::::::::::::::::::::::


ComponentMixerLine::ComponentMixerLine(QWidget* parent, int CompoID)
 : PixmapWidget( parent )
{
//	

	m_nComponentID = CompoID;

	m_fMaxPeak = 0.0;
	m_nActivity = 0;
	m_bIsSelected = false;
	m_nPeakTimer = 0;

	resize( nWidth, nHeight );
	setFixedSize( nWidth, nHeight );

	setPixmap( "/mixerPanel/componentmixerline_background.png" );

	// Mute button
	m_pMuteBtn = new Button( this, QSize( 22, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getSmallMuteButton(), true, QSize(), tr( "Mute" ) );
	m_pMuteBtn->move( 5, 16 );
	connect(m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteBtnClicked() ));

	// Solo button
	m_pSoloBtn = new Button( this, QSize( 22, 15 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getSmallSoloButton(), false, QSize(), tr( "Solo" ) );
	m_pSoloBtn->move( 28, 16 );
	connect(m_pSoloBtn, SIGNAL( clicked() ), this, SLOT( soloBtnClicked() ));

	Preferences *pPref = Preferences::get_instance();

	float fFalloffTemp = pPref->getMixerFalloffSpeed();
	fFalloffTemp = (fFalloffTemp * 20) - 2;
	m_nFalloff = (int)fFalloffTemp;

	QPixmap textBackground;
	bool ok = textBackground.load( Skin::getImagePath() + "/mixerPanel/mixerline_text_background.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	// instrument name widget
	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 6, 128 );
	m_pNameWidget->setToolTip( tr( "Component name" ) );

	// m_pFader
	m_pFader = new Fader( this, Fader::Type::Normal, tr( "Volume" ), false, false, 0.0, 1.5 );
	m_pFader->move( 23, 128 );
	connect( m_pFader, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( faderChanged( WidgetWithInput* ) ) );

	//pAction = new MidiAction("STRIP_VOLUME_ABSOLUTE");
	//pAction->setParameter1( QString::number(nInstr) );
	//m_pFader->setAction( pAction );


	m_pPeakLCD = new LCDDisplay( this, QSize( 41, 19 ), false, false );
	m_pPeakLCD->move( 8, 105 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setToolTip( tr( "Peak" ) );
	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Window, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );
}



ComponentMixerLine::~ComponentMixerLine() {
}

void ComponentMixerLine::updateMixerLine()
{
	if ( m_nPeakTimer > m_nFalloff ) {
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

void ComponentMixerLine::muteBtnClicked() {
	emit muteBtnClicked(this);
}

void ComponentMixerLine::soloBtnClicked() {
	emit soloBtnClicked(this);
}

void ComponentMixerLine::faderChanged( WidgetWithInput *pRef ) {

	assert( pRef );
	
	Hydrogen::get_instance()->setIsModified( true );
	emit volumeChanged(this);

	WidgetWithInput* pFader = static_cast<Fader*>( pRef );
	double value = (double) pFader->getValue();
	
	QString sMessage = tr( "Set volume [%1] of component" )
		.arg( value, 0, 'f', 2 );
	sMessage.append( QString( " [%1]" )
					 .arg( m_pNameWidget->text() ) );
	QString sCaller = QString( "%1:faderChanged:%2" )
		.arg( class_name() ).arg( m_pNameWidget->text() );
	
	( HydrogenApp::get_instance() )->
		showStatusBarMessage( sMessage, sCaller );
}

bool ComponentMixerLine::isMuteClicked() {
	return ( ( m_pMuteBtn->isChecked() && ! m_pMuteBtn->isDown() ) ||
			 ( ! m_pMuteBtn->isChecked() && m_pMuteBtn->isDown() ) );
}

void ComponentMixerLine::setMuteClicked(bool isClicked) {
	if ( ! m_pMuteBtn->isDown() ) {
		m_pMuteBtn->setChecked(isClicked);
	}
}

bool ComponentMixerLine::isSoloClicked() {
	return ( ( m_pSoloBtn->isChecked() && ! m_pSoloBtn->isDown() ) || ( ! m_pSoloBtn->isChecked() && m_pSoloBtn->isDown() ) );
}

void ComponentMixerLine::setSoloClicked(bool isClicked) {
	if ( ! m_pSoloBtn->isDown() ) {
		m_pSoloBtn->setChecked(isClicked);
	}
}

float ComponentMixerLine::getVolume()
{
	return m_pFader->getValue();
}

void ComponentMixerLine::setVolume( float value ) {
	m_pFader->setValue( value );
}

void ComponentMixerLine::setPeak_L( float peak ) {
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

float ComponentMixerLine::getPeak_L() {
	return m_pFader->getPeak_L();
}

void ComponentMixerLine::setPeak_R( float peak ) {
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

float ComponentMixerLine::getPeak_R() {
	return m_pFader->getPeak_R();
}


// ::::::::::::::::::::::::::::

MasterMixerLine::MasterMixerLine(QWidget* parent)
 : PixmapWidget( parent )
{
	m_fMaxPeak = 0.0f;
	m_nPeakTimer = 0;

	setMinimumSize( nWidth, nHeight );
	setMaximumSize( nWidth, nHeight );
	resize( nWidth, nHeight );
	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Window, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );

	// Background image
	setPixmap( "/mixerPanel/masterMixerline_background.png" );

	Preferences *pPref = Preferences::get_instance();

	float fFalloffTemp = pPref->getMixerFalloffSpeed();
	fFalloffTemp = (fFalloffTemp * 20) - 2;
	m_nFalloff = (int)fFalloffTemp;

	m_pMasterFader = new Fader( this, Fader::Type::Master, tr( "Master volume" ), false, false, 0.0, 1.5 );
	m_pMasterFader->move( 24, 75 );
	connect( m_pMasterFader, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( faderChanged( WidgetWithInput* ) ) );

	std::shared_ptr<Action> pAction = std::make_shared<Action>("MASTER_VOLUME_ABSOLUTE");
	m_pMasterFader->setAction( pAction );

	m_pPeakLCD = new LCDDisplay( this, QSize( 38, 18 ), false, false );
	m_pPeakLCD->move( 22, 51 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setToolTip( tr( "Peak" ) );
	QPalette lcdPalette;
	lcdPalette.setColor( QPalette::Window, QColor( 49, 53, 61 ) );
	m_pPeakLCD->setPalette( lcdPalette );

	m_pHumanizeVelocityRotary = new Rotary( this, Rotary::Type::Normal, tr( "Humanize velocity" ), false );
	m_pHumanizeVelocityRotary->move( 66, 88 );
	connect( m_pHumanizeVelocityRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	m_pHumanizeTimeRotary = new Rotary( this, Rotary::Type::Normal, tr( "Humanize time" ), false );
	m_pHumanizeTimeRotary->move( 66, 125 );
	connect( m_pHumanizeTimeRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	m_pSwingRotary = new Rotary( this,  Rotary::Type::Normal, tr( "16th-note Swing" ), false );
	m_pSwingRotary->move( 66, 162 );
	connect( m_pSwingRotary, SIGNAL( valueChanged( WidgetWithInput* ) ), this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	// Mute btn
	m_pMuteBtn = new Button( this, QSize( 42, 17 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getBigMuteButton(), true );
	m_pMuteBtn->setObjectName( "MixerMasterMuteButton" );
	m_pMuteBtn->move( 20, 31 );
	connect( m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteClicked() ) );
	pAction = std::make_shared<Action>("MUTE_TOGGLE");
	m_pMuteBtn->setAction( pAction );

	m_pMasterLbl = new ClickableLabel( this, QSize( 55, 15 ), HydrogenApp::get_instance()->getCommonStrings()->getMasterLabel(), ClickableLabel::Color::Dark );
	m_pMasterLbl->move( 14, 8 );
	m_pHumanizeLbl = new ClickableLabel( this, QSize( 51, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getHumanizeLabel(), ClickableLabel::Color::Dark );
	m_pHumanizeLbl->move( 62, 79 );
	m_pSwingLbl = new ClickableLabel( this, QSize( 51, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getVelocityLabel(), ClickableLabel::Color::Dark );
	m_pSwingLbl->move( 62, 116 );
	m_pTimingLbl = new ClickableLabel( this, QSize( 51, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getTimingLabel(), ClickableLabel::Color::Dark );
	m_pTimingLbl->move( 62, 153 );
	m_pVelocityLbl = new ClickableLabel( this, QSize( 51, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getSwingLabel(), ClickableLabel::Color::Dark );
	m_pVelocityLbl->move( 62, 190 );
}

MasterMixerLine::~MasterMixerLine()
{
	m_fMaxPeak = 0.0;
}

void MasterMixerLine::muteClicked()
{
	Hydrogen::get_instance()->getCoreActionController()->setMasterIsMuted( m_pMuteBtn->isChecked() );
}

void MasterMixerLine::faderChanged( WidgetWithInput *pRef )
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

float MasterMixerLine::getVolume()
{
	return m_pMasterFader->getValue();
}

void MasterMixerLine::setVolume( float value ) {
	m_pMasterFader->setValue( value );
}

void MasterMixerLine::setPeak_L(float peak)
{
	if ( peak != getPeak_L() ) {
		m_pMasterFader->setPeak_L(peak);
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			m_pPeakLCD->setText( QString( "%1" ).arg( m_fMaxPeak, 0, 'f', 2 ) );
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

float MasterMixerLine::getPeak_L() {
	return m_pMasterFader->getPeak_L();
}

void MasterMixerLine::setPeak_R(float peak) {
	if ( peak != getPeak_R() ) {
		m_pMasterFader->setPeak_R(peak);
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

float MasterMixerLine::getPeak_R() {
	return m_pMasterFader->getPeak_R();
}

void MasterMixerLine::updateMixerLine()
{

	if ( m_nPeakTimer > m_nFalloff ) {
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

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	if ( pSong ) {
		m_pHumanizeTimeRotary->setValue( pSong->getHumanizeTimeValue() );
		m_pHumanizeVelocityRotary->setValue( pSong->getHumanizeVelocityValue() );
		m_pSwingRotary->setValue( pSong->getSwingFactor() );
		if ( ! m_pMuteBtn->isDown() ) {
			m_pMuteBtn->setChecked( pSong->getIsMuted() );
		}
	}
	else {
		WARNINGLOG( "pSong == NULL ");
	}
}

void MasterMixerLine::rotaryChanged( WidgetWithInput *pRef )
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


////////////////////////////////

InstrumentNameWidget::InstrumentNameWidget(QWidget* parent)
 : PixmapWidget( parent )
{
//	infoLog( "INIT" );
	m_nWidgetWidth = 17;
	m_nWidgetHeight = 116;

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &InstrumentNameWidget::onPreferencesChanged );

	setPixmap( "/mixerPanel/mixerline_label_background.png" );

	this->resize( m_nWidgetWidth, m_nWidgetHeight );
}

InstrumentNameWidget::~InstrumentNameWidget()
{
//	infoLog( "DESTROY" );
}



void InstrumentNameWidget::paintEvent( QPaintEvent* ev )
{

	auto pPref = H2Core::Preferences::get_instance();

	PixmapWidget::paintEvent( ev );

	QPainter p( this );
	
	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );

	p.setPen( QColor(230, 230, 230) );
	p.setFont( font );
	p.rotate( -90 );
	p.drawText( -m_nWidgetHeight + 5, 0, m_nWidgetHeight - 10, m_nWidgetWidth, Qt::AlignVCenter, m_sInstrName );
}

void InstrumentNameWidget::setText( QString text )
{
	if (m_sInstrName != text ) {
		m_sInstrName = text;
		update();
	}
}

QString InstrumentNameWidget::text()
{
	return m_sInstrName;
}

void InstrumentNameWidget::mousePressEvent( QMouseEvent * e )
{
	UNUSED( e );
	emit clicked();
}

void InstrumentNameWidget::mouseDoubleClickEvent( QMouseEvent * e )
{
	UNUSED( e );
	emit doubleClicked();
}

void InstrumentNameWidget::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	if ( changes & H2Core::Preferences::Changes::Font ) {
		update();
	}
}

// :::::::::::::::::::::



LadspaFXMixerLine::LadspaFXMixerLine(QWidget* parent)
 : PixmapWidget( parent )
{
	resize( 194, 43 );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	setPixmap( "/mixerPanel/fxline_background.png" );

	// active button
	m_pBypassBtn = new Button( this, QSize( 34, 14 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getBypassButton(), true, QSize(), tr( "FX bypass") );
	m_pBypassBtn->setObjectName( "MixerFXBypassButton" );
	m_pBypassBtn->move( 52, 25 );
	connect( m_pBypassBtn, SIGNAL( clicked() ), this, SLOT( bypassBtnClicked() ) );
	

	// edit button
	m_pEditBtn = new Button( this, QSize( 34, 14 ), Button::Type::Push, "", HydrogenApp::get_instance()->getCommonStrings()->getEditButton(), false, QSize(), tr( "Edit FX parameters") );
	m_pEditBtn->setObjectName( "MixerFXEditButton" );
	m_pEditBtn->move( 86, 25 );
	connect( m_pEditBtn, SIGNAL( clicked() ), this, SLOT( editBtnClicked() ) );

	// instrument name widget
	m_pNameLCD = new LCDDisplay( this, QSize( 108, 15 ), false, false );
	m_pNameLCD->move( 11, 9 );
	m_pNameLCD->setText( "No name" );
	m_pNameLCD->setToolTip( tr( "Ladspa FX name" ) );

	// m_pRotary
	m_pRotary = new Rotary( this, Rotary::Type::Normal, tr( "Effect return" ), false );
	m_pRotary->setDefaultValue( m_pRotary->getMax() );
	m_pRotary->move( 124, 4 );
	m_pRotary->setIsActive( false );
	connect( m_pRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	m_pReturnLbl = new ClickableLabel( this, QSize( 46, 9 ), HydrogenApp::get_instance()->getCommonStrings()->getReturnLabel(), ClickableLabel::Color::Dark );
	m_pReturnLbl->move( 123, 30 );
}



LadspaFXMixerLine::~LadspaFXMixerLine()
{
//	infoLog( "DESTROY" );
}

void LadspaFXMixerLine::setName(QString name)
{
	m_pNameLCD->setText( name );
}


void LadspaFXMixerLine::bypassBtnClicked() {
	emit bypassBtnClicked( this );
}
void LadspaFXMixerLine::editBtnClicked() {
	emit editBtnClicked( this );
}

bool LadspaFXMixerLine::isFxBypassed()
{
	return ( ( m_pBypassBtn->isChecked() && ! m_pBypassBtn->isDown() ) ||
			 ( ! m_pBypassBtn->isChecked() && m_pBypassBtn->isDown() ) );
}

void LadspaFXMixerLine::setFxBypassed( bool bBypassed )
{
	if ( ! m_pBypassBtn->isDown() ) {
		m_pBypassBtn->setChecked( bBypassed );
	}
	m_pRotary->setIsActive( ! bBypassed );
}

void LadspaFXMixerLine::rotaryChanged( WidgetWithInput *ref)
{
	emit volumeChanged( this );
	UNUSED( ref );
}

void LadspaFXMixerLine::setPeaks( float fPeak_L, float fPeak_R )
{
	UNUSED( fPeak_L );
	UNUSED( fPeak_R );
}

void LadspaFXMixerLine::getPeaks( float *fPeak_L, float *fPeak_R )
{
	UNUSED( fPeak_L );
	UNUSED( fPeak_R );
}

float LadspaFXMixerLine::getVolume()
{
	return m_pRotary->getValue();
}


void LadspaFXMixerLine::setVolume(float value)
{
	m_pRotary->setValue( value );
}
