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
 * along with this program. If not, see 
https://www.gnu.org/licenses
 *
 */


#include "MidiControlDialog.h"

#include "MainToolBar.h"
#include "MidiActionTable.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../Widgets/LCDSpinBox.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

using namespace H2Core;

MidiControlDialog::MidiControlDialog( QWidget* pParent )
	: QDialog( pParent )
{
	const auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setMinimumSize( MidiControlDialog::nColumnActionWidth +
					MidiControlDialog::nColumnInstrumentWidth +
					MidiControlDialog::nColumnTimestampWidth +
					MidiControlDialog::nColumnTypeWidth +
					3 * MidiControlDialog::nColumnValueWidth + 20,
					MidiControlDialog::nMinimumHeight );
	setFocusPolicy( Qt::NoFocus );
	setObjectName( "MidiControlDialog" );
	// Not translated because it would make explanation within tickets more
	// difficult.
	setWindowTitle( pCommonStrings->getMidiControl() );

	auto pMainLayout = new QVBoxLayout( this );
	setLayout( pMainLayout );

	m_pTabWidget = new QTabWidget( this );
	pMainLayout->addWidget( m_pTabWidget );

	////////////////////////////////////////////////////////////////////////////

	const QColor borderColor( 80, 80, 80 );
	const int nHeaderTextSize = 20;
	const int nSettingTextSize = 16;

	auto createSeparator = [&]( QWidget* pParent, bool bHorizontal ) {
		auto pSeparator = new QWidget( pParent );
		if ( bHorizontal ) {
			pSeparator->setFixedHeight( 1 );
		}
		else {
			pSeparator->setFixedWidth( 1 );
		}
		pSeparator->setStyleSheet( QString( "\
background-color: %1;" ).arg( borderColor.name() ) );

		return pSeparator;
	};

	auto addHeaderLabel = [&]( QWidget* pParent, const QString& sText ) {
		auto pLabel = new QLabel( sText, pParent );
		pLabel->setAlignment( Qt::AlignCenter );
		pLabel->setFixedHeight( 32 );
		pLabel->setStyleSheet( QString( "\
font-size: %1px;" ).arg( nHeaderTextSize ) );
		pParent->layout()->addWidget( pLabel );

		// Well, `border-bottom` does not seem to work on QLabel.
		auto pSeparator = createSeparator( pParent, true );
		pParent->layout()->addWidget( pSeparator );
	};

	////////////////////////////////////////////////////////////////////////////

	auto pSettingsWidget = new QWidget( m_pTabWidget );
	m_pTabWidget->addTab( pSettingsWidget, pCommonStrings->getSettings() );
	auto pSettingsLayout = new QVBoxLayout( pSettingsWidget );
	pSettingsWidget->setLayout( pSettingsLayout );

	auto pConfigWidget = new QWidget( pSettingsWidget );
	pConfigWidget->setStyleSheet( QString( "\
#MidiInputSettings {\
    border-right: 1px solid %1;\
}" ).arg( borderColor.name() ) );
	pSettingsLayout->addWidget( pConfigWidget );
	auto pConfigLayout = new QHBoxLayout( pConfigWidget );
	pConfigLayout->setSpacing( 0 );
	pConfigWidget->setLayout( pConfigLayout );

	auto pInputSettingsWidget = new QWidget( pConfigWidget );
	pInputSettingsWidget->setObjectName( "MidiInputSettings" );
	pConfigLayout->addWidget( pInputSettingsWidget );
	auto pInputSettingsLayout = new QVBoxLayout( pInputSettingsWidget );
	pInputSettingsLayout->setAlignment( Qt::AlignTop );
	pInputSettingsWidget->setLayout( pInputSettingsLayout );

	addHeaderLabel( pInputSettingsWidget, pCommonStrings->getMidiInputLabel() );

	m_pInputIgnoreNoteOffCheckBox = new QCheckBox( pInputSettingsWidget );
	m_pInputIgnoreNoteOffCheckBox->setChecked( pPref->m_bMidiNoteOffIgnore );
	/*: The character after the '&' symbol can be used as a shortcut via the Alt
	 *  modifier. It should not coincide with any other shortcut in the Settings
	 *  tab of the MidiControlDialog. If in question, you can just drop the
	 *  '&'. */
	m_pInputIgnoreNoteOffCheckBox->setText( tr( "&Ignore note-off" ) );
	pInputSettingsLayout->addWidget( m_pInputIgnoreNoteOffCheckBox );
	connect( m_pInputIgnoreNoteOffCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->m_bMidiNoteOffIgnore =
			m_pInputIgnoreNoteOffCheckBox->isChecked();
	} );

	auto pInputMidiClockCheckBox = new QCheckBox( pInputSettingsWidget );
	pInputMidiClockCheckBox->setChecked( pPref->getMidiClockInputHandling() );
	pInputMidiClockCheckBox->setText( tr( "Handle MIDI Clock input" ) );
	pInputSettingsLayout->addWidget( pInputMidiClockCheckBox );
	connect( pInputMidiClockCheckBox, &QAbstractButton::toggled, [=]() {
		CoreActionController::setMidiClockInputHandling(
			pInputMidiClockCheckBox->isChecked() );
	} );

	auto pInputMidiTransportCheckBox = new QCheckBox( pInputSettingsWidget );
	pInputMidiTransportCheckBox->setChecked( pPref->getMidiTransportInputHandling() );
	/*: The character combination "\n" indicates a new line and must be
	 *  conserved. All the capitalized words that follow are defined in the MIDI
	 *  standard. Only translate them if you are sure the translated versions
	 *  are of common usage. */
	pInputMidiTransportCheckBox->setText(
		tr( "Handle MIDI sync message\nSTART, STOP, CONTINUE, SONG_POSITION, SONG_SELECT" ) );
	pInputSettingsLayout->addWidget( pInputMidiTransportCheckBox );
	connect( pInputMidiTransportCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->setMidiTransportInputHandling(
			pInputMidiTransportCheckBox->isChecked() );
	} );

	auto pInputActionChannelWidget = new QWidget( pInputSettingsWidget );
	pInputSettingsLayout->addWidget( pInputActionChannelWidget );
	auto pInputActionChannelLayout = new QHBoxLayout( pInputActionChannelWidget );
	pInputActionChannelWidget->setLayout( pInputActionChannelLayout );

	auto pInputChannelFilterLabel = new QLabel(
		tr( "Channel for MIDI actions and clock" ) );
	pInputActionChannelLayout->addWidget( pInputChannelFilterLabel );
	m_pInputActionChannelSpinBox = new LCDSpinBox(
		pInputActionChannelWidget,
		QSize( MidiControlDialog::nColumnMappingWidth,
			  MidiControlDialog::nMappingBoxHeight ), LCDSpinBox::Type::Int,
		MidiMessage::nChannelAll, MidiMessage::nChannelMaximum,
		LCDSpinBox::Flag::MinusOneAsOff | LCDSpinBox::Flag::MinusTwoAsAll );
	pInputActionChannelLayout->addWidget( m_pInputActionChannelSpinBox );
	m_pInputActionChannelSpinBox->setValue( pPref->m_nMidiActionChannel );
	connect( m_pInputActionChannelSpinBox,
			 QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
			[=]( double fValue ) {
				Preferences::get_instance()->m_nMidiActionChannel =
					static_cast<int>( fValue );
	} );

	auto pOutputSettingsWidget = new QWidget( pConfigWidget );
	pConfigLayout->addWidget( pOutputSettingsWidget );
	auto pOutputSettingsLayout = new QVBoxLayout( pOutputSettingsWidget );
	pOutputSettingsLayout->setAlignment( Qt::AlignTop );
	pOutputSettingsWidget->setLayout( pOutputSettingsLayout );

	addHeaderLabel( pOutputSettingsWidget, pCommonStrings->getMidiOutLabel() );

	m_pOutputEnableMidiFeedbackCheckBox = new QCheckBox( pOutputSettingsWidget );
	m_pOutputEnableMidiFeedbackCheckBox->setChecked( pPref->m_bEnableMidiFeedback );
	/*: The character after the '&' symbol can be used as a shortcut via the Alt
	 *  modifier. It should not coincide with any other shortcut in the Settings
	 *  tab of the MidiControlDialog. If in question, you can just drop the
	 *  '&'. */
	m_pOutputEnableMidiFeedbackCheckBox->setText( tr( "&Enable MIDI feedback" ) );
	pOutputSettingsLayout->addWidget( m_pOutputEnableMidiFeedbackCheckBox );
	connect( m_pOutputEnableMidiFeedbackCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->m_bEnableMidiFeedback =
			m_pOutputEnableMidiFeedbackCheckBox->isChecked();
	} );

	auto pOutputMidiClockCheckBox = new QCheckBox( pOutputSettingsWidget );
	pOutputMidiClockCheckBox->setChecked( pPref->getMidiClockOutputSend() );
	pOutputMidiClockCheckBox->setText( tr( "Send MIDI Clock messages" ) );
	pOutputSettingsLayout->addWidget( pOutputMidiClockCheckBox );
	connect( pOutputMidiClockCheckBox, &QAbstractButton::toggled, [=]() {
		CoreActionController::setMidiClockOutputSend(
			pOutputMidiClockCheckBox->isChecked() );
	} );

	auto pOutputMidiTransportCheckBox = new QCheckBox( pOutputSettingsWidget );
	pOutputMidiTransportCheckBox->setChecked( pPref->getMidiTransportOutputSend() );
	pOutputMidiTransportCheckBox->setText(
		tr( "Send MIDI START, STOP, CONTINUE, and SONG_POSITION" ) );
	pOutputSettingsLayout->addWidget( pOutputMidiTransportCheckBox );
	connect( pOutputMidiTransportCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->setMidiTransportOutputSend(
			pOutputMidiTransportCheckBox->isChecked() );
	} );

	const int nLinkHeight = 24;

	auto pPreferencesLinkWidget = new QWidget( pSettingsWidget );
	pPreferencesLinkWidget->setFixedHeight( nLinkHeight );
	pSettingsLayout->addWidget( pPreferencesLinkWidget );
	auto pPreferencesLinkLayout = new QHBoxLayout( pPreferencesLinkWidget );
	pPreferencesLinkLayout->setContentsMargins( 0, 0, 0, 0 );

	pPreferencesLinkLayout->addStretch();
	auto pPreferencesLinkLabel = new QLabel(
		tr( "MIDI driver settings can be found in the Preferences Dialog" ) );
	pPreferencesLinkLabel->setFixedHeight( nLinkHeight );
	pPreferencesLinkLayout->addWidget( pPreferencesLinkLabel );
	auto pPreferencesLinkButton = new QToolButton( pPreferencesLinkWidget );
	pPreferencesLinkButton->setFixedSize( nLinkHeight, nLinkHeight );
	pPreferencesLinkLayout->addWidget( pPreferencesLinkButton );
	pPreferencesLinkButton->setIcon(
		QIcon( Skin::getSvgImagePath() + "/icons/black/cog.svg" ) );
	connect( pPreferencesLinkButton, &QToolButton::clicked, [&]() {
		HydrogenApp::get_instance()->showPreferencesDialog();
	} );

	////////////////////////////////////////////////////////////////////////////

	const auto pMidiInstrumentMap = pPref->getMidiInstrumentMap();

	auto pMappingTab = new QWidget( m_pTabWidget );
	/*: Tab of the MIDI control dialog dedicated to mapping MIDI notes to
	 *  instruments of the current drumkit. */
	m_pTabWidget->addTab( pMappingTab, tr( "Instrument Mapping" ) );

	auto pMappingWrapperLayout = new QVBoxLayout();
	pMappingWrapperLayout->setSpacing( 5 );
	pMappingTab->setLayout( pMappingWrapperLayout );

	auto pMappingSettingsWidget = new QWidget( pMappingTab );
	auto pMappingGridLayout = new QGridLayout();
	pMappingSettingsWidget->setLayout( pMappingGridLayout );
	pMappingWrapperLayout->addWidget( pMappingSettingsWidget );
	pMappingGridLayout->setVerticalSpacing( 5 );
	pMappingGridLayout->setContentsMargins( 0, 0, 0, 0 );

	auto pVSeparatorInput = createSeparator( pMappingSettingsWidget, false );
	pMappingGridLayout->addWidget( pVSeparatorInput, 0, 2, 4, 1 );
	auto pVSeparatorOutput = createSeparator( pMappingSettingsWidget, false );
	pMappingGridLayout->addWidget( pVSeparatorOutput, 0, 4, 4, 1 );

	auto pInputMappingHeader = new QWidget( pMappingSettingsWidget );
	pInputMappingHeader->setFixedWidth(
		MidiControlDialog::nColumnMappingWidth * 2 );
	auto pInputMappingHeaderLayout = new QVBoxLayout( pInputMappingHeader );
	pInputMappingHeaderLayout->setContentsMargins( 0, 0, 0, 0 );
	pInputMappingHeader->setLayout( pInputMappingHeaderLayout );
	addHeaderLabel( pInputMappingHeader, pCommonStrings->getMidiInputLabel() );

	pMappingGridLayout->addWidget( pInputMappingHeader, 0, 0, 1, 2,
							  Qt::AlignCenter );

	auto pSeparatorHeader = createSeparator( pMappingSettingsWidget, true );
	pMappingGridLayout->addWidget( pSeparatorHeader, 0, 3, Qt::AlignBottom );

	auto pOutputMappingHeader = new QWidget( pMappingSettingsWidget );
	pOutputMappingHeader->setFixedWidth(
		MidiControlDialog::nColumnMappingWidth * 2 );
	auto pOutputMappingHeaderLayout = new QVBoxLayout( pOutputMappingHeader );
	pOutputMappingHeaderLayout->setContentsMargins( 0, 0, 0, 0 );
	pOutputMappingHeader->setLayout( pOutputMappingHeaderLayout );
	addHeaderLabel( pOutputMappingHeader, pCommonStrings->getMidiOutLabel() );
	pMappingGridLayout->addWidget( pOutputMappingHeader, 0, 5, 1, 2,
							  Qt::AlignCenter );


	m_pInputNoteMappingComboBox = new QComboBox( pMappingTab );
	m_pInputNoteMappingComboBox->setFixedSize(
		MidiControlDialog::nColumnMappingWidth * 2,
		MidiControlDialog::nMappingBoxHeight );
	m_pInputNoteMappingComboBox->insertItems( 0,
		QStringList() << MidiInstrumentMap::InputToQString(
				MidiInstrumentMap::Input::None )
			<< MidiInstrumentMap::InputToQString(
				MidiInstrumentMap::Input::AsOutput )
			<< MidiInstrumentMap::InputToQString(
				MidiInstrumentMap::Input::Custom )
			<< MidiInstrumentMap::InputToQString(
				MidiInstrumentMap::Input::SelectedInstrument )
			<< MidiInstrumentMap::InputToQString(
				MidiInstrumentMap::Input::Order ) );
	m_pInputNoteMappingComboBox->setCurrentIndex(
		static_cast<int>( pMidiInstrumentMap->getInput() ) );
	connect( m_pInputNoteMappingComboBox,
			 QOverload<int>::of( &QComboBox::activated ), [=]( int ) {
		auto pMidiInstrumentMap =
			Preferences::get_instance()->getMidiInstrumentMap();
		auto input = static_cast<MidiInstrumentMap::Input>(
			m_pInputNoteMappingComboBox->currentIndex() );
		if ( pMidiInstrumentMap->getInput() != input ) {
			pMidiInstrumentMap->setInput( input );
			updateInstrumentTable();
		}
	} );

	pMappingGridLayout->addWidget( m_pInputNoteMappingComboBox, 1, 0, 1, 2,
							  Qt::AlignCenter );

	/*: Label of an option in the mapping tab of the MIDI control dialog
	 *  specifying how incoming and outgoing MIDI notes and the instruments of
	 *  the current drumkit should relate to eachother. */
	auto pNoteMappingLabel = new QLabel( tr( "Note Mapping" ), pMappingTab );
	pNoteMappingLabel->setStyleSheet( QString( "\
font-size: %1px;" ).arg( nSettingTextSize ) );
	pMappingGridLayout->addWidget( pNoteMappingLabel, 1, 3, Qt::AlignCenter );

	m_pOutputNoteMappingComboBox = new QComboBox( pMappingTab );
	m_pOutputNoteMappingComboBox->setFixedSize(
		MidiControlDialog::nColumnMappingWidth * 2,
		MidiControlDialog::nMappingBoxHeight );
	m_pOutputNoteMappingComboBox->insertItems( 0,
		QStringList() << MidiInstrumentMap::OutputToQString(
				MidiInstrumentMap::Output::None )
			<< MidiInstrumentMap::OutputToQString(
				MidiInstrumentMap::Output::Offset )
			<< MidiInstrumentMap::OutputToQString(
				MidiInstrumentMap::Output::Constant ) );
	m_pOutputNoteMappingComboBox->setCurrentIndex(
		static_cast<int>( pMidiInstrumentMap->getOutput() ) );
	connect( m_pOutputNoteMappingComboBox,
			 QOverload<int>::of( &QComboBox::activated ), [=]( int ) {
		auto pMidiInstrumentMap =
			Preferences::get_instance()->getMidiInstrumentMap();
		auto output = static_cast<MidiInstrumentMap::Output>(
			m_pOutputNoteMappingComboBox->currentIndex() );
		if ( pMidiInstrumentMap->getOutput() != output ) {
			pMidiInstrumentMap->setOutput( output );
			updateInstrumentTable();
		}
	} );

	pMappingGridLayout->addWidget( m_pOutputNoteMappingComboBox, 1, 5, 1, 2,
							  Qt::AlignCenter );

	auto pSeparatorNoteMapping = createSeparator( pMappingSettingsWidget, true );
	pMappingGridLayout->addWidget( pSeparatorNoteMapping, 2, 0, 1, 7,
								  Qt::AlignBottom );


	// -1 does not turn this channel "off". Instead, the combo box above it can
	// be set to None.
	m_pGlobalInputChannelSpinBox = new LCDSpinBox(
		pMappingTab, QSize( MidiControlDialog::nColumnMappingWidth,
						   MidiControlDialog::nMappingBoxHeight ),
		LCDSpinBox::Type::Int, MidiMessage::nChannelAll,
		MidiMessage::nChannelMaximum,
		LCDSpinBox::Flag::MinusOneAsOff | LCDSpinBox::Flag::MinusTwoAsAll );
	m_pGlobalInputChannelSpinBox->setValue(
		pMidiInstrumentMap->getGlobalInputChannel() );
	m_pGlobalInputChannelSpinBox->setEnabled(
		pMidiInstrumentMap->getUseGlobalInputChannel() );
	connect( m_pGlobalInputChannelSpinBox,
			QOverload<double>::of(&QDoubleSpinBox::valueChanged),
			[&](double fValue) {
				Preferences::get_instance()->getMidiInstrumentMap()
					->setGlobalInputChannel( static_cast<int>( fValue ) );
				updateInstrumentTable();
	});
	pMappingGridLayout->addWidget( m_pGlobalInputChannelSpinBox, 3, 0,
							  Qt::AlignCenter );

	m_pGlobalInputChannelCheckBox = new QCheckBox( pMappingTab );
	m_pGlobalInputChannelCheckBox->setChecked(
		pMidiInstrumentMap->getUseGlobalInputChannel() );
	connect( m_pGlobalInputChannelCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->getMidiInstrumentMap()
			->setUseGlobalInputChannel(
				m_pGlobalInputChannelCheckBox->isChecked() );
		m_pGlobalInputChannelSpinBox->setEnabled(
			m_pGlobalInputChannelCheckBox->isChecked() );
		updateInstrumentTable();
	} );
	pMappingGridLayout->addWidget( m_pGlobalInputChannelCheckBox, 3, 1,
							  Qt::AlignCenter );

	/*: Label of an option in the mapping tab of the MIDI control dialog
	 *  specifying whether (and which) all instruments of the current drumkit
	 *  should feature the same incoming and outgoing MIDI channel. */
	auto pGlobalChannelLabel = new QLabel( tr( "Global Channel" ), pMappingTab );
	pGlobalChannelLabel->setStyleSheet( QString( "\
font-size: %1px;" ).arg( nSettingTextSize ) );
	pMappingGridLayout->addWidget( pGlobalChannelLabel, 3, 3, Qt::AlignCenter );

	// -1 does not turn this channel "off". Instead, the combo box above it can
	// be set to None.
	m_pGlobalOutputChannelSpinBox = new LCDSpinBox(
		pMappingTab, QSize( MidiControlDialog::nColumnMappingWidth,
						   MidiControlDialog::nMappingBoxHeight ),
		LCDSpinBox::Type::Int, MidiMessage::nChannelOff,
		MidiMessage::nChannelMaximum, LCDSpinBox::Flag::MinusOneAsOff );
	m_pGlobalOutputChannelSpinBox->setValue(
		pMidiInstrumentMap->getGlobalOutputChannel() );
	m_pGlobalOutputChannelSpinBox->setEnabled(
		pMidiInstrumentMap->getUseGlobalOutputChannel() );
	connect( m_pGlobalOutputChannelSpinBox,
			QOverload<double>::of(&QDoubleSpinBox::valueChanged),
			[&](double fValue) {
				Preferences::get_instance()->getMidiInstrumentMap()
					->setGlobalOutputChannel( static_cast<int>( fValue ) );
				updateInstrumentTable();
	});
	pMappingGridLayout->addWidget( m_pGlobalOutputChannelSpinBox, 3, 6,
							  Qt::AlignCenter );

	m_pGlobalOutputChannelCheckBox = new QCheckBox( pMappingTab );
	m_pGlobalOutputChannelCheckBox->setChecked(
		pMidiInstrumentMap->getUseGlobalOutputChannel() );
	connect( m_pGlobalOutputChannelCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->getMidiInstrumentMap()
			->setUseGlobalOutputChannel(
				m_pGlobalOutputChannelCheckBox->isChecked() );
		m_pGlobalOutputChannelSpinBox->setEnabled(
			m_pGlobalOutputChannelCheckBox->isChecked() );
		updateInstrumentTable();
	} );
	pMappingGridLayout->addWidget( m_pGlobalOutputChannelCheckBox, 3, 5,
							  Qt::AlignCenter );


	pMappingGridLayout->setColumnMinimumWidth( 0, MidiControlDialog::nColumnMappingWidth );
	pMappingGridLayout->setColumnMinimumWidth( 1, MidiControlDialog::nColumnMappingWidth );
	pMappingGridLayout->setColumnMinimumWidth( 3, MidiControlDialog::nColumnMappingWidth );
	pMappingGridLayout->setColumnMinimumWidth( 5, MidiControlDialog::nColumnMappingWidth );
	pMappingGridLayout->setColumnMinimumWidth( 6, MidiControlDialog::nColumnMappingWidth );
	pMappingGridLayout->setColumnStretch( 0, 0 );
	pMappingGridLayout->setColumnStretch( 1, 0 );
	pMappingGridLayout->setColumnStretch( 2, 0 );
	pMappingGridLayout->setColumnStretch( 3, 1 );
	pMappingGridLayout->setColumnStretch( 4, 0 );
	pMappingGridLayout->setColumnStretch( 5, 0 );
	pMappingGridLayout->setColumnStretch( 6, 0 );

	m_pInstrumentTable = new QTableWidget( pMappingTab );
	m_pInstrumentTable->setSizePolicy( QSizePolicy::Expanding,
									  QSizePolicy::Expanding );
	m_pInstrumentTable->setSelectionMode( QAbstractItemView::NoSelection );
	m_pInstrumentTable->setRowCount( 0 );
	m_pInstrumentTable->setColumnCount( 5 );
	m_pInstrumentTable->setColumnWidth( 0, MidiControlDialog::nColumnMappingWidth );;
	m_pInstrumentTable->setColumnWidth( 1, MidiControlDialog::nColumnMappingWidth );;
	m_pInstrumentTable->setColumnWidth( 3, MidiControlDialog::nColumnMappingWidth );;
	m_pInstrumentTable->setColumnWidth( 4, MidiControlDialog::nColumnMappingWidth );;
	m_pInstrumentTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Fixed );
	m_pInstrumentTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::Fixed );
	m_pInstrumentTable->horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::Stretch );
	m_pInstrumentTable->horizontalHeader()->setSectionResizeMode(
		3, QHeaderView::Fixed );
	m_pInstrumentTable->horizontalHeader()->setSectionResizeMode(
		4, QHeaderView::Fixed );
	m_pInstrumentTable->verticalHeader()->hide();
	m_pInstrumentTable->setHorizontalHeaderLabels(
		QStringList() << pCommonStrings->getMidiOutChannelLabel()
				<< pCommonStrings->getMidiOutNoteLabel()
				<< pCommonStrings->getInstrumentButton()
				<< pCommonStrings->getMidiOutNoteLabel()
				<< pCommonStrings->getMidiOutChannelLabel() );

	pMappingWrapperLayout->addWidget( m_pInstrumentTable );

	////////////////////////////////////////////////////////////////////////////

	m_pMidiActionTable = new MidiActionTable( this );
	m_pTabWidget->addTab( m_pMidiActionTable, tr( "Midi Actions" ) );

	connect( m_pMidiActionTable, &MidiActionTable::changed, [=]() {
		m_pMidiActionTable->saveMidiActionTable();
		H2Core::EventQueue::get_instance()->pushEvent(
			H2Core::Event::Type::MidiEventMapChanged, 0 );
	});

	////////////////////////////////////////////////////////////////////////////

	auto pInputWidget = new QWidget( m_pTabWidget );
	m_pTabWidget->addTab( pInputWidget, tr( "Incoming" ) );
	auto pInputLayout = new QVBoxLayout( pInputWidget );
	pInputLayout->setContentsMargins( 0, 0, 0, 0 );
	pInputLayout->setSpacing( 1 );
	pInputWidget->setLayout( pInputLayout );

	m_pMidiInputTable = new QTableWidget( this );
	m_pMidiInputTable->setColumnCount( 7 );
	m_pMidiInputTable->setHorizontalHeaderLabels(
		QStringList() << tr( "Timestamp" ) << tr( "Type" ) << tr( "Data1" ) <<
		tr( "Data2" ) << tr( "Channel" ) << tr( "Action" ) << tr( "Instrument" ) );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::Stretch );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		3, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		4, QHeaderView::Fixed );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		5, QHeaderView::Stretch );
	m_pMidiInputTable->horizontalHeader()->setSectionResizeMode(
		6, QHeaderView::Stretch );

	m_pMidiInputTable->setColumnWidth(
		0, MidiControlDialog::nColumnTimestampWidth );
	m_pMidiInputTable->setColumnWidth( 1, MidiControlDialog::nColumnTypeWidth );
	m_pMidiInputTable->setColumnWidth(
		2, MidiControlDialog::nColumnValueWidth );
	m_pMidiInputTable->setColumnWidth(
		3, MidiControlDialog::nColumnValueWidth );
	m_pMidiInputTable->setColumnWidth(
		4, MidiControlDialog::nColumnValueWidth );
	m_pMidiInputTable->setColumnWidth(
		5, MidiControlDialog::nColumnActionWidth );
	m_pMidiInputTable->setColumnWidth(
		6, MidiControlDialog::nColumnInstrumentWidth );
	pInputLayout->addWidget( m_pMidiInputTable );

	const auto binButtonSize = QSize(
		MidiControlDialog::nBinButtonHeight * Skin::fButtonWidthHeightRatio,
		MidiControlDialog::nBinButtonHeight );

	auto addBinButton = [&]( QWidget* pParent ) {
		auto pContainerWidget = new QWidget( pParent );
		pParent->layout()->addWidget( pContainerWidget );
		auto pContainerLayout = new QHBoxLayout( pContainerWidget );
		pContainerLayout->setAlignment( Qt::AlignRight );
		pContainerLayout->setContentsMargins( 1, 1, 1, 1 );
		pContainerWidget->setLayout( pContainerLayout );

		auto pBinButton = new QToolButton( pContainerWidget );
		pBinButton->setCheckable( false );
		pBinButton->setFixedSize( binButtonSize );
		pBinButton->setIconSize(
			binButtonSize - QSize( MidiControlDialog::nBinButtonMargin,
								   MidiControlDialog::nBinButtonMargin ) );
		pContainerLayout->addWidget( pBinButton );

		return pBinButton;
	};
	m_pInputBinButton = addBinButton( pInputWidget );
	connect( m_pInputBinButton, &QToolButton::clicked, [&]() {
		auto pMidiDriver = Hydrogen::get_instance()->getMidiDriver();
		if ( pMidiDriver != nullptr ) {
			pMidiDriver->clearHandledInput();
		}
		updateInputTable();
	});

	////////////////////////////////////////////////////////////////////////////

	auto pOutputWidget = new QWidget( m_pTabWidget );
	m_pTabWidget->addTab( pOutputWidget, tr( "Outgoing" ) );
	auto pOutputLayout = new QVBoxLayout( pOutputWidget );
	pOutputLayout->setContentsMargins( 0, 0, 0, 0 );
	pOutputLayout->setSpacing( 1 );
	pOutputWidget->setLayout( pOutputLayout );

	m_pMidiOutputTable = new QTableWidget( pOutputWidget );
	m_pMidiOutputTable->setColumnCount( 5 );
	m_pMidiOutputTable->setHorizontalHeaderLabels(
		QStringList() << tr( "Timestamp" ) << tr( "Type" ) << tr( "Data1" ) <<
		tr( "Data2" ) << tr( "Channel" ) );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		1, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		2, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		3, QHeaderView::Stretch );
	m_pMidiOutputTable->horizontalHeader()->setSectionResizeMode(
		4, QHeaderView::Stretch );

	pOutputLayout->addWidget( m_pMidiOutputTable );
	m_pOutputBinButton = addBinButton( pOutputWidget );
	connect( m_pOutputBinButton, &QToolButton::clicked, [&]() {
		auto pMidiDriver = Hydrogen::get_instance()->getMidiDriver();
		if ( pMidiDriver != nullptr ) {
			pMidiDriver->clearHandledOutput();
		}
		updateOutputTable();
	});

	////////////////////////////////////////////////////////////////////////////

	// Since we only update the message tables in case they are visible, we have
	// to ensure to update them when bringing them into view.
	connect( m_pTabWidget, &QTabWidget::currentChanged, [&]() {
		updateInputTable();
		updateOutputTable();
	});

	updateFont();
	updateIcons();
	updateInstrumentTable();
	updateInputTable();
	updateOutputTable();

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &MidiControlDialog::onPreferencesChanged );
}

