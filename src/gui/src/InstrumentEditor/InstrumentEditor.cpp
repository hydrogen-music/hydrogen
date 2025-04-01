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

#include "InstrumentEditorPanel.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../Widgets/Button.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/Rotary.h"

using namespace H2Core;

InstrumentEditor::InstrumentEditor( InstrumentEditorPanel* pPanel )
	: QWidget( pPanel )
	, m_pInstrumentEditorPanel( pPanel )
	, m_fPreviousMidiOutChannel( -1.0 )
{
	setFixedWidth( InstrumentRack::nWidth );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	// Instrument properties top
	m_pInstrumentProp = new PixmapWidget( this );
	m_pInstrumentProp->move( 0, 0 );
	m_pInstrumentProp->setPixmap( "/instrumentEditor/instrumentTab.png" );

	m_pNameLbl = new ClickableLabel( m_pInstrumentProp, QSize( 279, 27 ), "",
									 ClickableLabel::Color::Bright, true );
	m_pNameLbl->move( 5, 4 );
	m_pNameLbl->setScaledContents( true );

	/////////////
	//Midi Out

	ClickableLabel* pMidiOutLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
													  pCommonStrings->getMidiOutLabel() );
	pMidiOutLbl->move( 22, 281 );

	m_pMidiOutChannelLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										   LCDSpinBox::Type::Int, -1, 16,
										   true, true );
	m_pMidiOutChannelLCD->move( 98, 257 );
	m_pMidiOutChannelLCD->setToolTip(QString(tr("Midi out channel")));
	connect( m_pMidiOutChannelLCD, SIGNAL( valueChanged( double ) ),
			 this, SLOT( midiOutChannelChanged( double ) ) );
	m_pMidiOutChannelLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											   pCommonStrings->getMidiOutChannelLabel() );
	m_pMidiOutChannelLbl->move( 96, 281 );

	///
	m_pMidiOutNoteLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										LCDSpinBox::Type::Int, 0, 127, true );
	m_pMidiOutNoteLCD->move( 161, 257 );
	m_pMidiOutNoteLCD->setToolTip(QString(tr("Midi out note")));
	connect( m_pMidiOutNoteLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setMidiOutNote(
			static_cast<int>(m_pMidiOutNoteLCD->value()) );
	});
	m_pMidiOutNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											pCommonStrings->getMidiOutNoteLabel() );
	m_pMidiOutNoteLbl->move( 159, 281 );

	/////////////

	connect( m_pNameLbl, &ClickableLabel::labelClicked, this, [=](){
		auto pSong = Hydrogen::get_instance()->getSong();
		auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
		if ( pInstrument != nullptr && pSong != nullptr &&
			 pSong->getDrumkit() != nullptr ) {
			MainForm::action_drumkit_renameInstrument(
				pSong->getDrumkit()->getInstruments()->index( pInstrument ) );}
	} );

	m_pPitchLCD = new LCDDisplay( m_pInstrumentProp, QSize( 56, 20 ), false, false );
	m_pPitchLCD->move( 24, 213 );
	m_pPitchLbl = new ClickableLabel( m_pInstrumentProp, QSize( 54, 10 ),
									  pCommonStrings->getPitchLabel() );
	m_pPitchLbl->move( 25, 235 );
	
	m_pPitchCoarseRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Center, tr( "Pitch offset (Coarse)" ),
		true, Instrument::fPitchMin + InstrumentEditorPanel::nPitchFineControl,
		Instrument::fPitchMax - InstrumentEditorPanel::nPitchFineControl );
	m_pPitchCoarseRotary->move( 84, 210 );
	connect( m_pPitchCoarseRotary, &Rotary::valueChanged, [&]() {
		//round fVal, since Coarse is the integer number of half steps
		const float fNewPitch = round( m_pPitchCoarseRotary->getValue() ) +
			m_pPitchFineRotary->getValue();
		m_pInstrumentEditorPanel->getInstrument()->setPitchOffset( fNewPitch );
		updateEditor(); // LCD update
	});
	m_pPitchCoarseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
											pCommonStrings->getPitchCoarseLabel() );
	m_pPitchCoarseLbl->move( 82, 235 );

	m_pPitchFineRotary = new Rotary(
		m_pInstrumentProp, Rotary::Type::Center, tr( "Pitch offset (Fine)" ),
		false, -InstrumentEditorPanel::nPitchFineControl,
		InstrumentEditorPanel::nPitchFineControl );
	//it will have resolution of 100 steps between Min and Max => quantum delta = 0.01
	m_pPitchFineRotary->move( 138, 210 );
	connect( m_pPitchFineRotary, &Rotary::valueChanged, [&]() {
		//round fVal, since Coarse is the integer number of half steps
		const float fNewPitch = round( m_pPitchCoarseRotary->getValue() ) +
			m_pPitchFineRotary->getValue();
		m_pInstrumentEditorPanel->getInstrument()->setPitchOffset( fNewPitch );
		updateEditor(); // LCD update
	});
	m_pPitchFineLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										  pCommonStrings->getPitchFineLabel() );
	m_pPitchFineLbl->move( 136, 235 );

	

	m_pRandomPitchRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									   tr( "Random pitch factor" ), false );
	m_pRandomPitchRotary->move( 194, 210 );
	connect( m_pRandomPitchRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setRandomPitchFactor(
			m_pRandomPitchRotary->getValue() );
	});
	m_pPitchRandomLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
											pCommonStrings->getPitchRandomLabel() );
	m_pPitchRandomLbl->move( 192, 235 );

	// Filter
	m_pFilterBypassBtn = new Button( m_pInstrumentProp, QSize( 36, 15 ), Button::Type::Toggle,
									 "", pCommonStrings->getBypassButton(), true,
									 QSize( 0, 0 ), "", false, true );
	connect( m_pFilterBypassBtn, &Button::clicked, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setFilterActive(
			! m_pFilterBypassBtn->isChecked() );
	});
	m_pFilterBypassBtn->move( 67, 169 );

	m_pCutoffRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								  tr( "Filter Cutoff" ), false );
	m_pCutoffRotary->setDefaultValue( m_pCutoffRotary->getMax() );
	connect( m_pCutoffRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setFilterCutoff(
			m_pCutoffRotary->getValue() );
	});
	m_pCutoffLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									   pCommonStrings->getCutoffLabel() );
	m_pCutoffLbl->move( 107, 189 );

	m_pResonanceRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									 tr( "Filter resonance" ), false );
	connect( m_pResonanceRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setFilterResonance(
			std::min( 0.95f, m_pResonanceRotary->getValue() ) );
	});
	m_pResonanceLbl = new ClickableLabel( m_pInstrumentProp, QSize( 56, 10 ),
										  pCommonStrings->getResonanceLabel() );
	m_pResonanceLbl->move( 157, 189 );

	m_pCutoffRotary->move( 109, 164 );
	m_pResonanceRotary->move( 163, 164 );
	// ~ Filter

	// ADSR
	m_pAttackRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								  tr( "Length of Attack phase.\n\nValue" ), false );
	connect( m_pAttackRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setAttack(
			100000 * m_pAttackRotary->getValue() * m_pAttackRotary->getValue() );
	});
	m_pDecayRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								 tr( "Length of Decay phase.\n\nValue" ), false );
	connect( m_pDecayRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setDecay(
			100000 * m_pDecayRotary->getValue() * m_pDecayRotary->getValue() );
	});
	m_pSustainRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								   tr( "Sample volume in Sustain phase.\n\nValue" ), false );
	m_pSustainRotary->setDefaultValue( m_pSustainRotary->getMax() );
	connect( m_pSustainRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setSustain(
			m_pSustainRotary->getValue() );
	});
	m_pReleaseRotary = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
								   tr( "Length of Release phase.\n\nValue" ), false );
	m_pReleaseRotary->setDefaultValue( 0.09 );
	connect( m_pReleaseRotary, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->getAdsr()->setRelease(
			256.0 +
			100000 * m_pReleaseRotary->getValue() * m_pReleaseRotary->getValue() );
	});
	m_pAttackRotary->move( 45, 52 );
	m_pDecayRotary->move( 97, 52 );
	m_pSustainRotary->move( 149, 52 );
	m_pReleaseRotary->move( 201, 52 );

	m_pAttackLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									   pCommonStrings->getAttackLabel() );
	m_pAttackLbl->move( 43, 78 );
	m_pDecayLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									  pCommonStrings->getDecayLabel() );
	m_pDecayLbl->move( 95, 78 );
	m_pSustainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										pCommonStrings->getSustainLabel() );
	m_pSustainLbl->move( 147, 78 );
	m_pReleaseLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
										pCommonStrings->getReleaseLabel() );
	m_pReleaseLbl->move( 199, 78 );
	// ~ ADSR

	// instrument gain
	m_pInstrumentGainLCD = new LCDDisplay( m_pInstrumentProp, QSize( 43, 20 ), false, false );
	m_pInstrumentGain = new Rotary( m_pInstrumentProp, Rotary::Type::Normal,
									tr( "Instrument gain" ), false, 0.0, 5.0 );
	m_pInstrumentGain->setDefaultValue( 1.0 );
	connect( m_pInstrumentGain, &Rotary::valueChanged, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setGain(
			m_pInstrumentGain->getValue() );
		updateEditor(); // LCD update
	});
	m_pInstrumentGainLCD->move( 62, 103 );
	m_pInstrumentGain->move( 109, 100 );
	m_pGainLbl = new ClickableLabel( m_pInstrumentProp, QSize( 48, 10 ),
									 pCommonStrings->getGainLabel() );
	m_pGainLbl->move( 107, 125 );


	m_pMuteGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
									  LCDSpinBox::Type::Int, -1, 100,
									  true, true );
	m_pMuteGroupLCD->move( 160, 101 );
	connect( m_pMuteGroupLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setMuteGroup(
			static_cast<int>(m_pMuteGroupLCD->value()) );
	});
	m_pMuteGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
										  pCommonStrings->getMuteGroupLabel() );
	m_pMuteGroupLbl->move( 159, 125 );

	m_pIsStopNoteCheckBox = new QCheckBox( "", m_pInstrumentProp );
	m_pIsStopNoteCheckBox->move( 42, 139 );
	m_pIsStopNoteCheckBox->adjustSize();
	m_pIsStopNoteCheckBox->setFixedSize( 14, 14 );
	m_pIsStopNoteCheckBox->setToolTip( tr( "Stop the current playing instrument-note before trigger the next note sample" ) );
	m_pIsStopNoteCheckBox->setFocusPolicy ( Qt::NoFocus );
	connect( m_pIsStopNoteCheckBox, &QCheckBox::clicked, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setStopNotes(
			static_cast<int>(m_pIsStopNoteCheckBox->isChecked()) );
		Hydrogen::get_instance()->setIsModified( true );
	});
	m_pIsStopNoteLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
										   pCommonStrings->getIsStopNoteLabel() );
	m_pIsStopNoteLbl->move( 59, 144 );

	m_pApplyVelocity = new QCheckBox( "", m_pInstrumentProp );
	m_pApplyVelocity->move( 153, 139 );
	m_pApplyVelocity->adjustSize();
	m_pApplyVelocity->setFixedSize( 14, 14 );
	m_pApplyVelocity->setToolTip( tr( "Don't change the layers' gain based on velocity" ) );
	m_pApplyVelocity->setFocusPolicy( Qt::NoFocus );
	connect( m_pApplyVelocity, &QCheckBox::clicked, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setApplyVelocity(
			static_cast<int>(m_pApplyVelocity->isChecked()) );
		Hydrogen::get_instance()->setIsModified( true );
	});
	m_pApplyVelocityLbl = new ClickableLabel( m_pInstrumentProp, QSize( 87, 10 ),
											  pCommonStrings->getApplyVelocityLabel() );
	m_pApplyVelocityLbl->move( 170, 144 );

	//////////////////////////
	// HiHat setup

	m_pHihatGroupLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
									   LCDSpinBox::Type::Int, -1, 32,
									   true, true );
	m_pHihatGroupLCD->move( 28, 303 );
	connect( m_pHihatGroupLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setHihatGrp(
			static_cast<int>(m_pHihatGroupLCD->value()) );
	});
	m_pHihatGroupLbl = new ClickableLabel( m_pInstrumentProp, QSize( 69, 10 ),
										   pCommonStrings->getHihatGroupLabel() );
	m_pHihatGroupLbl->move( 22, 327 );

	m_pHihatMinRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										  LCDSpinBox::Type::Int, 0, 127, true );
	m_pHihatMinRangeLCD->move( 138, 303 );
	connect( m_pHihatMinRangeLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setLowerCc(
			static_cast<int>(m_pHihatMinRangeLCD->value()) );
		m_pHihatMaxRangeLCD->setMinimum(
			static_cast<int>(m_pHihatMinRangeLCD->value()) );
	});
	m_pHihatMinRangeLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											  pCommonStrings->getHihatMinRangeLabel() );
	m_pHihatMinRangeLbl->move( 136, 327 );

	m_pHihatMaxRangeLCD = new LCDSpinBox( m_pInstrumentProp, QSize( 59, 24 ),
										  LCDSpinBox::Type::Int, 0, 127, true );
	m_pHihatMaxRangeLCD->move( 203, 303 );
	connect( m_pHihatMaxRangeLCD, &LCDSpinBox::valueAdjusted, [&]() {
		m_pInstrumentEditorPanel->getInstrument()->setHigherCc(
			static_cast<int>(m_pHihatMaxRangeLCD->value()) );
		m_pHihatMinRangeLCD->setMaximum(
			static_cast<int>(m_pHihatMaxRangeLCD->value()) );
	});
	m_pHihatMaxRangeLbl = new ClickableLabel( m_pInstrumentProp, QSize( 61, 10 ),
											  pCommonStrings->getHihatMaxRangeLabel() );
	m_pHihatMaxRangeLbl->move( 201, 327 );

	updateEditor();
}

