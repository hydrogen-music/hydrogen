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

#include "InstrumentEditor.h"

#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>

#include "Rack.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Widgets/Button.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/InlineEdit.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/Rotary.h"
#include "core/Midi/Midi.h"

using namespace H2Core;

InstrumentEditor::InstrumentEditor( QWidget* pParent )
	: QWidget( pParent )
{
	setFixedWidth( Rack::nWidth );

    auto pHydrogenApp = HydrogenApp::get_instance();
	auto pCommonStrings = pHydrogenApp->getCommonStrings();

	// Instrument properties top
	m_pInstrumentProp = new PixmapWidget( this );
	m_pInstrumentProp->move( 0, 0 );
	m_pInstrumentProp->setPixmap( "/instrumentEditor/instrumentTab.png" );

	m_pInlineEdit = new InlineEdit( m_pInstrumentProp );
	m_pInlineEdit->hide();
	m_pInlineEdit->setAlignment( Qt::AlignCenter );
	m_pInlineEdit->setTextMargins( 0, 0, 0, 0 );
	m_pInlineEdit->setContentsMargins( 0, 0, 0, 0 );
	m_pInlineEdit->setStyleSheet( "\
font-weight: bold;\
font-size: 21px;" );

	m_pNameLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( Rack::nWidth -
								  InstrumentEditor::nMargin * 2, 27 ), "",
		ClickableLabel::DefaultColor::Bright, true );
	m_pNameLbl->move( 5, 4 );
	m_pNameLbl->setScaledContents( true );

	/////////////
	//Midi Out

	ClickableLabel* pMidiOutLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 61, 10 ), pCommonStrings->getMidiOutLabel() );
	pMidiOutLbl->move( 28, 281 );

	m_pMidiOutChannelLCD = new LCDSpinBox(
		m_pInstrumentProp, QSize( 59, 24 ), LCDSpinBox::Type::Int,
		static_cast<int>( Midi::ChannelOff ),
		static_cast<int>( Midi::ChannelMaximum ),
		LCDSpinBox::Flag::ModifyOnChange | LCDSpinBox::Flag::ZeroAsOff
	);
	m_pMidiOutChannelLCD->move( 146, 257 );
	m_pMidiOutChannelLCD->setToolTip( QString( tr( "Midi out channel" ) ) );
	connect(
		m_pMidiOutChannelLCD,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double fValue ) {
			auto pInstrument =
				Hydrogen::get_instance()->getSelectedInstrument();
			if ( pInstrument == nullptr ) {
				return;
			}
			auto pSong = Hydrogen::get_instance()->getSong();
			if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
				return;
			}
			CoreActionController::setInstrumentMidiOutChannel(
				pSong->getDrumkit()->getInstruments()->index( pInstrument ),
				Midi::channelFromIntClamp( static_cast<int>( fValue ) ), nullptr
			);
		}
	);
	m_pMidiOutChannelLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 61, 10 ),
		pCommonStrings->getMidiOutChannelLabel()
	);
	m_pMidiOutChannelLbl->move( 144, 281 );

	///
	m_pMidiOutNoteLCD = new LCDSpinBox(
		m_pInstrumentProp, QSize( 59, 24 ), LCDSpinBox::Type::Int,
		static_cast<int>( Midi::NoteMinimum ),
		static_cast<int>( Midi::NoteMaximum ), LCDSpinBox::Flag::ModifyOnChange
	);
	m_pMidiOutNoteLCD->move( 210, 257 );
	m_pMidiOutNoteLCD->setToolTip( QString( tr( "Midi out note" ) ) );
	connect(
		m_pMidiOutNoteLCD,
		QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
		[&]( double fValue ) {
			auto pInstrument =
				Hydrogen::get_instance()->getSelectedInstrument();
			if ( pInstrument == nullptr ) {
				return;
			}
			auto pSong = Hydrogen::get_instance()->getSong();
			if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
				return;
			}
			CoreActionController::setInstrumentMidiOutNote(
				pSong->getDrumkit()->getInstruments()->index( pInstrument ),
				Midi::noteFromIntClamp( static_cast<int>( fValue ) ), nullptr
			);
		}
	);
	m_pMidiOutNoteLbl =
		new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ) );
	m_pMidiOutNoteLbl->move( 208, 281 );

	/////////////

	connect( m_pNameLbl, &ClickableLabel::labelDoubleClicked, this, [=](){
		m_pInlineEdit->startEditing( m_pNameLbl->geometry(),
									m_pNameLbl->text() );
		m_pNameLbl->hide();
	} );

	connect( m_pInlineEdit, &InlineEdit::editAccepted, [=]() {
		if ( ! m_pInlineEdit->isVisible() ) {
			// Already rejected
			return;
		}

		m_pNameLbl->show();
		m_pInlineEdit->hide();
		auto pSong = Hydrogen::get_instance()->getSong();
		auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
		if ( pInstrument != nullptr && pSong != nullptr &&
			 pSong->getDrumkit() != nullptr ) {
			MainForm::action_drumkit_renameInstrument(
				pSong->getDrumkit()->getInstruments()->index( pInstrument ),
				m_pInlineEdit->text() );}
	} );

	connect( m_pInlineEdit, &InlineEdit::editRejected, [=]() {
		m_pNameLbl->show();
		m_pInlineEdit->hide();
		} );

	m_pPitchLCD = new LCDDisplay(
		m_pInstrumentProp, QSize( 56, 22 ), false, false );
	m_pPitchLCD->move( 33, 212 );
	m_pPitchLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 54, 10 ), pCommonStrings->getPitchLabel() );
	m_pPitchLbl->move( 34, 235 );
	
	m_pPitchCoarseRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Center, tr( "Pitch offset (Coarse)" ),
		true, Instrument::fPitchOffsetMinimum + InstrumentEditor::nPitchFineControl,
		Instrument::fPitchOffsetMaximum - InstrumentEditor::nPitchFineControl );
	m_pPitchCoarseRotary->move( 94, 210 );
	connect( m_pPitchCoarseRotary, &Rotary::valueChanged, [&]() {
		//round fVal, since Coarse is the integer number of half steps
		const float fNewPitch = round( m_pPitchCoarseRotary->getValue() ) +
			m_pPitchFineRotary->getValue();
		Hydrogen::get_instance()->getSelectedInstrument()->setPitchOffset( fNewPitch );
		updateEditor(); // LCD update
	});
	m_pPitchCoarseLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 48, 10 ), pCommonStrings->getPitchCoarseLabel() );
	m_pPitchCoarseLbl->move( 92, 235 );

	m_pPitchFineRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Center, tr( "Pitch offset (Fine)" ),
		false, -InstrumentEditor::nPitchFineControl,
		InstrumentEditor::nPitchFineControl );
	//it will have resolution of 100 steps between Min and Max => quantum delta = 0.01
	m_pPitchFineRotary->move( 151, 210 );
	connect( m_pPitchFineRotary, &Rotary::valueChanged, [&]() {
		//round fVal, since Coarse is the integer number of half steps
		const float fNewPitch = round( m_pPitchCoarseRotary->getValue() ) +
			m_pPitchFineRotary->getValue();
		Hydrogen::get_instance()->getSelectedInstrument()->setPitchOffset( fNewPitch );
		updateEditor(); // LCD update
	});
	m_pPitchFineLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 48, 10 ), pCommonStrings->getPitchFineLabel() );
	m_pPitchFineLbl->move( 149, 235 );

	m_pRandomPitchRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal, tr( "Random pitch factor" ),
		false );
	m_pRandomPitchRotary->move( 208, 210 );
	connect( m_pRandomPitchRotary, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setRandomPitchFactor(
			m_pRandomPitchRotary->getValue() );
	});
	m_pPitchRandomLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 48, 10 ), pCommonStrings->getPitchRandomLabel() );
	m_pPitchRandomLbl->move( 205, 235 );

	// Filter
	m_pFilterBypassBtn = new Button(
		m_pInstrumentProp, QSize( 36, 15 ), Button::Type::Toggle, "",
		pCommonStrings->getBypassButton(), QSize( 0, 0 ), "", true );
	connect( m_pFilterBypassBtn, &Button::clicked, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setFilterActive(
			! m_pFilterBypassBtn->isChecked() );
	});
	m_pFilterBypassBtn->move( 75, 169 );

	m_pCutoffRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal, tr( "Filter Cutoff" ), false );
	m_pCutoffRotary->setDefaultValue( m_pCutoffRotary->getMax() );
	m_pCutoffRotary->move( 124, 164 );
	connect( m_pCutoffRotary, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setFilterCutoff(
			m_pCutoffRotary->getValue() );
	});
	m_pCutoffLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 48, 10 ), pCommonStrings->getCutoffLabel() );
	m_pCutoffLbl->move( 122, 189 );

	m_pResonanceRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal, tr( "Filter resonance" ), false );
	connect( m_pResonanceRotary, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setFilterResonance(
			std::min( 0.95f, m_pResonanceRotary->getValue() ) );
	});
	m_pResonanceLbl = new ClickableLabel( m_pInstrumentProp, QSize( 56, 10 ),
										  pCommonStrings->getResonanceLabel() );
	m_pResonanceLbl->move( 175, 189 );

	m_pResonanceRotary->move( 181, 164 );
	// ~ Filter

	// ADSR
	m_pAttackRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal,
		tr( "Length of Attack phase" ), false );
	m_pAttackRotary->move( 45, 52 );
	connect( m_pAttackRotary, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->getAdsr()->setAttack(
			100000 * m_pAttackRotary->getValue() * m_pAttackRotary->getValue() );
	});
	m_pAttackLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									   pCommonStrings->getAttackLabel() );
	m_pAttackLbl->move( 43, 78 );

	m_pDecayRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal,
		tr( "Length of Decay phase" ), false );
	m_pDecayRotary->move( 101, 52 );
	connect( m_pDecayRotary, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->getAdsr()->setDecay(
			100000 * m_pDecayRotary->getValue() * m_pDecayRotary->getValue() );
	});
	m_pDecayLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									  pCommonStrings->getDecayLabel() );
	m_pDecayLbl->move( 99, 78 );

	m_pSustainRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal,
		tr( "Sample volume in Sustain phase" ), false );
	m_pSustainRotary->setDefaultValue( m_pSustainRotary->getMax() );
	m_pSustainRotary->move( 157, 52 );
	connect( m_pSustainRotary, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->getAdsr()->setSustain(
			m_pSustainRotary->getValue() );
	});
	m_pSustainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										pCommonStrings->getSustainLabel() );
	m_pSustainLbl->move( 155, 78 );

	m_pReleaseRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal,
		tr( "Length of Release phase" ), false );
	m_pReleaseRotary->setDefaultValue( 0.09 );
	m_pReleaseRotary->move( 213, 52 );
	connect( m_pReleaseRotary, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->getAdsr()->setRelease(
			256.0 +
			100000 * m_pReleaseRotary->getValue() * m_pReleaseRotary->getValue() );
	});
	m_pReleaseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										pCommonStrings->getReleaseLabel() );
	m_pReleaseLbl->move( 211, 78 );
	// ~ ADSR

	// instrument gain
	m_pInstrumentGainLCD = new LCDDisplay(
		m_pInstrumentProp, QSize( 43, 22 ), false, false );
	m_pInstrumentGainLCD->move( 73, 102 );

	m_pInstrumentGain = new Rotary(
		m_pInstrumentProp, Rotary::Type::Normal, tr( "Instrument gain" ), false,
		0.0, 5.0 );
	m_pInstrumentGain->setDefaultValue( 1.0 );
	m_pInstrumentGain->move( 122, 100 );
	connect( m_pInstrumentGain, &Rotary::valueChanged, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setGain(
			m_pInstrumentGain->getValue() );
		updateEditor(); // LCD update
	});
	m_pGainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									 pCommonStrings->getGainLabel() );
	m_pGainLbl->move( 120, 125 );


	m_pMuteGroupLCD = new LCDSpinBox(
		m_pInstrumentProp, QSize( 59, 24 ), LCDSpinBox::Type::Int, -1, 100,
		LCDSpinBox::Flag::ModifyOnChange | LCDSpinBox::Flag::MinusOneAsOff );
	m_pMuteGroupLCD->move( 210, 101 );
	connect( m_pMuteGroupLCD, &LCDSpinBox::valueAdjusted, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setMuteGroup(
			static_cast<int>(m_pMuteGroupLCD->value()) );
	});
	m_pMuteGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
										  pCommonStrings->getMuteGroupLabel() );
	m_pMuteGroupLbl->move( 209, 125 );

	m_pIsStopNoteCheckBox = new QCheckBox( "", m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 42, 139 );
	m_pIsStopNoteCheckBox->adjustSize();
	m_pIsStopNoteCheckBox->setFixedSize( 14, 14 );
	m_pIsStopNoteCheckBox->setToolTip( tr( "Stop the current playing instrument-note before trigger the next note sample" ) );
	m_pIsStopNoteCheckBox->setFocusPolicy ( Qt::NoFocus );
	connect( m_pIsStopNoteCheckBox, &QCheckBox::clicked, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setStopNotes(
			static_cast<int>(m_pIsStopNoteCheckBox->isChecked()) );
		Hydrogen::get_instance()->setIsModified( true );
	});
	m_pIsStopNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
										   pCommonStrings->getIsStopNoteLabel() );
	m_pIsStopNoteLbl->move( 59, 144 );

	m_pApplyVelocity = new QCheckBox( "", m_pInstrumentProp );
	m_pApplyVelocity->move( 166, 139 );
	m_pApplyVelocity->adjustSize();
	m_pApplyVelocity->setFixedSize( 14, 14 );
	m_pApplyVelocity->setToolTip( tr( "Don't change the layers' gain based on velocity" ) );
	m_pApplyVelocity->setFocusPolicy( Qt::NoFocus );
	connect( m_pApplyVelocity, &QCheckBox::clicked, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setApplyVelocity(
			static_cast<int>(m_pApplyVelocity->isChecked()) );
		Hydrogen::get_instance()->setIsModified( true );
	});
	m_pApplyVelocityLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
											  pCommonStrings->getApplyVelocityLabel() );
	m_pApplyVelocityLbl->move( 184, 144 );

	//////////////////////////
	// HiHat setup

	m_pHihatGroupLCD = new LCDSpinBox(
		m_pInstrumentProp, QSize( 59, 24 ), LCDSpinBox::Type::Int, -1, 32,
		LCDSpinBox::Flag::ModifyOnChange | LCDSpinBox::Flag::MinusOneAsOff );
	m_pHihatGroupLCD->move( 33, 303 );
	connect( m_pHihatGroupLCD, &LCDSpinBox::valueAdjusted, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setHihatGrp(
			static_cast<int>(m_pHihatGroupLCD->value()) );
	});
	m_pHihatGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 69, 10 ),
										   pCommonStrings->getHihatGroupLabel() );
	m_pHihatGroupLbl->move( 28, 327 );

	m_pHihatMinRangeLCD = new LCDSpinBox(
		m_pInstrumentProp, QSize( 59, 24 ), LCDSpinBox::Type::Int,
		static_cast<int>( Midi::ParameterMinimum ),
		static_cast<int>( Midi::ParameterMaximum ),
		LCDSpinBox::Flag::ModifyOnChange
	);
	m_pHihatMinRangeLCD->move( 146, 303 );
	connect( m_pHihatMinRangeLCD, &LCDSpinBox::valueAdjusted, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setLowerCc(
			Midi::parameterFromIntClamp(
				static_cast<int>( m_pHihatMinRangeLCD->value() )
			)
		);
		m_pHihatMaxRangeLCD->setMinimum(
			static_cast<int>( m_pHihatMinRangeLCD->value() )
		);
	} );
	m_pHihatMinRangeLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 61, 10 ),
		pCommonStrings->getHihatMinRangeLabel()
	);
	m_pHihatMinRangeLbl->move( 144, 327 );

	m_pHihatMaxRangeLCD = new LCDSpinBox(
		m_pInstrumentProp, QSize( 59, 24 ), LCDSpinBox::Type::Int,
		static_cast<int>( Midi::ParameterMinimum ),
		static_cast<int>( Midi::ParameterMaximum ),
		LCDSpinBox::Flag::ModifyOnChange
	);
	m_pHihatMaxRangeLCD->move( 210, 303 );
	connect( m_pHihatMaxRangeLCD, &LCDSpinBox::valueAdjusted, [&]() {
		Hydrogen::get_instance()->getSelectedInstrument()->setHigherCc(
			Midi::parameterFromIntClamp(
				static_cast<int>( m_pHihatMaxRangeLCD->value() )
			)
		);
		m_pHihatMinRangeLCD->setMaximum(
			static_cast<int>( m_pHihatMaxRangeLCD->value() )
		);
	} );
	m_pHihatMaxRangeLbl = new ClickableLabel(
		m_pInstrumentProp, QSize( 61, 10 ),
		pCommonStrings->getHihatMaxRangeLabel()
	);
	m_pHihatMaxRangeLbl->move( 208, 327 );

	updateColors();
	updateEditor();
	updateMidiNoteLabel();

    pHydrogenApp->addEventListener( this );
	connect( pHydrogenApp, &HydrogenApp::preferencesChanged,
			 this, &InstrumentEditor::onPreferencesChanged );
}