MidiControlDialog::~MidiControlDialog() {
}


void MidiControlDialog::drumkitLoadedEvent() {
	updateInstrumentTable();
}

void MidiControlDialog::instrumentParametersChangedEvent( int ) {
	updateInstrumentTable();
}

void MidiControlDialog::midiDriverChangedEvent() {
}

void MidiControlDialog::midiInputEvent() {
	updateInputTable();
}

void MidiControlDialog::midiOutputEvent() {
	updateOutputTable();
}

void MidiControlDialog::updatePreferencesEvent( int nValue ) {
	if ( nValue == 1 ) {
		// new preferences loaded within the core
		const auto pPref = H2Core::Preferences::get_instance();

		m_pInputActionChannelSpinBox->setValue( pPref->m_nMidiActionChannel );
		m_pInputIgnoreNoteOffCheckBox->setChecked( pPref->m_bMidiNoteOffIgnore );
		m_pOutputEnableMidiFeedbackCheckBox->setChecked(
			pPref->m_bEnableMidiFeedback );

		m_pMidiActionTable->setupMidiActionTable();
	}
}

void MidiControlDialog::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateInstrumentTable();
	}
}

void MidiControlDialog::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes )
{
	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcons();
	}
	if ( changes & H2Core::Preferences::Changes::Font ) {
		updateFont();
	}
}

