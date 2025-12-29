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

#include "MidiSenseWidget.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"

#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEvent.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Preferences/Preferences.h>

MidiSenseWidget::MidiSenseWidget(
	QWidget* pParent,
	bool bDirectWrite,
	std::shared_ptr<MidiAction> pAction
)
	: QDialog( pParent ),
	  m_lastMidiEvent( H2Core::MidiEvent::Type::Null ),
	  m_nLastMidiEventParameter( 0 )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	m_bDirectWrite = bDirectWrite;
	m_pAction = pAction;

	setWindowTitle( pCommonStrings->getMidiSenseWindowTitle() );
	setMinimumWidth( 280 );

	auto pMainLayout = new QVBoxLayout( this );
    pMainLayout->setSpacing( 10 );
	setLayout( pMainLayout );

	m_pActionLabel = new QLabel( this );
	m_pActionLabel->setObjectName( "MidiSenseWidgetActionLabel" );
	m_pActionLabel->setAlignment( Qt::AlignCenter );
	pMainLayout->addWidget( m_pActionLabel );

	m_pCurrentBindingsLabel = new QLabel( this );
	m_pCurrentBindingsLabel->setObjectName(
		"MidiSenseWidgetCurrentBindingsLabel"
	);
	m_pCurrentBindingsLabel->setAlignment( Qt::AlignCenter );
	m_pCurrentBindingsLabel->setText(
		pCommonStrings->getMidiSenseCurrentBindings()
	);
	pMainLayout->addWidget( m_pCurrentBindingsLabel );

	m_pCurrentBindingsList = new QLabel( this );
	m_pCurrentBindingsList->setObjectName(
		"MidiSenseWidgetCurrentBindingsList"
	);
	m_pCurrentBindingsList->setIndent( 10 );
	m_pCurrentBindingsList->setAlignment( Qt::AlignLeft );
	pMainLayout->addWidget( m_pCurrentBindingsList );

	m_pSeparator = new QWidget( this );
	m_pSeparator->setObjectName( "MidiSenseWidgetSeparator" );
	m_pSeparator->setFixedHeight( 1 );
    pMainLayout->addWidget( m_pSeparator );

	m_pTextLabel = new QLabel( this );
	m_pTextLabel->setAlignment( Qt::AlignCenter );
	pMainLayout->addWidget( m_pTextLabel );

	H2Core::Hydrogen* pHydrogen = H2Core::Hydrogen::get_instance();
	pHydrogen->setLastMidiEvent( H2Core::MidiEvent::Type::Null );
	pHydrogen->setLastMidiEventParameter( 0 );

	m_pUpdateTimer = new QTimer( this );

	updateLabels();
    updateStyleSheet();

	if ( m_pAction != nullptr || !m_bDirectWrite ) {
		/*
		 * If the widget is not midi operable, we can omit
		 * starting the timer which listens to midi input..
		 */

		connect(
			m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateMidi() )
		);
		m_pUpdateTimer->start( 100 );
	}
};

MidiSenseWidget::~MidiSenseWidget()
{
	m_pUpdateTimer->stop();
}

void MidiSenseWidget::updateMidi()
{
	H2Core::Hydrogen* pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen->getLastMidiEvent() != H2Core::MidiEvent::Type::Null ) {
		m_lastMidiEvent = pHydrogen->getLastMidiEvent();
		m_nLastMidiEventParameter = pHydrogen->getLastMidiEventParameter();

		if ( m_bDirectWrite ) {
			// write the Midiaction / parameter combination to the midiMap
			auto pMidiEventMap =
				H2Core::Preferences::get_instance()->getMidiEventMap();

			assert( m_pAction );

			auto pAction = std::make_shared<MidiAction>( m_pAction );
			pAction->setValue( "0" );

			switch ( m_lastMidiEvent ) {
				case H2Core::MidiEvent::Type::CC:
					pMidiEventMap->registerCCEvent(
						m_nLastMidiEventParameter, pAction
					);
					break;

				case H2Core::MidiEvent::Type::Note:
					pMidiEventMap->registerNoteEvent(
						m_nLastMidiEventParameter, pAction
					);
					break;

				case H2Core::MidiEvent::Type::PC:
					pMidiEventMap->registerPCEvent( pAction );
					break;

				case H2Core::MidiEvent::Type::Null:
					return;

				default:
					// MMC event
					pMidiEventMap->registerMMCEvent( m_lastMidiEvent, pAction );
			}

			H2Core::EventQueue::get_instance()->pushEvent(
				H2Core::Event::Type::MidiEventMapChanged, 0
			);
		}

		close();
	}
}

void MidiSenseWidget::updateLabels()
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	if ( m_pAction != nullptr ) {
		m_pActionLabel->setVisible( true );
		m_pActionLabel->setText( MidiAction::typeToQString( m_pAction->getType()
		) );

		// Bindings
		QStringList bindings;
		for ( const auto& [eevent, nnParam] :
			  H2Core::Preferences::get_instance()
				  ->getMidiEventMap()
				  ->getRegisteredMidiEvents( m_pAction ) ) {
			if ( eevent == H2Core::MidiEvent::Type::Note ||
				 eevent == H2Core::MidiEvent::Type::CC ) {
				bindings << QString( "\t- %1 : %2" )
								.arg( H2Core::MidiEvent::TypeToQString(
									eevent
								) )
								.arg( nnParam );
			}
			else {
				// PC and MMC_x do not have a parameter.
				bindings << QString( "\t- %1" )
								.arg( H2Core::MidiEvent::TypeToQString(
									eevent
								) );
			}
		}

		if ( bindings.size() > 0 ) {
			m_pCurrentBindingsLabel->setVisible( true );
			m_pCurrentBindingsList->setVisible( true );
			m_pCurrentBindingsList->setText( bindings.join( "\n" ) );
            m_pSeparator->setVisible( true );
		}
		else {
			m_pCurrentBindingsLabel->setVisible( false );
			m_pCurrentBindingsList->setVisible( false );
            m_pSeparator->setVisible( false );
		}
		m_pTextLabel->setText( pCommonStrings->getMidiSenseInput() );
	}
	else {
		/* Check if this widget got called from the MidiActionTable within the
		 * MIDI dialog - directWrite=false - or by clicking on a
		 * midiLearn-capable gui item - directWrite=true.
		 */
		if ( m_bDirectWrite ) {
			m_pTextLabel->setText( pCommonStrings->getMidiSenseUnavailable() );
		}
		else {
			m_pTextLabel->setText( pCommonStrings->getMidiSenseInput() );
		}
		m_pActionLabel->setVisible( false );
		m_pCurrentBindingsLabel->setVisible( false );
		m_pCurrentBindingsList->setVisible( false );
        m_pSeparator->setVisible( false );
	}
}

// Beware: this method is _not_ called whenever the colors in the preferences do
// change. This is because this widget is considered to be transient one not
// opened long enough by the user to justify more sophisticated event handling.
void MidiSenseWidget::updateStyleSheet()
{
	const auto pColorTheme =
		H2Core::Preferences::get_instance()->getColorTheme();

    const auto backgroundColor = pColorTheme->m_windowColor;
    const auto textColor = pColorTheme->m_windowTextColor;

	setStyleSheet( QString( "\
QWidget {\
    background-color: %1;\
    color: %2;\
}\
QWidget#MidiSenseWidgetSeparator {    \
    border: 1px solid %2;\
}\
QLabel#MidiSenseWidgetActionLabel {\
    border: 1px solid %2;\
    font-size: 18px;\
    padding: 2px;\
}\
" )
					   .arg( backgroundColor.name() )
					   .arg( textColor.name() ) );
}
