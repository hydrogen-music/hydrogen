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
 * (at your option  any later version.
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
#include "../CommonStrings.h"

#include "qmessagebox.h"
#include "qstylefactory.h"

#include <QPixmap>
#include <QFontDatabase>
#include <QTreeWidgetItemIterator>
#include "../Widgets/MidiTable.h"

#include <core/EventQueue.h>
#include <core/MidiMap.h>
#include <core/Hydrogen.h>
#include <core/IO/MidiInput.h>
#include <core/Lash/LashClient.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Translations.h>
#include <core/Sampler/Sampler.h>
#include "../SongEditor/SongEditor.h"
#include "../SongEditor/SongEditorPanel.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/FileDialog.h"

#include <core/IO/PortAudioDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/AlsaAudioDriver.h>


using namespace H2Core;


DeviceComboBox::DeviceComboBox( QWidget *pParent )
	: LCDCombo( pParent)
{
	m_driver = Preferences::AudioDriver::None;
}

void DeviceComboBox::showPopup()
{
	clear();
	QApplication::setOverrideCursor( Qt::WaitCursor );
	if ( m_driver == Preferences::AudioDriver::PortAudio ) {
#ifdef H2CORE_HAVE_PORTAUDIO
		// Get device list for PortAudio based on current value of the API combo box
		for ( QString s : PortAudioDriver::getDevices( m_sHostAPI ) ) {
			addItem( s );
		}
#endif
	}
	else if ( m_driver == Preferences::AudioDriver::CoreAudio ) {
#ifdef H2CORE_HAVE_COREAUDIO
		for ( QString s : CoreAudioDriver::getDevices() ) {
			addItem( s );
		}
#endif
	}
	else if ( m_driver == Preferences::AudioDriver::Alsa ) {
#ifdef H2CORE_HAVE_ALSA
		for ( QString s : AlsaAudioDriver::getDevices() ) {
			addItem( s );
		}
#endif
	}
	QApplication::restoreOverrideCursor();
	LCDCombo::showPopup();
}


HostAPIComboBox::HostAPIComboBox( QWidget *pParent )
	: LCDCombo( pParent )
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
	addItems( PortAudioDriver::getHostAPIs() );
	QApplication::restoreOverrideCursor();
#endif

	LCDCombo::showPopup();
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
	, m_nCurrentId( 0 )
	, m_changes( H2Core::Preferences::Changes::None )
	, m_bMidiTableChanged( false ) {
	
	m_pCurrentTheme = std::make_shared<H2Core::Theme>( H2Core::Preferences::get_instance()->getTheme() );
	m_pPreviousTheme = std::make_shared<H2Core::Theme>( H2Core::Preferences::get_instance()->getTheme() );
	setupUi( this );

	setWindowTitle( tr( "Preferences" ) );
	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	connect( this, &PreferencesDialog::rejected, this, &PreferencesDialog::onRejected );

	Preferences *pPref = Preferences::get_instance();
	
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();

	///////
	// General tab
	QSize generalTabWidgetSize( 60, 24 );
	
	restoreLastUsedSongCheckbox->setChecked( pPref->isRestoreLastSongEnabled() );
	restoreLastUsedPlaylistCheckbox->setChecked( pPref->isRestoreLastPlaylistEnabled() );
	useRelativePlaylistPathsCheckbox->setChecked( pPref->isPlaylistUsingRelativeFilenames() );
	hideKeyboardCursor->setChecked( pPref->hideKeyboardCursor() );

	// General tab - restore the right m_bsetlash value
	if ( pPref->m_brestartLash == true ){
		if (pPref->m_bsetLash == false ){
			pPref->m_bsetLash = true ;
			pPref->m_brestartLash = false;
		}

	}
	useLashCheckbox->setChecked( pPref->m_bsetLash );

	sBcountOffset->setSize( generalTabWidgetSize );
	sBcountOffset->setValue( pPref->m_countOffset );
	sBstartOffset->setSize( generalTabWidgetSize );
	sBstartOffset->setValue( pPref->m_startOffset );

	sBmaxBars->setSize( generalTabWidgetSize );
	sBmaxBars->setValue( pPref->getMaxBars() );
	sBmaxLayers->setSize( generalTabWidgetSize );
	sBmaxLayers->setValue( pPref->getMaxLayers() );
	autosaveSpinBox->setSize( generalTabWidgetSize );
	autosaveSpinBox->setValue( pPref->m_nAutosavesPerHour );

	QString pathtoRubberband = pPref->m_rubberBandCLIexecutable;
	rubberbandLineEdit->setText( pathtoRubberband );

#ifdef H2CORE_HAVE_RUBBERBAND
	pathToRubberbandExLable->hide();
	rubberbandLineEdit->hide();
#endif

	// General tab - Language selection menu
	languageComboBox->setSize( QSize( 310, 24 ) );
	for ( QString sLang : Translations::availableTranslations( "hydrogen" ) ) {
		QLocale loc( sLang );
		QString sLabel = loc.nativeLanguageName() + " (" + loc.nativeCountryName() + ')';
		languageComboBox->addItem( sLabel, QVariant( sLang ) );
	}
	// General tab - Find preferred language and select that in menu
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

	//////
	// Audio tab

	// For everything above the restart button
	QSize audioTabWidgetSizeTop( 240, 24 );
	// and everything below
	QSize audioTabWidgetSizeBottom( 184, 24 );

	driverComboBox->setSize( audioTabWidgetSizeTop );
	driverComboBox->clear();
	driverComboBox->addItem(
		Preferences::audioDriverToQString( Preferences::AudioDriver::Auto ) );
	for ( const auto& ddriver : Preferences::getSupportedAudioDrivers() ) {
		driverComboBox->addItem(
			Preferences::audioDriverToQString( ddriver ) );
	}

	const auto sAudioDriver =
		Preferences::audioDriverToQString( pPref->m_audioDriver );
	const auto nAudioDriverIndex = driverComboBox->findText( sAudioDriver );
	if ( nAudioDriverIndex > -1 ) {
		driverComboBox->setCurrentIndex( nAudioDriverIndex );
	}
	else {
		driverInfoLbl->setText( tr("Select your Audio Driver" ));
		ERRORLOG( QString( "Unknown audio driver from preferences [%1]" )
				  .arg( sAudioDriver ) );
	}
	connect( driverComboBox, SIGNAL(activated(int)), this,
			 SLOT(driverComboBoxActivated(int)));

	// Audio tab - Set the PortAudio HostAPI combo box to the current
	// selected value.
	portaudioHostAPIComboBox->setSize( audioTabWidgetSizeTop );
	portaudioHostAPIComboBox->setValue( pPref->m_sPortAudioHostAPI );
	connect( portaudioHostAPIComboBox, SIGNAL(activated(int)), this,
			 SLOT(portaudioHostAPIComboBoxActivated(int)));
	
	m_pAudioDeviceTxt->setSize( audioTabWidgetSizeTop );
	m_pAudioDeviceTxt->setHostAPI( pPref->m_sPortAudioHostAPI );
	connect( m_pAudioDeviceTxt, SIGNAL(currentTextChanged(const QString&)),
			 this, SLOT( audioDeviceTxtChanged(const QString&)));

	latencyTargetSpinBox->setSize( QSize( 55, 23 ) );
	latencyTargetSpinBox->setValue( pPref->m_nLatencyTarget );
	connect( latencyTargetSpinBox, SIGNAL(valueChanged(int)), this,
			 SLOT(latencyTargetSpinBoxValueChanged(int)));

	bufferSizeSpinBox->setSize( audioTabWidgetSizeTop );
	bufferSizeSpinBox->setValue( pPref->m_nBufferSize );
	connect( bufferSizeSpinBox, SIGNAL(valueChanged(int)), this,
			 SLOT(bufferSizeSpinBoxValueChanged(int)));

	sampleRateComboBox->setSize( audioTabWidgetSizeTop );
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
	connect( sampleRateComboBox, SIGNAL(editTextChanged(const QString&)), this,
			 SLOT(sampleRateComboBoxEditTextChanged(const QString&)));

	// Audio tab - JACK
	trackOutsCheckBox->setChecked( pPref->m_bJackTrackOuts );
	connect(trackOutsCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleTrackOutsCheckBox( bool )));

	connectDefaultsCheckBox->setChecked( pPref->m_bJackConnectDefaults );
	enableTimebaseCheckBox->setChecked( pPref->m_bJackTimebaseEnabled );

	trackOutputComboBox->setSize( audioTabWidgetSizeTop );
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
	// Audio tab - ~JACK

	// Audio tab - metronome volume
	metronomeVolumeSpinBox->setSize( audioTabWidgetSizeBottom );
	uint metronomeVol = (uint)( pPref->m_fMetronomeVolume * 100.0 );
	metronomeVolumeSpinBox->setValue(metronomeVol);

	// Audio tab - max voices
	maxVoicesTxt->setSize( audioTabWidgetSizeBottom );
	maxVoicesTxt->setValue( pPref->m_nMaxNotes );

	resampleComboBox->setSize( audioTabWidgetSizeBottom );
	resampleComboBox->setCurrentIndex( static_cast<int>(pHydrogen->getAudioEngine()->getSampler()->getInterpolateMode() ) );

	updateDriverInfo();

	//////////////////////////////////////////////////////////////////
	// MIDI tab
	//////////////////////////////////////////////////////////////////
	QSize midiTabWidgetSize = QSize( 309, 24 );
	m_pMidiDriverComboBox->setSize( midiTabWidgetSize );
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

	if ( m_pMidiDriverComboBox->findText(pPref->m_sMidiDriver) > -1 ) {
		m_pMidiDriverComboBox->setCurrentIndex(m_pMidiDriverComboBox->findText(pPref->m_sMidiDriver));
	} else {
		driverInfoLbl->setText( tr("Select your MIDI Driver" ) );
		ERRORLOG( "Unknown MIDI input from preferences [" + pPref->m_sMidiDriver + "]" );
	}
	connect(m_pMidiDriverComboBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT( onMidiDriverComboBoxIndexChanged(int) ));
	
	midiPortChannelComboBox->setSize( midiTabWidgetSize );
	midiPortChannelComboBox->setEnabled( false );
	midiPortComboBox->setSize( midiTabWidgetSize );
	midiPortComboBox->setEnabled( false );
	connect( midiPortComboBox, SIGNAL(activated(int)), this,
			 SLOT(midiPortComboBoxActivated(int)));
	
	// MIDI tab - list midi input ports
	midiPortComboBox->clear();
	midiPortComboBox->addItem( pCommonStrings->getPreferencesNone() );
	if ( pHydrogen->getMidiInput() != nullptr ) {
		std::vector<QString> midiOutList = pHydrogen->getMidiInput()->getOutputPortList();

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
	
	// MIDI tab - list midi output ports
	midiOutportComboBox->setSize( midiTabWidgetSize );
	midiOutportComboBox->clear();
	midiOutportComboBox->addItem( pCommonStrings->getPreferencesNone() );
	if ( pHydrogen->getMidiOutput() != nullptr ) {
		std::vector<QString> midiOutList = pHydrogen->getMidiOutput()->getInputPortList();

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
	connect( midiOutportComboBox, SIGNAL(activated(int)), this,
			 SLOT(midiOutportComboBoxActivated(int)));

	if ( pPref->m_nMidiChannelFilter == -1 ) {
		midiPortChannelComboBox->setCurrentIndex( 0 );
	}
	else {
		midiPortChannelComboBox->setCurrentIndex( pPref->m_nMidiChannelFilter + 1 );
	}

	m_pIgnoreNoteOffCheckBox->setChecked( pPref->m_bMidiNoteOffIgnore );
	m_pEnableMidiFeedbackCheckBox->setChecked( pPref->m_bEnableMidiFeedback );
	m_pDiscardMidiMsgCheckbox->setChecked( pPref->m_bMidiDiscardNoteAfterAction );
	m_pFixedMapping->setChecked( pPref->m_bMidiFixedMapping );

	connect( midiTable, &MidiTable::changed,
			 [=]() {
				 m_changes =
					 static_cast<H2Core::Preferences::Changes>(
							m_changes | H2Core::Preferences::Changes::MidiTab );
				 m_bMidiTableChanged = true;
			 });

	//////
	// OSC tab
	enableOscCheckbox->setChecked( pPref->getOscServerEnabled() );
	enableOscFeedbackCheckbox->setChecked( pPref->getOscFeedbackEnabled() );
	connect(enableOscCheckbox, SIGNAL(toggled(bool)), this, SLOT(toggleOscCheckBox( bool )));
	incomingOscPortSpinBox->setSize( QSize( 66, 24 ) );
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

	/////
	// Appearance tab
	QSize appearanceTabWidgetSize = QSize( 277, 24 );
	connect( importThemeButton, SIGNAL(clicked(bool)), SLOT(importTheme()));
	connect( exportThemeButton, SIGNAL(clicked(bool)), SLOT(exportTheme()));
	connect( resetThemeButton, SIGNAL(clicked(bool)), this, SLOT(resetTheme()));
	
	// Appearance tab - Fonts
	fontSizeComboBox->setSize( appearanceTabWidgetSize );
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

	styleComboBox->setSize( appearanceTabWidgetSize );
	mixerFalloffComboBox->setSize( appearanceTabWidgetSize );
	uiLayoutComboBox->setSize( appearanceTabWidgetSize );
	uiScalingPolicyComboBox->setSize( appearanceTabWidgetSize );
	iconColorComboBox->setSize( appearanceTabWidgetSize );
	coloringMethodAuxSpinBox->setSize( appearanceTabWidgetSize );

	connect( styleComboBox, SIGNAL( activated(int) ), this,
			 SLOT( styleComboBoxActivated(int) ) );
	connect( uiLayoutComboBox, SIGNAL( currentIndexChanged(int) ), this,
			 SLOT( onUILayoutChanged(int) ) );
	connect( uiScalingPolicyComboBox, SIGNAL( currentIndexChanged(int) ), this,
			 SLOT( uiScalingPolicyComboBoxCurrentIndexChanged(int) ) );
	connect( mixerFalloffComboBox, SIGNAL( currentIndexChanged(int) ), this,
			 SLOT( mixerFalloffComboBoxCurrentIndexChanged(int) ) );
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
	
	coloringMethodCombo->setSize( appearanceTabWidgetSize );
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
	pTopLevelItem = new ColorTreeItem( 0x000, colorTree, tr( "General" ) );
	new ColorTreeItem( 0x100, pTopLevelItem, tr( "Window" ) );
	new ColorTreeItem( 0x101, pTopLevelItem, tr( "Window Text" ) );
	new ColorTreeItem( 0x102, pTopLevelItem, tr( "Base" ) );
	new ColorTreeItem( 0x103, pTopLevelItem, tr( "Alternate Base" ) );
	new ColorTreeItem( 0x104, pTopLevelItem, tr( "Text" ) );
	new ColorTreeItem( 0x105, pTopLevelItem, tr( "Button" ) );
	new ColorTreeItem( 0x106, pTopLevelItem, tr( "Button Text" ) );
	new ColorTreeItem( 0x107, pTopLevelItem, tr( "Light" ) );
	new ColorTreeItem( 0x108, pTopLevelItem, tr( "Mid Light" ) );
	new ColorTreeItem( 0x109, pTopLevelItem, tr( "Mid" ) );
	new ColorTreeItem( 0x10a, pTopLevelItem, tr( "Dark" ) );
	new ColorTreeItem( 0x10b, pTopLevelItem, tr( "Shadow Text" ) );
	new ColorTreeItem( 0x10c, pTopLevelItem, tr( "Highlight" ) );
	new ColorTreeItem( 0x10d, pTopLevelItem, tr( "Highlight Text" ) );
	new ColorTreeItem( 0x10e, pTopLevelItem, tr( "Selection Highlight" ) );
	new ColorTreeItem( 0x10f, pTopLevelItem, tr( "Selection Inactive" ) );
	new ColorTreeItem( 0x110, pTopLevelItem, tr( "Tool Tip Base" ) );
	new ColorTreeItem( 0x111, pTopLevelItem, tr( "Tool Tip Text" ) );
	
	auto pWidgetItem = new ColorTreeItem( 0x000, colorTree, tr( "Widgets" ) );
	auto pDefaultItem = new ColorTreeItem( 0x200, pWidgetItem, tr( "Widget" ) );
	new ColorTreeItem( 0x201, pWidgetItem, tr( "Widget Text" ) );
	new ColorTreeItem( 0x202, pWidgetItem, tr( "Accent" ) );
	new ColorTreeItem( 0x203, pWidgetItem, tr( "Accent Text" ) );
	new ColorTreeItem( 0x204, pWidgetItem, tr( "Button Red" ) );
	new ColorTreeItem( 0x205, pWidgetItem, tr( "Button Red Text" ) );
	new ColorTreeItem( 0x206, pWidgetItem, tr( "Spin Box" ) );
	new ColorTreeItem( 0x207, pWidgetItem, tr( "Spin Box Text" ) );
	new ColorTreeItem( 0x208, pWidgetItem, tr( "Playhead" ) );
	new ColorTreeItem( 0x209, pWidgetItem, tr( "Cursor" ) );
	
	pTopLevelItem = new ColorTreeItem( 0x000, colorTree, tr( "Song Editor" ) );
	new ColorTreeItem( 0x300, pTopLevelItem, tr( "Background" ) );
	new ColorTreeItem( 0x301, pTopLevelItem, tr( "Alternate Row" ) );
	new ColorTreeItem( 0x302, pTopLevelItem, tr( "Virtual Row" ) );
	new ColorTreeItem( 0x303, pTopLevelItem, tr( "Selected Row" ) );
	new ColorTreeItem( 0x304, pTopLevelItem, tr( "Selected Row Text" ) );
	new ColorTreeItem( 0x305, pTopLevelItem, tr( "Line" ) );
	new ColorTreeItem( 0x306, pTopLevelItem, tr( "Text" ) );
	new ColorTreeItem( 0x307, pTopLevelItem, tr( "Automation Background" ) );
	new ColorTreeItem( 0x308, pTopLevelItem, tr( "Automation Line" ) );
	new ColorTreeItem( 0x309, pTopLevelItem, tr( "Automation Node" ) );
	new ColorTreeItem( 0x30a, pTopLevelItem, tr( "Stacked Mode On" ) );
	new ColorTreeItem( 0x30b, pTopLevelItem, tr( "Stacked Mode On Next" ) );
	new ColorTreeItem( 0x30c, pTopLevelItem, tr( "Stacked Mode Off Next" ) );
	
	pTopLevelItem = new ColorTreeItem( 0x000, colorTree, tr( "Pattern Editor" ) );
	new ColorTreeItem( 0x400, pTopLevelItem, tr( "Background" ) );
	new ColorTreeItem( 0x401, pTopLevelItem, tr( "Alternate Row" ) );
	new ColorTreeItem( 0x402, pTopLevelItem, tr( "Selected Row" ) );
	new ColorTreeItem( 0x403, pTopLevelItem, tr( "Selected Row Text" ) );
	new ColorTreeItem( 0x404, pTopLevelItem, tr( "Octave Row" ) );
	new ColorTreeItem( 0x405, pTopLevelItem, tr( "Text" ) );
	new ColorTreeItem( 0x406, pTopLevelItem, tr( "Note (Full Velocity)" ) );
	new ColorTreeItem( 0x407, pTopLevelItem, tr( "Note (Default Velocity)" ) );
	new ColorTreeItem( 0x408, pTopLevelItem, tr( "Note (Half Velocity)" ) );
	new ColorTreeItem( 0x409, pTopLevelItem, tr( "Note (Zero Velocity)" ) );
	new ColorTreeItem( 0x40a, pTopLevelItem, tr( "Note Off" ) );
	new ColorTreeItem( 0x40b, pTopLevelItem, tr( "Grid Line 1" ) );
	new ColorTreeItem( 0x40c, pTopLevelItem, tr( "Grid Line 2" ) );
	new ColorTreeItem( 0x40d, pTopLevelItem, tr( "Grid Line 3" ) );
	new ColorTreeItem( 0x40e, pTopLevelItem, tr( "Grid Line 4" ) );
	new ColorTreeItem( 0x40f, pTopLevelItem, tr( "Grid Line 5" ) );
	new ColorTreeItem( 0x410, pTopLevelItem, tr( "Grid Line 6" ) );

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
	
	// We initialize the tree by expanding the "Widget" node and
	// selecting the first color. This looks more nice than with no
	// color selected at all and works better with the Shotlist.
	colorTree->expandItem( pWidgetItem );
	colorTree->setCurrentItem( pDefaultItem );

	updateColorTree();

	updateAppearanceTab( m_pCurrentTheme );

	m_bNeedDriverRestart = false;
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
	HydrogenApp::get_instance()->changePreferences( m_changes );

	reject();
}

void PreferencesDialog::audioDeviceTxtChanged( const QString& )
{
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::updateDriverPreferences() {
	Preferences *pPref = Preferences::get_instance();
	auto pAudioDriver = Hydrogen::get_instance()->getAudioOutput();

	bool bAudioOptionAltered = false;
	const auto prevAudioDriver = pPref->m_audioDriver;
	const auto selectedDriver = Preferences::parseAudioDriver(
		driverComboBox->currentText() );

	if ( prevAudioDriver != selectedDriver ) {
		if ( selectedDriver != Preferences::AudioDriver::None ) {
			pPref->m_audioDriver = selectedDriver;
			bAudioOptionAltered = true;
		}
		else {
			ERRORLOG( QString( "Invalid audio driver: [%1]" )
					  .arg( driverComboBox->currentText() ) );
		}
	}
	
	// Driver-specific settings
	if ( selectedDriver == Preferences::AudioDriver::Alsa ||
			  ( selectedDriver == Preferences::AudioDriver::Auto &&
				dynamic_cast<H2Core::AlsaAudioDriver*>(pAudioDriver) != nullptr ) ) {
		if ( pPref->m_sAlsaAudioDevice != m_pAudioDeviceTxt->lineEdit()->text() ) {
			pPref->m_sAlsaAudioDevice = m_pAudioDeviceTxt->lineEdit()->text();
			bAudioOptionAltered = true;
		}
	}
	else if ( selectedDriver == Preferences::AudioDriver::Oss ||
			  ( selectedDriver == Preferences::AudioDriver::Auto &&
				dynamic_cast<H2Core::OssDriver*>(pAudioDriver) != nullptr ) ) {
		if ( pPref->m_sOSSDevice != m_pAudioDeviceTxt->lineEdit()->text() ) {
			pPref->m_sOSSDevice = m_pAudioDeviceTxt->lineEdit()->text();
			bAudioOptionAltered = true;
		}
	}
	else if ( selectedDriver == Preferences::AudioDriver::PortAudio ||
			 ( selectedDriver == Preferences::AudioDriver::Auto &&
			   dynamic_cast<H2Core::PortAudioDriver*>(pAudioDriver) != nullptr ) ) {
		if ( pPref->m_sPortAudioDevice != m_pAudioDeviceTxt->lineEdit()->text() ) {
			pPref->m_sPortAudioDevice = m_pAudioDeviceTxt->lineEdit()->text();
			bAudioOptionAltered = true;
		}
		if ( pPref->m_sPortAudioHostAPI != portaudioHostAPIComboBox->currentText() ) {
			pPref->m_sPortAudioHostAPI = portaudioHostAPIComboBox->currentText();
			bAudioOptionAltered = true;
		}
		if ( pPref->m_nLatencyTarget != latencyTargetSpinBox->value() ) {
			pPref->m_nLatencyTarget = latencyTargetSpinBox->value();
			bAudioOptionAltered = true;
		}
	}
	else if (selectedDriver == Preferences::AudioDriver::CoreAudio ||
			 ( selectedDriver == Preferences::AudioDriver::Auto &&
			   dynamic_cast<H2Core::CoreAudioDriver*>(pAudioDriver) != nullptr ) ) {
		if ( pPref->m_sCoreAudioDevice != m_pAudioDeviceTxt->lineEdit()->text() ) {
			pPref->m_sCoreAudioDevice = m_pAudioDeviceTxt->lineEdit()->text();
			bAudioOptionAltered = true;
		}
	}

	// JACK
	if ( pPref->m_bJackConnectDefaults != connectDefaultsCheckBox->isChecked() ) {
		pPref->m_bJackConnectDefaults = connectDefaultsCheckBox->isChecked();
		bAudioOptionAltered = true;
	}

	if ( pPref->m_bJackTrackOuts != trackOutsCheckBox->isChecked() ) {
		pPref->m_bJackTrackOuts = trackOutsCheckBox->isChecked();
		bAudioOptionAltered = true;
	}
	
	if ( pPref->m_bJackTimebaseEnabled != enableTimebaseCheckBox->isChecked() ) {
		pPref->m_bJackTimebaseEnabled = enableTimebaseCheckBox->isChecked();
		bAudioOptionAltered = true;
	}

	switch ( trackOutputComboBox->currentIndex() ) {
	case 0:
		if ( pPref->m_JackTrackOutputMode !=
			 Preferences::JackTrackOutputMode::postFader ) {
			pPref->m_JackTrackOutputMode =
				Preferences::JackTrackOutputMode::postFader;
			bAudioOptionAltered = true;
		}
		break;
	case 1:
		if ( pPref->m_JackTrackOutputMode !=
			 Preferences::JackTrackOutputMode::preFader ) {
			pPref->m_JackTrackOutputMode =
				Preferences::JackTrackOutputMode::preFader;
			bAudioOptionAltered = true;
		}
		break;
	default:
		ERRORLOG( QString( "Unexpected track output value" ) );
	}
	// ~ JACK

	if ( pPref->m_nBufferSize != bufferSizeSpinBox->value() ) {
		pPref->m_nBufferSize = bufferSizeSpinBox->value();
		bAudioOptionAltered = true;
	}
	
	if ( sampleRateComboBox->currentText() == "44100" &&
		 pPref->m_nSampleRate != 44100 ) {
		pPref->m_nSampleRate = 44100;
		bAudioOptionAltered = true;
	}
	else if ( sampleRateComboBox->currentText() == "48000" &&
		 pPref->m_nSampleRate != 48000 ) {
		pPref->m_nSampleRate = 48000;
		bAudioOptionAltered = true;
	}
	else if ( sampleRateComboBox->currentText() == "88200" &&
		 pPref->m_nSampleRate != 88200 ) {
		pPref->m_nSampleRate = 88200;
		bAudioOptionAltered = true;
	}
	else if ( sampleRateComboBox->currentText() == "96000" &&
		 pPref->m_nSampleRate != 96000 ) {
		pPref->m_nSampleRate = 96000;
		bAudioOptionAltered = true;
	}

	if ( bAudioOptionAltered ) {
		m_changes =
			static_cast<H2Core::Preferences::Changes>(
				m_changes | H2Core::Preferences::Changes::AudioTab );
	}
}


void PreferencesDialog::on_okBtn_clicked()
{
	auto pH2App = HydrogenApp::get_instance();
	auto pCommonStrings = pH2App->getCommonStrings();
	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	//////////////////////////////////////////////////////////////////
	// Audio tab
	//////////////////////////////////////////////////////////////////
	bool bAudioOptionAltered = false;

	updateDriverPreferences();

	if ( m_bNeedDriverRestart ) {
		if ( QMessageBox::information(
				 this, "Hydrogen",
				 tr( "Driver restart required.\n Restart driver?"),
				 QMessageBox::Ok | QMessageBox::Cancel,
				 QMessageBox::Cancel ) == QMessageBox::Cancel ) {
			// Don't save the Preferences and don't close the PreferencesDialog
			return;
		}
	}

	// Check whether the current audio driver is valid
	if ( pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(pHydrogen->getAudioOutput()) != nullptr ) {
		if ( QMessageBox::warning(
				 this, "Hydrogen", QString( "%1\n" )
				 .arg( pCommonStrings->getAudioDriverNotPresent() )
				 .append( tr( "Are you sure you want to proceed?" ) ),
				 QMessageBox::Ok | QMessageBox::Cancel,
				 QMessageBox::Cancel ) == QMessageBox::Cancel ) {
			return;
		}
	}
	
	if ( pPref->m_fMetronomeVolume != metronomeVolumeSpinBox->value() / 100.0 ) {
		pPref->m_fMetronomeVolume = metronomeVolumeSpinBox->value() / 100.0;
		bAudioOptionAltered = true;
	}

	// Polyphony
	if ( pPref->m_nMaxNotes != maxVoicesTxt->value() ) {
		pPref->m_nMaxNotes = maxVoicesTxt->value();
		bAudioOptionAltered = true;
	}

	// Interpolation
	if ( static_cast<int>( pHydrogen->getAudioEngine()->getSampler()->getInterpolateMode() ) !=
		 resampleComboBox->currentIndex() ) {
		switch ( resampleComboBox->currentIndex() ){
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
		bAudioOptionAltered = true;
	}

	if ( bAudioOptionAltered ) {
		m_changes =
			static_cast<H2Core::Preferences::Changes>(
				m_changes | H2Core::Preferences::Changes::AudioTab );
	}
	
	//////////////////////////////////////////////////////////////////
	// MIDI tab
	//////////////////////////////////////////////////////////////////
	bool bMidiOptionAltered = false;

	MidiMap *mM = MidiMap::get_instance();
	mM->reset_instance();

	midiTable->saveMidiTable();
	if ( m_bMidiTableChanged ) {
		H2Core::EventQueue::get_instance()->push_event( H2Core::EVENT_MIDI_MAP_CHANGED, 0 );
	}

	if ( m_pMidiDriverComboBox->currentText() == "ALSA" &&
		 pPref->m_sMidiDriver != "ALSA" ) {
		pPref->m_sMidiDriver = "ALSA";
		bMidiOptionAltered = true;
	}
	else if ( m_pMidiDriverComboBox->currentText() == "PortMidi" &&
			  pPref->m_sMidiDriver != "PortMidi" ) {
		pPref->m_sMidiDriver = "PortMidi";
		bMidiOptionAltered = true;
	}
	else if ( m_pMidiDriverComboBox->currentText() == "CoreMIDI" &&
			  pPref->m_sMidiDriver != "CoreMIDI" ) {
		pPref->m_sMidiDriver = "CoreMIDI";
		bMidiOptionAltered = true;
	}
	else if ( m_pMidiDriverComboBox->currentText() == "JACK-MIDI" &&
			  pPref->m_sMidiDriver != "JACK-MIDI" ) {
		pPref->m_sMidiDriver = "JACK-MIDI";
		bMidiOptionAltered = true;
	}

	if ( pPref->m_bMidiNoteOffIgnore != m_pIgnoreNoteOffCheckBox->isChecked() ) {
		pPref->m_bMidiNoteOffIgnore = m_pIgnoreNoteOffCheckBox->isChecked();
		bMidiOptionAltered = true;
	}
	
	if ( pPref->m_bMidiFixedMapping != m_pFixedMapping->isChecked() ) {
		pPref->m_bMidiFixedMapping = m_pFixedMapping->isChecked();
		bMidiOptionAltered = true;
	}
	
	if ( pPref->m_bMidiDiscardNoteAfterAction !=
		 m_pDiscardMidiMsgCheckbox->isChecked() ) {
		pPref->m_bMidiDiscardNoteAfterAction = m_pDiscardMidiMsgCheckbox->isChecked();
		bMidiOptionAltered = true;
	}
		
	if ( pPref->m_bEnableMidiFeedback !=
		 m_pEnableMidiFeedbackCheckBox->isChecked() ) {
		pPref->m_bEnableMidiFeedback = m_pEnableMidiFeedbackCheckBox->isChecked();
		bMidiOptionAltered = true;
	}

	QString sNewMidiPortName = midiPortComboBox->currentText();
	if ( midiPortComboBox->currentIndex() == 0 ) {
		sNewMidiPortName = Preferences::getNullMidiPort();
	}
	if ( pPref->m_sMidiPortName != sNewMidiPortName ) {
		pPref->m_sMidiPortName = sNewMidiPortName;
		bMidiOptionAltered = true;
		m_bNeedDriverRestart = true;
	}
	
	QString sNewMidiOutputPortName = midiOutportComboBox->currentText();
	if ( midiOutportComboBox->currentIndex() == 0 ) {
		sNewMidiOutputPortName = Preferences::getNullMidiPort();
	}
	if ( pPref->m_sMidiOutputPortName != sNewMidiOutputPortName ) {
		pPref->m_sMidiOutputPortName = sNewMidiOutputPortName;
		bMidiOptionAltered = true;
		m_bNeedDriverRestart = true;
	}

	if ( pPref->m_nMidiChannelFilter !=
		 midiPortChannelComboBox->currentIndex() - 1 ) {
		pPref->m_nMidiChannelFilter = midiPortChannelComboBox->currentIndex() - 1;
		bMidiOptionAltered = true;
	}

	if ( bMidiOptionAltered ) {
		m_changes =
			static_cast<H2Core::Preferences::Changes>(
				m_changes | H2Core::Preferences::Changes::MidiTab );
	}
	
	//////////////////////////////////////////////////////////////////
	// OSC tab
	//////////////////////////////////////////////////////////////////
	bool bOscOptionAltered = false;

	if ( enableOscCheckbox->isChecked() != pPref->getOscServerEnabled() ) {
		pPref->setOscServerEnabled( enableOscCheckbox->isChecked() );
		pHydrogen->toggleOscServer( enableOscCheckbox->isChecked() );
		bOscOptionAltered = true;
	}
	
	if ( pPref->getOscFeedbackEnabled() !=
		 enableOscFeedbackCheckbox->isChecked() ) {
		pPref->setOscFeedbackEnabled( enableOscFeedbackCheckbox->isChecked() );
		bOscOptionAltered = true;
	}
	
	if ( incomingOscPortSpinBox->value() != pPref->getOscServerPort() ) {
		pPref->setOscServerPort( incomingOscPortSpinBox->value() );
		pHydrogen->recreateOscServer();
		bOscOptionAltered = true;
	}

	if ( bOscOptionAltered ) {
		m_changes =
			static_cast<H2Core::Preferences::Changes>(
				m_changes | H2Core::Preferences::Changes::OscTab );
	}
	
	//////////////////////////////////////////////////////////////////
	// General tab
	//////////////////////////////////////////////////////////////////
	bool bGeneralOptionAltered = false;
	
	if ( pPref->isRestoreLastSongEnabled() !=
		 restoreLastUsedSongCheckbox->isChecked() ) {
		pPref->setRestoreLastSongEnabled( restoreLastUsedSongCheckbox->isChecked() );
		bGeneralOptionAltered = true;
	}
	
	if ( pPref->isRestoreLastPlaylistEnabled() !=
		 restoreLastUsedPlaylistCheckbox->isChecked() ) {
		pPref->setRestoreLastPlaylistEnabled( restoreLastUsedPlaylistCheckbox->isChecked() );
		bGeneralOptionAltered = true;
	}
	
	if ( pPref->isPlaylistUsingRelativeFilenames() !=
		 useRelativePlaylistPathsCheckbox->isChecked() ) {
		pPref->setUseRelativeFilenamesForPlaylists( useRelativePlaylistPathsCheckbox->isChecked() );
		bGeneralOptionAltered = true;
	}
	
	if ( pPref->m_bsetLash != useLashCheckbox->isChecked() ) {
		pPref->m_bsetLash = useLashCheckbox->isChecked(); //restore m_bsetLash after saving pref
		bGeneralOptionAltered = true;
	}
	
	if ( pPref->hideKeyboardCursor() != hideKeyboardCursor->isChecked() ) {
		pPref->setHideKeyboardCursor( hideKeyboardCursor->isChecked() );
		bGeneralOptionAltered = true;
	}

	//path to rubberband
	if ( pPref->m_rubberBandCLIexecutable != rubberbandLineEdit->text() ) {
		pPref->m_rubberBandCLIexecutable = rubberbandLineEdit->text();
		bGeneralOptionAltered = true;
	}

	//check preferences
	if ( pPref->m_brestartLash == true ){
		pPref->m_bsetLash = true ;
	}

	if ( pPref->m_countOffset != sBcountOffset->value() ) {
		pPref->m_countOffset = sBcountOffset->value();
		pHydrogen->setBcOffsetAdjust();
		bGeneralOptionAltered = true;
	}
	
	if ( pPref->m_startOffset != sBstartOffset->value() ) {
		pPref->m_startOffset = sBstartOffset->value();
		pHydrogen->setBcOffsetAdjust();
		bGeneralOptionAltered = true;
	}

	if ( pPref->getMaxBars() != sBmaxBars->value() ) {
		pPref->setMaxBars( sBmaxBars->value() );
		bGeneralOptionAltered = true;
	}
	
	if ( pPref->getMaxLayers() != sBmaxLayers->value() ) {
		pPref->setMaxLayers( sBmaxLayers->value() );
		bGeneralOptionAltered = true;
	}

	if ( pPref->m_nAutosavesPerHour != autosaveSpinBox->value() ) {
		pPref->m_nAutosavesPerHour = autosaveSpinBox->value();
		m_changes =
			static_cast<H2Core::Preferences::Changes>( m_changes |
													   H2Core::Preferences::Changes::GeneralTab );
	}

	QString sPreferredLanguage = languageComboBox->currentData().toString();
	if ( sPreferredLanguage != m_sInitialLanguage ) {
		QMessageBox::information( this, "Hydrogen", tr( "Hydrogen must be restarted for language change to take effect" ));
		pPref->setPreferredLanguage( sPreferredLanguage );
		bGeneralOptionAltered = true;
	}

	if ( bGeneralOptionAltered ) {
		m_changes =
			static_cast<H2Core::Preferences::Changes>(
				m_changes | H2Core::Preferences::Changes::GeneralTab );
	}

	//////////////////////////////////////////////////////////////////

	pPref->setTheme( m_pCurrentTheme );

	if ( m_bNeedDriverRestart ) {
		// Restart audio and MIDI drivers now that we updated all
		// values in Preferences.
		QApplication::setOverrideCursor( Qt::WaitCursor );
		pHydrogen->restartDrivers();
		QApplication::restoreOverrideCursor();
	}

	pH2App->changePreferences( m_changes );
	
	pPref->savePreferences();
	accept();
}


void PreferencesDialog::driverComboBoxActivated( int index )
{
	UNUSED( index );
	updateDriverInfo();
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::portaudioHostAPIComboBoxActivated( int index )
{
	m_pAudioDeviceTxt->setHostAPI( portaudioHostAPIComboBox->currentText() );
	updateDriverInfo();
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::updateDriverInfo()
{
	Preferences *pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pAudioDriver = Hydrogen::get_instance()->getAudioOutput();

	// Reset info text
	updateDriverInfoLabel();

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

	const auto selectedAudioDriver = Preferences::parseAudioDriver(
		driverComboBox->currentText() );

	if ( selectedAudioDriver == Preferences::AudioDriver::Auto ) {

		if ( dynamic_cast<H2Core::JackAudioDriver*>(pAudioDriver) != nullptr ) {
			setDriverInfoJack();
		}
		else if ( dynamic_cast<H2Core::AlsaAudioDriver*>(pAudioDriver) != nullptr ) {
			setDriverInfoAlsa();
		}
		else if ( dynamic_cast<H2Core::PortAudioDriver*>(pAudioDriver) != nullptr ) {
			setDriverInfoPortAudio();
		}
		else if ( dynamic_cast<H2Core::CoreAudioDriver*>(pAudioDriver) != nullptr ) {
			setDriverInfoCoreAudio();
		}
		else if ( dynamic_cast<H2Core::PulseAudioDriver*>(pAudioDriver) != nullptr ) {
			setDriverInfoPulseAudio();
		}
		else if ( dynamic_cast<H2Core::OssDriver*>(pAudioDriver) != nullptr ) {
			setDriverInfoOss();
		}
		else {
		
			m_pAudioDeviceTxt->setDriver( Preferences::AudioDriver::Null );
			m_pAudioDeviceTxt->setIsActive( false );
			m_pAudioDeviceTxt->lineEdit()->setText( "" );
			bufferSizeSpinBox->setIsActive( false );
			sampleRateComboBox->setIsActive( false );
			bufferSizeSpinBox->setToolTip( "" );
			sampleRateComboBox->setToolTip( "" );
			trackOutputComboBox->setIsActive( false );
			trackOutputComboBox->hide();
			trackOutputLbl->hide();
			trackOutsCheckBox->setEnabled( false );
			trackOutsCheckBox->hide();
			connectDefaultsCheckBox->setEnabled( false );
			connectDefaultsCheckBox->hide();
			enableTimebaseCheckBox->setEnabled( false );
			enableTimebaseCheckBox->hide();
			portaudioHostAPIComboBox->hide();
			portaudioHostAPILabel->hide();
			latencyTargetLabel->hide();
			latencyTargetSpinBox->hide();
			latencyValueLabel->hide();
		}
	}
	else if ( selectedAudioDriver == Preferences::AudioDriver::Oss ) {
		setDriverInfoOss();
	}
	else if ( selectedAudioDriver == Preferences::AudioDriver::Jack ) {
		setDriverInfoJack();
	}
	else if ( selectedAudioDriver == Preferences::AudioDriver::Alsa ) {
		setDriverInfoAlsa();
	}
	else if ( selectedAudioDriver == Preferences::AudioDriver::PortAudio ) {
		setDriverInfoPortAudio();
	}
	else if ( selectedAudioDriver == Preferences::AudioDriver::CoreAudio ) {
		setDriverInfoCoreAudio();
	}
	else if ( selectedAudioDriver == Preferences::AudioDriver::PulseAudio ) {
		setDriverInfoPulseAudio();
	}
	else {
		ERRORLOG( QString( "Unknown driver [%1]" )
				  .arg( Preferences::audioDriverToQString( selectedAudioDriver ) ) );
	}

	metronomeVolumeSpinBox->setEnabled(true);
	bufferSizeSpinBox->setValue( pPref->m_nBufferSize );
}

void PreferencesDialog::updateDriverInfoLabel() {

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	auto pAudioDriver = Hydrogen::get_instance()->getAudioOutput();
	QString sInfo;

	if ( driverComboBox->currentText() ==
		 Preferences::audioDriverToQString( Preferences::AudioDriver::Auto ) ) {
		sInfo.append( tr("Automatic driver selection") )
			.append( "<br><br>" );
	}
	
	if ( dynamic_cast<H2Core::JackAudioDriver*>(pAudioDriver) != nullptr ) {		
		sInfo.append( "<b>" )
			.append( tr( "JACK Audio Connection Kit Driver" ) )
			.append( "</b><br>" )
			.append( tr( "Low latency audio driver" ) );
#ifndef H2CORE_HAVE_JACK
		sInfo.append( "<br><b><font color=" )
			.append( m_sColorRed ).append( ">")
			.append( pCommonStrings->getPreferencesNotCompiled() )
			.append( "</font></b>" );
#endif
	}
	else if ( dynamic_cast<H2Core::AlsaAudioDriver*>(pAudioDriver) != nullptr ) {
		sInfo.append( "<b>" ).append( tr( "ALSA Driver" ) )
			.append( "</b><br>" );
#ifndef H2CORE_HAVE_ALSA
		sInfo.append( "<br><b><font color=" )
			.append( m_sColorRed ).append( ">")
			.append( pCommonStrings->getPreferencesNotCompiled() )
			.append( "</font></b>" );
#else
		auto pAlsaDriver =
			dynamic_cast<H2Core::AlsaAudioDriver*>(pAudioDriver);
		if ( pAlsaDriver != nullptr ) {
			sInfo.append( "<br>" ).append( tr( "Currently connected to device: " ) )
				.append( "<b>" ).append( pAlsaDriver->m_sAlsaAudioDevice )
				.append( "</b>" );
		} else {
			ERRORLOG( "ALSA driver selected in PreferencesDialog but no ALSA driver running?" );
		}
#endif
	}
	else if ( dynamic_cast<H2Core::PortAudioDriver*>(pAudioDriver) != nullptr ) {
		sInfo.append( "<b>" ).append( tr( "PortAudio Driver" ) )
			.append( "</b><br>" );
#ifndef H2CORE_HAVE_PORTAUDIO
		sInfo.append( "<br><b><font color=" )
			.append( m_sColorRed ).append( ">")
			.append( pCommonStrings->getPreferencesNotCompiled() )
			.append( "</font></b>" );
#endif
	}
	else if ( dynamic_cast<H2Core::CoreAudioDriver*>(pAudioDriver) != nullptr ) {	
		sInfo.append( "<b>" ).append( tr( "CoreAudio Driver" ) )
			.append( "</b><br>" );
#ifndef H2CORE_HAVE_COREAUDIO
		sInfo.append( "<br><b><font color=" )
			.append( m_sColorRed ).append( ">")
			.append( pCommonStrings->getPreferencesNotCompiled() )
			.append( "</font></b>" );
#endif
	}
	else if ( dynamic_cast<H2Core::PulseAudioDriver*>(pAudioDriver) != nullptr ) {		
		sInfo.append( "<b>" ).append( tr( "PulseAudio Driver" ) )
			.append( "</b><br>" );
#ifndef H2CORE_HAVE_PULSEAUDIO
		sInfo.append( "<br><b><font color=" )
			.append( m_sColorRed ).append( ">")
			.append( pCommonStrings->getPreferencesNotCompiled() )
			.append( "</font></b>" );
#endif
	}
	else if ( dynamic_cast<H2Core::OssDriver*>(pAudioDriver) != nullptr ) {
		sInfo.append( "<b>" ).append( tr( "Open Sound System" ) )
			.append( "</b><br>" )
			.append( tr( "Simple audio driver [/dev/dsp]" ) );
#ifndef H2CORE_HAVE_OSS
		sInfo.append( "<br><b><font color=" )
			.append( m_sColorRed ).append( ">")
			.append( pCommonStrings->getPreferencesNotCompiled() )
			.append( "</font></b>" );
#endif
	}
	else {
		
		if ( driverComboBox->currentText() ==
			 Preferences::audioDriverToQString( Preferences::AudioDriver::Auto ) ) {
		
			// Display the selected driver as well.
			sInfo.append( "<b>" )
				.append( tr( "Error starting audio driver" ) )
				.append( "</b><br><br>" );
		}
		
		// Display the selected driver as well.
		sInfo.append( "NullDriver" )
			.append( "<br><br><b><font color=" )
			.append( m_sColorRed ).append( ">")
			.append( pCommonStrings->getAudioDriverNotPresent() )
			.append( "</font></b>" );
	}
	
	driverInfoLbl->setText( sInfo );
}

void PreferencesDialog::setDriverInfoOss() {
	auto pPref = H2Core::Preferences::get_instance();
	
	m_pAudioDeviceTxt->setDriver( Preferences::AudioDriver::Oss );
	m_pAudioDeviceTxt->setIsActive(true);
	m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sOSSDevice );
	bufferSizeSpinBox->setIsActive(true);
	sampleRateComboBox->setIsActive(true);
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

	bufferSizeSpinBox->setToolTip( "" );
	sampleRateComboBox->setToolTip( "" );
}

void PreferencesDialog::setDriverInfoAlsa() {
	auto pPref = H2Core::Preferences::get_instance();

	m_pAudioDeviceTxt->setDriver( Preferences::AudioDriver::Alsa );
	m_pAudioDeviceTxt->setIsActive(true);
	m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sAlsaAudioDevice );
	bufferSizeSpinBox->setIsActive(true);
	sampleRateComboBox->setIsActive(true);
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

	bufferSizeSpinBox->setToolTip( "" );
	sampleRateComboBox->setToolTip( "" );
}

void PreferencesDialog::setDriverInfoJack() {
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	m_pAudioDeviceTxt->setDriver( Preferences::AudioDriver::Jack );
	m_pAudioDeviceTxt->setIsActive(false);
	m_pAudioDeviceTxt->lineEdit()->setText( "" );
	bufferSizeSpinBox->setIsActive(false);
	sampleRateComboBox->setIsActive(false);
	trackOutputComboBox->setIsActive( true );
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

	bufferSizeSpinBox->setToolTip( pCommonStrings->getPreferencesJackTooltip() );
	sampleRateComboBox->setToolTip( pCommonStrings->getPreferencesJackTooltip() );
}

void PreferencesDialog::setDriverInfoCoreAudio() {
	auto pPref = H2Core::Preferences::get_instance();

	m_pAudioDeviceTxt->setDriver( Preferences::AudioDriver::CoreAudio );
	m_pAudioDeviceTxt->setIsActive( true );
	m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sCoreAudioDevice );
	bufferSizeSpinBox->setIsActive( true );
	sampleRateComboBox->setIsActive(true);
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

	bufferSizeSpinBox->setToolTip( "" );
	sampleRateComboBox->setToolTip( "" );
}

void PreferencesDialog::setDriverInfoPortAudio() {
	auto pPref = H2Core::Preferences::get_instance();

	m_pAudioDeviceTxt->setDriver( Preferences::AudioDriver::PortAudio );
	m_pAudioDeviceTxt->setIsActive( true );
	m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sPortAudioDevice );
	bufferSizeSpinBox->setIsActive(false);
	sampleRateComboBox->setIsActive(true);
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

	const auto pAudioDriver = H2Core::Hydrogen::get_instance()->getAudioOutput();
	int nLatency;
	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return;
	}
	else {
		nLatency = pAudioDriver->getLatency();
	}
	latencyValueLabel->setText( QString("Current: %1 frames").arg( nLatency ) );

	bufferSizeSpinBox->setToolTip( "" );
	sampleRateComboBox->setToolTip( "" );
}

void PreferencesDialog::setDriverInfoPulseAudio() {
	
	m_pAudioDeviceTxt->setDriver( Preferences::AudioDriver::PulseAudio );
	m_pAudioDeviceTxt->setIsActive(false);
	m_pAudioDeviceTxt->lineEdit()->setText("");
	bufferSizeSpinBox->setIsActive(true);
	sampleRateComboBox->setIsActive(true);
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

	bufferSizeSpinBox->setToolTip( "" );
	sampleRateComboBox->setToolTip( "" );
}

void PreferencesDialog::onApplicationFontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();

	m_pCurrentTheme->getFontTheme()->m_sApplicationFontFamily = font.family();
	pPref->setApplicationFontFamily( font.family() );

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::Font );
		
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onLevel2FontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();

	m_pCurrentTheme->getFontTheme()->m_sLevel2FontFamily = font.family();
	pPref->setLevel2FontFamily( font.family() );

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::Font );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onLevel3FontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();

	m_pCurrentTheme->getFontTheme()->m_sLevel3FontFamily = font.family();
	pPref->setLevel3FontFamily( font.family() );

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::Font );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onRejected() {

	updateAppearanceTab( m_pPreviousTheme );

	HydrogenApp::get_instance()->changePreferences( m_changes );
}

