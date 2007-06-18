/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: MixerLine.cpp,v 1.23 2005/07/14 21:34:04 comix Exp $
 *
 */

#include <stdio.h>
#include <qpainter.h>

#include "gui/InstrumentEditor/InstrumentEditor.h"
#include "gui/widgets/Fader.h"
#include "gui/HydrogenApp.h"
#include "gui/Skin.h"
#include "gui/widgets/Rotary.h"
#include "gui/widgets/Button.h"
#include "lib/Hydrogen.h"
#include "lib/Preferences.h"
#include "MixerLine.h"


#define MIXERLINE_WIDTH			56
#define MIXERLINE_HEIGHT		254
#define MASTERMIXERLINE_WIDTH	126
#define MASTERMIXERLINE_HEIGHT	284
#define MIXERLINE_LABEL_H		115
#define MASTERMIXERLINE_FADER_H	75

QPixmap* MixerLine::m_pMixerLineBackground = NULL;
QPixmap* MixerLine::m_pMixerLineBackground_selected = NULL;


MixerLine::MixerLine(QWidget* parent)
 : QWidget(parent)
 , Object( "MixerLine" )
{
	m_nWidth = MIXERLINE_WIDTH;
	m_nHeight = MIXERLINE_HEIGHT;
	m_fMaxPeak = 0.0;
	m_nActivity = 0;
	m_bIsSelected = false;
	m_nPeakTimer = 0;

	setMinimumSize( m_nWidth, m_nHeight );
	setMaximumSize( m_nWidth, m_nHeight );
	resize( m_nWidth, m_nHeight );
	setPaletteBackgroundColor( QColor( 58, 62, 72 ) );


	// MixerLine Background image
	if (m_pMixerLineBackground == NULL ) {
		//infoLog( "loading background pixmap" );
		string mixerLineBackground_path = Skin::getImagePath() + string("/mixerPanel/mixerline_background.png");
		m_pMixerLineBackground = new QPixmap();
		bool ok = m_pMixerLineBackground->load(mixerLineBackground_path.c_str());
		if( ok == false ){
			errorLog( string("Error loading pixmap ") + mixerLineBackground_path );
		}
	}
	if (m_pMixerLineBackground_selected == NULL ) {
		//infoLog( "loading background pixmap" );
		string mixerLineBackground_path = Skin::getImagePath() + string( "/mixerPanel/mixerline_background_on.png");
		m_pMixerLineBackground_selected = new QPixmap();
		bool ok = m_pMixerLineBackground_selected->load(mixerLineBackground_path.c_str());
		if( ok == false ){
			errorLog( string("Error loading pixmap ") + mixerLineBackground_path );
		}
	}
	setPaletteBackgroundPixmap( *m_pMixerLineBackground );


	// Play sample button
	string playSample_on_path = Skin::getImagePath() + string( "/mixerPanel/btn_play_on.png" );
	string playSample_off_path = Skin::getImagePath() + string( "/mixerPanel/btn_play_off.png" );
	string playSample_over_path = Skin::getImagePath() + string( "/mixerPanel/btn_play_over.png" );
	m_pPlaySampleBtn = new Button(this, QSize( 18, 13 ), playSample_on_path, playSample_off_path, playSample_over_path);
	m_pPlaySampleBtn->move( 8, 2 );
	QToolTip::add( m_pPlaySampleBtn, trUtf8( "Play sample" ) );
	connect(m_pPlaySampleBtn, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));
	connect(m_pPlaySampleBtn, SIGNAL(rightClicked(Button*)), this, SLOT(rightClick(Button*)));

	// Trigger sample LED
	string triggerSample_on_path = Skin::getImagePath() + string( "/mixerPanel/led_trigger_on.png" );
	string triggerSample_off_path = Skin::getImagePath() + string( "/mixerPanel/led_trigger_off.png" );
	m_pTriggerSampleLED = new Button(this, QSize( 5, 13 ), triggerSample_on_path, triggerSample_off_path, triggerSample_off_path);
	m_pTriggerSampleLED->move( 26, 2 );
	connect(m_pTriggerSampleLED, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));

	// Mute button
	string mute_on_path = Skin::getImagePath() + string( "/mixerPanel/btn_mute_on.png");
	string mute_off_path = Skin::getImagePath() + string( "/mixerPanel/btn_mute_off.png");
	string mute_over_path = Skin::getImagePath() + string( "/mixerPanel/btn_mute_over.png");
	m_pMuteBtn = new ToggleButton(this, QSize( 18, 13 ), mute_on_path, mute_off_path, mute_over_path);
	m_pMuteBtn->move( 8, 17 );
	QToolTip::add( m_pMuteBtn, trUtf8( "Mute" ) );
	connect(m_pMuteBtn, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));

	// Solo button
	string solo_on_path = Skin::getImagePath() + string( "/mixerPanel/btn_solo_on.png");
	string solo_off_path = Skin::getImagePath() + string( "/mixerPanel/btn_solo_off.png");
	string solo_over_path = Skin::getImagePath() + string( "/mixerPanel/btn_solo_over.png");
	m_pSoloBtn = new ToggleButton(this, QSize( 18, 13 ), solo_on_path, solo_off_path, solo_over_path);
	m_pSoloBtn->move( 30, 17);
	QToolTip::add( m_pSoloBtn, trUtf8( "Solo" ) );
	connect(m_pSoloBtn, SIGNAL(clicked(Button*)), this, SLOT(click(Button*)));

	// pan rotary
	m_pPanRotary = new Rotary( this, Rotary::TYPE_CENTER, trUtf8( "Pan" ), true);
	m_pPanRotary->move( 14, 32 );
	connect( m_pPanRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( panChanged(Rotary*) ) );

	// FX send
	uint y = 0;
	for (uint i = 0; i < MAX_FX; i++) {
		m_pKnob[i] = new Knob(this);
		if ( (i % 2) == 0 ) {
			m_pKnob[i]->move( 9, 63 + (20 * y) );
		}
		else {
			m_pKnob[i]->move( 30, 63 + (20 * y) );
			y++;
		}
		connect( m_pKnob[i], SIGNAL( valueChanged(Knob*) ), this, SLOT( knobChanged(Knob*) ) );
	}

	Preferences *pref = Preferences::getInstance();

	QString family = pref->getMixerFontFamily().c_str();
	int size = pref->getMixerFontPointSize();
	QFont mixerFont( family, size );
	float m_fFalloffTemp = pref->getMixerFalloffSpeed();
	m_fFalloffTemp = (m_fFalloffTemp * 20) - 2;
	m_nFalloff = (int)m_fFalloffTemp;

	string mixerline_text_path = Skin::getImagePath() + string( "/mixerPanel/mixerline_text_background.png");
	QPixmap textBackground;
	bool ok = textBackground.load( mixerline_text_path .c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap" );
	}

	// instrument name widget
	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 6, 128 );
	QToolTip::add( m_pNameWidget, trUtf8( "Instrument name (double click to edit)" ) );
	connect( m_pNameWidget, SIGNAL( doubleClicked () ), this, SLOT( nameClicked() ) );
	connect( m_pNameWidget, SIGNAL( clicked () ), this, SLOT( nameSelected() ) );

	// m_pFader
	m_pFader = new Fader(this);
	m_pFader->move( 23, 128 );
	connect( m_pFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );


	m_pPeakLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4 );
	m_pPeakLCD->move( 10, 106 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setPaletteBackgroundColor( QColor( 49, 53, 61 ) );
}