InstrumentEditor::~InstrumentEditor() {
}

void InstrumentEditor::drumkitLoadedEvent() {
	updateEditor();
}

void InstrumentEditor::instrumentParametersChangedEvent(
	int nInstrumentNumber
)
{
	auto pSong = H2Core::Hydrogen::get_instance()->getSong();
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();

	// Check if either this particular line or all lines should be updated.
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
		 pInstrument != nullptr && nInstrumentNumber != -1 &&
		 pInstrument !=
		 pSong->getDrumkit()->getInstruments()->get( nInstrumentNumber ) ) {
		// In case nInstrumentNumber does not belong to the currently
		// selected instrument we don't have to do anything.
	}
	else {
		updateEditor();
	}
}

void InstrumentEditor::selectedInstrumentChangedEvent() {
	updateEditor();
}

void InstrumentEditor::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		updateEditor();
	}
}

void InstrumentEditor::updateColors() {
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	m_pFilterBypassBtn->setCheckedBackgroundColor(
		pColorTheme->m_muteColor );
	m_pFilterBypassBtn->setCheckedBackgroundTextColor(
		pColorTheme->m_muteTextColor );
}

void InstrumentEditor::updateEditor() {
	auto pHydrogen = Hydrogen::get_instance();
	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	updateActivation();

	if ( pInstrument != nullptr ) {

		m_pNameLbl->setText( pInstrument->getName() );

		// ADSR
		m_pAttackRotary->setValue(
			sqrtf(pInstrument->getAdsr()->getAttack() / 100000.0), false,
			Event::Trigger::Suppress );
		m_pDecayRotary->setValue(
			sqrtf(pInstrument->getAdsr()->getDecay() / 100000.0), false,
			Event::Trigger::Suppress );
		m_pSustainRotary->setValue( pInstrument->getAdsr()->getSustain(),
									false, Event::Trigger::Suppress );
		const float fRelease =
			std::max( pInstrument->getAdsr()->getRelease() - 256.0, 0.0 );
		m_pReleaseRotary->setValue( sqrtf( fRelease / 100000.0 ), false,
									Event::Trigger::Suppress );
		// ~ ADSR

		// filter
		m_pFilterBypassBtn->setChecked( ! pInstrument->isFilterActive() );
		m_pCutoffRotary->setValue( pInstrument->getFilterCutoff(), false,
								   Event::Trigger::Suppress );
		m_pResonanceRotary->setValue( pInstrument->getFilterResonance(),
									  false, Event::Trigger::Suppress );
		// ~ filter

		// pitch
		const QString sNewPitch = QString( "%1" )
			.arg( pInstrument->getPitchOffset(), -2, 'f', 2, '0' );

		if ( m_pPitchLCD->text() != sNewPitch ) {
			m_pPitchLCD->setText( sNewPitch );
		}

		// pitch offset

		/* fCoarsePitch is the closest integer to pitch_offset (represents the
		   pitch shift interval in half steps) while it is an integer number,
		   it's defined float to be used in next lines */
		float fCoarsePitch;

		// Since the 'fine' rotary covers values from -0.5 to 0.5 e.g. 1.5 can
		// be represented by both coarse: 2, fine: -0.5 and coarse: 1, fine:
		// 0.5. We need some extra logic to avoid unexpected jumping of the
		// rotaries.
		if ( m_pPitchFineRotary->getValue() == 0.5 &&
			 pInstrument->getPitchOffset() -
			 trunc( pInstrument->getPitchOffset() ) == 0.5 ) {
			fCoarsePitch = trunc( pInstrument->getPitchOffset() );
		}
		else if ( m_pPitchFineRotary->getValue() == -0.5 &&
				  trunc( pInstrument->getPitchOffset() ) -
				  pInstrument->getPitchOffset() == 0.5 ) {
			fCoarsePitch = ceil( pInstrument->getPitchOffset() );
		}
		else {
			fCoarsePitch = round( pInstrument->getPitchOffset() );
		}

		// fFinePitch represents the fine adjustment (between -0.5 and +0.5) if
		// pitch_offset has decimal part
		const float fFinePitch = pInstrument->getPitchOffset() - fCoarsePitch;

		if ( m_pPitchCoarseRotary->getValue() != fCoarsePitch ) {
			m_pPitchCoarseRotary->setValue( fCoarsePitch, false,
											Event::Trigger::Suppress );
		}
		if ( m_pPitchFineRotary->getValue() != fFinePitch ) {
			m_pPitchFineRotary->setValue( fFinePitch, false,
										  Event::Trigger::Suppress );
		}

		// pitch random
		m_pRandomPitchRotary->setValue( pInstrument->getRandomPitchFactor(),
										false, Event::Trigger::Suppress );
		// ~ pitch

		//Stop Note
		m_pIsStopNoteCheckBox->setChecked( pInstrument->isStopNotes() );

		//Ignore Velocity
		m_pApplyVelocity->setChecked( pInstrument->getApplyVelocity() );

		// instr gain
		m_pInstrumentGainLCD->setText(
			QString( "%1" ).arg( pInstrument->getGain(), -2, 'f', 2, '0' ) );
		m_pInstrumentGain->setValue( pInstrument->getGain(), false,
									 Event::Trigger::Suppress );

		// instr mute group
		m_pMuteGroupLCD->setValue(
			pInstrument->getMuteGroup(), Event::Trigger::Suppress );

		// midi out channel
		if ( pInstrument->getMidiOutChannel() == Midi::ChannelOff ||
             pInstrument->getMidiOutChannel() == Midi::ChannelAll ||
             pInstrument->getMidiOutChannel() == Midi::ChannelInvalid ) {
			// turn off
			m_pMidiOutChannelLCD->setValue( 0, Event::Trigger::Suppress );
		}
		else {
			m_pMidiOutChannelLCD->setValue(
				static_cast<int>( pInstrument->getMidiOutChannel() ),
				Event::Trigger::Suppress
			);
		}

		//midi out note
		m_pMidiOutNoteLCD->setValue(
			static_cast<int>( pInstrument->getMidiOutNote() ),
			Event::Trigger::Suppress
		);

		// hihat
		m_pHihatGroupLCD->setValue(
			pInstrument->getHihatGrp(), Event::Trigger::Suppress
		);
		m_pHihatMinRangeLCD->setValue(
			static_cast<int>( pInstrument->getLowerCc() ),
			Event::Trigger::Suppress
		);
		m_pHihatMaxRangeLCD->setValue(
			static_cast<int>( pInstrument->getHigherCc() ),
			Event::Trigger::Suppress
		);
		m_pHihatMinRangeLCD->setMaximum(
			static_cast<int>( pInstrument->getHigherCc() )
		);
		m_pHihatMaxRangeLCD->setMinimum(
			static_cast<int>( pInstrument->getLowerCc() )
		);
	}
	else {
		m_pNameLbl->setText( "" );
	}
}

