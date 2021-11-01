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
 , m_currentColors( H2Core::UIStyle( H2Core::Preferences::get_instance()->getDefaultUIStyle() ) )
 , m_previousColors( H2Core::UIStyle( H2Core::Preferences::get_instance()->getDefaultUIStyle() ) )
{
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

	QFontDatabase fontDB;
	m_fontFamilies = fontDB.families();
	
	// Appearance tab
	// Appearance tab - Fonts
	m_sPreviousApplicationFontFamily = pPref->getApplicationFontFamily();
	m_sPreviousLevel2FontFamily = pPref->getLevel2FontFamily();
	m_sPreviousLevel3FontFamily = pPref->getLevel3FontFamily();
	applicationFontComboBox->setCurrentFont( QFont( m_sPreviousApplicationFontFamily ) );
	level2FontComboBox->setCurrentFont( QFont( m_sPreviousLevel2FontFamily ) );
	level3FontComboBox->setCurrentFont( QFont( m_sPreviousLevel3FontFamily ) );
	connect( applicationFontComboBox, &QFontComboBox::currentFontChanged, this, &PreferencesDialog::onApplicationFontChanged );
	connect( level2FontComboBox, &QFontComboBox::currentFontChanged, this, &PreferencesDialog::onLevel2FontChanged );
	connect( level3FontComboBox, &QFontComboBox::currentFontChanged, this, &PreferencesDialog::onLevel3FontChanged );

	m_previousFontSize = pPref->getFontSize();
	switch( m_previousFontSize ) {
	case Preferences::FontSize::Small:
		fontSizeComboBox->setCurrentIndex( 0 );
		break;
	case Preferences::FontSize::Normal:
		fontSizeComboBox->setCurrentIndex( 1 );
		break;
	case Preferences::FontSize::Large:
		fontSizeComboBox->setCurrentIndex( 2 );
		break;
	default:
		ERRORLOG( QString( "Unknown font size: %1" )
				  .arg( static_cast<int>( m_previousFontSize ) ) );
	}
	connect( fontSizeComboBox, SIGNAL( currentIndexChanged(int) ),
			 this, SLOT( onFontSizeChanged(int) ) );
	
	// Appearance tab - Interface
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
	
	UIChangeWarningLabel->hide();
	UIChangeWarningLabel->setText( QString( "<b><i><font color=" )
								   .append( m_sColorRed )
								   .append( ">" )
								   .append( tr( "For changes of the interface layout to take effect Hydrogen must be restarted." ) )
								   .append( "</font></i></b>" ) );
	uiLayoutComboBox->setCurrentIndex(  pPref->getDefaultUILayout() );
	connect( uiLayoutComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( onUILayoutChanged(int) ) );
	

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
	m_nPreviousVisiblePatternColors = pPref->getVisiblePatternColors();

	if ( coloringMethod == 0 ) {
		coloringMethodAuxSpinBox->hide();
		colorSelectionLabel->hide();
	} else {
		coloringMethodAuxSpinBox->show();
		colorSelectionLabel->show();
	}
	coloringMethodCombo->clear();
	coloringMethodCombo->addItem(tr("Automatic"));
	coloringMethodCombo->addItem(tr("Custom"));

	coloringMethodCombo->setCurrentIndex( coloringMethod );
	coloringMethodAuxSpinBox->setValue( m_nPreviousVisiblePatternColors );
	QSize size( uiScalingPolicyComboBox->width(), coloringMethodAuxSpinBox->height() );

	m_previousPatternColors = pPref->getPatternColors();

	int nMaxPatternColors = pPref->getMaxPatternColors();
	m_colorSelectionButtons = std::vector<ColorSelectionButton*>( nMaxPatternColors );
	int nButtonSize = fontSizeComboBox->height();
	// float fLineWidth = static_cast<float>(fontSizeComboBox->width());
	// Using a fixed one size resizing of the widget seems to happen
	// after the constructor is called.
	float fLineWidth = 308;
	int nButtonsPerLine = std::floor( fLineWidth / static_cast<float>(nButtonSize + 6) );

	colorSelectionGrid->setHorizontalSpacing( 4 );
	for ( int ii = 0; ii < nMaxPatternColors; ii++ ) {
		ColorSelectionButton* bbutton = new ColorSelectionButton( this, m_previousPatternColors[ ii ], nButtonSize );
		bbutton->hide();
		connect( bbutton, &ColorSelectionButton::clicked, this, &PreferencesDialog::onColorSelectionClicked );
		colorSelectionGrid->addWidget( bbutton,
									   std::floor( static_cast<float>( ii ) /
												   static_cast<float>( nButtonsPerLine ) ),
									   (ii % nButtonsPerLine) + 1); //+1 to take the hspace into account.
		m_colorSelectionButtons[ ii ] = bbutton;
	}

	if ( coloringMethod != 0 ) {
		for ( int ii = 0; ii < m_nPreviousVisiblePatternColors; ii++ ) {
			m_colorSelectionButtons[ ii ]->show();
		}
	}
	connect( coloringMethodAuxSpinBox, SIGNAL( valueChanged(int)), this, SLOT( onColorNumberChanged( int ) ) );
	connect( coloringMethodCombo, SIGNAL( currentIndexChanged(int) ), this, SLOT( onColoringMethodChanged(int) ) );

	// Appearance tab - Colors
	colorwidget->setAutoFillBackground(true);
	  
	m_pPalette = new QButtonGroup( paletteBox );
	m_pPalette->addButton( palette0, 0 );
	m_pPalette->addButton( palette1, 1 );
	m_pPalette->addButton( palette2, 2 );
	m_pPalette->addButton( palette3, 3 );
	m_pPalette->addButton( palette4, 4 );
	m_pPalette->addButton( palette5, 5 );
	m_pPalette->addButton( palette6, 6 );
	m_pPalette->addButton( palette7, 7 );
	m_pPalette->addButton( palette8, 8 );
	m_pPalette->addButton( palette9, 9 );
	m_pPalette->addButton( palette10, 10 );
	m_pPalette->addButton( palette11, 11 );
	m_pPalette->addButton( palette12, 12 );
	m_pPalette->addButton( palette13, 13 );
	m_pPalette->addButton( palette14, 14 );
	m_pPalette->addButton( palette15, 15 );
	m_pPalette->setExclusive( true );
	
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
	new ColorTreeItem( 0x206, pTopLevelItem, "Spin Box Selection" );
	new ColorTreeItem( 0x207, pTopLevelItem, "Spin Box Selection Text" );
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

	colorNameLineEdit->setEnabled(false);

	// connect(loadColorsButton, SIGNAL(clicked(bool)), SLOT(loadColors()));
	// connect(saveColorsButton, SIGNAL(clicked(bool)), SLOT(saveColors()));
	// connect(pickColorButton, SIGNAL(clicked(bool)), SLOT(chooseColorClicked()));
	// connect(colorwidget,     SIGNAL(clicked()),     SLOT(chooseColorClicked()));
      
	// connect(colorNameLineEdit, SIGNAL(editingFinished()), SLOT(colorNameEditFinished()));
	// connect(colorTree, SIGNAL(itemSelectionChanged()), SLOT(colorItemSelectionChanged()));
	// connect(aPalette, SIGNAL(buttonClicked(int)), SLOT(paletteClicked(int)));
	// connect(globalAlphaSlider, SIGNAL(valueChanged(int)), SLOT(asliderChanged(int)));
	// connect(rslider, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
	// connect(gslider, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
	// connect(bslider, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
	// connect(hslider, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
	// connect(sslider, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
	// connect(vslider, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

	// connect(globalAlphaVal, SIGNAL(valueChanged(int)), SLOT(aValChanged(int)));
	// connect(rval, SIGNAL(valueChanged(int)), SLOT(rsliderChanged(int)));
	// connect(gval, SIGNAL(valueChanged(int)), SLOT(gsliderChanged(int)));
	// connect(bval, SIGNAL(valueChanged(int)), SLOT(bsliderChanged(int)));
	// connect(hval, SIGNAL(valueChanged(int)), SLOT(hsliderChanged(int)));
	// connect(sval, SIGNAL(valueChanged(int)), SLOT(ssliderChanged(int)));
	// connect(vval, SIGNAL(valueChanged(int)), SLOT(vsliderChanged(int)));

	// connect(addToPalette, SIGNAL(clicked()), SLOT(addToPaletteClicked()));
	updateColorTree();
	
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

	HydrogenApp *pH2App = HydrogenApp::get_instance();
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
		bufferSizeSpinBox->setEnabled( true );
		sampleRateComboBox->setEnabled( true );
		trackOutputComboBox->setEnabled( false );
		connectDefaultsCheckBox->setEnabled( false );
		enableTimebaseCheckBox->setEnabled( false );
		trackOutsCheckBox->setEnabled( false );
		jackBBTSyncComboBox->setEnabled( false );
		jackBBTSyncLbl->setEnabled( false );
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
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
		m_pAudioDeviceTxt->lineEdit()->setText( pPref->m_sOSSDevice );
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
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
		jackBBTSyncComboBox->show();
		jackBBTSyncLbl->show();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
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
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
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
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
		portaudioHostAPIComboBox->show();
		portaudioHostAPILabel->show();
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
		bufferSizeSpinBox->setEnabled(true);
		sampleRateComboBox->setEnabled(true);
		trackOutputComboBox->hide();
		trackOutputLbl->hide();
		connectDefaultsCheckBox->hide();
		enableTimebaseCheckBox->hide();
		trackOutsCheckBox->hide();
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
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
		jackBBTSyncComboBox->hide();
		jackBBTSyncLbl->hide();
		portaudioHostAPIComboBox->hide();
		portaudioHostAPILabel->hide();
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
	
	pPref->setApplicationFontFamily( font.family() );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onLevel2FontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();
	
	pPref->setLevel2FontFamily( font.family() );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onLevel3FontChanged( const QFont& font ) {
	auto pPref = Preferences::get_instance();
	
	pPref->setLevel3FontFamily( font.family() );

	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onRejected() {
	auto pPref = Preferences::get_instance();
	
	pPref->setApplicationFontFamily( m_sPreviousApplicationFontFamily );
	pPref->setLevel2FontFamily( m_sPreviousLevel2FontFamily );
	pPref->setLevel3FontFamily( m_sPreviousLevel3FontFamily );
	pPref->setFontSize( m_previousFontSize );
	pPref->setPatternColors( m_previousPatternColors );
	pPref->setVisiblePatternColors( m_nPreviousVisiblePatternColors );

	HydrogenApp::get_instance()->changePreferences( static_cast<H2Core::Preferences::Changes>( H2Core::Preferences::Changes::Font |
																							   H2Core::Preferences::Changes::Colors |
																							   H2Core::Preferences::Changes::AppearanceTab ) );
}

void PreferencesDialog::onFontSizeChanged( int nIndex ) {
	auto pPref = Preferences::get_instance();

	switch ( nIndex ) {
	case 0:
		pPref->setFontSize( Preferences::FontSize::Small );
		break;
	case 1:
		pPref->setFontSize( Preferences::FontSize::Normal );
		break;
	case 2:
		pPref->setFontSize( Preferences::FontSize::Large );
		break;
	default:
		ERRORLOG( QString( "Unknown font size: %1" ).arg( nIndex ) );
	}
	
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::Font );
}

void PreferencesDialog::onUILayoutChanged( int nIndex ) {
	UIChangeWarningLabel->show();
}

void PreferencesDialog::onColorNumberChanged( int nIndex ) {
	Preferences::get_instance()->setVisiblePatternColors( nIndex );
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

	Preferences::get_instance()->setPatternColors( colors );
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

void PreferencesDialog::onColoringMethodChanged( int nIndex ) {
	Preferences::get_instance()->setColoringMethod( nIndex );

	if ( nIndex == 0 ) {
		coloringMethodAuxSpinBox->hide();
		coloringMethodAuxLabel->hide();
		colorSelectionLabel->hide();
		for ( int ii = 0; ii < Preferences::get_instance()->getMaxPatternColors(); ii++ ) {
			m_colorSelectionButtons[ ii ]->hide();
		}
	} else {
		coloringMethodAuxSpinBox->show();
		coloringMethodAuxLabel->show();
		colorSelectionLabel->show();
		for ( int ii = 0; ii < m_nPreviousVisiblePatternColors; ii++ ) {
			m_colorSelectionButtons[ ii ]->show();
		}
	}
	HydrogenApp::get_instance()->changePreferences( H2Core::Preferences::Changes::AppearanceTab );
}

// void PreferencesDialog::onCustomizePaletteClicked() {

// 	PaletteDialog* pPaletteDialog = new PaletteDialog( nullptr );

// 	pPaletteDialog->exec();
// 	delete pPaletteDialog;
// }

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

QColor* PreferencesDialog::getColorFromId( int nId, H2Core::UIStyle* uiStyle ) const {
	switch( nId ) {
	case 0x100: return &uiStyle->m_windowColor;
	case 0x101: return &uiStyle->m_windowTextColor;
	case 0x102: return &uiStyle->m_baseColor;
	case 0x103: return &uiStyle->m_alternateBaseColor;
	case 0x104: return &uiStyle->m_textColor;
	case 0x105: return &uiStyle->m_buttonColor;
	case 0x106: return &uiStyle->m_buttonTextColor;
	case 0x107: return &uiStyle->m_lightColor;
	case 0x108: return &uiStyle->m_midLightColor;
	case 0x109: return &uiStyle->m_midColor;
	case 0x10a: return &uiStyle->m_darkColor;
	case 0x10b: return &uiStyle->m_shadowTextColor;
	case 0x10c: return &uiStyle->m_highlightColor;
	case 0x10d: return &uiStyle->m_highlightedTextColor;
	case 0x10e: return &uiStyle->m_selectionHighlightColor;
	case 0x10f: return &uiStyle->m_selectionInactiveColor;
	case 0x110: return &uiStyle->m_toolTipBaseColor;
	case 0x111: return &uiStyle->m_toolTipTextColor;
	case 0x200: return &uiStyle->m_widgetColor;
	case 0x201: return &uiStyle->m_widgetTextColor;
	case 0x202: return &uiStyle->m_accentColor;
	case 0x203: return &uiStyle->m_accentTextColor;
	case 0x204: return &uiStyle->m_buttonRedColor;
	case 0x205: return &uiStyle->m_buttonRedTextColor;
	case 0x206: return &uiStyle->m_spinBoxSelectionColor;
	case 0x207: return &uiStyle->m_spinBoxSelectionTextColor;
	case 0x208: return &uiStyle->m_automationColor;
	case 0x209: return &uiStyle->m_automationCircleColor;
	case 0x300: return &uiStyle->m_songEditor_backgroundColor;
	case 0x301: return &uiStyle->m_songEditor_alternateRowColor;
	case 0x302: return &uiStyle->m_songEditor_selectedRowColor;
	case 0x303: return &uiStyle->m_songEditor_lineColor;
	case 0x304: return &uiStyle->m_songEditor_textColor;
	case 0x400: return &uiStyle->m_patternEditor_backgroundColor;
	case 0x401: return &uiStyle->m_patternEditor_alternateRowColor;
	case 0x402: return &uiStyle->m_patternEditor_selectedRowColor;
	case 0x403: return &uiStyle->m_patternEditor_textColor;
	case 0x404: return &uiStyle->m_patternEditor_noteColor;
	case 0x405: return &uiStyle->m_patternEditor_noteoffColor;
	case 0x406: return &uiStyle->m_patternEditor_lineColor;
	case 0x407: return &uiStyle->m_patternEditor_line1Color;
	case 0x408: return &uiStyle->m_patternEditor_line2Color;
	case 0x409: return &uiStyle->m_patternEditor_line3Color;
	case 0x40a: return &uiStyle->m_patternEditor_line4Color;
	case 0x40b: return &uiStyle->m_patternEditor_line5Color;
	default: return nullptr;
	}

	return nullptr;
}

void PreferencesDialog::setColorTreeItemDirty( ColorTreeItem* pItem) {
	if( pItem == nullptr) {
		DEBUGLOG( "NULL item" );
		return;
	}
	
	int nId = pItem->getId();
	DEBUGLOG( nId );
	if( nId == 0 ) {
		// Node without a color used as a heading.
		return;
	}

	QColor* pCurrentColor = getColorFromId( nId, &m_currentColors );
	if ( pCurrentColor == nullptr ) {
		ERRORLOG( QString( "Unable to get current color for id [%1]" ).arg( nId ) );
		return;
	}
	QColor* pPreviousColor = getColorFromId( nId, &m_previousColors );
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
