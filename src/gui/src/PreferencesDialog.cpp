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

#include <cstring>

#include "Skin.h"
#include "PreferencesDialog.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include "qmessagebox.h"
#include "qstylefactory.h"

#include <QPixmap>
#include <QFontDialog>
#include "Widgets/MidiTable.h"

#include <core/MidiMap.h>
#include <core/Hydrogen.h>
#include <core/Preferences.h>
#include <core/IO/MidiInput.h>
#include <core/Lash/LashClient.h>
#include <core/AudioEngine.h>
#include <core/Helpers/Translations.h>
#include <core/Sampler/Sampler.h>
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"


using namespace H2Core;

const char* PreferencesDialog::__class_name = "PreferencesDialog";

QString PreferencesDialog::m_sColorRed = "#ca0003";

PreferencesDialog::PreferencesDialog(QWidget* parent)
 : QDialog( parent )
 , Object( __class_name )
{
	setupUi( this );

	setWindowTitle( tr( "Preferences" ) );

	setMinimumSize( width(), height() );

	Preferences *pPref = Preferences::get_instance();
	pPref->loadPreferences( false );	// reload user's preferences

	driverComboBox->clear();
	driverComboBox->addItem( "Auto" );
#ifdef H2CORE_HAVE_JACK
	driverComboBox->addItem( "JACK" );
#endif
#ifdef H2CORE_HAVE_ALSA
	driverComboBox->addItem( "ALSA" );
#endif
#ifdef H2CORE_HAVE_OSS
	driverComboBox->addItem( "OSS" );
#endif
#ifdef H2CORE_HAVE_PORTAUDIO
	driverComboBox->addItem( "PortAudio" );
#endif
#ifdef H2CORE_HAVE_COREAUDIO
	driverComboBox->addItem( "CoreAudio" );
#endif
#ifdef H2CORE_HAVE_PULSEAUDIO
	driverComboBox->addItem( "PulseAudio" );
#endif

	// Language selection menu
	for ( QString sLang : Translations::availableTranslations( "hydrogen" ) ) {
		QLocale loc( sLang );
		QString sLabel = loc.nativeLanguageName() + " (" + loc.nativeCountryName() + ')';
		languageComboBox->addItem( sLabel, QVariant( sLang ) );
	}
	// Find preferred language and select that in menu
	QStringList languages;
	QString sPreferredLanguage = pPref->getPreferredLanguage();
	if ( !sPreferredLanguage.isNull() ) {
		languages << sPreferredLanguage;
	}
	languages << QLocale::system().uiLanguages();
	QString sLanguage = Translations::findTranslation( languages, "hydrogen" );
	m_sInitialLanguage = sLanguage;
	int nLanguage = languageComboBox->findData( QVariant( sLanguage ) );
	if ( nLanguage != -1 ) {
		languageComboBox->setCurrentIndex( nLanguage );
	}

	if( driverComboBox->findText(pPref->m_sAudioDriver) > -1){
		driverComboBox->setCurrentIndex(driverComboBox->findText(pPref->m_sAudioDriver));
	}
	else
	{
		driverInfoLbl->setText( tr("Select your Audio Driver" ));
		ERRORLOG( "Unknown MIDI input from preferences [" + pPref->m_sAudioDriver + "]" );
	}


	m_pMidiDriverComboBox->clear();
#ifdef H2CORE_HAVE_ALSA
	m_pMidiDriverComboBox->addItem( "ALSA" );
#endif
#ifdef H2CORE_HAVE_PORTMIDI
	m_pMidiDriverComboBox->addItem( "PortMidi" );
#endif
#ifdef H2CORE_HAVE_COREMIDI
	m_pMidiDriverComboBox->addItem( "CoreMIDI" );
#endif
#ifdef H2CORE_HAVE_JACK
	m_pMidiDriverComboBox->addItem( "JACK-MIDI" );
#endif


	if( m_pMidiDriverComboBox->findText(pPref->m_sMidiDriver) > -1){
		m_pMidiDriverComboBox->setCurrentIndex(m_pMidiDriverComboBox->findText(pPref->m_sMidiDriver));
	}
	else
	{
		driverInfoLbl->setText( tr("Select your MIDI Driver" ) );
		ERRORLOG( "Unknown MIDI input from preferences [" + pPref->m_sMidiDriver + "]" );
	}

	m_pIgnoreNoteOffCheckBox->setChecked( pPref->m_bMidiNoteOffIgnore );
	m_pEnableMidiFeedbackCheckBox->setChecked( pPref->m_bEnableMidiFeedback );
	m_pDiscardMidiMsgCheckbox->setChecked( pPref->m_bMidiDiscardNoteAfterAction );
	m_pFixedMapping->setChecked( pPref->m_bMidiFixedMapping );

	updateDriverInfo();


	// metronome volume
	uint metronomeVol = (uint)( pPref->m_fMetronomeVolume * 100.0 );
	metronomeVolumeSpinBox->setValue(metronomeVol);

	// max voices
	maxVoicesTxt->setValue( pPref->m_nMaxNotes );

	// JACK
	trackOutsCheckBox->setChecked( pPref->m_bJackTrackOuts );
	connect(trackOutsCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleTrackOutsCheckBox( bool )));

	connectDefaultsCheckBox->setChecked( pPref->m_bJackConnectDefaults );
	enableTimebaseCheckBox->setChecked( pPref->m_bJackTimebaseEnabled );

	switch ( pPref->m_JackTrackOutputMode ) {
	case Preferences::JackTrackOutputMode::postFader:
 		trackOutputComboBox->setCurrentIndex( 0 );
		break;
	case Preferences::JackTrackOutputMode::preFader:
		trackOutputComboBox->setCurrentIndex( 1 );
		break;
	default:
		ERRORLOG( QString( "Unknown JACK track output mode [%1]" )
				  .arg( static_cast<int>( pPref->m_JackTrackOutputMode ) ) );
	}

	switch ( pPref->m_JackBBTSync ) {
	case Preferences::JackBBTSyncMethod::constMeasure:
		jackBBTSyncComboBox->setCurrentIndex( 0 );
		break;
	case Preferences::JackBBTSyncMethod::identicalBars:
		jackBBTSyncComboBox->setCurrentIndex( 1 );
		break;
	default:
		ERRORLOG( QString( "Unknown JACK BBT synchronization method [%1]" )
				  .arg( static_cast<int>(pPref->m_JackBBTSync) ) );
	}
	//~ JACK


	bufferSizeSpinBox->setValue( pPref->m_nBufferSize );
	switch ( pPref->m_nSampleRate ) {
	case 44100:
		sampleRateComboBox->setCurrentIndex( 0 );
		break;
	case 48000:
		sampleRateComboBox->setCurrentIndex( 1 );
		break;
	case 88200:
		sampleRateComboBox->setCurrentIndex( 2 );
		break;
	case 96000:
		sampleRateComboBox->setCurrentIndex( 3 );
		break;
	default:
		ERRORLOG( QString("Wrong samplerate: %1").arg( pPref->m_nSampleRate ) );
	}

	resampleComboBox->setCurrentIndex( (int) Hydrogen::get_instance()->getAudioEngine()->getSampler()->getInterpolateMode() );

	// Appearance tab
	m_sPreviousApplicationFontFamily = pPref->getApplicationFontFamily();
	m_nPreviousApplicationFontPointSize = pPref->getApplicationFontPointSize();

	QFont applicationFont( m_sPreviousApplicationFontFamily, m_nPreviousApplicationFontPointSize );
	applicationFontLbl->setFont( applicationFont );
	applicationFontLbl->setText( m_sPreviousApplicationFontFamily + QString("  %1").arg( m_nPreviousApplicationFontPointSize ) );

	QString mixerFamily = pPref->getMixerFontFamily();
	int mixerPointSize = pPref->getMixerFontPointSize();
	QFont mixerFont( mixerFamily, mixerPointSize );
	mixerFontLbl->setFont( mixerFont );
	mixerFontLbl->setText( mixerFamily + QString("  %1").arg( mixerPointSize ) );


	float falloffSpeed = pPref->getMixerFalloffSpeed();
	if (falloffSpeed == FALLOFF_SLOW) {
		mixerFalloffComboBox->setCurrentIndex(0);
	}
	else if (falloffSpeed == FALLOFF_NORMAL) {
		mixerFalloffComboBox->setCurrentIndex(1);
	}
	else if (falloffSpeed == FALLOFF_FAST) {
		mixerFalloffComboBox->setCurrentIndex(2);
	}
	else {
		ERRORLOG( QString("PreferencesDialog: wrong mixerFalloff value = %1").arg(falloffSpeed) );
	}

	uiLayoutComboBox->setCurrentIndex(  pPref->getDefaultUILayout() );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	uiScalingPolicyComboBox->setCurrentIndex( pPref->getUIScalingPolicy() );