void InstrumentEditor::updateMidiNoteLabel() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	if ( Preferences::get_instance()->getMidiInstrumentMap()->getOutput() ==
		 MidiInstrumentMap::Output::Offset ) {
		m_pMidiOutNoteLbl->setText( pCommonStrings->getMidiOutNoteOffsetLabel() );
	}
	else {
		m_pMidiOutNoteLbl->setText( pCommonStrings->getMidiOutNoteLabel() );
	}
}

void InstrumentEditor::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes
)
{
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateColors();
		setStyleSheet( QString( "QLabel { background: %1 }" )
						   .arg( pPref->getColorTheme()->m_windowColor.name() )
		);
	}
	if ( changes & ( H2Core::Preferences::Changes::MidiTab ) ) {
		updateMidiNoteLabel();
	}
}

void InstrumentEditor::updateActivation()
{
	if ( Hydrogen::get_instance()->getSelectedInstrument() != nullptr ) {
		m_pAttackRotary->setIsActive( true );
		m_pDecayRotary->setIsActive( true );
		m_pSustainRotary->setIsActive( true );
		m_pReleaseRotary->setIsActive( true );

		m_pPitchCoarseRotary->setIsActive( true );
		m_pPitchFineRotary->setIsActive( true );
		m_pRandomPitchRotary->setIsActive( true );

		m_pFilterBypassBtn->setIsActive( true );
		m_pCutoffRotary->setIsActive( true );
		m_pResonanceRotary->setIsActive( true );

		m_pInstrumentGain->setIsActive( true );

		m_pApplyVelocity->setEnabled( true );
		m_pMuteGroupLCD->setIsActive( true );

		m_pMidiOutChannelLCD->setIsActive( true );
		m_pMidiOutChannelLCD->update();
		m_pMidiOutNoteLCD->setIsActive( true );

		m_pHihatGroupLCD->setIsActive( true );
		m_pHihatMinRangeLCD->setIsActive( true );
		m_pHihatMaxRangeLCD->setIsActive( true );
	}
	else {
		m_pNameLbl->setEnabled( false );

		m_pAttackRotary->setIsActive( false );
		m_pDecayRotary->setIsActive( false );
		m_pSustainRotary->setIsActive( false );
		m_pReleaseRotary->setIsActive( false );

		m_pPitchCoarseRotary->setIsActive( false );
		m_pPitchFineRotary->setIsActive( false );
		m_pPitchLCD->clear();
		m_pRandomPitchRotary->setIsActive( false );

		m_pFilterBypassBtn->setIsActive( false );
		m_pFilterBypassBtn->setChecked( false );
		m_pCutoffRotary->setIsActive( false );
		m_pResonanceRotary->setIsActive( false );

		m_pInstrumentGainLCD->clear();
		m_pInstrumentGain->setIsActive( false );

		m_pApplyVelocity->setEnabled( false );
		m_pApplyVelocity->setChecked( false );
		m_pMuteGroupLCD->setIsActive( false );
		m_pMuteGroupLCD->clear();

		m_pMidiOutChannelLCD->setIsActive( false );
		m_pMidiOutChannelLCD->clear();
		m_pMidiOutNoteLCD->setIsActive( false );
		m_pMidiOutNoteLCD->clear();

		m_pHihatGroupLCD->setIsActive( false );
		m_pHihatGroupLCD->clear();
		m_pHihatMinRangeLCD->setIsActive( false );
		m_pHihatMinRangeLCD->clear();
		m_pHihatMaxRangeLCD->setIsActive( false );
		m_pHihatMaxRangeLCD->clear();
	}
}