InstrumentEditor::~InstrumentEditor() {
}

void InstrumentEditor::updateEditor() {
	auto pHydrogen = Hydrogen::get_instance();
	const auto pInstrument = m_pInstrumentEditorPanel->getInstrument();

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
		if ( pInstrument->getMidiOutChannel() == -1 ) {
			// turn off
			m_pMidiOutChannelLCD->setValue( -1, Event::Trigger::Suppress );
		}
		else {
			// The MIDI channels start at 1 instead of zero.
			m_pMidiOutChannelLCD->setValue(
				pInstrument->getMidiOutChannel() + 1,
				Event::Trigger::Suppress );
		}

		//midi out note
		m_pMidiOutNoteLCD->setValue( pInstrument->getMidiOutNote(),
									 Event::Trigger::Suppress );

		// hihat
		m_pHihatGroupLCD->setValue( pInstrument->getHihatGrp(),
									Event::Trigger::Suppress );
		m_pHihatMinRangeLCD->setValue( pInstrument->getLowerCc(),
									   Event::Trigger::Suppress );
		m_pHihatMaxRangeLCD->setValue( pInstrument->getHigherCc(),
									   Event::Trigger::Suppress );
		m_pHihatMinRangeLCD->setMaximum( pInstrument->getHigherCc() );
		m_pHihatMaxRangeLCD->setMinimum( pInstrument->getLowerCc() );
	}
	else {
		m_pNameLbl->setText( "" );
	}
}

void InstrumentEditor::updateActivation() {
	if ( m_pInstrumentEditorPanel->getInstrument() != nullptr ) {
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

void InstrumentEditor::midiOutChannelChanged( double fValue ) {
	auto pInstrument = m_pInstrumentEditorPanel->getInstrument();
	 if ( pInstrument == nullptr ) {
		 return;
	 }

	if ( fValue != 0.0 ) {
		pInstrument->setMidiOutChannel(
			std::max( static_cast<int>(fValue) - 1, -1 ) );
	} else {
		if ( m_fPreviousMidiOutChannel == -1.0 ) {
			m_pMidiOutChannelLCD->setValue( 1 );
			return;
		} else {
			m_pMidiOutChannelLCD->setValue( -1 );
			return;
		}
	}

	m_fPreviousMidiOutChannel = fValue;
}
