/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <cstring>

#include "PreferencesDialog.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"

#include "qmessagebox.h"
#include "qstylefactory.h"

#include <QPixmap>
#include <QFontDatabase>
#include <QTreeWidgetItemIterator>
#include "../Widgets/MidiTable.h"

#include <core/MidiMap.h>
#include <core/Hydrogen.h>
#include <core/IO/MidiInput.h>
#include <core/Lash/LashClient.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Translations.h>
#include <core/Sampler/Sampler.h>
#include "../SongEditor/SongEditor.h"
#include "../SongEditor/SongEditorPanel.h"

#include <core/IO/PortAudioDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/AlsaAudioDriver.h>


using namespace H2Core;


DeviceComboBox::DeviceComboBox( QWidget *pParent )
	: QComboBox( pParent)
{
	m_sDriver = "";
}

void DeviceComboBox::showPopup()
{
	clear();
	QApplication::setOverrideCursor( Qt::WaitCursor );
	if ( m_sDriver == "PortAudio" ) {
#ifdef H2CORE_HAVE_PORTAUDIO
		// Get device list for PortAudio based on current value of the API combo box
		for ( QString s : PortAudioDriver::getDevices( m_sHostAPI ) ) {
			addItem( s );
		}
#endif
	} else if ( m_sDriver == "CoreAudio" ) {
#ifdef H2CORE_HAVE_COREAUDIO
		for ( QString s : CoreAudioDriver::getDevices() ) {
			addItem( s );
		}
#endif
	} else if ( m_sDriver == "ALSA" ) {
#ifdef H2CORE_HAVE_ALSA
		for ( QString s : AlsaAudioDriver::getDevices() ) {
			addItem( s );
		}
#endif
	}
	QApplication::restoreOverrideCursor();
	QComboBox::showPopup();
}


HostAPIComboBox::HostAPIComboBox( QWidget *pParent )
	: QComboBox( pParent )
{
}

void HostAPIComboBox::setValue( QString sHostAPI ) {
	// The ComboBox doesn't have any item strings until it's actually opened,
	// so we must add the item to it temporarily
	clear();
	addItem( sHostAPI );
	setCurrentText( sHostAPI );
}

void HostAPIComboBox::showPopup()
{
	clear();
#ifdef H2CORE_HAVE_PORTAUDIO
	QApplication::setOverrideCursor( Qt::WaitCursor );
	for ( QString s : PortAudioDriver::getHostAPIs() ) {
		addItem( s );
	}
	QApplication::restoreOverrideCursor();
#endif

	QComboBox::showPopup();
}


ColorTreeItem::ColorTreeItem( int nId, QTreeWidgetItem* pParent, QString sLabel )
    : QTreeWidgetItem( pParent, QStringList( sLabel ) ) {
	m_nId = nId;
}
ColorTreeItem::ColorTreeItem( int nId, QTreeWidget* pParent, QString sLabel )
    : QTreeWidgetItem( pParent, QStringList( sLabel ) ) {
	m_nId = nId;
}
int ColorTreeItem::getId() const {
	return m_nId;
}

QString PreferencesDialog::m_sColorRed = "#ca0003";