void PreferencesDialog::onFontSizeChanged( int nIndex ) {
	auto pPref = Preferences::get_instance();

	switch ( nIndex ) {
	case 0:
		pPref->setFontSize( FontTheme::FontSize::Small );
		m_pCurrentTheme->getFontTheme()->m_fontSize = FontTheme::FontSize::Small;
		break;
	case 1:
		pPref->setFontSize( FontTheme::FontSize::Medium );
		m_pCurrentTheme->getFontTheme()->m_fontSize = FontTheme::FontSize::Medium;
		break;
	case 2:
		pPref->setFontSize( FontTheme::FontSize::Large );
		m_pCurrentTheme->getFontTheme()->m_fontSize = FontTheme::FontSize::Large;
		break;
	default:
		ERRORLOG( QString( "Unknown font size: %1" ).arg( nIndex ) );
	}
	
	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::Font );
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onUILayoutChanged( int nIndex ) {
	if ( static_cast<InterfaceTheme::Layout>(nIndex) !=
		 m_pPreviousTheme->getInterfaceTheme()->m_layout ||
		 m_pCurrentTheme->getInterfaceTheme()->m_scalingPolicy !=
		 m_pPreviousTheme->getInterfaceTheme()->m_scalingPolicy ) {
		UIChangeWarningLabel->show();
	} else {
		UIChangeWarningLabel->hide();
	}
	m_pCurrentTheme->getInterfaceTheme()->m_layout = static_cast<InterfaceTheme::Layout>(nIndex);
	Preferences::get_instance()->setDefaultUILayout( static_cast<InterfaceTheme::Layout>(nIndex) );

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::uiScalingPolicyComboBoxCurrentIndexChanged( int nIndex ) {
	if ( static_cast<InterfaceTheme::ScalingPolicy>(nIndex) !=
		 m_pPreviousTheme->getInterfaceTheme()->m_scalingPolicy ||
		 m_pCurrentTheme->getInterfaceTheme()->m_layout !=
		 m_pPreviousTheme->getInterfaceTheme()->m_layout ) {
		UIChangeWarningLabel->show();
	} else {
		UIChangeWarningLabel->hide();
	}
	m_pCurrentTheme->getInterfaceTheme()->m_scalingPolicy = static_cast<InterfaceTheme::ScalingPolicy>(nIndex);
	Preferences::get_instance()->setUIScalingPolicy( static_cast<InterfaceTheme::ScalingPolicy>(nIndex) );

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::onIconColorChanged( int nIndex ) {
	m_pCurrentTheme->getInterfaceTheme()->m_iconColor = static_cast<InterfaceTheme::IconColor>(nIndex);
	H2Core::Preferences::get_instance()->setIconColor( static_cast<InterfaceTheme::IconColor>(nIndex) );

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
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

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
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

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
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

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::mixerFalloffComboBoxCurrentIndexChanged( int nIndex ) {
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

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::latencyTargetSpinBoxValueChanged( int i )
{
	UNUSED( i );
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::bufferSizeSpinBoxValueChanged( int i )
{
	UNUSED( i );
	m_bNeedDriverRestart = true;
}




void PreferencesDialog::sampleRateComboBoxEditTextChanged( const QString&  )
{
	m_bNeedDriverRestart = true;
}



void PreferencesDialog::on_restartDriverBtn_clicked()
{
	QApplication::setOverrideCursor( Qt::WaitCursor );
	
	updateDriverPreferences();
	Preferences *pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	pHydrogen->restartDrivers();
	
	QApplication::restoreOverrideCursor();

	if ( pHydrogen->getAudioOutput() == nullptr ||
		 dynamic_cast<NullDriver*>(pHydrogen->getAudioOutput()) != nullptr ) {
		QMessageBox::critical( this, "Hydrogen",
							   tr( "Unable to start audio driver" ) );
	}
	
	pPref->savePreferences();
	m_bNeedDriverRestart = false;
	updateDriverInfo();
}

void PreferencesDialog::midiPortComboBoxActivated( int index )
{
	UNUSED( index );
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::midiOutportComboBoxActivated( int index )
{
	UNUSED( index );
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::styleComboBoxActivated( int index )
{
	UNUSED( index );
	
	QString sStyle = styleComboBox->currentText();
	if ( sStyle != m_pCurrentTheme->getInterfaceTheme()->m_sQTStyle ) {

		// Instant visual feedback.
		QApplication *pQApp = (HydrogenApp::get_instance())->getMainForm()->m_pQApp;
		pQApp->setStyle( sStyle );
		Preferences *pPref = Preferences::get_instance();
		pPref->setQTStyle( sStyle );

		m_pCurrentTheme->getInterfaceTheme()->m_sQTStyle = sStyle;

		m_changes =
			static_cast<H2Core::Preferences::Changes>(
				m_changes | H2Core::Preferences::Changes::AppearanceTab );
	
		HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
	}
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

void PreferencesDialog::onMidiDriverComboBoxIndexChanged ( int )
{
	m_bNeedDriverRestart = true;
}

void PreferencesDialog::toggleTrackOutsCheckBox( bool )
{
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
	case 0x208: return &pColorTheme->m_playheadColor;
	case 0x209: return &pColorTheme->m_cursorColor;
	case 0x300: return &pColorTheme->m_songEditor_backgroundColor;
	case 0x301: return &pColorTheme->m_songEditor_alternateRowColor;
	case 0x302: return &pColorTheme->m_songEditor_virtualRowColor;
	case 0x303: return &pColorTheme->m_songEditor_selectedRowColor;
	case 0x304: return &pColorTheme->m_songEditor_selectedRowTextColor;
	case 0x305: return &pColorTheme->m_songEditor_lineColor;
	case 0x306: return &pColorTheme->m_songEditor_textColor;
	case 0x307: return &pColorTheme->m_songEditor_automationBackgroundColor;
	case 0x308: return &pColorTheme->m_songEditor_automationLineColor;
	case 0x309: return &pColorTheme->m_songEditor_automationNodeColor;
	case 0x30a: return &pColorTheme->m_songEditor_stackedModeOnColor;
	case 0x30b: return &pColorTheme->m_songEditor_stackedModeOnNextColor;
	case 0x30c: return &pColorTheme->m_songEditor_stackedModeOffNextColor;
	case 0x400: return &pColorTheme->m_patternEditor_backgroundColor;
	case 0x401: return &pColorTheme->m_patternEditor_alternateRowColor;
	case 0x402: return &pColorTheme->m_patternEditor_selectedRowColor;
	case 0x403: return &pColorTheme->m_patternEditor_selectedRowTextColor;
	case 0x404: return &pColorTheme->m_patternEditor_octaveRowColor;
	case 0x405: return &pColorTheme->m_patternEditor_textColor;
	case 0x406: return &pColorTheme->m_patternEditor_noteVelocityFullColor;
	case 0x407: return &pColorTheme->m_patternEditor_noteVelocityDefaultColor;
	case 0x408: return &pColorTheme->m_patternEditor_noteVelocityHalfColor;
	case 0x409: return &pColorTheme->m_patternEditor_noteVelocityZeroColor;
	case 0x40a: return &pColorTheme->m_patternEditor_noteOffColor;
	case 0x40b: return &pColorTheme->m_patternEditor_lineColor;
	case 0x40c: return &pColorTheme->m_patternEditor_line1Color;
	case 0x40d: return &pColorTheme->m_patternEditor_line2Color;
	case 0x40e: return &pColorTheme->m_patternEditor_line3Color;
	case 0x40f: return &pColorTheme->m_patternEditor_line4Color;
	case 0x410: return &pColorTheme->m_patternEditor_line5Color;
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
	case 0x208:  pColorTheme->m_playheadColor = color;
		break;
	case 0x209:  pColorTheme->m_cursorColor = color;
		break;
	case 0x300:  pColorTheme->m_songEditor_backgroundColor = color;
		break;
	case 0x301:  pColorTheme->m_songEditor_alternateRowColor = color;
		break;
	case 0x302:  pColorTheme->m_songEditor_virtualRowColor = color;
		break;
	case 0x303:  pColorTheme->m_songEditor_selectedRowColor = color;
		break;
	case 0x304:  pColorTheme->m_songEditor_selectedRowTextColor = color;
		break;
	case 0x305:  pColorTheme->m_songEditor_lineColor = color;
		break;
	case 0x306:  pColorTheme->m_songEditor_textColor = color;
		break;
	case 0x307:  pColorTheme->m_songEditor_automationBackgroundColor = color;
		break;
	case 0x308:  pColorTheme->m_songEditor_automationLineColor = color;
		break;
	case 0x309:  pColorTheme->m_songEditor_automationNodeColor = color;
		break;
	case 0x30a:  pColorTheme->m_songEditor_stackedModeOnColor = color;
		break;
	case 0x30b:  pColorTheme->m_songEditor_stackedModeOnNextColor = color;
		break;
	case 0x30c:  pColorTheme->m_songEditor_stackedModeOffNextColor = color;
		break;
	case 0x400:  pColorTheme->m_patternEditor_backgroundColor = color;
		break;
	case 0x401:  pColorTheme->m_patternEditor_alternateRowColor = color;
		break;
	case 0x402:  pColorTheme->m_patternEditor_selectedRowColor = color;
		break;
	case 0x403:  pColorTheme->m_patternEditor_selectedRowTextColor = color;
		break;
	case 0x404:  pColorTheme->m_patternEditor_octaveRowColor = color;
		break;
	case 0x405:  pColorTheme->m_patternEditor_textColor = color;
		break;
	case 0x406:  pColorTheme->m_patternEditor_noteVelocityFullColor = color;
		break;
	case 0x407:  pColorTheme->m_patternEditor_noteVelocityDefaultColor = color;
		break;
	case 0x408:  pColorTheme->m_patternEditor_noteVelocityHalfColor = color;
		break;
	case 0x409:  pColorTheme->m_patternEditor_noteVelocityZeroColor = color;
		break;
	case 0x40a:  pColorTheme->m_patternEditor_noteOffColor = color;
		break;
	case 0x40b:  pColorTheme->m_patternEditor_lineColor = color;
		break;
	case 0x40c:  pColorTheme->m_patternEditor_line1Color = color;
		break;
	case 0x40d:  pColorTheme->m_patternEditor_line2Color = color;
		break;
	case 0x40e:  pColorTheme->m_patternEditor_line3Color = color;
		break;
	case 0x40f:  pColorTheme->m_patternEditor_line4Color = color;
		break;
	case 0x410:  pColorTheme->m_patternEditor_line5Color = color;
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

	  m_changes =
		  static_cast<H2Core::Preferences::Changes>(
			  m_changes | H2Core::Preferences::Changes::Colors );
	
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
	FileDialog fd( this );
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

	QFileInfo fileInfo( fd.selectedFiles().first() );
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

		m_changes =
			static_cast<H2Core::Preferences::Changes>(
				m_changes | H2Core::Preferences::Changes::Colors );
	  
		HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Colors );
	}
	updateAppearanceTab( m_pCurrentTheme );

	HydrogenApp::get_instance()->showStatusBarMessage( tr( "Theme imported from " ) + sSelectedPath );

	m_changes =
		static_cast<H2Core::Preferences::Changes>(
			m_changes | H2Core::Preferences::Changes::AppearanceTab );
		
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
	
}

void PreferencesDialog::exportTheme() {
	QString sPath = H2Core::Preferences::get_instance()->getLastExportThemeDirectory();
	if ( ! H2Core::Filesystem::dir_writable( sPath, false ) ){
		sPath = Filesystem::usr_theme_dir();
	}
	QString sTitle = tr( "Export Theme" );
	FileDialog fd( this );
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

	QFileInfo fileInfo( fd.selectedFiles().first() );
	QString sSelectedPath = fileInfo.absoluteFilePath();

	if ( sSelectedPath.isEmpty() ||
		 ! Theme::exportTheme( sSelectedPath, m_pCurrentTheme ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Theme can not be exported.") );
		return;
	}

	H2Core::Preferences::get_instance()->setLastExportThemeDirectory( fd.directory().absolutePath() );

	HydrogenApp::get_instance()->showStatusBarMessage( tr( "Theme exported to " ) +
													   sSelectedPath );
	
}

void PreferencesDialog::resetTheme() {
	m_pCurrentTheme = std::make_shared<Theme>( m_pPreviousTheme );
	H2Core::Preferences::get_instance()->setTheme( m_pCurrentTheme );
	updateAppearanceTab( m_pCurrentTheme );
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );

	HydrogenApp::get_instance()->showStatusBarMessage( tr( "Theme reset" ) );
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

	// Ensure the number of color buttons match.
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

	// Update their colors.
	for ( int ii = 0; ii < m_colorSelectionButtons.size(); ++ii ) {
		m_colorSelectionButtons[ ii ]->setColor( pTheme->getInterfaceTheme()->
												 m_patternColors[ ii ] );
	}

	// Display only the required number.
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
	case FontTheme::FontSize::Medium:
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