MixerLine::~MixerLine() {
//	cout << "MixerLine destroy" << endl;
	delete m_pFader;
	delete m_pMixerLineBackground;
	m_pMixerLineBackground = NULL;
}



void MixerLine::updateMixerLine() {
	m_pFader->updateFader();
	m_pPanRotary->updateRotary();

	if ( m_nPeakTimer > m_nFalloff ) {
		if ( m_fMaxPeak > 0.05f ) {
			m_fMaxPeak = m_fMaxPeak - 0.05f;
		}
		else {
		m_fMaxPeak = 0.0f;
		m_nPeakTimer = 0;
		}
		char tmp[20];
		sprintf(tmp, "%#.2f", m_fMaxPeak );
		m_pPeakLCD->setText(tmp);
		if ( m_fMaxPeak > 1.0 ) {
			m_pPeakLCD->setSmallRed();
		}
		else {
			m_pPeakLCD->setSmallBlue();
		}
	}
	m_nPeakTimer++;

	for (uint nFX = 0; nFX < MAX_FX; nFX++) {
		m_pKnob[nFX]->updateKnob();
	}
}



void MixerLine::click(Button *ref) {
	Song *song = (Hydrogen::getInstance())->getSong();

	if (ref == m_pMuteBtn) {
		song->m_bIsModified = true;
		emit muteBtnClicked(this);
	}
	else if (ref == m_pSoloBtn) {
		song->m_bIsModified = true;
		emit soloBtnClicked(this);
	}
	else if (ref == m_pPlaySampleBtn) {
		emit noteOnClicked(this);
	}
}