PreferencesDialog::PreferencesDialog(QWidget* parent)
 : QDialog( parent )
 , m_pCurrentColor( nullptr )
 , m_nCurrentId( 0 ) {
	
	m_pCurrentTheme = std::make_shared<H2Core::Theme>( H2Core::Preferences::get_instance()->getTheme() );
	m_pPreviousTheme = std::make_shared<H2Core::Theme>( H2Core::Preferences::get_instance()->getTheme() );
	setupUi( this );

	setWindowTitle( tr( "Preferences" ) );

	setMinimumSize( width(), height() );

	connect( this, &PreferencesDialog::rejected, this, &PreferencesDialog::onRejected );

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

	// Set the PortAudio HostAPI combo box to the current selected value.
	portaudioHostAPIComboBox->setValue( pPref->m_sPortAudioHostAPI );
	m_pAudioDeviceTxt->setHostAPI( pPref->m_sPortAudioHostAPI );

	latencyTargetSpinBox->setValue( pPref->m_nLatencyTarget );


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
		ERRORLOG( "Unknown audio driver from preferences [" + pPref->m_sAudioDriver + "]" );
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

	QFontDatabase fontDB;
	m_fontFamilies = fontDB.families();
	
	// Appearance tab
	
	connect( importThemeButton, SIGNAL(clicked(bool)), SLOT(importTheme()));
	connect( exportThemeButton, SIGNAL(clicked(bool)), SLOT(exportTheme()));
	connect( resetThemeButton, SIGNAL(clicked(bool)), this, SLOT(resetTheme()));
	
	// Appearance tab - Fonts
	connect( applicationFontComboBox, &QFontComboBox::currentFontChanged, this, &PreferencesDialog::onApplicationFontChanged );
	connect( level2FontComboBox, &QFontComboBox::currentFontChanged, this, &PreferencesDialog::onLevel2FontChanged );
	connect( level3FontComboBox, &QFontComboBox::currentFontChanged, this, &PreferencesDialog::onLevel3FontChanged );
	connect( fontSizeComboBox, SIGNAL( currentIndexChanged(int) ),
			 this, SLOT( onFontSizeChanged(int) ) );
	
	// Appearance tab - Interface
	UIChangeWarningLabel->hide();
	UIChangeWarningLabel->setText( QString( "<b><i><font color=" )
								   .append( m_sColorRed )
								   .append( ">" )
								   .append( tr( "For changes of the interface layout to take effect Hydrogen must be restarted." ) )
								   .append( "</font></i></b>" ) );
	connect( uiLayoutComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( onUILayoutChanged(int) ) );
	connect( iconColorComboBox, SIGNAL(currentIndexChanged(int)), this,
			 SLOT( onIconColorChanged(int)) );
	connect( coloringMethodAuxSpinBox, SIGNAL( valueChanged(int)), this, SLOT( onColorNumberChanged( int ) ) );

	m_colorSelectionButtons = std::vector<ColorSelectionButton*>( m_pCurrentTheme->getInterfaceTheme()->m_nMaxPatternColors );
	int nButtonSize = fontSizeComboBox->height();
	// float fLineWidth = static_cast<float>(fontSizeComboBox->width());
	// Using a fixed one size resizing of the widget seems to happen
	// after the constructor is called.
	float fLineWidth = 308;
	int nButtonsPerLine = std::floor( fLineWidth / static_cast<float>(nButtonSize + 6) );

	colorSelectionGrid->setHorizontalSpacing( 4 );
	for ( int ii = 0; ii < m_pCurrentTheme->getInterfaceTheme()->m_nMaxPatternColors; ii++ ) {
		ColorSelectionButton* bbutton =
			new ColorSelectionButton( this, m_pCurrentTheme->getInterfaceTheme()->m_patternColors[ ii ],
									  nButtonSize );
		bbutton->hide();
		connect( bbutton, &ColorSelectionButton::colorChanged, this,
				 &PreferencesDialog::onColorSelectionClicked );
		colorSelectionGrid->addWidget( bbutton,
									   std::floor( static_cast<float>( ii ) /
												   static_cast<float>( nButtonsPerLine ) ),
									   (ii % nButtonsPerLine) + 1); //+1 to take the hspace into account.
		m_colorSelectionButtons[ ii ] = bbutton;
	}
	
	coloringMethodCombo->clear();
	coloringMethodCombo->addItem(tr("Automatic"));
	coloringMethodCombo->addItem(tr("Custom"));
	connect( coloringMethodCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT( onColoringMethodChanged(int) ) );

	// Appearance tab - Colors
	colorButton->setAutoFillBackground(true);
	m_pColorSliderTimer = new QTimer( this );
	m_pColorSliderTimer->setSingleShot( true );
	connect( m_pColorSliderTimer, SIGNAL(timeout()), this, SLOT(updateColors()) );
	  	
	ColorTreeItem* pTopLevelItem;
	colorTree->clear();
	pTopLevelItem = new ColorTreeItem( 0x000, colorTree, "General" );
	new ColorTreeItem( 0x100, pTopLevelItem, "Window" );
	new ColorTreeItem( 0x101, pTopLevelItem, "Window Text" );
	new ColorTreeItem( 0x102, pTopLevelItem, "Base" );
	new ColorTreeItem( 0x103, pTopLevelItem, "Alternate Base" );
	new ColorTreeItem( 0x104, pTopLevelItem, "Text" );
	new ColorTreeItem( 0x105, pTopLevelItem, "Button" );
	new ColorTreeItem( 0x106, pTopLevelItem, "Button Text" );
	new ColorTreeItem( 0x107, pTopLevelItem, "Light" );
	new ColorTreeItem( 0x108, pTopLevelItem, "Mid Light" );
	new ColorTreeItem( 0x109, pTopLevelItem, "Mid" );
	new ColorTreeItem( 0x10a, pTopLevelItem, "Dark" );
	new ColorTreeItem( 0x10b, pTopLevelItem, "Shadow Text" );
	new ColorTreeItem( 0x10c, pTopLevelItem, "Highlight" );
	new ColorTreeItem( 0x10d, pTopLevelItem, "Highlight Text" );
	new ColorTreeItem( 0x10e, pTopLevelItem, "Selection Highlight" );
	new ColorTreeItem( 0x10f, pTopLevelItem, "Selection Inactive" );
	new ColorTreeItem( 0x110, pTopLevelItem, "Tool Tip Base" );
	new ColorTreeItem( 0x111, pTopLevelItem, "Tool Tip Text" );
	
	pTopLevelItem = new ColorTreeItem( 0x000, colorTree, "Widgets" );
	new ColorTreeItem( 0x200, pTopLevelItem, "Widget" );
	new ColorTreeItem( 0x201, pTopLevelItem, "Widget Text" );
	new ColorTreeItem( 0x202, pTopLevelItem, "Accent" );
	new ColorTreeItem( 0x203, pTopLevelItem, "Accent Text" );
	new ColorTreeItem( 0x204, pTopLevelItem, "Button Red" );
	new ColorTreeItem( 0x205, pTopLevelItem, "Button Red Text" );
	new ColorTreeItem( 0x206, pTopLevelItem, "Spin Box" );
	new ColorTreeItem( 0x207, pTopLevelItem, "Spin Box Text" );
	new ColorTreeItem( 0x208, pTopLevelItem, "Automation" );
	new ColorTreeItem( 0x209, pTopLevelItem, "Automation Circle" );
	pTopLevelItem = new ColorTreeItem( 0x000, colorTree, "Song Editor" );
	new ColorTreeItem( 0x300, pTopLevelItem, "Background" );
	new ColorTreeItem( 0x301, pTopLevelItem, "Alternate Row" );
	new ColorTreeItem( 0x302, pTopLevelItem, "Selected Row" );
	new ColorTreeItem( 0x303, pTopLevelItem, "Line" );
	new ColorTreeItem( 0x304, pTopLevelItem, "Text" );
	pTopLevelItem = new ColorTreeItem( 0x000, colorTree, "Pattern Editor" );
	new ColorTreeItem( 0x400, pTopLevelItem, "Background" );
	new ColorTreeItem( 0x401, pTopLevelItem, "Alternate Row" );
	new ColorTreeItem( 0x402, pTopLevelItem, "Selected Row" );
	new ColorTreeItem( 0x403, pTopLevelItem, "Text" );
	new ColorTreeItem( 0x404, pTopLevelItem, "Note" );
	new ColorTreeItem( 0x405, pTopLevelItem, "Note Off" );
	new ColorTreeItem( 0x406, pTopLevelItem, "Line" );
	new ColorTreeItem( 0x407, pTopLevelItem, "Line 1" );
	new ColorTreeItem( 0x408, pTopLevelItem, "Line 2" );
	new ColorTreeItem( 0x409, pTopLevelItem, "Line 3" );
	new ColorTreeItem( 0x40a, pTopLevelItem, "Line 4" );
	new ColorTreeItem( 0x40b, pTopLevelItem, "Line 5" );

	colorButton->setEnabled( false );

	connect( colorTree, SIGNAL(itemSelectionChanged()),
			 this, SLOT(colorTreeSelectionChanged()) );
	connect( colorButton, SIGNAL(colorChanged()),
			 this, SLOT(colorButtonChanged()) );
	connect(rslider, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
	connect(gslider, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
	connect(bslider, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
	connect(hslider, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
	connect(sslider, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
	connect(vslider, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

	connect(rval, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
	connect(gval, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
	connect(bval, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
	connect(hval, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
	connect(sval, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
	connect(vval, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

	updateColorTree();
	updateAppearanceTab( m_pCurrentTheme );
	
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
	patternFollowsSongCheckbox->setChecked( pPref->patternFollowsSong() );

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
	
	H2Core::Preferences::get_instance()->setTheme( m_pPreviousTheme );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Colors );

	reject();
}

void PreferencesDialog::on_m_pAudioDeviceTxt_currentTextChanged( QString str )
{
	m_bNeedDriverRestart = true;
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
		pPref->m_sAlsaAudioDevice = m_pAudioDeviceTxt->lineEdit()->text();
	}
	else if (driverComboBox->currentText() == "OSS" ) {
		pPref->m_sAudioDriver = "OSS";
		pPref->m_sOSSDevice = m_pAudioDeviceTxt->lineEdit()->text();
	}
	else if (driverComboBox->currentText() == "PortAudio" ) {
		pPref->m_sAudioDriver = "PortAudio";
		pPref->m_sPortAudioDevice = m_pAudioDeviceTxt->lineEdit()->text();
		pPref->m_sPortAudioHostAPI = portaudioHostAPIComboBox->currentText();
		pPref->m_nLatencyTarget = latencyTargetSpinBox->value();
	}
	else if (driverComboBox->currentText() == "CoreAudio" ) {
		pPref->m_sAudioDriver = "CoreAudio";
		pPref->m_sCoreAudioDevice = m_pAudioDeviceTxt->lineEdit()->text();
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
	pPref->setPatternFollowsSong( patternFollowsSongCheckbox->isChecked() );

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

	pPref->setTheme( m_pCurrentTheme );

	HydrogenApp *pH2App = HydrogenApp::get_instance();
	pH2App->changePreferences( static_cast<H2Core::Preferences::Changes>( H2Core::Preferences::Changes::Font |
																		  H2Core::Preferences::Changes::Colors |
																		  H2Core::Preferences::Changes::AppearanceTab ) );

	SongEditorPanel* pSongEditorPanel = pH2App->getSongEditorPanel();
	SongEditor * pSongEditor = pSongEditorPanel->getSongEditor();
	pSongEditor->updateEditorandSetTrue();

	QString sPreferredLanguage = languageComboBox->currentData().toString();
	if ( sPreferredLanguage != m_sInitialLanguage ) {
		QMessageBox::information( this, "Hydrogen", tr( "Hydrogen must be restarted for language change to take effect" ));
		pPref->setPreferredLanguage( sPreferredLanguage );
	}

	if (m_bNeedDriverRestart) {
		int res = QMessageBox::information( this, "Hydrogen", tr( "Driver restart required.\n Restart driver?"), tr("&Ok"), tr("&Cancel"), nullptr, 1 );
		if ( res == 0 ) {
			QApplication::setOverrideCursor( Qt::WaitCursor );
			Hydrogen::get_instance()->restartDrivers();
			QApplication::restoreOverrideCursor();
		} else {
			// Don't save the Preferences and don't close the PreferencesDialog
			return;
		}
	}
	
	pPref->savePreferences();
	accept();
}


void PreferencesDialog::on_driverComboBox_activated( int index )
{
	UNUSED( index );
	updateDriverInfo();
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::on_portaudioHostAPIComboBox_activated( int index )
{
	m_pAudioDeviceTxt->setHostAPI( portaudioHostAPIComboBox->currentText() );
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

	m_pAudioDeviceTxt->setDriver( driverComboBox->currentText() );
	if ( driverComboBox->currentText() == "Auto" ) {
		info += tr("Automatic driver selection");
		
		// Display the selected driver as well.
		if ( H2Core::Hydrogen::get_instance()->getAudioOutput() != nullptr ) {
			info.append( "<br><b>" )
				.append( H2Core::Hydrogen::get_instance()->getAudioOutput()->class_name() )
				.append( "</b> " ).append( tr( "selected") );
		}
		m_pAudioDeviceTxt->setEnabled( true );
		m_pAudioDeviceTxt->lineEdit()->setText( "" );
		bufferSizeSpinBox->setEnabled( false );
		sampleRateComboBox->setEnabled( true );
		trackOutputComboBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled( false );
		enableTimebaseCheckBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
		latencyTargetLabel->hide();
		latencyTargetSpinBox->hide();
		latencyValueLabel->hide();
		if ( std::strcmp( H2Core::Hydrogen::get_instance()->getAudioOutput()->class_name(),
						  "JackAudioDriver" ) == 0 ) {
			trackOutputComboBox->setEnabled( true );
			connectDefaultsCheckBox->setEnabled( true );
			enableTimebaseCheckBox->setEnabled( true );
			trackOutsCheckBox->setEnabled( true );
			trackOutputComboBox->show();
			trackOutputLbl->show();
			connectDefaultsCheckBox->show();
			trackOutsCheckBox->show();
			enableTimebaseCheckBox->show();
		} else {
			trackOutputComboBox->setEnabled( false );
			connectDefaultsCheckBox->setEnabled( false );
			enableTimebaseCheckBox->setEnabled( false );
			trackOutsCheckBox->setEnabled( false );
			trackOutputComboBox->hide();
			trackOutputLbl->hide();
			connectDefaultsCheckBox->hide();
			enableTimebaseCheckBox->hide();
			trackOutsCheckBox->hide();
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
		m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sOSSDevice );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
		latencyTargetLabel->hide();
		latencyTargetSpinBox->hide();
		latencyValueLabel->hide();
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
		m_pAudioDeviceTxt->lineEdit()->setText( "" );
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
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
		latencyTargetLabel->hide();
		latencyTargetSpinBox->hide();
		latencyValueLabel->hide();
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
		m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sAlsaAudioDevice );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
		latencyTargetLabel->hide();
		latencyTargetSpinBox->hide();
		latencyValueLabel->hide();
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
		m_pAudioDeviceTxt->setEnabled( true );
		m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sPortAudioDevice );
		bufferSizeSpinBox->setEnabled(false);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		portaudioHostAPIComboBox->show();
		portaudioHostAPILabel->show();
		latencyTargetLabel->show();
		latencyTargetSpinBox->show();
		latencyValueLabel->show();
		latencyValueLabel->setText( QString("Current: %1 frames").arg( H2Core::Hydrogen::get_instance()->getAudioOutput()->getLatency() ) );

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
		m_pAudioDeviceTxt->setEnabled( true );
		m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sCoreAudioDevice );
		bufferSizeSpinBox->setEnabled( false );
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
		latencyTargetLabel->hide();
		latencyTargetSpinBox->hide();
		latencyValueLabel->hide();
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
		m_pAudioDeviceTxt->lineEdit()->setText("");
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
		latencyTargetLabel->hide();
		latencyTargetSpinBox->hide();
		latencyValueLabel->hide();
	}
	else {
		QString selectedDriver = driverComboBox->currentText();
		ERRORLOG( "Unknown driver = " + selectedDriver );
	}

	metronomeVolumeSpinBox->setEnabled(true);
	bufferSizeSpinBox->setValue( pPref->m_nBufferSize );

	driverInfoLbl->setText(info);
}

void PreferencesDialog::onApplicationFontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();

	m_pCurrentTheme->getFontTheme()->m_sApplicationFontFamily = font.family();
	pPref->setApplicationFontFamily( font.family() );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onLevel2FontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();

	m_pCurrentTheme->getFontTheme()->m_sLevel2FontFamily = font.family();
	pPref->setLevel2FontFamily( font.family() );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onLevel3FontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();

	m_pCurrentTheme->getFontTheme()->m_sLevel3FontFamily = font.family();
	pPref->setLevel3FontFamily( font.family() );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onRejected() {
	auto pPref = Preferences::get_instance();

	updateAppearanceTab( m_pPreviousTheme );

	HydrogenApp::get_instance()->changePreferences( static_cast<H2Core::Preferences::Changes>( H2Core::Preferences::Changes::Font |
																							   H2Core::Preferences::Changes::Colors |
																							   H2Core::Preferences::Changes::AppearanceTab ) );
}

void PreferencesDialog::onFontSizeChanged( int nIndex ) {
	auto pPref = Preferences::get_instance();

	switch ( nIndex ) {
	case 0:
		pPref->setFontSize( FontTheme::FontSize::Small );
		m_pCurrentTheme->getFontTheme()->m_fontSize = FontTheme::FontSize::Small;
		break;
	case 1:
		pPref->setFontSize( FontTheme::FontSize::Normal );
		m_pCurrentTheme->getFontTheme()->m_fontSize = FontTheme::FontSize::Normal;
		break;
	case 2:
		pPref->setFontSize( FontTheme::FontSize::Large );
		m_pCurrentTheme->getFontTheme()->m_fontSize = FontTheme::FontSize::Large;
		break;
	default:
		ERRORLOG( QString( "Unknown font size: %1" ).arg( nIndex ) );
	}
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onUILayoutChanged( int nIndex ) {
	if ( static_cast<InterfaceTheme::Layout>(nIndex) !=
		 m_pPreviousTheme->getInterfaceTheme()->m_layout ||
		 m_pCurrentTheme->getInterfaceTheme()->m_scalingPolicy !=
		 m_pPreviousTheme->getInterfaceTheme()->m_scalingPolicy ) {
		UIChangeWarningLabel->show();
		INFOLOG( "hosw" );
	} else {
		INFOLOG( "hide" );
		UIChangeWarningLabel->hide();
	}
	m_pCurrentTheme->getInterfaceTheme()->m_layout = static_cast<InterfaceTheme::Layout>(nIndex);
	Preferences::get_instance()->setDefaultUILayout( static_cast<InterfaceTheme::Layout>(nIndex) );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::on_uiScalingPolicyComboBox_currentIndexChanged( int nIndex ) {
	if ( static_cast<InterfaceTheme::ScalingPolicy>(nIndex) !=
		 m_pPreviousTheme->getInterfaceTheme()->m_scalingPolicy ||
		 m_pCurrentTheme->getInterfaceTheme()->m_layout !=
		 m_pPreviousTheme->getInterfaceTheme()->m_layout ) {
		UIChangeWarningLabel->show();
		INFOLOG( "hosw" );
	} else {
		INFOLOG( "hide" );
		UIChangeWarningLabel->hide();
	}
	m_pCurrentTheme->getInterfaceTheme()->m_scalingPolicy = static_cast<InterfaceTheme::ScalingPolicy>(nIndex);
	Preferences::get_instance()->setUIScalingPolicy( static_cast<InterfaceTheme::ScalingPolicy>(nIndex) );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::onIconColorChanged( int nIndex ) {
	m_pCurrentTheme->getInterfaceTheme()->m_iconColor = static_cast<InterfaceTheme::IconColor>(nIndex);
	H2Core::Preferences::get_instance()->setIconColor( static_cast<InterfaceTheme::IconColor>(nIndex) );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::onColorNumberChanged( int nIndex ) {
	Preferences::get_instance()->setVisiblePatternColors( nIndex );
	m_pCurrentTheme->getInterfaceTheme()->m_nVisiblePatternColors = nIndex;
	for ( int ii = 0; ii < Preferences::get_instance()->getMaxPatternColors(); ii++ ) {
		if ( ii < nIndex ) {
			m_colorSelectionButtons[ ii ]->show();
		} else {
			m_colorSelectionButtons[ ii ]->hide();
		}
	}
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::onColorSelectionClicked() {
	int nMaxPatternColors = Preferences::get_instance()->getMaxPatternColors();
	std::vector<QColor> colors( nMaxPatternColors );
	for ( int ii = 0; ii < nMaxPatternColors; ii++ ) {
		colors[ ii ] = m_colorSelectionButtons[ ii ]->getColor();
	}
	m_pCurrentTheme->getInterfaceTheme()->m_patternColors = colors;
	Preferences::get_instance()->setPatternColors( colors );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::onColoringMethodChanged( int nIndex ) {
	m_pCurrentTheme->getInterfaceTheme()->m_coloringMethod = static_cast<H2Core::InterfaceTheme::ColoringMethod>(nIndex);
	Preferences::get_instance()->setColoringMethod( static_cast<H2Core::InterfaceTheme::ColoringMethod>(nIndex) );

	if ( nIndex == 0 ) {
		coloringMethodAuxSpinBox->hide();
		coloringMethodAuxLabel->hide();
		colorSelectionLabel->hide();
		for ( int ii = 0; ii < m_pCurrentTheme->getInterfaceTheme()->m_nMaxPatternColors; ii++ ) {
			m_colorSelectionButtons[ ii ]->hide();
		}
	} else {
		coloringMethodAuxSpinBox->show();
		coloringMethodAuxLabel->show();
		colorSelectionLabel->show();
		for ( int ii = 0; ii < m_pCurrentTheme->getInterfaceTheme()->m_nVisiblePatternColors; ii++ ) {
			m_colorSelectionButtons[ ii ]->show();
		}
	}
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::on_mixerFalloffComboBox_currentIndexChanged( int nIndex ) {
	Preferences *pPref = Preferences::get_instance();
	
	if ( nIndex == 0 ) {
		m_pCurrentTheme->getInterfaceTheme()->m_fMixerFalloffSpeed = InterfaceTheme::FALLOFF_SLOW;
		pPref->setMixerFalloffSpeed( InterfaceTheme::FALLOFF_SLOW );
	} else if ( nIndex == 1 ) {
		m_pCurrentTheme->getInterfaceTheme()->m_fMixerFalloffSpeed = InterfaceTheme::FALLOFF_NORMAL;
		pPref->setMixerFalloffSpeed( InterfaceTheme::FALLOFF_NORMAL );
	} else if ( nIndex == 2 ) {
		m_pCurrentTheme->getInterfaceTheme()->m_fMixerFalloffSpeed = InterfaceTheme::FALLOFF_FAST;
		pPref->setMixerFalloffSpeed( InterfaceTheme::FALLOFF_FAST );
	} else {
		ERRORLOG( QString("Wrong mixerFalloff value = %1").arg( nIndex ) );
	}
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::on_latencyTargetSpinBox_valueChanged( int i )
{
	UNUSED( i );
	m_bNeedDriverRestart = true;
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
	Hydrogen::get_instance()->restartDrivers();
	pPref->savePreferences();
	m_bNeedDriverRestart = false;
	updateDriverInfo();
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
	m_pCurrentTheme->getInterfaceTheme()->m_sQTStyle = sStyle;
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
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

QColor* PreferencesDialog::getColorById( int nId, std::shared_ptr<H2Core::ColorTheme> pColorTheme ) const {
	switch( nId ) {
	case 0x100: return &pColorTheme->m_windowColor;
	case 0x101: return &pColorTheme->m_windowTextColor;
	case 0x102: return &pColorTheme->m_baseColor;
	case 0x103: return &pColorTheme->m_alternateBaseColor;
	case 0x104: return &pColorTheme->m_textColor;
	case 0x105: return &pColorTheme->m_buttonColor;
	case 0x106: return &pColorTheme->m_buttonTextColor;
	case 0x107: return &pColorTheme->m_lightColor;
	case 0x108: return &pColorTheme->m_midLightColor;
	case 0x109: return &pColorTheme->m_midColor;
	case 0x10a: return &pColorTheme->m_darkColor;
	case 0x10b: return &pColorTheme->m_shadowTextColor;
	case 0x10c: return &pColorTheme->m_highlightColor;
	case 0x10d: return &pColorTheme->m_highlightedTextColor;
	case 0x10e: return &pColorTheme->m_selectionHighlightColor;
	case 0x10f: return &pColorTheme->m_selectionInactiveColor;
	case 0x110: return &pColorTheme->m_toolTipBaseColor;
	case 0x111: return &pColorTheme->m_toolTipTextColor;
	case 0x200: return &pColorTheme->m_widgetColor;
	case 0x201: return &pColorTheme->m_widgetTextColor;
	case 0x202: return &pColorTheme->m_accentColor;
	case 0x203: return &pColorTheme->m_accentTextColor;
	case 0x204: return &pColorTheme->m_buttonRedColor;
	case 0x205: return &pColorTheme->m_buttonRedTextColor;
	case 0x206: return &pColorTheme->m_spinBoxColor;
	case 0x207: return &pColorTheme->m_spinBoxTextColor;
	case 0x208: return &pColorTheme->m_automationColor;
	case 0x209: return &pColorTheme->m_automationCircleColor;
	case 0x300: return &pColorTheme->m_songEditor_backgroundColor;
	case 0x301: return &pColorTheme->m_songEditor_alternateRowColor;
	case 0x302: return &pColorTheme->m_songEditor_selectedRowColor;
	case 0x303: return &pColorTheme->m_songEditor_lineColor;
	case 0x304: return &pColorTheme->m_songEditor_textColor;
	case 0x400: return &pColorTheme->m_patternEditor_backgroundColor;
	case 0x401: return &pColorTheme->m_patternEditor_alternateRowColor;
	case 0x402: return &pColorTheme->m_patternEditor_selectedRowColor;
	case 0x403: return &pColorTheme->m_patternEditor_textColor;
	case 0x404: return &pColorTheme->m_patternEditor_noteColor;
	case 0x405: return &pColorTheme->m_patternEditor_noteoffColor;
	case 0x406: return &pColorTheme->m_patternEditor_lineColor;
	case 0x407: return &pColorTheme->m_patternEditor_line1Color;
	case 0x408: return &pColorTheme->m_patternEditor_line2Color;
	case 0x409: return &pColorTheme->m_patternEditor_line3Color;
	case 0x40a: return &pColorTheme->m_patternEditor_line4Color;
	case 0x40b: return &pColorTheme->m_patternEditor_line5Color;
	default: return nullptr;
	}

	return nullptr;
}

void PreferencesDialog::setColorById( int nId, const QColor& color,
									  std::shared_ptr<H2Core::ColorTheme> pColorTheme ) {
	switch( nId ) {
	case 0x100:  pColorTheme->m_windowColor = color;
		break;
	case 0x101:  pColorTheme->m_windowTextColor = color;
		break;
	case 0x102:  pColorTheme->m_baseColor = color;
		break;
	case 0x103:  pColorTheme->m_alternateBaseColor = color;
		break;
	case 0x104:  pColorTheme->m_textColor = color;
		break;
	case 0x105:  pColorTheme->m_buttonColor = color;
		break;
	case 0x106:  pColorTheme->m_buttonTextColor = color;
		break;
	case 0x107:  pColorTheme->m_lightColor = color;
		break;
	case 0x108:  pColorTheme->m_midLightColor = color;
		break;
	case 0x109:  pColorTheme->m_midColor = color;
		break;
	case 0x10a:  pColorTheme->m_darkColor = color;
		break;
	case 0x10b:  pColorTheme->m_shadowTextColor = color;
		break;
	case 0x10c:  pColorTheme->m_highlightColor = color;
		break;
	case 0x10d:  pColorTheme->m_highlightedTextColor = color;
		break;
	case 0x10e:  pColorTheme->m_selectionHighlightColor = color;
		break;
	case 0x10f:  pColorTheme->m_selectionInactiveColor = color;
		break;
	case 0x110:  pColorTheme->m_toolTipBaseColor = color;
		break;
	case 0x111:  pColorTheme->m_toolTipTextColor = color;
		break;
	case 0x200:  pColorTheme->m_widgetColor = color;
		break;
	case 0x201:  pColorTheme->m_widgetTextColor = color;
		break;
	case 0x202:  pColorTheme->m_accentColor = color;
		break;
	case 0x203:  pColorTheme->m_accentTextColor = color;
		break;
	case 0x204:  pColorTheme->m_buttonRedColor = color;
		break;
	case 0x205:  pColorTheme->m_buttonRedTextColor = color;
		break;
	case 0x206:  pColorTheme->m_spinBoxColor = color;
		break;
	case 0x207:  pColorTheme->m_spinBoxTextColor = color;
		break;
	case 0x208:  pColorTheme->m_automationColor = color;
		break;
	case 0x209:  pColorTheme->m_automationCircleColor = color;
		break;
	case 0x300:  pColorTheme->m_songEditor_backgroundColor = color;
		break;
	case 0x301:  pColorTheme->m_songEditor_alternateRowColor = color;
		break;
	case 0x302:  pColorTheme->m_songEditor_selectedRowColor = color;
		break;
	case 0x303:  pColorTheme->m_songEditor_lineColor = color;
		break;
	case 0x304:  pColorTheme->m_songEditor_textColor = color;
		break;
	case 0x400:  pColorTheme->m_patternEditor_backgroundColor = color;
		break;
	case 0x401:  pColorTheme->m_patternEditor_alternateRowColor = color;
		break;
	case 0x402:  pColorTheme->m_patternEditor_selectedRowColor = color;
		break;
	case 0x403:  pColorTheme->m_patternEditor_textColor = color;
		break;
	case 0x404:  pColorTheme->m_patternEditor_noteColor = color;
		break;
	case 0x405:  pColorTheme->m_patternEditor_noteoffColor = color;
		break;
	case 0x406:  pColorTheme->m_patternEditor_lineColor = color;
		break;
	case 0x407:  pColorTheme->m_patternEditor_line1Color = color;
		break;
	case 0x408:  pColorTheme->m_patternEditor_line2Color = color;
		break;
	case 0x409:  pColorTheme->m_patternEditor_line3Color = color;
		break;
	case 0x40a:  pColorTheme->m_patternEditor_line4Color = color;
		break;
	case 0x40b:  pColorTheme->m_patternEditor_line5Color = color;
		break;
	default: DEBUGLOG( "Unknown ID" );
	}
}

void PreferencesDialog::setColorTreeItemDirty( ColorTreeItem* pItem) {
	if( pItem == nullptr) {
		ERRORLOG( "NULL item" );
		return;
	}
	
	int nId = pItem->getId();
	if( nId == 0 ) {
		// Node without a color used as a heading.
		return;
	}

	QColor* pCurrentColor = getColorById( nId, m_pCurrentTheme->getColorTheme() );
	if ( pCurrentColor == nullptr ) {
		ERRORLOG( QString( "Unable to get current color for id [%1]" ).arg( nId ) );
		return;
	}
	QColor* pPreviousColor = getColorById( nId, m_pPreviousTheme->getColorTheme() );
	if ( pPreviousColor == nullptr ) {
		ERRORLOG( QString( "Unable to get previous color for id [%1]" ).arg( nId ) );
		return;
	}

	const QColor& currentColor = *pCurrentColor;
	const QColor& previousColor = *pPreviousColor;
  
	QFont font = pItem->font( 0 );
	font.setWeight( currentColor != previousColor ? QFont::Black : QFont::Normal );
	font.setItalic( currentColor != previousColor );
	pItem->setFont( 0, font );
	pItem->setData( 0, Qt::DecorationRole, currentColor );
}

void PreferencesDialog::updateColorTree() {
	QTreeWidgetItemIterator it( colorTree );
	while ( *it ) {
		setColorTreeItemDirty( static_cast<ColorTreeItem*>( *it ) );
		++it;
	}
}

void PreferencesDialog::colorTreeSelectionChanged() {
	ColorTreeItem* pItem = static_cast<ColorTreeItem*>(colorTree->selectedItems()[0]);
	
	if( pItem == nullptr ) {
		// Unset title
		m_pCurrentColor = nullptr;
        updateColors();
        return;
	}
      
	int nId = static_cast<ColorTreeItem*>(pItem)->getId();
	m_nCurrentId = nId;

	if ( nId == 0x000 ) {
		// A text node without color was clicked.
		m_pCurrentColor = nullptr;
	} else {
		m_pCurrentColor = getColorById( nId, m_pCurrentTheme->getColorTheme() );
	}
	updateColors();
}

void PreferencesDialog::colorButtonChanged() {
	setColorById( m_nCurrentId, colorButton->getColor(), m_pCurrentTheme->getColorTheme() );
	m_pCurrentColor = getColorById( m_nCurrentId, m_pCurrentTheme->getColorTheme() );
	updateColors();
}

void PreferencesDialog::updateColors() {
      int r, g, b, h, s, v;

	  // If m_pCurrentColor is nullptr, it will be converted to false.
      rslider->setEnabled( static_cast<bool>(m_pCurrentColor) );
      gslider->setEnabled( static_cast<bool>(m_pCurrentColor) );
      bslider->setEnabled( static_cast<bool>(m_pCurrentColor) );
      hslider->setEnabled( static_cast<bool>(m_pCurrentColor) );
      sslider->setEnabled( static_cast<bool>(m_pCurrentColor) );
      vslider->setEnabled( static_cast<bool>(m_pCurrentColor) );
      rval->setEnabled( static_cast<bool>(m_pCurrentColor) );
      gval->setEnabled( static_cast<bool>(m_pCurrentColor) );
      bval->setEnabled( static_cast<bool>(m_pCurrentColor) );
      hval->setEnabled( static_cast<bool>(m_pCurrentColor) );
      sval->setEnabled( static_cast<bool>(m_pCurrentColor) );
      vval->setEnabled( static_cast<bool>(m_pCurrentColor) );
      colorButton->setEnabled( static_cast<bool>(m_pCurrentColor) );
      if ( m_pCurrentColor ==  nullptr ) {
		  DEBUGLOG( "No current color yet" );
		  return;
	  }

      QColor currentColor(*m_pCurrentColor);
      
      colorButton->setColor( currentColor );

      m_pCurrentColor->getRgb(&r, &g, &b);
      m_pCurrentColor->getHsv(&h, &s, &v);

      rslider->blockSignals(true);
      gslider->blockSignals(true);
      bslider->blockSignals(true);
      hslider->blockSignals(true);
      sslider->blockSignals(true);
      vslider->blockSignals(true);
      rval->blockSignals(true);
      gval->blockSignals(true);
      bval->blockSignals(true);
      hval->blockSignals(true);
      sval->blockSignals(true);
      vval->blockSignals(true);

      rslider->setValue(r);
      gslider->setValue(g);
      bslider->setValue(b);
      hslider->setValue(h);
      sslider->setValue(s);
      vslider->setValue(v);
      rval->setValue(r);
      gval->setValue(g);
      bval->setValue(b);
      hval->setValue(h);
      sval->setValue(s);
      vval->setValue(v);

      rslider->blockSignals(false);
      gslider->blockSignals(false);
      bslider->blockSignals(false);
      hslider->blockSignals(false);
      sslider->blockSignals(false);
      vslider->blockSignals(false);
      rval->blockSignals(false);
      gval->blockSignals(false);
      bval->blockSignals(false);
      hval->blockSignals(false);
      sval->blockSignals(false);
      vval->blockSignals(false);

	  updateColorTree();
	  H2Core::Preferences::get_instance()->setColorTheme( m_pCurrentTheme->getColorTheme() );
	  HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Colors );
}

void PreferencesDialog::triggerColorSliderTimer() {
	if ( m_pColorSliderTimer->isActive() ) {
		m_pColorSliderTimer->stop();
	}
	m_pColorSliderTimer->start( 25 );
}

void PreferencesDialog::rsliderChanged( int nValue ) {
	int r, g, b;
	if ( m_pCurrentColor != nullptr ) {
		m_pCurrentColor->getRgb( &r, &g, &b );
		m_pCurrentColor->setRgb( nValue, g, b );
	}
	triggerColorSliderTimer();
}

void PreferencesDialog::gsliderChanged( int nValue ) {
	int r, g, b;
	if ( m_pCurrentColor != nullptr ) {
		m_pCurrentColor->getRgb( &r, &g, &b );
		m_pCurrentColor->setRgb( r, nValue, b );
	}
	triggerColorSliderTimer();
}

void PreferencesDialog::bsliderChanged( int nValue ) {
	int r, g, b;
	if ( m_pCurrentColor != nullptr ) {
		m_pCurrentColor->getRgb( &r, &g, &b );
		m_pCurrentColor->setRgb( r, g, nValue );
	}
	triggerColorSliderTimer();
}

void PreferencesDialog::hsliderChanged( int nValue ) {
	int h, s, v;
	if ( m_pCurrentColor != nullptr ) {
		m_pCurrentColor->getHsv( &h, &s, &v );
		m_pCurrentColor->setHsv( nValue, s, v );
	}
	triggerColorSliderTimer();
}

void PreferencesDialog::ssliderChanged( int nValue ) {
	int h, s, v;
	if ( m_pCurrentColor != nullptr ) {
		m_pCurrentColor->getHsv( &h, &s, &v );
		m_pCurrentColor->setHsv( h, nValue, v );
	}
	triggerColorSliderTimer();
}

void PreferencesDialog::vsliderChanged( int nValue ) {
	int h, s, v;
	if ( m_pCurrentColor != nullptr ) {
		m_pCurrentColor->getHsv( &h, &s, &v );
		m_pCurrentColor->setHsv( h, s, nValue );
	}
	triggerColorSliderTimer();
}

void PreferencesDialog::importTheme() {
	QString sPath = H2Core::Preferences::get_instance()->getLastImportThemeDirectory();
	if ( ! H2Core::Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::sys_theme_dir();
	}
	
	QString sTitle = tr( "Import Theme" );
	QFileDialog fd( this );
	fd.setWindowTitle( sTitle );
	fd.setDirectory( sPath );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setNameFilter( Filesystem::themes_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::sys_theme_dir() ) );
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::usr_theme_dir() ) );
	fd.setDefaultSuffix( Filesystem::themes_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QFileInfo fileInfo = fd.selectedFiles().first();
	QString sSelectedPath = fileInfo.absoluteFilePath();
	H2Core::Preferences::get_instance()->setLastImportThemeDirectory( fd.directory().absolutePath() );

	if ( sSelectedPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Theme couldn't be found.") );
		return;
	}

	auto pTheme = Theme::importTheme( sSelectedPath );
	m_pCurrentTheme = std::make_shared<Theme>( pTheme );
	H2Core::Preferences::get_instance()->setTheme( m_pCurrentTheme );
	if ( m_nCurrentId == 0 ) {
		m_pCurrentColor = nullptr;
		updateColorTree();
		H2Core::Preferences::get_instance()->setColorTheme( m_pCurrentTheme->getColorTheme() );
		HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Colors );
	}
	updateAppearanceTab( m_pCurrentTheme );

	HydrogenApp::get_instance()->setScrollStatusBarMessage( tr( "Theme imported from " ) + sSelectedPath, 2000 );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
	
}

void PreferencesDialog::exportTheme() {
	QString sPath = H2Core::Preferences::get_instance()->getLastExportThemeDirectory();
	if ( ! H2Core::Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::usr_theme_dir();
	}
	QString sTitle = tr( "Export Theme" );
	QFileDialog fd( this );
	fd.setWindowTitle( sTitle );
	fd.setDirectory( sPath );
	fd.setFileMode( QFileDialog::AnyFile );
	fd.setNameFilter( Filesystem::themes_filter_name );
	fd.setAcceptMode( QFileDialog::AcceptSave );
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::sys_theme_dir() ) );
	fd.setSidebarUrls( fd.sidebarUrls() << QUrl::fromLocalFile( Filesystem::usr_theme_dir() ) );
	fd.setDefaultSuffix( Filesystem::themes_ext );

	if ( fd.exec() != QDialog::Accepted ) {
		return;
	}

	QFileInfo fileInfo = fd.selectedFiles().first();
	QString sSelectedPath = fileInfo.absoluteFilePath();

	if ( sSelectedPath.isEmpty() ) {
		QMessageBox::warning( this, "Hydrogen", tr("Theme can not be exported.") );
		return;
	}

	H2Core::Preferences::get_instance()->setLastExportThemeDirectory( fd.directory().absolutePath() );
	Theme::exportTheme( sSelectedPath, m_pCurrentTheme );

	HydrogenApp::get_instance()->setScrollStatusBarMessage( tr( "Theme exported to " ) + sSelectedPath, 1200 );
	
}

void PreferencesDialog::resetTheme() {
	m_pCurrentTheme = std::make_shared<Theme>( m_pPreviousTheme );
	H2Core::Preferences::get_instance()->setTheme( m_pCurrentTheme );
	updateAppearanceTab( m_pCurrentTheme );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );

	HydrogenApp::get_instance()->setStatusBarMessage( tr( "Theme reseted" ), 10000 );
}


void PreferencesDialog::updateAppearanceTab( const std::shared_ptr<H2Core::Theme> pTheme ) {
	
	// Colors
	m_pCurrentColor = getColorById( m_nCurrentId, pTheme->getColorTheme() );
	updateColors();

	// Interface
	float fFalloffSpeed = pTheme->getInterfaceTheme()->m_fMixerFalloffSpeed;
	if ( fFalloffSpeed == InterfaceTheme::FALLOFF_SLOW ) {
		mixerFalloffComboBox->setCurrentIndex( 0 );
	}
	else if ( fFalloffSpeed == InterfaceTheme::FALLOFF_NORMAL ) {
		mixerFalloffComboBox->setCurrentIndex( 1 );
	}
	else if ( fFalloffSpeed == InterfaceTheme::FALLOFF_FAST ) {
		mixerFalloffComboBox->setCurrentIndex( 2 );
	}
	else {
		ERRORLOG( QString("PreferencesDialog: wrong mixerFalloff value = %1").arg( fFalloffSpeed ) );
	}
	uiLayoutComboBox->setCurrentIndex( static_cast<int>( pTheme->getInterfaceTheme()->m_layout ) );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	uiScalingPolicyComboBox->setCurrentIndex( static_cast<int>( pTheme->getInterfaceTheme()->m_scalingPolicy ) );
#else
	uiScalingPolicyComboBox->setEnabled( false );
	uiScalingPolicyLabel->setEnabled( false );
#endif

	iconColorComboBox->setCurrentIndex( static_cast<int>( pTheme->getInterfaceTheme()->m_iconColor ) );
	
	// Style
	QStringList list = QStyleFactory::keys();
	uint i = 0;
	styleComboBox->clear();
	for ( QStringList::Iterator it = list.begin(); it != list.end(); it++) {
		styleComboBox->addItem( *it );
		QString sStyle = (*it);
		if (sStyle == pTheme->getInterfaceTheme()->m_sQTStyle ) {
			styleComboBox->setCurrentIndex( i );
			HydrogenApp::get_instance()->getMainForm()->m_pQApp->setStyle( sStyle );
		}
		i++;
	}

	//SongEditor coloring
	int nColoringMethod = static_cast<int>(pTheme->getInterfaceTheme()->m_coloringMethod);
	if ( nColoringMethod == 0 ) {
		// "Automatic" selected 
		coloringMethodAuxSpinBox->hide();
		colorSelectionLabel->hide();
	} else {
		coloringMethodAuxSpinBox->show();
		colorSelectionLabel->show();
	}

	coloringMethodCombo->setCurrentIndex( nColoringMethod );
	coloringMethodAuxSpinBox->setValue( pTheme->getInterfaceTheme()->m_nVisiblePatternColors );
	QSize size( uiScalingPolicyComboBox->width(), coloringMethodAuxSpinBox->height() );

	if ( m_colorSelectionButtons.size() !=
		 pTheme->getInterfaceTheme()->m_nMaxPatternColors ) {
	
		m_colorSelectionButtons.resize( pTheme->getInterfaceTheme()->m_nMaxPatternColors );
		m_colorSelectionButtons.clear();
		int nButtonSize = fontSizeComboBox->height();
		// float fLineWidth = static_cast<float>(fontSizeComboBox->width());
		// Using a fixed one size resizing of the widget seems to happen
		// after the constructor is called.
		float fLineWidth = 308;
		int nButtonsPerLine = std::floor( fLineWidth / static_cast<float>(nButtonSize + 6) );

		colorSelectionGrid->setHorizontalSpacing( 4 );
		for ( int ii = 0; ii < pTheme->getInterfaceTheme()->m_nMaxPatternColors; ii++ ) {
			ColorSelectionButton* bbutton =
				new ColorSelectionButton( this, pTheme->getInterfaceTheme()->m_patternColors[ ii ],
										  nButtonSize );
			bbutton->hide();
			connect( bbutton, &ColorSelectionButton::colorChanged, this,
					 &PreferencesDialog::onColorSelectionClicked );
			colorSelectionGrid->addWidget( bbutton,
										   std::floor( static_cast<float>( ii ) /
													   static_cast<float>( nButtonsPerLine ) ),
										   (ii % nButtonsPerLine) + 1); //+1 to take the hspace into account.
			m_colorSelectionButtons[ ii ] = bbutton;
		}
	}

	if ( nColoringMethod != 0 ) {
		for ( int ii = 0; ii < pTheme->getInterfaceTheme()->m_nVisiblePatternColors; ii++ ) {
			m_colorSelectionButtons[ ii ]->show();
		}
	}

	// Fonts
	applicationFontComboBox->setCurrentFont( QFont( pTheme->getFontTheme()->m_sApplicationFontFamily ) );
	level2FontComboBox->setCurrentFont( QFont( pTheme->getFontTheme()->m_sLevel2FontFamily ) );
	level3FontComboBox->setCurrentFont( QFont( pTheme->getFontTheme()->m_sLevel3FontFamily ) );
	switch( pTheme->getFontTheme()->m_fontSize ) {
	case FontTheme::FontSize::Small:
		fontSizeComboBox->setCurrentIndex( 0 );
		break;
	case FontTheme::FontSize::Normal:
		fontSizeComboBox->setCurrentIndex( 1 );
		break;
	case FontTheme::FontSize::Large:
		fontSizeComboBox->setCurrentIndex( 2 );
		break;
	default:
		ERRORLOG( QString( "Unknown font size: %1" )
				  .arg( static_cast<int>( pTheme->getFontTheme()->m_fontSize ) ) );
	}
}