#else
	uiScalingPolicyComboBox->setEnabled( false );
        uiScalingPolicyLabel->setEnabled( false );
#endif

	// Style
	QStringList list = QStyleFactory::keys();
	uint i = 0;
	for ( QStringList::Iterator it = list.begin(); it != list.end(); it++) {
		styleComboBox->addItem( *it );
		//INFOLOG( "QT Stile: " + *it   );
		//string sStyle = (*it).latin1();
		QString sStyle = (*it);
		if (sStyle == pPref->getQTStyle() ) {
			styleComboBox->setCurrentIndex( i );
		}
		i++;
	}

	//SongEditor coloring
	int coloringMethod = pPref->getColoringMethod();
	int coloringMethodAuxValue = pPref->getColoringMethodAuxValue();

	coloringMethodCombo->clear();
	coloringMethodCombo->addItem(tr("Automatic"));
	coloringMethodCombo->addItem(tr("Steps"));
	coloringMethodCombo->addItem(tr("Fixed"));

	coloringMethodAuxSpinBox->setMaximum(300);

	coloringMethodCombo->setCurrentIndex( coloringMethod );
	coloringMethodAuxSpinBox->setValue( coloringMethodAuxValue );

	coloringMethodCombo_currentIndexChanged( coloringMethod );

	connect(coloringMethodCombo, SIGNAL(currentIndexChanged(int)), this, SLOT( coloringMethodCombo_currentIndexChanged(int) ));


	// midi tab
	midiPortChannelComboBox->setEnabled( false );
	midiPortComboBox->setEnabled( false );
	
	// list midi input ports
	midiPortComboBox->clear();
	midiPortComboBox->addItem( tr( "None" ) );
	if ( Hydrogen::get_instance()->getMidiInput() ) {
		std::vector<QString> midiOutList = Hydrogen::get_instance()->getMidiInput()->getOutputPortList();

		if ( midiOutList.size() != 0 ) {
			midiPortComboBox->setEnabled( true );
			midiPortChannelComboBox->setEnabled( true );
		}
		for (uint i = 0; i < midiOutList.size(); i++) {
			QString sPortName = midiOutList[i];
			midiPortComboBox->addItem( sPortName );

			if ( sPortName == pPref->m_sMidiPortName ) {
				midiPortComboBox->setCurrentIndex( i + 1 );
			}
		}
	}
	
	// list midi output ports
	midiOutportComboBox->clear();
	midiOutportComboBox->addItem( tr( "None" ) );
	if ( Hydrogen::get_instance()->getMidiOutput() ) {
		std::vector<QString> midiOutList = Hydrogen::get_instance()->getMidiOutput()->getInputPortList();

		if ( midiOutList.size() != 0 ) {
			midiOutportComboBox->setEnabled( true );
			midiPortChannelComboBox->setEnabled( true );
		}
		for (uint i = 0; i < midiOutList.size(); i++) {
			QString sPortName = midiOutList[i];
			midiOutportComboBox->addItem( sPortName );

			if ( sPortName == pPref->m_sMidiOutputPortName ) {
				midiOutportComboBox->setCurrentIndex( i + 1 );
			}
		}
	}

	if ( pPref->m_nMidiChannelFilter == -1 ) {
		midiPortChannelComboBox->setCurrentIndex( 0 );
	}
	else {
		midiPortChannelComboBox->setCurrentIndex( pPref->m_nMidiChannelFilter + 1 );
	}

	//OSC tab
	enableOscCheckbox->setChecked( pPref->getOscServerEnabled() );
	enableOscFeedbackCheckbox->setChecked( pPref->getOscFeedbackEnabled() );
	connect(enableOscCheckbox, SIGNAL(toggled(bool)), this, SLOT(toggleOscCheckBox( bool )));
	incomingOscPortSpinBox->setValue( pPref->getOscServerPort() );

	if ( pPref->m_nOscTemporaryPort != -1 ) {
		oscTemporaryPortLabel->show();
		oscTemporaryPortLabel->setText( QString( "<b><i><font color=" )
										.append( m_sColorRed )
										.append( ">" )
										.append( tr( "The select port is unavailable. This instance uses the following temporary port instead:" ) )
										.append( "</font></i></b>" ) );
		oscTemporaryPort->show();
		oscTemporaryPort->setEnabled( false );
		oscTemporaryPort->setText( QString::number( pPref->m_nOscTemporaryPort ) );
	} else {
		oscTemporaryPortLabel->hide();
		oscTemporaryPort->hide();
	}
	
	if ( ! pPref->getOscServerEnabled() ) {
		enableOscFeedbackCheckbox->hide();
		incomingOscPortSpinBox->hide();
		incomingOscPortLabel->hide();
		oscTemporaryPortLabel->hide();
		oscTemporaryPort->hide();
	}

	// General tab
	restoreLastUsedSongCheckbox->setChecked( pPref->isRestoreLastSongEnabled() );
	restoreLastUsedPlaylistCheckbox->setChecked( pPref->isRestoreLastPlaylistEnabled() );
	useRelativePlaylistPathsCheckbox->setChecked( pPref->isPlaylistUsingRelativeFilenames() );
	hideKeyboardCursor->setChecked( pPref->hideKeyboardCursor() );

	//restore the right m_bsetlash value
	if ( pPref->m_brestartLash == true ){
		if (pPref->m_bsetLash == false ){
			pPref->m_bsetLash = true ;
			pPref->m_brestartLash = false;
		}

	}
	useLashCheckbox->setChecked( pPref->m_bsetLash );

	sBcountOffset->setValue( pPref->m_countOffset );
	sBstartOffset->setValue( pPref->m_startOffset );

	sBmaxBars->setValue( pPref->getMaxBars() );
	sBmaxLayers->setValue( pPref->getMaxLayers() );

	QString pathtoRubberband = pPref->m_rubberBandCLIexecutable;


	rubberbandLineEdit->setText( pathtoRubberband );

