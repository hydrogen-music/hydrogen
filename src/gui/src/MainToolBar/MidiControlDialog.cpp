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

#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IO/MidiBaseDriver.h>
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
	setWindowTitle( "MidiControlDialog" );

	auto pMainLayout = new QVBoxLayout( this );
	setLayout( pMainLayout );

	m_pTabWidget = new QTabWidget( this );
	pMainLayout->addWidget( m_pTabWidget );

	////////////////////////////////////////////////////////////////////////////

	const QColor borderColor( 80, 80, 80 );
	const int nHeaderTextSize = 20;

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

	auto pInputLabel = new QLabel( pCommonStrings->getMidiInputLabel() );
	pInputLabel->setAlignment( Qt::AlignCenter );
	pInputLabel->setFixedHeight( 32 );
	pInputLabel->setStyleSheet( QString( "\
font-size: %1px;" ).arg( nHeaderTextSize ) );
	pInputSettingsLayout->addWidget( pInputLabel );

	// Well, `border-bottom` does not seem to work on QLabel.
	auto pInputSeparator = new QWidget( pInputSettingsWidget );
	pInputSeparator->setFixedHeight( 1 );
	pInputSeparator->setStyleSheet( QString( "\
background-color: %1;" ).arg( borderColor.name() ) );
	pInputSettingsLayout->addWidget( pInputSeparator );

	auto pInputChannelFilterWidget = new QWidget( pInputSettingsWidget );
	pInputSettingsLayout->addWidget( pInputChannelFilterWidget );
	auto pInputChannelFilterLayout = new QHBoxLayout( pInputChannelFilterWidget );
	pInputChannelFilterWidget->setLayout( pInputChannelFilterLayout );

	auto pInputChannelFilterLabel = new QLabel(
		pCommonStrings->getMidiOutChannelLabel() );
	pInputChannelFilterLayout->addWidget( pInputChannelFilterLabel );
	m_pInputChannelFilterComboBox = new QComboBox( pInputChannelFilterWidget );
	pInputChannelFilterLayout->addWidget( m_pInputChannelFilterComboBox );
	m_pInputChannelFilterComboBox->addItem( pCommonStrings->getAllLabel() );
	for ( int ii = 0; ii <= 15; ++ii ) {
		m_pInputChannelFilterComboBox->addItem( QString::number( ii ) );
	}
	if ( pPref->m_nMidiChannelFilter == -1 ) {
		m_pInputChannelFilterComboBox->setCurrentIndex( 0 );
	} else {
		m_pInputChannelFilterComboBox->setCurrentIndex(
			pPref->m_nMidiChannelFilter + 1 );
	}
	connect( m_pInputChannelFilterComboBox,
			 QOverload<int>::of( &QComboBox::activated ), [=]( int ) {
		auto pPref = Preferences::get_instance();
		if ( pPref->m_nMidiChannelFilter !=
			 m_pInputChannelFilterComboBox->currentIndex() - 1 ) {
			pPref->m_nMidiChannelFilter =
				m_pInputChannelFilterComboBox->currentIndex() - 1;
		}
	} );

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

	m_pInputDiscardAfterActionCheckBox = new QCheckBox( pInputSettingsWidget );
	m_pInputDiscardAfterActionCheckBox->setChecked(
		pPref->m_bMidiDiscardNoteAfterAction );
	/*: The character after the '&' symbol can be used as a shortcut via the Alt
	 *  modifier. It should not coincide with any other shortcut in the Settings
	 *  tab of the MidiControlDialog. If in question, you can just drop the
	 *  '&'. */
	m_pInputDiscardAfterActionCheckBox->setText(
		tr( "&Discard MIDI messages after action has been triggered" ) );
	pInputSettingsLayout->addWidget( m_pInputDiscardAfterActionCheckBox );
	connect( m_pInputDiscardAfterActionCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->m_bMidiDiscardNoteAfterAction =
			m_pInputDiscardAfterActionCheckBox->isChecked();
	} );

	m_pInputNoteAsOutputCheckBox = new QCheckBox( pInputSettingsWidget );
	m_pInputNoteAsOutputCheckBox->setChecked( pPref->m_bMidiFixedMapping );
	/*: The character after the '&' symbol can be used as a shortcut via the Alt
	 *  modifier. It should not coincide with any other shortcut in the Settings
	 *  tab of the MidiControlDialog. If in question, you can just drop the
	 *  '&'. */
	m_pInputNoteAsOutputCheckBox->setText( tr( "&Use output note as input note" ) );
	pInputSettingsLayout->addWidget( m_pInputNoteAsOutputCheckBox );
	connect( m_pInputNoteAsOutputCheckBox, &QAbstractButton::toggled, [=]() {
		Preferences::get_instance()->m_bMidiFixedMapping =
			m_pInputNoteAsOutputCheckBox->isChecked();
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

	auto pOutputSettingsWidget = new QWidget( pConfigWidget );
	pConfigLayout->addWidget( pOutputSettingsWidget );
	auto pOutputSettingsLayout = new QVBoxLayout( pOutputSettingsWidget );
	pOutputSettingsLayout->setAlignment( Qt::AlignTop );
	pOutputSettingsWidget->setLayout( pOutputSettingsLayout );

	auto pOutputLabel = new QLabel( pCommonStrings->getMidiOutLabel() );
	pOutputLabel->setAlignment( Qt::AlignCenter );
	pOutputLabel->setFixedHeight( 32 );
	pOutputLabel->setStyleSheet( QString( "\
font-size: %1px;" ).arg( nHeaderTextSize ) );
	pOutputSettingsLayout->addWidget( pOutputLabel );

	// Well, `border-bottom` does not seem to work on QLabel.
	auto pOutputSeparator = new QWidget( pOutputSettingsWidget );
	pOutputSeparator->setFixedHeight( 1 );
	pOutputSeparator->setStyleSheet( QString( "\
background-color: %1;" ).arg( borderColor.name() ) );
	pOutputSettingsLayout->addWidget( pOutputSeparator );

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

	m_pMidiActionTable = new MidiActionTable( this );
	m_pTabWidget->addTab( m_pMidiActionTable, tr( "Midi Actions" ) );

	connect( m_pMidiActionTable, &MidiActionTable::changed, [=]() {
		m_pMidiActionTable->saveMidiActionTable();
		H2Core::EventQueue::get_instance()->pushEvent(
			H2Core::Event::Type::MidiMapChanged, 0 );
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
	updateInputTable();
	updateOutputTable();

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &MidiControlDialog::onPreferencesChanged );
}

MidiControlDialog::~MidiControlDialog() {
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

		if ( pPref->m_nMidiChannelFilter == -1 ) {
			m_pInputChannelFilterComboBox->setCurrentIndex( 0 );
		}
		else {
			m_pInputChannelFilterComboBox->setCurrentIndex(
				pPref->m_nMidiChannelFilter + 1 );
		}

		m_pInputIgnoreNoteOffCheckBox->setChecked( pPref->m_bMidiNoteOffIgnore );
		m_pInputDiscardAfterActionCheckBox->setChecked(
			pPref->m_bMidiDiscardNoteAfterAction );
		m_pInputNoteAsOutputCheckBox->setChecked( pPref->m_bMidiFixedMapping );
		m_pOutputEnableMidiFeedbackCheckBox->setChecked(
			pPref->m_bEnableMidiFeedback );

		m_pMidiActionTable->setupMidiActionTable();
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

	QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
				getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	QFont childFont( pPref->getTheme().m_font.m_sLevel2FontFamily,
					 getPointSize( pPref->getTheme().m_font.m_fontSize ) );
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
	if ( Preferences::get_instance()->getTheme().m_interface.m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pInputBinButton->setIcon( QIcon( sIconPath + "bin.svg" ) );
	m_pOutputBinButton->setIcon( QIcon( sIconPath + "bin.svg" ) );

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