void MixerLine::rightClick(Button *ref)
{
	if (ref == m_pPlaySampleBtn) {
		emit noteOffClicked(this);
	}

}



void MixerLine::faderChanged(Fader *ref)
{
	Song *song = (Hydrogen::getInstance())->getSong();
	song->m_bIsModified = true;
	emit volumeChanged(this);

	char m_pFaderPos[100];
	float value = ref->getValue() / 100.0f;
	sprintf( m_pFaderPos, "%#.2f",  value);
	( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Set instrument volume [%1]" ).arg( m_pFaderPos ), 2000 );
}



bool MixerLine::isMuteClicked() {
	return m_pMuteBtn->isPressed();
}



void MixerLine::setMuteClicked(bool isClicked) {
	m_pMuteBtn->setPressed(isClicked);
}



bool MixerLine::isSoloClicked() {
	return m_pSoloBtn->isPressed();
}



void MixerLine::setSoloClicked(bool isClicked) {
	m_pSoloBtn->setPressed(isClicked);
}



float MixerLine::getVolume() {
	return (m_pFader->getValue() / 100.0);
}



void MixerLine::setVolume(float value) {
	m_pFader->setValue((int)(value * 100.0));
}



void MixerLine::setPeak_L( float peak ) {
	if (peak != getPeak_L() ) {
		m_pFader->setPeak_L( peak );
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			char tmp[20];
			sprintf(tmp, "%#.2f", peak);
			m_pPeakLCD->setText( tmp );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
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
			char tmp[20];
			sprintf(tmp, "%#.2f", peak);
			m_pPeakLCD->setText( tmp );
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
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





void MixerLine::panChanged(Rotary *ref)
{
	Song *song = ( Hydrogen::getInstance() )->getSong();
	song->m_bIsModified = true;
	emit panChanged( this );

	float panValue = ref->getValue();
	float pan_L = (1.0 - panValue) / 1.0;
	float pan_R = panValue / 1.0;

	char m_pFaderPos[100];
	sprintf( m_pFaderPos, "%#.2fL, %#.2fR",  pan_L, pan_R);
	HydrogenApp::getInstance()->setStatusBarMessage( trUtf8( "Set instrument pan [%1]" ).arg( m_pFaderPos ), 2000 );

	QToolTip::remove( m_pPanRotary );
	QToolTip::add( m_pPanRotary, QString("Pan ") + QString( m_pFaderPos ) );
}





int MixerLine::getPan()
{
	return m_pPanRotary->getValue() * 100.0;
}



void MixerLine::setPan(int value)
{
	float fValue = value / 100.0;

	if ( fValue != m_pPanRotary->getValue() ) {
		m_pPanRotary->setValue( fValue );
		float pan_L = (1.0 - fValue) / 1.0;
		float pan_R = fValue / 1.0;
		char m_pFaderPos[100];
		sprintf( m_pFaderPos, "Pan %#.2fL, %#.2fR",  pan_L, pan_R);
		QToolTip::remove( m_pPanRotary );
		QToolTip::add( m_pPanRotary, QString( m_pFaderPos ) );
	}
}


void MixerLine::setPlayClicked( bool clicked ) {
	m_pTriggerSampleLED->setPressed( clicked );
}


void MixerLine::knobChanged(Knob* pRef)
{
//	infoLog( "knobChanged" );
	for (uint i = 0; i < MAX_FX; i++) {
		if (m_pKnob[i] == pRef) {
			emit knobChanged( this, i );
			break;
		}
	}
}


void MixerLine::setFXLevel( uint nFX, float fValue )
{
	if (nFX > MAX_FX) {
		errorLog( "[setFXLevel] nFX > MAX_FX (nFX=" + toString(nFX) + ")" );
	}
	m_pKnob[nFX]->setValue( fValue );
}

float MixerLine::getFXLevel(uint nFX)
{
	if (nFX > MAX_FX) {
		errorLog( "[setFXLevel] nFX > MAX_FX (nFX=" + toString(nFX) + ")" );
	}
	return m_pKnob[nFX]->getValue();
}


void MixerLine::setSelected( bool bIsSelected )
{
	if (m_bIsSelected == bIsSelected )	return;

	m_bIsSelected = bIsSelected;
	if (m_bIsSelected) {
		setPaletteBackgroundPixmap( *m_pMixerLineBackground_selected );
	}
	else {
		setPaletteBackgroundPixmap( *m_pMixerLineBackground );
	}

}





// ::::::::::::::::::::::::::::




MasterMixerLine::MasterMixerLine(QWidget* parent) : QWidget(parent), Object( "MasterMixerLine" )
{
//	cout << "mixerLine init" << endl;
	m_nWidth = MASTERMIXERLINE_WIDTH;
	m_nHeight = MASTERMIXERLINE_HEIGHT;
	m_nPeakTimer = 0;

	setMinimumSize( m_nWidth, m_nHeight );
	setMaximumSize( m_nWidth, m_nHeight );
	resize( m_nWidth, m_nHeight );
	setPaletteBackgroundColor( QColor( 58, 62, 72 ) );

	// Background image
	string background_path = Skin::getImagePath() + string( "/mixerPanel/masterMixerline_background.png");
	bool ok = m_background.load(background_path.c_str());
	if( ok == false ){
		errorLog( string("Error loading pixmap ") + background_path );
	}
	setPaletteBackgroundPixmap(m_background);

	Preferences *pref = Preferences::getInstance();
	int size = pref->getMixerFontPointSize();
	QString family = pref->getMixerFontFamily().c_str();
	float m_fFalloffTemp = pref->getMixerFalloffSpeed();
	m_fFalloffTemp = (m_fFalloffTemp * 20) - 2;
	m_nFalloff = (int)m_fFalloffTemp;

	m_pMasterFader = new MasterFader( this );
	m_pMasterFader->move( 24, MASTERMIXERLINE_FADER_H );
	connect( m_pMasterFader, SIGNAL( valueChanged(MasterFader*) ), this, SLOT( faderChanged(MasterFader*) ) );

	QFont mixerFont( family, size );

	m_pPeakLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4 );
// 	m_pPeakLCD->move( 18, 48 );
	m_pPeakLCD->move( 23, 53 );
	m_pPeakLCD->setText( "0.00" );
	m_pPeakLCD->setPaletteBackgroundColor( QColor( 49, 53, 61 ) );


	m_pHumanizeVelocityRotary = new Rotary( this, Rotary::TYPE_NORMAL, trUtf8( "Humanize velocity" ) );
	m_pHumanizeVelocityRotary->move( 74, 88 );
	connect( m_pHumanizeVelocityRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pHumanizeTimeRotary = new Rotary( this, Rotary::TYPE_NORMAL, trUtf8( "Humanize time" ) );
	m_pHumanizeTimeRotary->move( 74, 125 );
	connect( m_pHumanizeTimeRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	m_pSwingRotary = new Rotary( this,  Rotary::TYPE_NORMAL, trUtf8( "Swing" ) );
	m_pSwingRotary->move( 74, 162 );
	connect( m_pSwingRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	// Mute btn
	string sMuteBtn_on = Skin::getImagePath() + string( "/mixerPanel/master_mute_on.png");
	string sMuteBtn_off = Skin::getImagePath() + string( "/mixerPanel/master_mute_off.png");
	string sMuteBtn_over = Skin::getImagePath() + string( "/mixerPanel/master_mute_over.png");
	m_pMuteBtn = new ToggleButton( this, QSize( 42, 13 ), sMuteBtn_on, sMuteBtn_off, sMuteBtn_over );
	m_pMuteBtn->move( 20, 32 );
	connect( m_pMuteBtn, SIGNAL( clicked(Button*) ), this, SLOT( muteClicked(Button*) ) );

}




MasterMixerLine::~MasterMixerLine() {
//	cout << "MixerLine destroy" << endl;
	m_fMaxPeak = 0.0;
}



void MasterMixerLine::muteClicked(Button* pBtn)
{
	Hydrogen::getInstance()->getSong()->m_bIsMuted = pBtn->isPressed();
}



void MasterMixerLine::faderChanged(MasterFader *ref)
{

	m_pMasterFader->setValue(ref->getValue());

	emit volumeChanged(this);

	Song *song = (Hydrogen::getInstance())->getSong();
	song->m_bIsModified = true;

	char m_pMasterFaderPos[100];
	float value = ref->getValue() / 100.0;
	sprintf( m_pMasterFaderPos, "%#.2f",  value);
	( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Set master volume [%1]" ).arg( m_pMasterFaderPos ), 2000 );

}




float MasterMixerLine::getVolume() {
	return (m_pMasterFader->getValue() / 100.0);
}



void MasterMixerLine::setVolume(float value) {
	m_pMasterFader->setValue((int)(value * 100.0));
}



void MasterMixerLine::setPeak_L(float peak) {
	if ( peak != getPeak_L() ) {
		m_pMasterFader->setPeak_L(peak);
		if (peak > m_fMaxPeak) {
			if ( peak < 0.1f ) {
				peak = 0.0f;
			}
			char tmp[20];
			sprintf(tmp, "%#.2f", peak);
			m_pPeakLCD->setText(tmp);
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
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
			char tmp[20];
			sprintf(tmp, "%#.2f", peak);
			m_pPeakLCD->setText(tmp);
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}
			m_fMaxPeak = peak;
			m_nPeakTimer = 0;
		}
	}
}



float MasterMixerLine::getPeak_R() {
	return m_pMasterFader->getPeak_R();
}



void MasterMixerLine::updateMixerLine() {
	m_pMasterFader->updateFader();

	if ( m_nPeakTimer > m_nFalloff ) {
		if ( m_fMaxPeak  > 0.05f ) {
			m_fMaxPeak = m_fMaxPeak - 0.05f;
		}
		else {
		m_fMaxPeak = 0.0f;
		m_nPeakTimer = 0;
		}
		char tmp[20];
		sprintf(tmp, "%#.2f", m_fMaxPeak );
		m_pPeakLCD->setText(tmp);
		if ( m_fMaxPeak > 1.0 ) {
			m_pPeakLCD->setSmallRed();
		}
		else {
			m_pPeakLCD->setSmallBlue();
		}
	}
	m_nPeakTimer++;

	Song *pSong = Hydrogen::getInstance()->getSong();
	if ( pSong ) {
		m_pHumanizeTimeRotary->setValue( pSong->getHumanizeTimeValue() );
		m_pHumanizeVelocityRotary->setValue( pSong->getHumanizeVelocityValue() );
		m_pSwingRotary->setValue( pSong->getSwingFactor() );
	}
	else {
		warningLog( "[updateMixerLine] pSong == NULL ");
	}
}


void MasterMixerLine::rotaryChanged( Rotary *pRef )
{
	QString sMsg;
	float fVal = pRef->getValue();
	char sVal[100];
	sprintf( sVal, "%#.2f", fVal);

	Hydrogen *pEngine = Hydrogen::getInstance();
	pEngine->lockEngine("MasterMixerLine::knobChanged");

	if ( pRef == m_pHumanizeTimeRotary ) {
		pEngine->getSong()->setHumanizeTimeValue( fVal );
		sMsg = trUtf8( "Set humanize time parameter [%1]").arg( sVal );
	}
	else if ( pRef == m_pHumanizeVelocityRotary ) {
		pEngine->getSong()->setHumanizeVelocityValue( fVal );
		sMsg = trUtf8( "Set humanize velocity parameter [%1]").arg( sVal );
	}
	else if ( pRef == m_pSwingRotary ) {
		pEngine->getSong()->setSwingFactor( fVal );
		sMsg = trUtf8( "Set swing factor [%1]").arg( sVal );
	}
	else {
		errorLog( "[knobChanged] Unhandled knob" );
	}

	pEngine->unlockEngine();

	( HydrogenApp::getInstance() )->setStatusBarMessage( sMsg, 2000 );
}




/////////////////////////////////////////


FxMixerLine::FxMixerLine(QWidget* parent) : QWidget(parent), Object( "FxMixerLine" )
{
//	cout << "mixerLine init" << endl;

	m_nWidth = MIXERLINE_WIDTH;
	m_nHeight = MIXERLINE_HEIGHT;

	setMinimumSize( m_nWidth, m_nHeight );
	setMaximumSize( m_nWidth, m_nHeight );
	resize( m_nWidth, m_nHeight );
	setPaletteBackgroundColor( QColor( 58, 62, 72 ) );

	m_fMaxPeak = 0.0;

	// MixerLine Background image
	string mixerLineBackground_path = Skin::getImagePath() + string( "/mixerPanel/mixerline_background.png");
	bool ok = m_mixerLineBackground.load(mixerLineBackground_path.c_str());
	if( ok == false ){
		errorLog( string("Error loading pixmap ") + mixerLineBackground_path );
	}
	setPaletteBackgroundPixmap(m_mixerLineBackground);

	// MixerLine LABEL Background image
	QPixmap mixerLineLabelBackground;
	string mixerLineLabelBackground_path = Skin::getImagePath() + string( "/mixerPanel/mixerline_label_background.png");
	ok = mixerLineLabelBackground.load(mixerLineLabelBackground_path.c_str());
	if( ok == false ){
		errorLog( string("Error loading pixmap ") + mixerLineLabelBackground_path );
	}

	string mixerline_text_path = Skin::getImagePath() + string( "/mixerPanel/mixerline_text_background.png");
	QPixmap textBackground;
	ok = textBackground.load( mixerline_text_path.c_str() );
	if( ok == false ){
		errorLog( string("Error loading pixmap ") + mixerline_text_path );
	}


	// active button
	string activeBtn_on_path = Skin::getImagePath() + string( "/mixerPanel/btn_on_on.png" );
	string activeBtn_off_path = Skin::getImagePath() + string( "/mixerPanel/btn_on_off.png" );
	string activeBtn_over_path = Skin::getImagePath() + string( "/mixerPanel/btn_on_over.png" );
	activeBtn = new ToggleButton( this, QSize( 18, 12 ), activeBtn_on_path, activeBtn_off_path, activeBtn_over_path);
	activeBtn->move( 2, 5 );
	QToolTip::add( activeBtn, trUtf8( "FX on/off") );
	connect( activeBtn, SIGNAL( clicked(Button*) ), this, SLOT( click(Button*) ) );

	Preferences *pref = Preferences::getInstance();

	// peak volume label
// 	QFont font1( family, size);
// 	m_pVolumeLbl = new QLabel( this );
// 	m_pVolumeLbl->move( 2, MIXERLINE_LABEL_H );
// 	m_pVolumeLbl->resize( 38, 18 );
// 	m_pVolumeLbl->setText( "0.0" );
// 	m_pVolumeLbl->setAlignment( AlignVCenter | AlignHCenter );
// 	m_pVolumeLbl->setPaletteBackgroundPixmap( textBackground );
// 	m_pVolumeLbl->setFont( font1 );
// //	QToolTip::add( m_pVolumeLbl, "Peak volume" );

	// m_pFader
	m_pFader = new Fader( this );
	m_pFader->move( 22, 106 );
	connect( m_pFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );

 	QString family = pref->getMixerFontFamily().c_str();
 	int size = pref->getMixerFontPointSize();
	QFont mixerFont( family, size );


	m_pNameWidget = new InstrumentNameWidget( this );
	m_pNameWidget->move( 2, 106 );
	m_pNameWidget->setText( trUtf8( "Master output" ) );

	m_pPeakLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4 );
	m_pPeakLCD->move( 2, MIXERLINE_LABEL_H );
	m_pPeakLCD->setText( "0.00" );
}



FxMixerLine::~FxMixerLine() {
//	cout << "MixerLine destroy" << endl;
	delete m_pFader;
}



void FxMixerLine::click(Button *ref) {
	Song *song = (Hydrogen::getInstance())->getSong();

	if (ref == activeBtn ) {
		song->m_bIsModified = true;
		emit activeBtnClicked( this );
	}
}



void FxMixerLine::faderChanged(Fader *ref) {
	m_fMaxPeak = 0.0;
	char tmp[20];
	sprintf( tmp, "%#.2f", m_fMaxPeak );
	m_pPeakLCD->setText( tmp );
	if ( m_fMaxPeak > 1.0 ) {
		m_pPeakLCD->setSmallRed();
	}
	else {
		m_pPeakLCD->setSmallBlue();
	}


	Song *song = ( Hydrogen::getInstance() )->getSong();
	song->m_bIsModified = true;
	emit volumeChanged( this );

}



float FxMixerLine::getVolume() {
	return (m_pFader->getValue() / 100.0);
}



void FxMixerLine::setVolume( float value ) {
	m_pFader->setValue( (int)(value * 100.0) );
}



void FxMixerLine::setPeak_L( float peak ) {
	if (peak != getPeak_L() ) {
		m_pFader->setPeak_L( peak );
		if (peak > m_fMaxPeak) {
			char tmp[20];
			sprintf(tmp, "%#.2f", peak);
			m_pPeakLCD->setText(tmp);
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}

			m_fMaxPeak = peak;
		}
	}
}



float FxMixerLine::getPeak_L() {
	return m_pFader->getPeak_L();
}




void FxMixerLine::setPeak_R(float peak) {
	if (peak != getPeak_R() ) {
		m_pFader->setPeak_R( peak );
		if (peak > m_fMaxPeak) {
			char tmp[20];
			sprintf(tmp, "%#.2f", peak);
			m_pPeakLCD->setText(tmp);
			if ( peak > 1.0 ) {
				m_pPeakLCD->setSmallRed();
			}
			else {
				m_pPeakLCD->setSmallBlue();
			}

			m_fMaxPeak = peak;
		}
	}
}




float FxMixerLine::getPeak_R() {
	return m_pFader->getPeak_R();
}




bool FxMixerLine::isFxActive() {
	return activeBtn->isPressed();
}




void FxMixerLine::setFxActive( bool active ) {
	activeBtn->setPressed( active );
}



void FxMixerLine::updateMixerLine() {
	m_pFader->updateFader();
}






////////////////////////////////

QPixmap* InstrumentNameWidget::m_pBackground = NULL;


InstrumentNameWidget::InstrumentNameWidget(QWidget* parent)
 : QWidget( parent , "InstrumentNameWidget", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "InstrumentNameWidget" )
{
//	infoLog( "INIT" );

	m_bChanged = true;
	m_nWidgetWidth = 17;
	m_nWidgetHeight = 116;

	Preferences *pref = Preferences::getInstance();
	QString family = pref->getMixerFontFamily().c_str();
	int size = pref->getMixerFontPointSize();
	m_mixerFont.setFamily( family );
	m_mixerFont.setPointSize( size );
//	m_mixerFont.setBold( true );
//	m_mixerFont.setItalic( true );

	// MixerLine LABEL Background image
	if (m_pBackground == NULL) {
//		infoLog( "loading background pixmap" );
		string mixerLineLabelBackground_path = Skin::getImagePath() + string( "/mixerPanel/mixerline_label_background.png" );
		m_pBackground = new QPixmap();
		bool ok = m_pBackground->load( mixerLineLabelBackground_path.c_str() );
		if( ok == false ){
			errorLog( string("Error loading pixmap ") + mixerLineLabelBackground_path );
		}
	}

	m_temp.resize( m_nWidgetWidth, m_nWidgetHeight );
	this->resize( m_nWidgetWidth, m_nWidgetHeight );
}




InstrumentNameWidget::~InstrumentNameWidget()
{
//	infoLog( "DESTROY" );
//	delete m_pBackground;
//	m_pBackground = NULL;
}



void InstrumentNameWidget::paintEvent( QPaintEvent* ev ) {
	if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;
		bitBlt(&m_temp, 0, 0, m_pBackground, 0, 0, m_nWidgetWidth, m_nWidgetHeight, CopyROP);
		QPainter p( &m_temp );
		p.setPen( QColor(230, 230, 230) );
		p.setFont( m_mixerFont );
		p.rotate( -90 );
		//p.drawText( -95, 15, m_sInstrName );
		p.drawText( -m_nWidgetHeight + 5, 0, m_nWidgetHeight - 10, m_nWidgetWidth, Qt::AlignVCenter, m_sInstrName );
	}
	bitBlt( this, 0, 0, &m_temp, 0, 0, m_nWidgetWidth, m_nWidgetHeight, CopyROP );
}




void InstrumentNameWidget::setText( QString text )
{
	if (m_sInstrName != text ) {
		m_sInstrName = text;
		m_bChanged = true;
		update();
	}
}



QString InstrumentNameWidget::text()
{
	return m_sInstrName;
}



void InstrumentNameWidget::mousePressEvent( QMouseEvent * e ) {
	emit clicked();
}



void InstrumentNameWidget::mouseDoubleClickEvent( QMouseEvent * e ) {
	emit doubleClicked();
}



// :::::::::::::::::::::




QPixmap* LadspaFXMixerLine::m_pMixerLineBackground = NULL;

LadspaFXMixerLine::LadspaFXMixerLine(QWidget* parent)
 : QWidget( parent )
 , Object( "LadspaFXMixerLine" )
{
	resize( 194, 43 );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );
	setPaletteBackgroundColor( QColor( 49, 53, 61 ) );


	// MixerLine Background image
	if (m_pMixerLineBackground == NULL ) {
		string mixerLineBackground_path = Skin::getImagePath() + string("/mixerPanel/fxline_background.png");
		m_pMixerLineBackground = new QPixmap();
		bool ok = m_pMixerLineBackground->load(mixerLineBackground_path.c_str());
		if( ok == false ){
			errorLog( string("Error loading pixmap ") + mixerLineBackground_path );
		}
	}
	setPaletteBackgroundPixmap( *m_pMixerLineBackground );

	// active button
	string activeBtn_on_path = Skin::getImagePath() + string( "/mixerPanel/bypass_on.png" );
	string activeBtn_off_path = Skin::getImagePath() + string( "/mixerPanel/bypass_off.png" );
	string activeBtn_over_path = Skin::getImagePath() + string( "/mixerPanel/bypass_over.png" );
	m_pActiveBtn = new ToggleButton( this, QSize( 30, 13 ), activeBtn_on_path, activeBtn_off_path, activeBtn_over_path);
	m_pActiveBtn->move( 55, 25 );
	QToolTip::add( m_pActiveBtn, trUtf8( "FX bypass") );
	connect( m_pActiveBtn, SIGNAL( clicked(Button*) ), this, SLOT( click(Button*) ) );

	// edit button
	string editBtn_on_path = Skin::getImagePath() + string( "/mixerPanel/edit_on.png" );
	string editBtn_off_path = Skin::getImagePath() + string( "/mixerPanel/edit_off.png" );
	string editBtn_over_path = Skin::getImagePath() + string( "/mixerPanel/edit_over.png" );
	m_pEditBtn = new Button( this, QSize( 30, 13 ), editBtn_on_path, editBtn_off_path, editBtn_over_path);
	m_pEditBtn->move( 87, 25 );
	QToolTip::add( m_pEditBtn, trUtf8( "Edit FX parameters") );
	connect( m_pEditBtn, SIGNAL( clicked(Button*) ), this, SLOT( click(Button*) ) );

	// instrument name widget
	m_pNameLCD = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 13 );
	m_pNameLCD->move( 11, 9 );
	m_pNameLCD->setText( "No name" );
	QToolTip::add( m_pNameLCD, trUtf8( "Ladspa FX name" ) );

	// m_pRotary
	m_pRotary = new Rotary( this,  Rotary::TYPE_NORMAL, trUtf8( "Effect return" ) );
	m_pRotary->move( 132, 4 );
	connect( m_pRotary, SIGNAL( valueChanged(Rotary*) ), this, SLOT( rotaryChanged(Rotary*) ) );

	// m_pFader
/*
	m_pFader = new Fader(this);
	m_pFader->move( 22, MIXERLINE_FADER_H );
	connect( m_pFader, SIGNAL( valueChanged(Fader*) ), this, SLOT( faderChanged(Fader*) ) );
*/
}