#ifdef H2CORE_HAVE_RUBBERBAND
	pathToRubberbandExLable->hide();
	rubberbandLineEdit->hide();
#endif

	m_bNeedDriverRestart = false;
	connect(m_pMidiDriverComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT( onMidiDriverComboBoxIndexChanged(int) ));
}




PreferencesDialog::~PreferencesDialog()
{
	INFOLOG("~PREFERENCES_DIALOG");
}



void PreferencesDialog::on_cancelBtn_clicked()
{
	Preferences *preferencesMng = Preferences::get_instance();
	preferencesMng->loadPreferences( false );	// reload old user's preferences

	//restore the right m_bsetlash value
	if ( preferencesMng->m_brestartLash == true ){
		if (preferencesMng->m_bsetLash == false ){
			preferencesMng->m_bsetLash = true ;
			preferencesMng->m_brestartLash = false;
		}

	}

	reject();
}

void PreferencesDialog::updateDriverPreferences() {
	Preferences *pPref = Preferences::get_instance();

	// Selected audio driver
	if (driverComboBox->currentText() == "Auto" ) {
		pPref->m_sAudioDriver = "Auto";
	}
	else if (driverComboBox->currentText() == "JACK" ) {
		pPref->m_sAudioDriver = "JACK";
	}
	else if (driverComboBox->currentText() == "ALSA" ) {
		pPref->m_sAudioDriver = "ALSA";
		pPref->m_sAlsaAudioDevice = m_pAudioDeviceTxt->text();
	}
	else if (driverComboBox->currentText() == "OSS" ) {
		pPref->m_sAudioDriver = "OSS";
		pPref->m_sOSSDevice = m_pAudioDeviceTxt->text();
	}
	else if (driverComboBox->currentText() == "PortAudio" ) {
		pPref->m_sAudioDriver = "PortAudio";
	}
	else if (driverComboBox->currentText() == "CoreAudio" ) {
		pPref->m_sAudioDriver = "CoreAudio";
	}
	else if (driverComboBox->currentText() == "PulseAudio" ) {
		pPref->m_sAudioDriver = "PulseAudio";
	}
	else {
		ERRORLOG( "[okBtnClicked] Invalid audio driver:" + driverComboBox->currentText() );
	}

	// JACK
	pPref->m_bJackConnectDefaults = connectDefaultsCheckBox->isChecked();
	pPref->m_bJackTimebaseEnabled = enableTimebaseCheckBox->isChecked();

	switch ( trackOutputComboBox->currentIndex() ) {
	case 0: 
		pPref->m_JackTrackOutputMode = Preferences::JackTrackOutputMode::postFader;
		break;
	case 1:
		pPref->m_JackTrackOutputMode = Preferences::JackTrackOutputMode::preFader;
		break;
	default:
		ERRORLOG( QString( "Unexpected track output value" ) );
	}

	switch ( jackBBTSyncComboBox->currentIndex() ) {
	case 0:
		pPref->m_JackBBTSync = Preferences::JackBBTSyncMethod::constMeasure;
		break;
	case 1:
		pPref->m_JackBBTSync = Preferences::JackBBTSyncMethod::identicalBars;
		break;
	default:
		ERRORLOG( QString( "Unexpected JACK BBT synchronization value" ) );
	}
	//~ JACK

	pPref->m_nBufferSize = bufferSizeSpinBox->value();
	if ( sampleRateComboBox->currentText() == "44100" ) {
		pPref->m_nSampleRate = 44100;
	}
	else if ( sampleRateComboBox->currentText() == "48000" ) {
		pPref->m_nSampleRate = 48000;
	}
	else if ( sampleRateComboBox->currentText() == "88200" ) {
		pPref->m_nSampleRate = 88200;
	}
	else if ( sampleRateComboBox->currentText() == "96000" ) {
		pPref->m_nSampleRate = 96000;
	}
}