void MidiControlDialog::hideEvent( QHideEvent* pEvent ) {
	UNUSED( pEvent );

	// Update corresponding button
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}

void MidiControlDialog::showEvent( QShowEvent* pEvent ) {
	UNUSED( pEvent );

	// Update corresponding button
	HydrogenApp::get_instance()->getMainToolBar()->updateActions();
}

void MidiControlDialog::updateFont() {
	const auto pPref = H2Core::Preferences::get_instance();
	const auto pFontTheme = pPref->getFontTheme();

	QFont font( pFontTheme->m_sApplicationFontFamily,
				getPointSize( pFontTheme->m_fontSize ) );
	QFont childFont( pFontTheme->m_sLevel2FontFamily,
					 getPointSize( pFontTheme->m_fontSize ) );
	setFont( font );

	// In order to affect the fonts of all child widgets in the table as well,
	// we have to use its stylesheet instead of the setFont() method.
	const auto sTableStyle = QString( "\
font-family: %1; \
font-size: %2; \
" )
		.arg( childFont.family() )
		.arg( childFont.pixelSize() );

	m_pMidiInputTable->setStyleSheet( sTableStyle );
	m_pMidiOutputTable->setStyleSheet( sTableStyle );
}

void MidiControlDialog::updateIcons() {
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pInputBinButton->setIcon( QIcon( sIconPath + "bin.svg" ) );
	m_pOutputBinButton->setIcon( QIcon( sIconPath + "bin.svg" ) );

}