LadspaFXMixerLine::~LadspaFXMixerLine()
{
//	infoLog( "DESTROY" );
	delete m_pMixerLineBackground;
	m_pMixerLineBackground  = NULL;
}




void LadspaFXMixerLine::click(Button *ref) {
	if ( ref == m_pActiveBtn ) {
		emit activeBtnClicked( this );
	}
	else if( ref == m_pEditBtn ) {
		emit editBtnClicked( this );
	}
}



bool LadspaFXMixerLine::isFxActive() {
	return !m_pActiveBtn->isPressed();
}




void LadspaFXMixerLine::setFxActive( bool active ) {
	m_pActiveBtn->setPressed( !active );
}



void LadspaFXMixerLine::rotaryChanged(Rotary *ref) {
	m_fMaxPeak = 0.0;
//	char tmp[20];
//	sprintf(tmp, "%#.1f", fMaxPeak);
//	m_pVolumeLbl->setText(tmp);

	Song *song = (Hydrogen::getInstance())->getSong();
	song->m_bIsModified = true;
	emit volumeChanged(this);
}



void LadspaFXMixerLine::setPeaks( float fPeak_L, float fPeak_R )
{
/*
	m_pPeakmeter->setPeak_L( fPeak_L );
	m_pPeakmeter->setPeak_R( fPeak_R );
	m_pPeakmeter->updateFader();
*/
}



void LadspaFXMixerLine::getPeaks( float *fPeak_L, float *fPeak_R )
{
/*
	(*fPeak_L) = m_pFader->getPeak_L();
	(*fPeak_R) = m_pFader->getPeak_R();
*/
}



float LadspaFXMixerLine::getVolume() {
	return (m_pRotary->getValue() );
}



void LadspaFXMixerLine::setVolume(float value) {
	m_pRotary->setValue( value );
// 	m_pRotary->updateRotary();
}



