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


#include "Skin.h"
#include "PreferencesDialog.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include "qmessagebox.h"
#include "qstylefactory.h"

#include <QPixmap>
#include <QFontDialog>
#include "widgets/MidiTable.h"

#include <hydrogen/midi_map.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/IO/MidiInput.h>
#include <hydrogen/LashClient.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/sampler/Sampler.h>
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"


using namespace H2Core;

const char* PreferencesDialog::__class_name = "PreferencesDialog";

PreferencesDialog::PreferencesDialog(QWidget* parent)
 : QDialog( parent )
 , Object( __class_name )
{
	setupUi( this );

	setWindowTitle( trUtf8( "Preferences" ) );
	setWindowIcon( QPixmap( Skin::getImagePath()  + "/icon16.png" ) );

	setMinimumSize( width(), height() );

	Preferences *pPref = Preferences::get_instance();
	pPref->loadPreferences( false );	// reload user's preferences

	driverComboBox->clear();
	driverComboBox->addItem( "Auto" );
#ifdef H2CORE_HAVE_JACK
	driverComboBox->addItem( "Jack" );
#endif
#ifdef H2CORE_HAVE_ALSA
	driverComboBox->addItem( "Alsa" );
#endif
#ifdef H2CORE_HAVE_OSS
	driverComboBox->addItem( "Oss" );
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


	if( driverComboBox->findText(pPref->m_sAudioDriver) > -1){
		driverComboBox->setCurrentIndex(driverComboBox->findText(pPref->m_sAudioDriver));
	}
	else
	{
		driverInfoLbl->setText("Select your Audio Driver");
		ERRORLOG( "Unknown midi input from preferences [" + pPref->m_sAudioDriver + "]" );
	}


	m_pMidiDriverComboBox->clear();
#ifdef H2CORE_HAVE_ALSA
	m_pMidiDriverComboBox->addItem( "ALSA" );
#endif
#ifdef H2CORE_HAVE_PORTMIDI
	m_pMidiDriverComboBox->addItem( "PortMidi" );
#endif
#ifdef H2CORE_HAVE_COREMIDI
	m_pMidiDriverComboBox->addItem( "CoreMidi" );
#endif
#ifdef H2CORE_HAVE_JACK
	m_pMidiDriverComboBox->addItem( "JackMidi" );
#endif


	if( m_pMidiDriverComboBox->findText(pPref->m_sMidiDriver) > -1){
		m_pMidiDriverComboBox->setCurrentIndex(m_pMidiDriverComboBox->findText(pPref->m_sMidiDriver));
	}
	else
	{
		driverInfoLbl->setText("Select your Midi Driver");
		ERRORLOG( "Unknown midi input from preferences [" + pPref->m_sMidiDriver + "]" );
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
	connect(trackOutsCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleTrackOutsCheckBox( bool )));

	connectDefaultsCheckBox->setChecked( pPref->m_bJackConnectDefaults );
	trackOutputComboBox->setCurrentIndex( pPref->m_nJackTrackOutputMode );
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

	resampleComboBox->setCurrentIndex( (int) AudioEngine::get_instance()->get_sampler()->getInterpolateMode() );

	// Appearance tab
	QString applicationFamily = pPref->getApplicationFontFamily();
	int applicationPointSize = pPref->getApplicationFontPointSize();

	QFont applicationFont( applicationFamily, applicationPointSize );
	applicationFontLbl->setFont( applicationFont );
	applicationFontLbl->setText( applicationFamily + QString("  %1").arg( applicationPointSize ) );

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
	coloringMethodCombo->addItem(trUtf8("Automatic"));
	coloringMethodCombo->addItem(trUtf8("Steps"));
	coloringMethodCombo->addItem(trUtf8("Fixed"));

	coloringMethodAuxSpinBox->setMaximum(300);

	coloringMethodCombo->setCurrentIndex( coloringMethod );
	coloringMethodAuxSpinBox->setValue( coloringMethodAuxValue );

	coloringMethodCombo_currentIndexChanged( coloringMethod );

	connect(coloringMethodCombo, SIGNAL(currentIndexChanged(int)), this, SLOT( coloringMethodCombo_currentIndexChanged(int) ));


	// midi tab
	midiPortChannelComboBox->setEnabled( false );
	midiPortComboBox->setEnabled( false );
	// list midi output ports
	midiPortComboBox->clear();
	midiPortComboBox->addItem( "None" );
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

	if ( pPref->m_nMidiChannelFilter == -1 ) {
		midiPortChannelComboBox->setCurrentIndex( 0 );
	}
	else {
		midiPortChannelComboBox->setCurrentIndex( pPref->m_nMidiChannelFilter + 1 );
	}


	// General tab
	restoreLastUsedSongCheckbox->setChecked( pPref->isRestoreLastSongEnabled() );
	restoreLastUsedPlaylistCheckbox->setChecked( pPref->isRestoreLastPlaylistEnabled() );

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

    dataDirectoryLineEdit->setText( pPref->getDataDirectory() );

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


void PreferencesDialog::on_okBtn_clicked()
{
	//	m_bNeedDriverRestart = true;

	Preferences *pPref = Preferences::get_instance();

	MidiMap *mM = MidiMap::get_instance();
	mM->reset_instance();

	midiTable->saveMidiTable();

	// Selected audio driver
	if (driverComboBox->currentText() == "Auto" ) {
		pPref->m_sAudioDriver = "Auto";
	}
	else if (driverComboBox->currentText() == "Jack" ) {
		pPref->m_sAudioDriver = "Jack";
	}
	else if (driverComboBox->currentText() == "Alsa" ) {
		pPref->m_sAudioDriver = "Alsa";
		pPref->m_sAlsaAudioDevice = m_pAudioDeviceTxt->text();
	}
	else if (driverComboBox->currentText() == "Oss" ) {
		pPref->m_sAudioDriver = "Oss";
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

	/*
	 * 0: Post-Fader
	 * 1: Pre-Fader
	 */

	if (trackOutputComboBox->currentIndex() == Preferences::POST_FADER)
	{
		pPref->m_nJackTrackOutputMode = Preferences::POST_FADER;
	} else {
		pPref->m_nJackTrackOutputMode = Preferences::PRE_FADER;
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
	else if ( m_pMidiDriverComboBox->currentText() == "CoreMidi" ) {
		pPref->m_sMidiDriver = "CoreMidi";
	}
	else if ( m_pMidiDriverComboBox->currentText() == "JackMidi" ) {
		pPref->m_sMidiDriver = "JackMidi";
	}



	pPref->m_bMidiNoteOffIgnore = m_pIgnoreNoteOffCheckBox->isChecked();

	// Mixer falloff
	QString falloffStr = mixerFalloffComboBox->currentText();
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
		ERRORLOG( "[okBtnClicked] Unknown mixerFallOffSpeed: " + falloffStr );
	}

	QString sNewMidiPortName = midiPortComboBox->currentText();

	if ( pPref->m_sMidiPortName != sNewMidiPortName ) {
		pPref->m_sMidiPortName = sNewMidiPortName;
		m_bNeedDriverRestart = true;
	}

	if ( pPref->m_nMidiChannelFilter != midiPortChannelComboBox->currentIndex() - 1 ) {
		//m_bNeedDriverRestart = true;
	}
	pPref->m_nMidiChannelFilter = midiPortChannelComboBox->currentIndex() - 1;


	// General tab
	pPref->setRestoreLastSongEnabled( restoreLastUsedSongCheckbox->isChecked() );
	pPref->setRestoreLastPlaylistEnabled( restoreLastUsedPlaylistCheckbox->isChecked() );
	pPref->m_bsetLash = useLashCheckbox->isChecked(); //restore m_bsetLash after saving pref.

    // Path to data directory
    pPref->setDataDirectory( dataDirectoryLineEdit->text() );

	//path to rubberband
	pPref-> m_rubberBandCLIexecutable = rubberbandLineEdit->text();

	//check preferences
	if ( pPref->m_brestartLash == true ){
		pPref->m_bsetLash = true ;
	}

	pPref->m_countOffset = sBcountOffset->value();
	pPref->m_startOffset = sBstartOffset->value();

	pPref->setMaxBars( sBmaxBars->value() );

	Hydrogen::get_instance()->setBcOffsetAdjust();

	pPref->setDefaultUILayout( uiLayoutComboBox->currentIndex() );

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

	pPref->savePreferences();


	if (m_bNeedDriverRestart) {
		int res = QMessageBox::information( this, "Hydrogen", tr( "Driver restart required.\n Restart driver?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
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
		info += trUtf8("<b>Automatic driver selection</b>");

		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled( false );
		sampleRateComboBox->setEnabled( false );
		trackOutputComboBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled( false );
	}
	else if ( driverComboBox->currentText() == "OSS" ) {	// OSS
		info += trUtf8("<b>Open Sound System</b><br>Simple audio driver [/dev/dsp]");
		if ( !bOss_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(true);
		m_pAudioDeviceTxt->setText( pPref->m_sOSSDevice );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled(false);
	}
	else if ( driverComboBox->currentText() == "Jack" ) {	// JACK
		info += trUtf8("<b>Jack Audio Connection Kit Driver</b><br>Low latency audio driver");
		if ( !bJack_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled(false);
		sampleRateComboBox->setEnabled(false);
		trackOutputComboBox->setEnabled( true );
		connectDefaultsCheckBox->setEnabled(true);
		trackOutsCheckBox->setEnabled( true );
	}
	else if ( driverComboBox->currentText() == "Alsa" ) {	// ALSA
		info += trUtf8("<b>ALSA Driver</b><br>");
		if ( !bAlsa_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(true);
		m_pAudioDeviceTxt->setText( pPref->m_sAlsaAudioDevice );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled(false);
	}
	else if ( driverComboBox->currentText() == "PortAudio" ) {
		info += trUtf8( "<b>PortAudio Driver</b><br>" );
		if ( !bPortAudio_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutsCheckBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled(false);
	}
	else if ( driverComboBox->currentText() == "CoreAudio" ) {
		info += trUtf8( "<b>CoreAudio Driver</b><br>" );
		if ( !bCoreAudio_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText( "" );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled(false);
	}
	else if ( driverComboBox->currentText() == "PulseAudio" ) {
		info += trUtf8("<b>PulseAudio Driver</b><br>");
		if ( !bPulseAudio_support ) {
			info += trUtf8("<br><b><font color=\"red\">Not compiled</font></b>");
		}
		m_pAudioDeviceTxt->setEnabled(false);
		m_pAudioDeviceTxt->setText("");
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled(false);
	}
	else {
		QString selectedDriver = driverComboBox->currentText();
		ERRORLOG( "Unknown driver = " + selectedDriver );
	}

	metronomeVolumeSpinBox->setEnabled(true);
	bufferSizeSpinBox->setValue( pPref->m_nBufferSize );

	driverInfoLbl->setText(info);
}



void PreferencesDialog::on_selectApplicationFontBtn_clicked()
{
	Preferences *preferencesMng = Preferences::get_instance();

	QString family = preferencesMng->getApplicationFontFamily();
	int pointSize = preferencesMng->getApplicationFontPointSize();

	bool ok;
	QFont font = QFontDialog::getFont( &ok, QFont( family, pointSize ), this );
	if ( ok ) {
		// font is set to the font the user selected
		family = font.family();
		pointSize = font.pointSize();
		QString familyStr = family;
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




void PreferencesDialog::on_bufferSizeSpinBox_valueChanged( int i )
{
	UNUSED( i );
	m_bNeedDriverRestart = false;
}




void PreferencesDialog::on_sampleRateComboBox_editTextChanged( const QString&  )
{
	m_bNeedDriverRestart = true;
}



void PreferencesDialog::on_restartDriverBtn_clicked()
{
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
	QMessageBox::information ( this, "Hydrogen", trUtf8 ( "Please restart hydrogen to enable/disable LASH support" ) );
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
			coloringMethodAuxLabel->setText( trUtf8("Number of steps") );
			coloringMethodAuxSpinBox->setMinimum(1);
			coloringMethodAuxSpinBox->show();
			break;
		case 2:
			coloringMethodAuxLabel->setText( trUtf8("Color (Hue value)") );
			coloringMethodAuxSpinBox->setMinimum(0);
			coloringMethodAuxSpinBox->show();
			break;
	}
}

void PreferencesDialog::on_resampleComboBox_currentIndexChanged ( int index )
{
	switch ( index ){
	case 0:
		AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::LINEAR );
		break;
	case 1:
		AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::COSINE );
		break;
	case 2:
		AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::THIRD );
		break;
	case 3:
		AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::CUBIC );
		break;
	case 4:
		AudioEngine::get_instance()->get_sampler()->setInterpolateMode( Sampler::HERMITE );
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