void PreferencesDialog::on_okBtn_clicked()
{
	//	m_bNeedDriverRestart = true;

	Preferences *pPref = Preferences::get_instance();

	MidiMap *mM = MidiMap::get_instance();
	mM->reset_instance();

	midiTable->saveMidiTable();

	updateDriverPreferences();


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
	else if ( m_pMidiDriverComboBox->currentText() == "CoreMIDI" ) {
		pPref->m_sMidiDriver = "CoreMIDI";
	}
	else if ( m_pMidiDriverComboBox->currentText() == "JACK-MIDI" ) {
		pPref->m_sMidiDriver = "JACK-MIDI";
	}



	pPref->m_bMidiNoteOffIgnore = m_pIgnoreNoteOffCheckBox->isChecked();
	pPref->m_bMidiFixedMapping = m_pFixedMapping->isChecked();
	pPref->m_bMidiDiscardNoteAfterAction = m_pDiscardMidiMsgCheckbox->isChecked();
	pPref->m_bEnableMidiFeedback = m_pEnableMidiFeedbackCheckBox->isChecked();
			
	// Mixer falloff
	switch ( mixerFalloffComboBox->currentIndex() ) {
	case 0:
		pPref->setMixerFalloffSpeed(FALLOFF_SLOW);
		break;
	case 1:
		pPref->setMixerFalloffSpeed(FALLOFF_NORMAL);
		break;
	case 2:
		pPref->setMixerFalloffSpeed(FALLOFF_FAST);
		break;
	default:
		ERRORLOG( "[okBtnClicked] Unknown mixerFallOffSpeed: " + mixerFalloffComboBox->currentText() );
	}

	QString sNewMidiPortName = midiPortComboBox->currentText();
	if ( midiPortComboBox->currentIndex() == 0 ) {
		sNewMidiPortName = "None";
	}
	if ( pPref->m_sMidiPortName != sNewMidiPortName ) {
		pPref->m_sMidiPortName = sNewMidiPortName;
		m_bNeedDriverRestart = true;
	}
	
	QString sNewMidiOutputPortName = midiOutportComboBox->currentText();
	if ( midiOutportComboBox->currentIndex() == 0 ) {
		sNewMidiOutputPortName = "None";
	}
	if ( pPref->m_sMidiOutputPortName != sNewMidiOutputPortName ) {
		pPref->m_sMidiOutputPortName = sNewMidiOutputPortName;
		m_bNeedDriverRestart = true;
	}

	if ( pPref->m_nMidiChannelFilter != midiPortChannelComboBox->currentIndex() - 1 ) {
		//m_bNeedDriverRestart = true;
	}
	pPref->m_nMidiChannelFilter = midiPortChannelComboBox->currentIndex() - 1;

	//OSC tab
	if ( enableOscCheckbox->isChecked() != pPref->getOscServerEnabled() ) {
		pPref->setOscServerEnabled( enableOscCheckbox->isChecked() );
		H2Core::Hydrogen::get_instance()->toggleOscServer( enableOscCheckbox->isChecked() );
	}
	
	pPref->setOscFeedbackEnabled( enableOscFeedbackCheckbox->isChecked() );
	
	if ( incomingOscPortSpinBox->value() != pPref->getOscServerPort() ) {
		pPref->setOscServerPort( incomingOscPortSpinBox->value() );
		H2Core::Hydrogen::get_instance()->recreateOscServer();
	}
	
	// General tab
	pPref->setRestoreLastSongEnabled( restoreLastUsedSongCheckbox->isChecked() );
	pPref->setRestoreLastPlaylistEnabled( restoreLastUsedPlaylistCheckbox->isChecked() );
	pPref->setUseRelativeFilenamesForPlaylists( useRelativePlaylistPathsCheckbox->isChecked() );
	pPref->m_bsetLash = useLashCheckbox->isChecked(); //restore m_bsetLash after saving pref.
	pPref->setHideKeyboardCursor( hideKeyboardCursor->isChecked() );

	//path to rubberband
	pPref-> m_rubberBandCLIexecutable = rubberbandLineEdit->text();

	//check preferences
	if ( pPref->m_brestartLash == true ){
		pPref->m_bsetLash = true ;
	}

	pPref->m_countOffset = sBcountOffset->value();
	pPref->m_startOffset = sBstartOffset->value();

	pPref->setMaxBars( sBmaxBars->value() );
	pPref->setMaxLayers( sBmaxLayers->value() );

	Hydrogen::get_instance()->setBcOffsetAdjust();

	pPref->setDefaultUILayout( uiLayoutComboBox->currentIndex() );
#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	pPref->setUIScalingPolicy( uiScalingPolicyComboBox->currentIndex() );
#endif


	int coloringMethod = coloringMethodCombo->currentIndex();

	pPref->setColoringMethod( coloringMethod );

	switch( coloringMethod )
	{
		case 0:
			//Automatic
			pPref->setColoringMethodAuxValue(0);
			break;
		case 1:
			pPref->setColoringMethodAuxValue( coloringMethodAuxSpinBox->value() );
			break;
		case 2:
			pPref->setColoringMethodAuxValue( coloringMethodAuxSpinBox->value() );
			break;
	}

	HydrogenApp *pH2App = HydrogenApp::get_instance();
	SongEditorPanel* pSongEditorPanel = pH2App->getSongEditorPanel();
	SongEditor * pSongEditor = pSongEditorPanel->getSongEditor();
	pSongEditor->updateEditorandSetTrue();

	QString sPreferredLanguage = languageComboBox->currentData().toString();
	if ( sPreferredLanguage != m_sInitialLanguage ) {
		QMessageBox::information( this, "Hydrogen", tr( "Hydrogen must be restarted for language change to take effect" ));
		pPref->setPreferredLanguage( sPreferredLanguage );
	}

	pPref->savePreferences();


	if (m_bNeedDriverRestart) {
		int res = QMessageBox::information( this, "Hydrogen", tr( "Driver restart required.\n Restart driver?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
		if ( res == 0 ) {
			Hydrogen::get_instance()->restartDrivers();
		}
	}
	accept();
}


void PreferencesDialog::on_driverComboBox_activated( int index )
{
	UNUSED( index );
	QString selectedDriver = driverComboBox->currentText();
	updateDriverInfo();
	m_bNeedDriverRestart = true;
}


void PreferencesDialog::updateDriverInfo()
{
	Preferences *pPref = Preferences::get_instance();
	QString info;

	bool bJack_support = false;
#ifdef H2CORE_HAVE_JACK
	bJack_support = true;
#endif

	bool bAlsa_support = false;
#ifdef H2CORE_HAVE_ALSA
	bAlsa_support = true;
#endif

	bool bOss_support = false;
#ifdef H2CORE_HAVE_OSS
	bOss_support = true;
#endif

	bool bPortAudio_support = false;
#ifdef H2CORE_HAVE_PORTAUDIO
	bPortAudio_support = true;
#endif

	bool bCoreAudio_support = false;
#ifdef H2CORE_HAVE_COREAUDIO
	bCoreAudio_support = true;
#endif

	bool bPulseAudio_support = false;
#ifdef H2CORE_HAVE_PULSEAUDIO
	bPulseAudio_support = true;
#endif

	if ( driverComboBox->currentText() == "Auto" ) {
		info += tr("Automatic driver selection");
		
		// Display the selected driver as well.
		if ( H2Core::Hydrogen::get_instance()->getAudioOutput() != nullptr ) {
			info.append( "<br><b>" )
				.append( H2Core::Hydrogen::get_instance()->getAudioOutput()->class_name() )
				.append( "</b> " ).append( tr( "selected") );
		}
		m_pAudioDeviceTxt->setEnabled( true );
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled( true );
		sampleRateComboBox->setEnabled( true );
		trackOutputComboBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled( false );
		enableTimebaseCheckBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		jackBBTSyncComboBox->setEnabled( false );
		jackBBTSyncLbl->setEnabled( false );

		if ( std::strcmp( H2Core::Hydrogen::get_instance()->getAudioOutput()->class_name(),
						  "JackAudioDriver" ) == 0 ) {
			trackOutputComboBox->setEnabled( true );
			connectDefaultsCheckBox->setEnabled( true );
			enableTimebaseCheckBox->setEnabled( true );
			trackOutsCheckBox->setEnabled( true );
			jackBBTSyncComboBox->setEnabled( true );
			jackBBTSyncLbl->setEnabled( true );
			trackOutputComboBox->show();
			trackOutputLbl->show();
			connectDefaultsCheckBox->show();
			trackOutsCheckBox->show();
			enableTimebaseCheckBox->show();
			jackBBTSyncComboBox->show();
			jackBBTSyncLbl->show();
		} else {
			trackOutputComboBox->setEnabled( false );
			connectDefaultsCheckBox->setEnabled( false );
			enableTimebaseCheckBox->setEnabled( false );
			trackOutsCheckBox->setEnabled( false );
			jackBBTSyncComboBox->setEnabled( false );
			jackBBTSyncLbl->setEnabled( false );
			trackOutputComboBox->hide();
			trackOutputLbl->hide();
			connectDefaultsCheckBox->hide();
			enableTimebaseCheckBox->hide();
			trackOutsCheckBox->hide();
			jackBBTSyncComboBox->hide();
			jackBBTSyncLbl->hide();
		}
	}
	else if ( driverComboBox->currentText() == "OSS" ) {	// OSS
		info.append( "<b>" ).append( tr( "Open Sound System" ) )
			.append( "</b><br>" )
			.append( tr( "Simple audio driver [/dev/dsp]" ) );
		if ( !bOss_support ) {
			info.append( "<br><b><font color=\"red\">" )
				.append( tr( "Not compiled" ) )
				.append( "</font></b>" );
		}
		m_pAudioDeviceTxt->setEnabled(true);
		m_pAudioDeviceTxt->setText( pPref->m_sOSSDevice );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
	}
	else if ( driverComboBox->currentText() == "JACK" ) {	// JACK
		info.append( "<b>" )
			.append( tr( "JACK Audio Connection Kit Driver" ) )
			.append( "</b><br>" )
			.append( tr( "Low latency audio driver" ) );
		if ( !bJack_support ) {
			info += QString("<br><b><font color=")
				.append( m_sColorRed ).append( ">")
				.append( tr( "Not compiled" ) )
				.append( "</font></b>" );
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled(false);
		sampleRateComboBox->setEnabled(false);
		trackOutputComboBox->setEnabled( true );
		connectDefaultsCheckBox->setEnabled( true );
		enableTimebaseCheckBox->setEnabled( true );
		trackOutsCheckBox->setEnabled( true );
		trackOutputComboBox->show();
		trackOutputLbl->show();
		connectDefaultsCheckBox->show();
		enableTimebaseCheckBox->show();
		trackOutsCheckBox->show();
		jackBBTSyncComboBox->show();
		jackBBTSyncLbl->show();
	}
	else if ( driverComboBox->currentText() == "ALSA" ) {	// ALSA
		info.append( "<b>" ).append( tr( "ALSA Driver" ) )
			.append( "</b><br>" );
		if ( !bAlsa_support ) {
			info += QString("<br><b><font color=")
				.append( m_sColorRed ).append( ">")
				.append( tr( "Not compiled" ) )
				.append( "</font></b>" );
		}
		m_pAudioDeviceTxt->setEnabled(true);
		m_pAudioDeviceTxt->setText( pPref->m_sAlsaAudioDevice );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
	}
	else if ( driverComboBox->currentText() == "PortAudio" ) {
		info.append( "<b>" ).append( tr( "PortAudio Driver" ) )
			.append( "</b><br>" );
		if ( !bPortAudio_support ) {
			info += QString("<br><b><font color=")
				.append( m_sColorRed ).append( ">")
				.append( tr( "Not compiled" ) )
				.append( "</font></b>" );
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
	}
	else if ( driverComboBox->currentText() == "CoreAudio" ) {
		info.append( "<b>" ).append( tr( "CoreAudio Driver" ) )
			.append( "</b><br>" );
		if ( !bCoreAudio_support ) {
			info += QString("<br><b><font color=")
				.append( m_sColorRed ).append( ">")
				.append( tr( "Not compiled" ) )
				.append( "</font></b>" );
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
	}
	else if ( driverComboBox->currentText() == "PulseAudio" ) {
		info.append( "<b>" ).append( tr( "PulseAudio Driver" ) )
			.append( "</b><br>" );
		if ( !bPulseAudio_support ) {
			info += QString("<br><b><font color=")
				.append( m_sColorRed ).append( ">")
				.append( tr( "Not compiled" ) )
				.append( "</font></b>" );
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText("");
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
	}
	else {
		QString selectedDriver = driverComboBox->currentText();
		ERRORLOG( "Unknown driver = " + selectedDriver );
	}

	metronomeVolumeSpinBox->setEnabled(true);
	bufferSizeSpinBox->setValue( pPref->m_nBufferSize );

	driverInfoLbl->setText(info);
}

void PreferencesDialog::onCurrentApplicationFontChanged( const QFont& font ) {

	DEBUGLOG("");
	
	auto pPref = Preferences::get_instance();
	
	pPref->setApplicationFontFamily( font.family() );
	pPref->setApplicationFontPointSize( font.pointSize() );

	HydrogenApp::get_instance()->changePreferences( true );
}

void PreferencesDialog::onApplicationFontSelected( const QFont& font ) {

	DEBUGLOG("");
	
	onCurrentApplicationFontChanged( font );

	applicationFontLbl->setFont( font );
	applicationFontLbl->setText( font.family() + QString("  %1").arg( font.pointSize() ) );
}

void PreferencesDialog::onApplicationFontRejected() {

	DEBUGLOG("");

	auto pPref = Preferences::get_instance();
	
	pPref->setApplicationFontFamily( m_sPreviousApplicationFontFamily );
	pPref->setApplicationFontPointSize( m_nPreviousApplicationFontPointSize );

	HydrogenApp::get_instance()->changePreferences( true );
}

void PreferencesDialog::on_selectApplicationFontBtn_clicked()
{
	auto pPref = Preferences::get_instance();

	m_sPreviousApplicationFontFamily = pPref->getApplicationFontFamily();
	m_nPreviousApplicationFontPointSize = pPref->getApplicationFontPointSize();

	QFontDialog* pFontDialog = new QFontDialog( QFont( m_sPreviousApplicationFontFamily,
													   m_nPreviousApplicationFontPointSize ) );

	connect( pFontDialog, &QFontDialog::currentFontChanged,
			 this, &PreferencesDialog::onCurrentApplicationFontChanged );
	connect( pFontDialog, &QFontDialog::fontSelected,
			 this, &PreferencesDialog::onApplicationFontSelected );
	connect( pFontDialog, &QFontDialog::rejected,
			 this, &PreferencesDialog::onApplicationFontRejected );

	pFontDialog->setModal( true );
	pFontDialog->open();
}




void PreferencesDialog::on_bufferSizeSpinBox_valueChanged( int i )
{
	UNUSED( i );
	m_bNeedDriverRestart = true;
}




void PreferencesDialog::on_sampleRateComboBox_editTextChanged( const QString&  )
{
	m_bNeedDriverRestart = true;
}



void PreferencesDialog::on_restartDriverBtn_clicked()
{
	updateDriverPreferences();
	Preferences *pPref = Preferences::get_instance();
	pPref->savePreferences();
	Hydrogen::get_instance()->restartDrivers();
	m_bNeedDriverRestart = false;
}



void PreferencesDialog::on_selectMixerFontBtn_clicked()
{
	Preferences *preferencesMng = Preferences::get_instance();

	QString family = preferencesMng->getMixerFontFamily();
	int pointSize = preferencesMng->getMixerFontPointSize();

	bool ok;
	QFont font = QFontDialog::getFont( &ok, QFont( family, pointSize ), this );
	if ( ok ) {
		// font is set to the font the user selected
		family = font.family();
		pointSize = font.pointSize();
		QString familyStr = family;
		preferencesMng->setMixerFontFamily(familyStr);
		preferencesMng->setMixerFontPointSize(pointSize);
	}
	QFont newFont(family, pointSize);
	mixerFontLbl->setFont(newFont);
	mixerFontLbl->setText(family + QString("  %1").arg(pointSize));
}

void PreferencesDialog::on_midiPortComboBox_activated( int index )
{
	UNUSED( index );
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::on_midiOutportComboBox_activated( int index )
{
	UNUSED( index );
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::on_styleComboBox_activated( int index )
{
	UNUSED( index );
	QApplication *pQApp = (HydrogenApp::get_instance())->getMainForm()->m_pQApp;
	QString sStyle = styleComboBox->currentText();
	pQApp->setStyle( sStyle );

	Preferences *pPref = Preferences::get_instance();
	pPref->setQTStyle( sStyle );
}



void PreferencesDialog::on_useLashCheckbox_clicked()
{
	if ( useLashCheckbox->isChecked() ){
		Preferences::get_instance()->m_brestartLash = true;
	}
	else
	{
		Preferences::get_instance()->m_bsetLash = false ;
	}
	QMessageBox::information ( this, "Hydrogen", tr ( "Please restart hydrogen to enable/disable LASH support" ) );
}

void PreferencesDialog::coloringMethodCombo_currentIndexChanged (int index)
{
	switch(index)
	{
		case 0:
			coloringMethodAuxLabel->setText( "" );
			coloringMethodAuxSpinBox->hide();
			break;
		case 1:
			coloringMethodAuxLabel->setText( tr("Number of steps") );
			coloringMethodAuxSpinBox->setMinimum(1);
			coloringMethodAuxSpinBox->show();
			break;
		case 2:
			coloringMethodAuxLabel->setText( tr("Color (Hue value)") );
			coloringMethodAuxSpinBox->setMinimum(0);
			coloringMethodAuxSpinBox->show();
			break;
	}
}

void PreferencesDialog::on_resampleComboBox_currentIndexChanged ( int index )
{
	switch ( index ){
	case 0:
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Linear );
		break;
	case 1:
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Cosine );
		break;
	case 2:
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Third );
		break;
	case 3:
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Cubic );
		break;
	case 4:
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->setInterpolateMode( Interpolation::InterpolateMode::Hermite );
		break;
	}

}

void PreferencesDialog::onMidiDriverComboBoxIndexChanged ( int index )
{
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::toggleTrackOutsCheckBox(bool toggled)
{
	Preferences::get_instance()->m_bJackTrackOuts = toggled;
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::toggleOscCheckBox(bool toggled)
{
	if ( toggled ) {
		enableOscFeedbackCheckbox->show();
		incomingOscPortSpinBox->show();
		incomingOscPortLabel->show();
		if ( Preferences::get_instance()->m_nOscTemporaryPort != -1 ) {
			oscTemporaryPortLabel->show();
			oscTemporaryPort->show();
		}
	} else {
		enableOscFeedbackCheckbox->hide();
		incomingOscPortSpinBox->hide();
		incomingOscPortLabel->hide();
		oscTemporaryPortLabel->hide();
		oscTemporaryPort->hide();
	}
}