void MidiControlDialog::updateInstrumentTable() {
	m_instrumentMap.clear();

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();

	const auto pMidiInstrumentMap =
		Preferences::get_instance()->getMidiInstrumentMap();

	const int nNewRowCount = pInstrumentList->size();
	while ( m_pInstrumentTable->rowCount() < nNewRowCount ) {
		// We first add enough "blank" rows to match the current drumkit and
		// fill their content later on. This increases rendering speed by a
		// margin when updating the contents for just the current drumkit.
		addInstrumentTableRow();
	}

	int nnRow = 0;
	for ( const auto ppInstrument : *pInstrumentList ) {
		if ( ppInstrument == nullptr ) {
			continue;
		}

		updateInstrumentTableRow( nnRow, ppInstrument );
		++nnRow;
	}

	m_pInstrumentTable->setRowCount( nnRow );
}

void MidiControlDialog::addInstrumentTableRow() {
	const int nNewRowCount = m_pInstrumentTable->rowCount() + 1;
	m_pInstrumentTable->setRowCount( nNewRowCount );

	auto pInputChannelSpinBox = new LCDSpinBox(
		m_pInstrumentTable, QSize( MidiControlDialog::nColumnMappingWidth,
								   MidiControlDialog::nMappingBoxHeight ),
		LCDSpinBox::Type::Int, MidiMessage::nChannelAll,
		MidiMessage::nChannelMaximum,
		LCDSpinBox::Flag::MinusOneAsOff | LCDSpinBox::Flag::MinusTwoAsAll );
	pInputChannelSpinBox->setSizePolicy( QSizePolicy::Expanding,
										 QSizePolicy::Fixed );

	auto pInputNoteSpinBox = new LCDSpinBox(
		m_pInstrumentTable, QSize( MidiControlDialog::nColumnMappingWidth,
								   MidiControlDialog::nMappingBoxHeight ),
		LCDSpinBox::Type::Int, MidiMessage::nNoteMinimum,
		MidiMessage::nNoteMaximum, LCDSpinBox::Flag::None );
	pInputNoteSpinBox->setSizePolicy( QSizePolicy::Expanding,
									  QSizePolicy::Fixed );


	auto pInstrumentLabel = new QLabel( "", m_pInstrumentTable );
	pInstrumentLabel->setAlignment( Qt::AlignCenter );
	pInstrumentLabel->setSizePolicy( QSizePolicy::Expanding,
									 QSizePolicy::Fixed );

	auto pOutputNoteSpinBox = new LCDSpinBox(
		m_pInstrumentTable, QSize( MidiControlDialog::nColumnMappingWidth,
								   MidiControlDialog::nMappingBoxHeight ),
		LCDSpinBox::Type::Int, MidiMessage::nNoteMinimum,
		MidiMessage::nNoteMaximum, LCDSpinBox::Flag::ModifyOnChange );

	auto pOutputChannelSpinBox = new LCDSpinBox(
		m_pInstrumentTable, QSize( MidiControlDialog::nColumnMappingWidth,
								   MidiControlDialog::nMappingBoxHeight ),
		LCDSpinBox::Type::Int, MidiMessage::nChannelOff,
		MidiMessage::nChannelMaximum,
		LCDSpinBox::Flag::ModifyOnChange | LCDSpinBox::Flag::MinusOneAsOff );
	pOutputChannelSpinBox->setSizePolicy( QSizePolicy::Expanding,
										  QSizePolicy::Fixed );

	m_pInstrumentTable->setCellWidget( nNewRowCount - 1, 0, pInputChannelSpinBox );
	m_pInstrumentTable->setCellWidget( nNewRowCount - 1, 1, pInputNoteSpinBox );
	m_pInstrumentTable->setCellWidget( nNewRowCount - 1, 2, pInstrumentLabel );
	m_pInstrumentTable->setCellWidget( nNewRowCount - 1, 3, pOutputNoteSpinBox );
	m_pInstrumentTable->setCellWidget( nNewRowCount - 1, 4, pOutputChannelSpinBox );

	return;
}

