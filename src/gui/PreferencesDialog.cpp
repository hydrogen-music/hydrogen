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
 * $Id: PreferencesDialog.cpp,v 1.21 2005/05/01 19:50:58 comix Exp $
 *
 */


#include "Skin.h"
#include "PreferencesDialog.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include "qmessagebox.h"
#include "qstylefactory.h"

#include "lib/Hydrogen.h"
#include "lib/Preferences.h"
#include "lib/drivers/MidiDriver.h"

PreferencesDialog::PreferencesDialog(QWidget* parent) : PreferencesDialog_UI(parent, 0, true), Object( "PreferencesDialog" )
{
	setCaption( trUtf8( "Preferences" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	Preferences *pPref = Preferences::getInstance();
	pPref->loadPreferences( false );	// reload user's preferences

	driverComboBox->clear();
	driverComboBox->insertItem( "Auto" );
	driverComboBox->insertItem( "JACK" );
	driverComboBox->insertItem( "ALSA" );
	driverComboBox->insertItem( "OSS" );
	driverComboBox->insertItem( "PortAudio" );

	// Selected audio Driver
	string sAudioDriver = pPref->m_sAudioDriver;
	if (sAudioDriver == "Auto") {
		driverComboBox->setCurrentItem(0);
	}
	else if (sAudioDriver == "Jack") {
		driverComboBox->setCurrentItem(1);
	}
	else if ( sAudioDriver == "Alsa" ) {
		driverComboBox->setCurrentItem(2);
	}
	else if ( sAudioDriver == "Oss" ) {
		driverComboBox->setCurrentItem(3);
	}
	else if ( sAudioDriver == "PortAudio" ) {
		driverComboBox->setCurrentItem(4);
	}
	else {
		errorLog( "Unknown audio driver from preferences [" + sAudioDriver + "]" );
	}


	m_pMidiDriverComboBox->clear();
	m_pMidiDriverComboBox->insertItem( "ALSA" );
	m_pMidiDriverComboBox->insertItem( "PortMidi" );

	if ( pPref->m_sMidiDriver == "ALSA" ) {
		m_pMidiDriverComboBox->setCurrentItem(0);
	}
	else if ( pPref->m_sMidiDriver == "PortMidi" ) {
		m_pMidiDriverComboBox->setCurrentItem(1);
	}

	m_pIgnoreNoteOffCheckBox->setChecked( pPref->m_bMidiNoteOffIgnore );

	updateDriverInfo();


	// metronome volume
	uint metronomeVol = (uint)( pPref->m_fMetronomeVolume * 100.0 );
	metronomeVolumeSpinBox->setValue(metronomeVol);

	// max voices
	maxVoicesTxt->setValue( pPref->m_nMaxNotes );

	// JACK
	trackOutsCheckBox->setChecked( pPref->m_bJackTrackOuts );
	connectDefaultsCheckBox->setChecked( pPref->m_bJackConnectDefaults );
	//~ JACK


	m_pBufferSizeTxt->setValue( pPref->m_nBufferSize );

	switch ( pPref->m_nSampleRate ) {
		case 44100:
			m_pSampleRateComboBox->setCurrentItem( 0 );
			break;
		case 48000:
			m_pSampleRateComboBox->setCurrentItem( 1 );
			break;
		case 88200:
			m_pSampleRateComboBox->setCurrentItem( 2 );
			break;
		case 96000:
			m_pSampleRateComboBox->setCurrentItem( 3 );
			break;
		default:
			errorLog( "Wrong samplerate: " + toString( pPref->m_nSampleRate ) );
	}


	// Appearance tab
	QString applicationFamily = pPref->getApplicationFontFamily().c_str();
	int applicationPointSize = pPref->getApplicationFontPointSize();

	QFont applicationFont( applicationFamily, applicationPointSize );
	applicationFontLbl->setFont( applicationFont );
	applicationFontLbl->setText( applicationFamily + QString("  %1").arg( applicationPointSize ) );

	QString mixerFamily = pPref->getMixerFontFamily().c_str();
	int mixerPointSize = pPref->getMixerFontPointSize();
	QFont mixerFont( mixerFamily, mixerPointSize );
	mixerFontLbl->setFont( mixerFont );
	mixerFontLbl->setText( mixerFamily + QString("  %1").arg( mixerPointSize ) );


	float falloffSpeed = pPref->getMixerFalloffSpeed();
	if (falloffSpeed == FALLOFF_SLOW) {
		mixerFalloffComboBox->setCurrentItem(0);
	}
	else if (falloffSpeed == FALLOFF_NORMAL) {
		mixerFalloffComboBox->setCurrentItem(1);
	}
	else if (falloffSpeed == FALLOFF_FAST) {
		mixerFalloffComboBox->setCurrentItem(2);
	}
	else {
		errorLog( "PreferencesDialog: wrong mixerFalloff value = " + toString(falloffSpeed) );
	}

	switch ( pPref->getInterfaceMode() ) {
		case Preferences::MDI:
			interfaceModeComboBox->setCurrentItem( 0 );
			break;

		case Preferences::TOP_LEVEL:
			interfaceModeComboBox->setCurrentItem( 1 );
			break;

		case Preferences::SINGLE_PANED:
			interfaceModeComboBox->setCurrentItem( 2 );
			break;

		default:
			errorLog( "[INIT] Wrong interfaceMode: " + toString( pPref->getInterfaceMode() ) );
	}

	// Style
	QStringList list = QStyleFactory::keys();
	uint i = 0;
	for ( QStringList::Iterator it = list.begin(); it != list.end(); it++) {
		m_styleComboBox->insertItem( *it );
		//infoLog( "QT Stile: " + *it   );
		//string sStyle = (*it).latin1();
		string sStyle = (*it).latin1();
		if (sStyle == pPref->getQTStyle() ) {
			m_styleComboBox->setCurrentItem( i );
		}
		i++;
	}


	// midi tab
	midiPortChannelComboBox->setEnabled( false );
	midiPortComboBox->setEnabled( false );
	// list midi output ports
	midiPortComboBox->clear();
	midiPortComboBox->insertItem( "None" );
	if ( Hydrogen::getInstance()->getMidiDriver() ) {
		vector<string> midiOutList = Hydrogen::getInstance()->getMidiDriver()->getOutputPortList();
		if ( midiOutList.size() != 0 ) {
			midiPortComboBox->setEnabled( true );
			midiPortChannelComboBox->setEnabled( true );
		}
		for (uint i = 0; i < midiOutList.size(); i++) {
			string sPortName = midiOutList[i];
			midiPortComboBox->insertItem( QString( sPortName.c_str() ) );

			if ( sPortName == pPref->m_sMidiPortName ) {
				midiPortComboBox->setCurrentItem( i + 1 );
			}
		}
	}

	if ( pPref->m_nMidiChannelFilter == -1 ) {
		midiPortChannelComboBox->setCurrentItem( 0 );
	}
	else {
		midiPortChannelComboBox->setCurrentItem( pPref->m_nMidiChannelFilter + 1 );
	}

	// General tab
	restoreLastUsedSongCheckbox->setChecked( pPref->isRestoreLastSongEnabled() );

	m_bNeedDriverRestart = false;
}




PreferencesDialog::~PreferencesDialog() {
//	cout << "PreferencesDIalog Destroy" << endl;
}



/**
 * Cancel Button clicked
 */
void PreferencesDialog::cancelBtnClicked() {
	Preferences *preferencesMng = Preferences::getInstance();
	preferencesMng->loadPreferences( false );	// reload old user's preferences
	reject();
}


/**
 * Ok button clicked
 */
void PreferencesDialog::okBtnClicked() {
	m_bNeedDriverRestart = true;

	Preferences *pPref = Preferences::getInstance();

	// Selected audio driver
	if (driverComboBox->currentText() == "Auto" ) {
		pPref->m_sAudioDriver = "Auto";
	}
	else if (driverComboBox->currentText() == "JACK" ) {
		pPref->m_sAudioDriver = "Jack";
	}
	else if (driverComboBox->currentText() == "ALSA" ) {
		pPref->m_sAudioDriver = "Alsa";
		pPref->m_sAlsaAudioDevice = m_pAudioDeviceTxt->text().ascii();
	}
	else if (driverComboBox->currentText() == "OSS" ) {
		pPref->m_sAudioDriver = "Oss";
		pPref->m_sOSSDevice = m_pAudioDeviceTxt->text().ascii();
	}
	else if (driverComboBox->currentText() == "PortAudio" ) {
		pPref->m_sAudioDriver = "PortAudio";
	}
	else {
		errorLog( "[okBtnClicked] Invalid audio driver" );
	}

	// JACK
	pPref->m_bJackTrackOuts = trackOutsCheckBox->isChecked();
	pPref->m_bJackConnectDefaults = connectDefaultsCheckBox->isChecked();
	//~ JACK

	pPref->m_nBufferSize = m_pBufferSizeTxt->value();
	if ( m_pSampleRateComboBox->currentText() == "44100" ) {
		pPref->m_nSampleRate = 44100;
	}
	else if ( m_pSampleRateComboBox->currentText() == "48000" ) {
		pPref->m_nSampleRate = 48000;
	}
	else if ( m_pSampleRateComboBox->currentText() == "88200" ) {
		pPref->m_nSampleRate = 88200;
	}
	else if ( m_pSampleRateComboBox->currentText() == "96000" ) {
		pPref->m_nSampleRate = 96000;
	}


	// metronome
	pPref->m_fMetronomeVolume = (metronomeVolumeSpinBox->value()) / 100.0;

	// maxVoices
	pPref->m_nMaxNotes = maxVoicesTxt->value();



	if ( m_pMidiDriverComboBox->currentText() == "ALSA" ) {
		pPref->m_sMidiDriver = "ALSA";
	}
	else if ( m_pMidiDriverComboBox->currentText() == "PortMidi" ) {
		pPref->m_sMidiDriver = "PortMidi";
	}

	pPref->m_bMidiNoteOffIgnore = m_pIgnoreNoteOffCheckBox->isChecked();

	// Mixer falloff
	QString falloffStr = mixerFalloffComboBox->currentText().latin1();
	if ( falloffStr== trUtf8("Slow") ) {
		pPref->setMixerFalloffSpeed(FALLOFF_SLOW);
	}
	else if ( falloffStr == trUtf8("Normal") ) {
		pPref->setMixerFalloffSpeed(FALLOFF_NORMAL);
	}
	else if ( falloffStr == trUtf8("Fast") ) {
		pPref->setMixerFalloffSpeed(FALLOFF_FAST);
	}
	else {
		errorLog( "[okBtnClicked()] Unknown mixerFallOffSpeed: " + string(falloffStr.latin1()) );
	}

	// interface mode
	switch( interfaceModeComboBox->currentItem() ) {
		case Preferences::TOP_LEVEL:
			infoLog( "[okBtnClicked] TOP LEVEL interface selected" );
			pPref->setInterfaceMode( Preferences::TOP_LEVEL );
			break;

		case Preferences::MDI:
			infoLog( "[okBtnClicked] MDI interface selected" );
			pPref->setInterfaceMode( Preferences::MDI );
			break;

		case Preferences::SINGLE_PANED:
			infoLog( "[okBtnClicked] SINGLE PANED interface selected" );
			pPref->setInterfaceMode( Preferences::SINGLE_PANED );
			break;

		default:
			errorLog( "[okBtnClicked] Unknown interface mode: " + toString( interfaceModeComboBox->currentItem() ) + ", " + interfaceModeComboBox->currentText().latin1() );
	}



	string sNewMidiPortName = midiPortComboBox->currentText().latin1();

	if ( pPref->m_sMidiPortName != sNewMidiPortName ) {
		pPref->m_sMidiPortName = sNewMidiPortName;
		m_bNeedDriverRestart = true;
	}

	if ( pPref->m_nMidiChannelFilter != midiPortChannelComboBox->currentItem() - 1 ) {
		m_bNeedDriverRestart = true;
	}
	pPref->m_nMidiChannelFilter = midiPortChannelComboBox->currentItem() - 1;


	// General tab
	pPref->setRestoreLastSongEnabled( restoreLastUsedSongCheckbox->isChecked() );

	pPref->savePreferences();

	if (m_bNeedDriverRestart) {
		(Hydrogen::getInstance())->restartDrivers();
	}
	accept();
}



void PreferencesDialog::driverChanged() {
	string selectedDriver = (driverComboBox->currentText()).latin1();
	updateDriverInfo();
	m_bNeedDriverRestart = true;
}



void PreferencesDialog::updateDriverInfo() {
	Preferences *pPref = Preferences::getInstance();
	QString info = "";

	bool bJack_support = false;
	#ifdef JACK_SUPPORT
	bJack_support = true;
	#endif

	bool bAlsa_support = false;
	#ifdef ALSA_SUPPORT
	bAlsa_support = true;
	#endif

	bool bOss_support = false;
	#ifdef OSS_SUPPORT
	bOss_support = true;
	#endif

	bool bPortAudio_support = false;
	#ifdef PORTAUDIO_SUPPORT
	bPortAudio_support = true;
	#endif


	if ( driverComboBox->currentText() == "Auto" ) {
		info += trUtf8("<b>Automatic driver selection</b>");

		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		m_pBufferSizeTxt->setEnabled( false );
		m_pSampleRateComboBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled( false );
	}
	else if ( driverComboBox->currentText() == "OSS" ) {	// OSS
		info += trUtf8("<b>Open Sound System</b><br>Simple audio driver [/dev/dsp]");
		if ( !bOss_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(true);
		m_pAudioDeviceTxt->setText( pPref->m_sOSSDevice.c_str() );
		m_pBufferSizeTxt->setEnabled(true);
		m_pSampleRateComboBox->setEnabled(true);
		trackOutsCheckBox->setEnabled(false);
		connectDefaultsCheckBox->setEnabled(false);
	}
	else if ( driverComboBox->currentText() == "JACK" ) {	// JACK
		info += trUtf8("<b>Jack Audio Connection Kit Driver</b><br>Low latency audio driver");
		if ( !bJack_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		m_pBufferSizeTxt->setEnabled(false);
		m_pSampleRateComboBox->setEnabled(false);
		trackOutsCheckBox->setEnabled(true);
		connectDefaultsCheckBox->setEnabled(true);
	}
	else if ( driverComboBox->currentText() == "ALSA" ) {	// ALSA
		info += trUtf8("<b>ALSA Driver</b><br>");
		if ( !bAlsa_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(true);
		m_pAudioDeviceTxt->setText( pPref->m_sAlsaAudioDevice.c_str() );
		m_pBufferSizeTxt->setEnabled(true);
		m_pSampleRateComboBox->setEnabled(true);
		trackOutsCheckBox->setEnabled(false);
		connectDefaultsCheckBox->setEnabled(false);
	}
	else if ( driverComboBox->currentText() == "PortAudio" ) {
		info += trUtf8( "<b>PortAudio Driver</b><br>" );
		if ( !bPortAudio_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		m_pBufferSizeTxt->setEnabled(true);
		m_pSampleRateComboBox->setEnabled(true);
		trackOutsCheckBox->setEnabled(false);
		connectDefaultsCheckBox->setEnabled(false);
	}
	else {
		string selectedDriver = (driverComboBox->currentText()).latin1();
		errorLog( "Unknown driver = " + selectedDriver );
	}
	m_pBufferSizeTxt->setValue( pPref->m_nBufferSize );

	driverInfoLbl->setText(info);
}



void PreferencesDialog::selectApplicationFont() {
	Preferences *preferencesMng = Preferences::getInstance();

	QString family = (preferencesMng->getApplicationFontFamily()).c_str();
	int pointSize = preferencesMng->getApplicationFontPointSize();


	bool ok;
	QFont font = QFontDialog::getFont( &ok, QFont( family, pointSize ), this );
	if ( ok ) {
		// font is set to the font the user selected
		family = font.family();
		pointSize = font.pointSize();
		string familyStr = family.latin1();
		preferencesMng->setApplicationFontFamily(familyStr);
		preferencesMng->setApplicationFontPointSize(pointSize);


	} else {
		// the user cancelled the dialog; font is set to the initial
		// value, in this case Times, 12.
	}

	QFont newFont(family, pointSize);
	applicationFontLbl->setFont(newFont);
	applicationFontLbl->setText(family + QString("  %1").arg(pointSize));
}




void PreferencesDialog::bufferSizeChanged() {
	m_bNeedDriverRestart = true;
}




void PreferencesDialog::sampleRateChanged() {
	m_bNeedDriverRestart = true;
}



void PreferencesDialog::restartDriverBtnClicked() {

	Preferences *pPref = Preferences::getInstance();

	// Audio buffer size
//	pPref->setBufferSize( m_pOSSBufferSizeTxt->value() );

	// Selected audio driver
//	string selectedDriver = (driverComboBox->currentText()).latin1();
//	pPref->setAudioDriver(selectedDriver);

/*	// sample rate
	uint sampleRate = 44100;
	string sampleRateTxt = (m_pOSSSampleRateComboBox->currentText()).latin1();
	if (sampleRateTxt == "44100") {
		sampleRate = 44100;
	}
	else if (sampleRateTxt == "48000") {
		sampleRate = 48000;
	}
	pPref->setSampleRate(sampleRate);
*/
	Hydrogen::getInstance()->restartDrivers();
	m_bNeedDriverRestart = false;
}



void PreferencesDialog::selectMixerFont() {
	Preferences *preferencesMng = Preferences::getInstance();

	QString family = (preferencesMng->getMixerFontFamily()).c_str();
	int pointSize = preferencesMng->getMixerFontPointSize();

	bool ok;
	QFont font = QFontDialog::getFont( &ok, QFont( family, pointSize ), this );
	if ( ok ) {
		// font is set to the font the user selected
		family = font.family();
		pointSize = font.pointSize();
		string familyStr = family.latin1();
		preferencesMng->setMixerFontFamily(familyStr);
		preferencesMng->setMixerFontPointSize(pointSize);
	}
	QFont newFont(family, pointSize);
	mixerFontLbl->setFont(newFont);
	mixerFontLbl->setText(family + QString("  %1").arg(pointSize));
}





void PreferencesDialog::midiPortChannelChanged()
{
	m_bNeedDriverRestart = true;
}


void PreferencesDialog::guiStyleChanged()
{
//	infoLog( "[guiStyleChanged]" );
	QApplication *pQApp = (HydrogenApp::getInstance())->getMainForm()->m_pQApp;
	QString sStyle = m_styleComboBox->currentText();
	pQApp->setStyle( sStyle );

	Preferences *pPref = Preferences::getInstance();
	pPref->setQTStyle( sStyle.latin1() );
}