void MidiControlDialog::updateInstrumentTableRow(
	int nRow, std::shared_ptr<Instrument> pInstrument ) {
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "Invalid instrument for row [%1]" ).arg( nRow ) );
		return;
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	const auto pMidiInstrumentMap =
		Preferences::get_instance()->getMidiInstrumentMap();

	const auto instrumentHandle = std::make_pair( pInstrument->getType(),
												  pInstrument->getId() );
	m_instrumentMap[ instrumentHandle ] = pInstrument;
	const auto inputMapping =
		pMidiInstrumentMap->getInputMapping( pInstrument,
											 pSong->getDrumkit() );
	auto pInputChannelSpinBox =
		static_cast<LCDSpinBox*>(m_pInstrumentTable->cellWidget( nRow, 0 ));
	auto pInputNoteSpinBox =
		static_cast<LCDSpinBox*>(m_pInstrumentTable->cellWidget( nRow, 1 ) );
	if ( pInputChannelSpinBox != nullptr && pInputNoteSpinBox != nullptr ) {
		if ( ! inputMapping.isNull() ) {
			pInputChannelSpinBox->setValue( inputMapping.nChannel );
			pInputNoteSpinBox->setValue( inputMapping.nNote );
		}
		else {
			pInputChannelSpinBox->setValue( MidiMessage::nChannelOff );
		}

		if ( pMidiInstrumentMap->getInput() != MidiInstrumentMap::Input::Custom ) {
			pInputNoteSpinBox->setEnabled( false );
			pInputChannelSpinBox->setEnabled( false );
		}
		else {
			pInputNoteSpinBox->setEnabled( true );
			pInputChannelSpinBox->setEnabled(
				! pMidiInstrumentMap->getUseGlobalInputChannel() );
		}

		disconnect( pInputChannelSpinBox );
		connect( pInputChannelSpinBox,
				 QOverload<double>::of(&QDoubleSpinBox::valueChanged),
				 [=](double fValue) {
					 auto pInstrument = m_instrumentMap.at( instrumentHandle );
					 if ( pInstrument != nullptr ) {
						 Preferences::get_instance()->getMidiInstrumentMap()
							 ->insertCustomInputMapping( pInstrument,
														 pInputNoteSpinBox->value(),
														 static_cast<int>( fValue ) );
					 }
					 else {
						 ERRORLOG( QString( "No instr. for [%1 : %2]" )
								   .arg( instrumentHandle.first )
								   .arg( instrumentHandle.second ) );
					 }
		});
		disconnect( pInputNoteSpinBox );
		connect( pInputNoteSpinBox,
				 QOverload<double>::of(&QDoubleSpinBox::valueChanged),
				 [=](double fValue) {
					 auto pInstrument = m_instrumentMap.at( instrumentHandle );
					 if ( pInstrument != nullptr ) {
						 Preferences::get_instance()->getMidiInstrumentMap()
							 ->insertCustomInputMapping( pInstrument,
														 static_cast<int>( fValue ),
														 pInputChannelSpinBox->value() );
					 }
					 else {
						 ERRORLOG( QString( "No instr. for [%1 : %2]" )
								   .arg( instrumentHandle.first )
								   .arg( instrumentHandle.second ) );
					 }
		});
	}
	else {
		ERRORLOG( QString( "Unable to obtain input channel or note for row [%1]" )
				  .arg( nRow ) );
	}

	auto pInstrumentLabel =
		static_cast<QLabel*>(m_pInstrumentTable->cellWidget( nRow, 2 ) );
	if ( pInstrumentLabel != nullptr ) {
		pInstrumentLabel->setText( pInstrument->getName() );
	}
	else {
		ERRORLOG( QString( "Unable to obtain instrument label for row [%1]" )
				  .arg( nRow ) );
	}

	const auto outputMapping =
			pMidiInstrumentMap->getOutputMapping( nullptr, pInstrument );
	auto pOutputNoteSpinBox =
		static_cast<LCDSpinBox*>(m_pInstrumentTable->cellWidget( nRow, 3 ) );
	if ( pOutputNoteSpinBox != nullptr ) {
		if ( ! outputMapping.isNull() ) {
			pOutputNoteSpinBox->setValue( outputMapping.nNote );
		}
		pOutputNoteSpinBox->setEnabled(
			pMidiInstrumentMap->getOutput() != MidiInstrumentMap::Output::None );
		disconnect( pOutputNoteSpinBox );
		connect( pOutputNoteSpinBox,
				 QOverload<double>::of(&QDoubleSpinBox::valueChanged),
				 [=](double fValue) {
					 auto pInstrument = m_instrumentMap.at( instrumentHandle );
					 if ( pInstrument != nullptr ) {
						 long nEventId = Event::nInvalidId;
						 CoreActionController::setInstrumentMidiOutNote(
							 pInstrument->getId(), static_cast<int>(fValue),
							 &nEventId );
						 if ( nEventId != Event::nInvalidId ) {
							 // Ensure we do not act on the queued event ourself.
							 blacklistEventId( nEventId );
						 }
					 }
					 else {
						 ERRORLOG( QString( "No instr. for [%1 : %2]" )
								   .arg( instrumentHandle.first )
								   .arg( instrumentHandle.second ) );
					 }

					 // Tweaking the output note could result in the input note
					 // to change as well since the former is used as fallback
					 // in many scenarios.
					 auto pSong = Hydrogen::get_instance()->getSong();
					 if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
						 return;
					 }
					 const auto instrumentHandle =
						 std::make_pair( pInstrument->getType(),
										 pInstrument->getId() );
					 const auto inputMapping =
						 pMidiInstrumentMap->getInputMapping(
							 pInstrument,
							 pSong->getDrumkit() );
					 if ( ! inputMapping.isNull() ) {
						 pInputNoteSpinBox->setValue( inputMapping.nNote );
					 }
			});
	}
	else {
		ERRORLOG( QString( "Unable to obtain output note for row [%1]" )
				  .arg( nRow ) );
	}

	auto pOutputChannelSpinBox =
		static_cast<LCDSpinBox*>(m_pInstrumentTable->cellWidget( nRow, 4 ) );
	if ( pOutputChannelSpinBox != nullptr ) {
		if ( ! outputMapping.isNull() ) {
			pOutputChannelSpinBox->setValue( outputMapping.nChannel );
		}
		else {
			pOutputChannelSpinBox->setValue( MidiMessage::nChannelOff );
		}
		pOutputChannelSpinBox->setEnabled(
			pMidiInstrumentMap->getOutput() != MidiInstrumentMap::Output::None &&
			! pMidiInstrumentMap->getUseGlobalOutputChannel() );
		disconnect( pOutputChannelSpinBox );
		connect( pOutputChannelSpinBox,
				 QOverload<double>::of(&QDoubleSpinBox::valueChanged),
				 [=](double fValue) {
					 auto pInstrument = m_instrumentMap.at( instrumentHandle );
					 if ( pInstrument != nullptr ) {
						 long nEventId = Event::nInvalidId;
						 CoreActionController::setInstrumentMidiOutChannel(
							 pInstrument->getId(), static_cast<int>(fValue),
							 &nEventId );
						 if ( nEventId != Event::nInvalidId ) {
							 // Ensure we do not act on the queued event ourself.
							 blacklistEventId( nEventId );
						 }
					 }
					 else {
						 ERRORLOG( QString( "No instr. for [%1 : %2]" )
								   .arg( instrumentHandle.first )
								   .arg( instrumentHandle.second ) );
					 }

					 // Tweaking the output channel could result in the input
					 // channel to change as well since the former is used as
					 // fallback in many scenarios.
					 auto pSong = Hydrogen::get_instance()->getSong();
					 if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
						 return;
					 }
					 const auto instrumentHandle =
						 std::make_pair( pInstrument->getType(),
										 pInstrument->getId() );
					 const auto inputMapping =
						 pMidiInstrumentMap->getInputMapping(
							 pInstrument,
							 pSong->getDrumkit() );
					 if ( ! inputMapping.isNull() ) {
						 pInputChannelSpinBox->setValue( inputMapping.nChannel );
					 }
					 else {
						 pInputChannelSpinBox->setValue( MidiMessage::nChannelOff );
					 }
		});
	}
	else {
		ERRORLOG( QString( "Unable to obtain output channel for row [%1]" )
				  .arg( nRow ) );
	}
}

void MidiControlDialog::updateInputTable() {
	if ( ! m_pMidiInputTable->isVisible() ) {
		return;
	}

	auto pMidiDriver = Hydrogen::get_instance()->getMidiDriver();

	std::vector< std::shared_ptr<MidiInput::HandledInput> > handledInputs;
	if ( pMidiDriver != nullptr ) {
		handledInputs = pMidiDriver->getHandledInputs();
	}

	const int nOldRowCount = m_pMidiInputTable->rowCount();
	m_pMidiInputTable->setRowCount( handledInputs.size() );

	// First, we check whether the table holds mostly the same entries and we
	// just have to insert a new one at the bottom. But after
	// MidiBaseDriver::nBacklogSize events the first one will be poped and a new
	// one appended.
	bool bInvalid = false;
	for ( int ii = 0; ii < handledInputs.size(); ++ii ) {
		if ( ii < nOldRowCount ) {
			auto ppLabel = dynamic_cast<QLabel*>(
				m_pMidiInputTable->cellWidget( ii, 0 ) );
			if ( ppLabel == nullptr || handledInputs[ ii ] != nullptr ||
				 ppLabel->text() != H2Core::timePointToQString(
					 handledInputs[ ii ]->timePoint ) ) {
				bInvalid = true;
				break;
			}
		}
	}

	auto newLabel = [&]( const QString& sText ) {
		auto pLabel = new QLabel( m_pMidiInputTable );
		pLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		pLabel->setAlignment( Qt::AlignCenter );
		pLabel->setText( sText );

		return pLabel;
	};

	auto addRow = [&]( std::shared_ptr<MidiBaseDriver::HandledInput> pHandledInput,
					   int nRow ) {
		if ( pHandledInput == nullptr ) {
			return;
		}
		m_pMidiInputTable->setCellWidget(
			nRow, 0, newLabel( H2Core::timePointToQString( pHandledInput->timePoint ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 1, newLabel( MidiMessage::TypeToQString( pHandledInput->type ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 2, newLabel( QString::number( pHandledInput->nData1 ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 3, newLabel( QString::number( pHandledInput->nData2 ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 4, newLabel( QString::number( pHandledInput->nChannel ) ) );

		QStringList types;
		for ( const auto& ttype : pHandledInput->actionTypes ) {
			types << MidiAction::typeToQString( ttype );
		}
		m_pMidiInputTable->setCellWidget(
			nRow, 5, newLabel( types.join( ", " ) ) );
		m_pMidiInputTable->setCellWidget(
			nRow, 6, newLabel( pHandledInput->mappedInstruments.join( ", " ) ) );
	};

	auto updateRow = [&]( std::shared_ptr<MidiBaseDriver::HandledInput> pHandledInput,
						  int nRow ) {
		if ( pHandledInput == nullptr ) {
			return;
		}
		auto ppLabelTimestamp = dynamic_cast<QLabel*>(
			m_pMidiInputTable->cellWidget( nRow, 0 ) );
		if ( ppLabelTimestamp != nullptr ) {
			ppLabelTimestamp->setText( H2Core::timePointToQString( pHandledInput->timePoint ) );
		}
		auto ppLabelType = dynamic_cast<QLabel*>(
			m_pMidiInputTable->cellWidget( nRow, 1 ) );
		if ( ppLabelType != nullptr ) {
			ppLabelType->setText( MidiMessage::TypeToQString( pHandledInput->type ) );
		}
		auto ppLabelData1 = dynamic_cast<QLabel*>(
			m_pMidiInputTable->cellWidget( nRow, 2 ) );
		if ( ppLabelData1 != nullptr ) {
			ppLabelData1->setText( QString::number( pHandledInput->nData1 ) );
		}
		auto ppLabelData2 = dynamic_cast<QLabel*>(
			m_pMidiInputTable->cellWidget( nRow, 3 ) );
		if ( ppLabelData2 != nullptr ) {
			ppLabelData2->setText( QString::number( pHandledInput->nData2 ) );
		}
		auto ppLabelChannel = dynamic_cast<QLabel*>(
			m_pMidiInputTable->cellWidget( nRow, 4 ) );
		if ( ppLabelChannel != nullptr ) {
			ppLabelChannel->setText( QString::number( pHandledInput->nChannel ) );
		}

		QStringList types;
		for ( const auto& ttype : pHandledInput->actionTypes ) {
			types << MidiAction::typeToQString( ttype );
		}
		auto ppLabelAction = dynamic_cast<QLabel*>(
			m_pMidiInputTable->cellWidget( nRow, 5 ) );
		if ( ppLabelAction != nullptr ) {
			ppLabelAction->setText( types.join( ", " ) );
		}
		auto ppLabelInstruments = dynamic_cast<QLabel*>(
			m_pMidiInputTable->cellWidget( nRow, 6 ) );
		if ( ppLabelInstruments != nullptr ) {
			ppLabelInstruments->setText(
				pHandledInput->mappedInstruments.join( ", " ) );
		}
	};

	// Table is in prestine shape. We just add the missing rows.
	if ( ! bInvalid ) {
		for ( int ii = nOldRowCount; ii < handledInputs.size(); ++ii ) {
			addRow( handledInputs[ ii ], ii );
		}
	}
	else {
		// Renew the whole table.
		for ( int ii = 0; ii < handledInputs.size(); ++ii ) {
			// Check whether there are already labels within this row. (Should
			// be. But let's be sure.)
			auto ppLabel = dynamic_cast<QLabel*>(
				m_pMidiInputTable->cellWidget( ii, 0 ) );
			if ( ppLabel == nullptr ) {
				addRow( handledInputs[ ii ], ii );
			}
			else {
				updateRow( handledInputs[ ii ], ii );
			}
		}
	}
}

void MidiControlDialog::updateOutputTable() {
	if ( ! m_pMidiOutputTable->isVisible() ) {
		return;
	}

	auto pMidiDriver = Hydrogen::get_instance()->getMidiDriver();

	std::vector< std::shared_ptr<MidiOutput::HandledOutput> > handledOutputs;
	if ( pMidiDriver != nullptr ) {
		handledOutputs = pMidiDriver->getHandledOutputs();
	}

	const int nOldRowCount = m_pMidiOutputTable->rowCount();
	m_pMidiOutputTable->setRowCount( handledOutputs.size() );

	// First, we check whether the table holds mostly the same entries and we
	// just have to insert a new one at the bottom.
	bool bInvalid = false;
	for ( int ii = 0; ii < handledOutputs.size(); ++ii ) {
		if ( ii < m_pMidiOutputTable->rowCount() ) {
			auto ppLabel = dynamic_cast<QLabel*>(
				m_pMidiOutputTable->cellWidget( ii, 0 ) );
			if ( ppLabel == nullptr || handledOutputs[ ii ] == nullptr ||
				 ppLabel->text() != H2Core::timePointToQString(
					 handledOutputs[ ii ]->timePoint ) ) {
				bInvalid = true;
				break;
			}
		}
	}

	auto newLabel = [&]( const QString& sText ) {
		auto pLabel = new QLabel( this );
		pLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
		pLabel->setAlignment( Qt::AlignCenter );
		pLabel->setText( sText );
		pLabel->setToolTip( sText );

		return pLabel;
	};

	auto addRow = [&]( std::shared_ptr<MidiBaseDriver::HandledOutput> pHandledOutput,
					   int nRow ) {
		if ( pHandledOutput == nullptr ) {
			return;
		}
		m_pMidiOutputTable->setCellWidget(
			nRow, 0, newLabel( H2Core::timePointToQString( pHandledOutput->timePoint ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 1, newLabel( MidiMessage::TypeToQString( pHandledOutput->type ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 2, newLabel( QString::number( pHandledOutput->nData1 ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 3, newLabel( QString::number( pHandledOutput->nData2 ) ) );
		m_pMidiOutputTable->setCellWidget(
			nRow, 4, newLabel( QString::number( pHandledOutput->nChannel ) ) );
	};

	auto updateRow = [&]( std::shared_ptr<MidiBaseDriver::HandledOutput> pHandledOutput,
						  int nRow ) {
		if ( pHandledOutput == nullptr ) {
			return;
		}
		auto ppLabelTimestamp = dynamic_cast<QLabel*>(
			m_pMidiOutputTable->cellWidget( nRow, 0 ) );
		if ( ppLabelTimestamp != nullptr ) {
			ppLabelTimestamp->setText( H2Core::timePointToQString( pHandledOutput->timePoint ) );
		}
		auto ppLabelType = dynamic_cast<QLabel*>(
			m_pMidiOutputTable->cellWidget( nRow, 1 ) );
		if ( ppLabelType != nullptr ) {
			ppLabelType->setText( MidiMessage::TypeToQString( pHandledOutput->type ) );
		}
		auto ppLabelData1 = dynamic_cast<QLabel*>(
			m_pMidiOutputTable->cellWidget( nRow, 2 ) );
		if ( ppLabelData1 != nullptr ) {
			ppLabelData1->setText( QString::number( pHandledOutput->nData1 ) );
		}
		auto ppLabelData2 = dynamic_cast<QLabel*>(
			m_pMidiOutputTable->cellWidget( nRow, 3 ) );
		if ( ppLabelData2 != nullptr ) {
			ppLabelData2->setText( QString::number( pHandledOutput->nData2 ) );
		}
		auto ppLabelChannel = dynamic_cast<QLabel*>(
			m_pMidiOutputTable->cellWidget( nRow, 4 ) );
		if ( ppLabelChannel != nullptr ) {
			ppLabelChannel->setText( QString::number( pHandledOutput->nChannel ) );
		}
	};

	// Table is in prestine shape. We just add the missing rows.
	if ( ! bInvalid ) {
		for ( int ii = nOldRowCount; ii < handledOutputs.size(); ++ii ) {
			addRow( handledOutputs[ ii ], ii );
		}
	}
	else {
		// Renew the whole table.
		for ( int ii = 0; ii < handledOutputs.size(); ++ii ) {
			// Check whether there are already labels within this row. (Should
			// be. But let's be sure.)
			auto ppLabel = dynamic_cast<QLabel*>(
				m_pMidiOutputTable->cellWidget( ii, 0 ) );
			if ( ppLabel == nullptr ) {
				addRow( handledOutputs[ ii ], ii );
			}
			else {
				updateRow( handledOutputs[ ii ], ii );
			}
		}
	}
}
