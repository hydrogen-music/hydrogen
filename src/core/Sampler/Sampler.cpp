﻿/*
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

#include <cassert>
#include <cmath>
#include <cstdlib>

#include <core/IO/AudioOutput.h>
#include <core/IO/JackAudioDriver.h>

#include <core/Basics/Adsr.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Globals.h>
#include <core/Hydrogen.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Note.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Helpers/Filesystem.h>
#include <core/EventQueue.h>

#include <core/FX/Effects.h>
#include <core/Sampler/Sampler.h>

#include <iostream>
#include <QDebug>

namespace H2Core
{

static std::shared_ptr<Instrument> createInstrument(int id, const QString& filepath, float volume )
{
	auto pInstrument = std::make_shared<Instrument>( id, filepath );
	pInstrument->set_volume( volume );
	auto pLayer = std::make_shared<InstrumentLayer>( Sample::load( filepath ) );
	auto pComponent = std::make_shared<InstrumentComponent>( 0 );
	
	pComponent->set_layer( pLayer, 0 );
	pInstrument->get_components()->push_back( pComponent );
	return pInstrument;
}

Sampler::Sampler()
		: m_pMainOut_L( nullptr )
		, m_pMainOut_R( nullptr )
		, m_pPreviewInstrument( nullptr )
		, m_interpolateMode( Interpolation::InterpolateMode::Linear )
{
	
	
	m_pMainOut_L = new float[ MAX_BUFFER_SIZE ];
	m_pMainOut_R = new float[ MAX_BUFFER_SIZE ];

	m_nMaxLayers = InstrumentComponent::getMaxLayers();

	QString sEmptySampleFilename = Filesystem::empty_sample_path();

	// instrument used in file preview
	m_pPreviewInstrument = createInstrument( EMPTY_INSTR_ID, sEmptySampleFilename, 0.8 );
	m_pPreviewInstrument->set_is_preview_instrument( true );

	// dummy instrument used for playback track
	m_pPlaybackTrackInstrument = createInstrument( PLAYBACK_INSTR_ID, sEmptySampleFilename, 0.8 );
	m_nPlayBackSamplePosition = 0;
}


Sampler::~Sampler()
{
	INFOLOG( "DESTROY" );

	delete[] m_pMainOut_L;
	delete[] m_pMainOut_R;

	m_pPreviewInstrument = nullptr;
	m_pPlaybackTrackInstrument = nullptr;
}

void Sampler::process( uint32_t nFrames )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song" );
		return;
	}
	
	memset( m_pMainOut_L, 0, nFrames * sizeof( float ) );
	memset( m_pMainOut_R, 0, nFrames * sizeof( float ) );

	// Track output queues are zeroed by
	// audioEngine_process_clearAudioBuffers()

	for ( auto& pComponent : *pSong->getComponents() ) {
		pComponent->reset_outs(nFrames);
	}

	// Max notes limit
	int nMaxNotes = Preferences::get_instance()->m_nMaxNotes;
	while ( ( int )m_playingNotesQueue.size() > nMaxNotes ) {
		Note * pOldNote = m_playingNotesQueue[ 0 ];
		m_playingNotesQueue.erase( m_playingNotesQueue.begin() );
		pOldNote->get_instrument()->dequeue();
		WARNINGLOG( QString( "Number of playing notes [%1] exceeds maximum [%2]. Dropping note [%3]" )
					.arg( m_playingNotesQueue.size() ).arg( nMaxNotes )
					.arg( pOldNote->toQString() ) );
		delete  pOldNote;	// FIXME: send note-off instead of removing the note from the list?
	}

	// Render next `nFrames` audio frames of all playing notes.
	unsigned i = 0;
	Note* pNote;
	while ( i < m_playingNotesQueue.size() ) {
		pNote = m_playingNotesQueue[ i ];
		if ( renderNote( pNote, nFrames ) ) {
			// End of note was reached during rendering.
			m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
			pNote->get_instrument()->dequeue();
			m_queuedNoteOffs.push_back( pNote );
		} else {
			// As finished notes are poped above 
			++i;
		}
	}

	if ( m_queuedNoteOffs.size() > 0 ) {
		MidiOutput* pMidiOut = pHydrogen->getMidiOutput();
		if ( pMidiOut != nullptr ) {
			//Queue midi note off messages for notes that have a length specified for them
			while ( ! m_queuedNoteOffs.empty() ) {
				pNote =  m_queuedNoteOffs[0];
		
				if ( ! pNote->get_instrument()->is_muted() ){
					pMidiOut->handleQueueNoteOff(
						pNote->get_instrument()->get_midi_out_channel(), 
						pNote->get_midi_key(),
						pNote->get_midi_velocity() );
				}
		
				m_queuedNoteOffs.erase( m_queuedNoteOffs.begin() );
		
				if ( pNote != nullptr ){
					delete pNote;
				}
		
				pNote = nullptr;
			}
		}
	}

	processPlaybackTrack(nFrames);
}

bool Sampler::isRenderingNotes() const {
	return m_playingNotesQueue.size() > 0;
}

void Sampler::noteOn(Note *pNote )
{
	assert( pNote );
	if ( pNote == nullptr ) {
		ERRORLOG( "Invalid note" );
		return;
	}

	pNote->get_adsr()->attack();
	auto pInstr = pNote->get_instrument();

	// mute group
	int nMuteGrp = pInstr->get_mute_group();
	if ( nMuteGrp != -1 ) {
		// remove all notes using the same mute group
		for ( const auto& pOtherNote: m_playingNotesQueue ) {	// delete older note
			if ( ( pOtherNote->get_instrument() != pInstr )  &&
				 ( pOtherNote->get_instrument()->get_mute_group() == nMuteGrp ) ) {
				pOtherNote->get_adsr()->release();
			}
		}
	}

	//note off notes
	if ( pNote->get_note_off() ){
		for ( const auto& pOtherNote: m_playingNotesQueue ) {
			if ( ( pOtherNote->get_instrument() == pInstr ) ) {
				//ERRORLOG("note_off");
				pOtherNote->get_adsr()->release();
			}
		}
	}

	pInstr->enqueue();
	if ( ! pNote->get_note_off() ){
		m_playingNotesQueue.push_back( pNote );
	}
}

void Sampler::midiKeyboardNoteOff( int key )
{
	for ( const auto& pNote: m_playingNotesQueue ) {
		if ( ( pNote->get_midi_msg() == key) ) {
			pNote->get_adsr()->release();
		}
	}
}


/// This old note_off function is only used by right click on mixer channel strip play button
/// all other note_off stuff will handle in midi_keyboard_note_off() and note_on()
void Sampler::noteOff(Note* pNote )
{
	auto pInstr = pNote->get_instrument();
	// find the notes using the same instrument, and release them
	for ( const auto& pNote: m_playingNotesQueue ) {
		if ( pNote->get_instrument() == pInstr ) {
			pNote->get_adsr()->release();
		}
	}
	
	delete pNote;
}


// functions for pan parameters and laws-----------------

float Sampler::getRatioPan( float fPan_L, float fPan_R ) {
	if ( fPan_L < 0. || fPan_R < 0. || ( fPan_L == 0. && fPan_R == 0.) ) { // invalid input
		WARNINGLOG( "Invalid (panL, panR): both zero or some is negative. Pan set to center." );
		return 0.; // default central value
	} else {
		if ( fPan_L >= fPan_R ) {
			return fPan_R / fPan_L - 1.;
		} else {
			return 1. - fPan_L / fPan_R;
		}
	}
}

	
float Sampler::ratioStraightPolygonalPanLaw( float fPan ) {
	// the straight polygonal pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1.;
	} else {
		return ( 1. - fPan );
	}
}

float Sampler::ratioConstPowerPanLaw( float fPan ) {
	// the constant power pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1. / sqrt( 1 + ( 1. + fPan ) * ( 1. + fPan ) );
	} else {
		return ( 1. - fPan ) / sqrt( 1 + ( 1. - fPan ) * ( 1. - fPan ) );
	}
}

float Sampler::ratioConstSumPanLaw( float fPan ) {
	// the constant Sum pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1. / ( 2. + fPan );
	} else {
		return ( 1. - fPan ) / ( 2. - fPan );
	}
}

float Sampler::linearStraightPolygonalPanLaw( float fPan ) {
	// the constant power pan law interpreting fPan as the "linear" parameter
	if ( fPan <= 0 ) {
		return 1.;
	} else {
		return ( 1. - fPan ) / ( 1. + fPan );
	}
}

float Sampler::linearConstPowerPanLaw( float fPan ) {
	// the constant power pan law interpreting fPan as the "linear" parameter
	return ( 1. - fPan ) / sqrt( 2. * ( 1 + fPan * fPan ) );
}

float Sampler::linearConstSumPanLaw( float fPan ) {
	// the constant Sum pan law interpreting fPan as the "linear" parameter
	return ( 1. - fPan ) * 0.5;
}

float Sampler::polarStraightPolygonalPanLaw( float fPan ) {
	// the constant power pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	if ( fPan <= 0 ) {
		return 1.;
	} else {
		return cos( fTheta ) / sin( fTheta );
	}
}

float Sampler::polarConstPowerPanLaw( float fPan ) {
	// the constant power pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	return cos( fTheta );
}

float Sampler::polarConstSumPanLaw( float fPan ) {
	// the constant Sum pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	return cos( fTheta ) / ( cos( fTheta ) + sin( fTheta ) );
}

float Sampler::quadraticStraightPolygonalPanLaw( float fPan ) {
	// the straight polygonal pan law interpreting fPan as the "quadratic" parameter
	if ( fPan <= 0 ) {
		return 1.;
	} else {
		return sqrt( ( 1. - fPan ) / ( 1. + fPan ) );
	}
}

float Sampler::quadraticConstPowerPanLaw( float fPan ) {
	// the constant power pan law interpreting fPan as the "quadratic" parameter
	return sqrt( ( 1. - fPan ) * 0.5 );
}

float Sampler::quadraticConstSumPanLaw( float fPan ) {
	// the constant Sum pan law interpreting fPan as the "quadratic" parameter
	return sqrt( 1. - fPan ) / ( sqrt( 1. - fPan ) +  sqrt( 1. + fPan ) );
}

float Sampler::linearConstKNormPanLaw( float fPan, float k ) {
	// the constant k norm pan law interpreting fPan as the "linear" parameter
	return ( 1. - fPan ) / pow( ( pow( (1. - fPan), k ) + pow( (1. + fPan), k ) ), 1./k );
}

float Sampler::quadraticConstKNormPanLaw( float fPan, float k ) {
	// the constant k norm pan law interpreting fPan as the "quadratic" parameter
	return sqrt( 1. - fPan ) / pow( ( pow( (1. - fPan), 0.5 * k ) + pow( (1. + fPan), 0.5 * k ) ), 1./k );
}

float Sampler::polarConstKNormPanLaw( float fPan, float k ) {
	// the constant k norm pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	float cosTheta = cos( fTheta );
	return cosTheta / pow( ( pow( cosTheta, k ) + pow( sin( fTheta ), k ) ), 1./k );
}

float Sampler::ratioConstKNormPanLaw( float fPan, float k) {
	// the constant k norm pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1. / pow( ( 1. + pow( (1. + fPan), k ) ), 1./k );
	} else {
		return ( 1. - fPan ) / pow( ( 1. + pow( (1. - fPan), k ) ), 1./k );
	}
}

// function to direct the computation to the selected pan law.
inline float Sampler::panLaw( float fPan, std::shared_ptr<Song> pSong ) {
	int nPanLawType = pSong->getPanLawType();
	if ( nPanLawType == RATIO_STRAIGHT_POLYGONAL ) {
		return ratioStraightPolygonalPanLaw( fPan );
	} else if ( nPanLawType == RATIO_CONST_POWER ) {
		return ratioConstPowerPanLaw( fPan );
	} else if ( nPanLawType == RATIO_CONST_SUM ) {
		return ratioConstSumPanLaw( fPan );
	} else if ( nPanLawType == LINEAR_STRAIGHT_POLYGONAL ) {
		return linearStraightPolygonalPanLaw( fPan );
	} else if ( nPanLawType == LINEAR_CONST_POWER ) {
		return linearConstPowerPanLaw( fPan );
	} else if ( nPanLawType == LINEAR_CONST_SUM ) {
		return linearConstSumPanLaw( fPan );
	} else if ( nPanLawType == POLAR_STRAIGHT_POLYGONAL ) {
		return polarStraightPolygonalPanLaw( fPan );
	} else if ( nPanLawType == POLAR_CONST_POWER ) {
		return polarConstPowerPanLaw( fPan );
	} else if ( nPanLawType == POLAR_CONST_SUM ) {
		return polarConstSumPanLaw( fPan );
	} else if ( nPanLawType == QUADRATIC_STRAIGHT_POLYGONAL ) {
		return quadraticStraightPolygonalPanLaw( fPan );
	} else if ( nPanLawType == QUADRATIC_CONST_POWER ) {
		return quadraticConstPowerPanLaw( fPan );
	} else if ( nPanLawType == QUADRATIC_CONST_SUM ) {
		return quadraticConstSumPanLaw( fPan );
	} else if ( nPanLawType == LINEAR_CONST_K_NORM ) {
		return linearConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	} else if ( nPanLawType == POLAR_CONST_K_NORM ) {
		return polarConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	} else if ( nPanLawType == RATIO_CONST_K_NORM ) {
		return ratioConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	} else if ( nPanLawType == QUADRATIC_CONST_K_NORM ) {
		return quadraticConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	} else {
		WARNINGLOG( "Unknown pan law type. Set default." );
		pSong->setPanLawType( RATIO_STRAIGHT_POLYGONAL );
		return ratioStraightPolygonalPanLaw( fPan );
	}
}

void Sampler::handleTimelineOrTempoChange() {
	if ( m_playingNotesQueue.size() == 0 ) {
		return;
	}

	for ( auto ppNote : m_playingNotesQueue ) {
		ppNote->computeNoteStart();

		// For notes of custom length we have to rescale the amount of the
		// sample still left for rendering to properly adopt the tempo change.
		//
		// This is only done on manual tempo changes, like changes through the
		// BPM widget, but not when passing a tempo marker. In case
		// UsedTickSize() of a note is -1, the timeline was activated when
		// picked up by the audio engine.
		//
		// BUG adding/deleting a tempo marker or toggling the timeline is not
		// properly handled in here. But since this only occurs seldomly and
		// this code only takes effect if a note with custom length is currently
		// rendered, we skip this edge case.
		if ( ppNote->isPartiallyRendered() && ppNote->get_length() != -1 &&
			 ppNote->getUsedTickSize() != -1 ) {

			double fTickMismatch;

			// Do so for all layers of all components current processed.
			for ( auto& [ nnCompo, ppLayer ] : ppNote->get_layers_selected() ) {
				const auto pSample = ppNote->getSample( nnCompo,
														ppLayer->nSelectedLayer );
				const int nNewNoteLength =
					TransportPosition::computeFrameFromTick(
						ppNote->get_position() + ppNote->get_length(),
						&fTickMismatch, pSample->get_sample_rate() ) -
					TransportPosition::computeFrameFromTick(
						ppNote->get_position(), &fTickMismatch,
						pSample->get_sample_rate() );

				// The ratio between the old and new note length determines the
				// scaling of the length. This is only applied to the part of
				// the sample _not_ rendered yet to ensure consistency.
				//
				// BUG When handling several tempo changes while rendering a
				// single note, calculating the ratio between note lengths is
				// not a proper drop in for the tempo change anymore as the
				// original note length and not the patched one from the last
				// change is required. But this is too much of an edge-case and
				// won't be covered here.
				const int nSamplePosition =
					static_cast<int>(std::floor(ppLayer->fSamplePosition));

				ppLayer->nNoteLength = nSamplePosition +
					static_cast<int>(std::round(
						static_cast<float>(ppLayer->nNoteLength - nSamplePosition) *
						nNewNoteLength /
						static_cast<float>(ppLayer->nNoteLength)));
			}
		}
	}
}

void Sampler::handleSongSizeChange() {
	if ( m_playingNotesQueue.size() == 0 ) {
		return;
	}

	const long nTickOffset =
		static_cast<long>(std::floor(Hydrogen::get_instance()->getAudioEngine()->
									 getTransportPosition()->getTickOffsetSongSize()));
	
	for ( auto nnote : m_playingNotesQueue ) {
		
		// DEBUGLOG( QString( "pos: %1 -> %2, nTickOffset: %3, note: %4" )
		// 		  .arg( nnote->get_position() )
		// 		  .arg( std::max( nnote->get_position() + nTickOffset,
		// 						  static_cast<long>(0) ) )
		// 		  .arg( nTickOffset )
		// 		  .arg( nnote->toQString( "", true ) ) );
		
		nnote->set_position( std::max( nnote->get_position() + nTickOffset,
									   static_cast<long>(0) ) );
		nnote->computeNoteStart();
		
		// DEBUGLOG( QString( "new note: %1" )
		// 		  .arg( nnote->toQString( "", true ) ) );
		
	}
}

//------------------------------------------------------------------

bool Sampler::renderNote( Note* pNote, unsigned nBufferSize )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song" );
		return true;
	}

	auto pInstr = pNote->get_instrument();
	if ( pInstr == nullptr ) {
		ERRORLOG( "NULL instrument" );
		return true;
	}

	long long nFrame;
	auto pAudioDriver = pHydrogen->getAudioOutput();
	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return true;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	if ( pAudioEngine->getState() == AudioEngine::State::Playing ||
		 pAudioEngine->getState() == AudioEngine::State::Testing ) {
		nFrame = pAudioEngine->getTransportPosition()->getFrame();
	} else {
		// use this to support realtime events when transport is not
		// rolling.
		nFrame = pAudioEngine->getRealtimeFrame();
	}

	// Only if the Sampler has not started rendering the note yet we
	// care about its starting position. Else we would encounter
	// glitches when relocating transport during playback or starting
	// transport while using realtime playback.
	long long nInitialBufferPos = 0;
	if ( ! pNote->isPartiallyRendered() ) {
		long long nNoteStartInFrames = pNote->getNoteStart();

		// DEBUGLOG(QString( "nFrame: %1, note pos: %2, pAudioEngine->getTransportPosition()->getTickSize(): %3, pAudioEngine->getTransportPosition()->getTick(): %4, pAudioEngine->getTransportPosition()->getFrame(): %5, nNoteStartInFrames: %6 ")
		// 		 .arg( nFrame ).arg( pNote->get_position() )
		//       .arg( pAudioEngine->getTransportPosition()->getTickSize() )
		//       .arg( pAudioEngine->getTransportPosition()->getTick() )
		//       .arg( pAudioEngine->getTransportPosition()->getFrame() )
		// 		 .arg( nNoteStartInFrames )
		// 		 .append( pNote->toQString( "", true ) ) );

		if ( nNoteStartInFrames > nFrame ) {
			// The note doesn't start right at the beginning of the
			// buffer rendered in this cycle.
			nInitialBufferPos = nNoteStartInFrames - nFrame;
			
			if ( nBufferSize < nInitialBufferPos ) {
				// this note is not valid. it's in the future...let's skip it....
				ERRORLOG( QString( "Note pos in the future?? nFrame: %1, note start: %2, nInitialBufferPos: %3, nBufferSize: %4" )
						  .arg( nFrame ).arg( pNote->getNoteStart() )
						  .arg( nInitialBufferPos ).arg( nBufferSize ) );

				return true;
			}
		}
	}

	// new instrument and note pan interaction--------------------------
	// notePan moves the RESULTANT pan in a smaller pan range centered at instrumentPan

   /** Get the RESULTANT pan, following a "matryoshka" multi panning, like in this graphic:
    *
    *   L--------------instrPan---------C------------------------------>R			(instrumentPan = -0.4)
    *                     |
    *                     V
    *   L-----------------C---notePan-------->R									    (notePan = +0.3)
    *                            |
    *                            V
    *   L----------------------resPan---C------------------------------>R		    (resultantPan = -0.22)
    *
    * Explanation:
	* notePan moves the RESULTANT pan in a smaller pan range centered at instrumentPan value,
	* whose extension depends on instrPan value:
	*	if instrPan is central, notePan moves the signal in the whole pan range (really from left to right);
	*	if instrPan is sided, notePan moves the signal in a progressively smaller pan range centered at instrPan;
	*	if instrPan is HARD-sided, notePan doesn't have any effect.
	*/
	float fPan = pInstr->getPan() + pNote->getPan() * ( 1 - fabs( pInstr->getPan() ) );
	
	// Pass fPan to the Pan Law
	float fPan_L = panLaw( fPan, pSong );
	float fPan_R = panLaw( -fPan, pSong );

	// In PreFader mode of the per track output of the JACK driver we
	// disregard the instrument pan along with all other settings
	// available in the Mixer. The Note pan, however, will be used.
	float fNotePan_L = 0;
	float fNotePan_R = 0;
	if ( pHydrogen->hasJackAudioDriver() &&
		 Preferences::get_instance()->m_JackTrackOutputMode ==
		 Preferences::JackTrackOutputMode::preFader ) {
		fNotePan_L = panLaw( pNote->getPan(), pSong );
		fNotePan_R = panLaw( -1 * pNote->getPan(), pSong );
	}
	//---------------------------------------------------------

	auto pComponents = pInstr->get_components();
	bool nReturnValues[ pComponents->size() ];

	for( int i = 0; i < pComponents->size(); i++ ){
		nReturnValues[i] = false;
	}

	int nReturnValueIndex = 0;
	int nAlreadySelectedLayer = -1;
	bool bComponentFound = false;

	for ( const auto& pCompo : *pComponents ) {
		std::shared_ptr<DrumkitComponent> pMainCompo = nullptr;

		if ( pNote->get_specific_compo_id() != -1 &&
			 pNote->get_specific_compo_id() != pCompo->get_drumkit_componentID() ) {
			nReturnValueIndex++;
			continue;
		}
		bComponentFound = true;

		if ( pInstr->is_preview_instrument() ||
			 pInstr->is_metronome_instrument() ){
			pMainCompo = pSong->getComponents()->front();
		} else {
			int nComponentID = pCompo->get_drumkit_componentID();
			if ( nComponentID >= 0 ) {
				pMainCompo = pSong->getComponent( nComponentID );
			} else {
				/* Invalid component found. This is possible on loading older or broken song files. */
				pMainCompo = pSong->getComponents()->front();
			}
		}

		assert(pMainCompo);

		auto pSample = pNote->getSample( pCompo->get_drumkit_componentID(),
										 nAlreadySelectedLayer );
		if ( pSample == nullptr ) {
			nReturnValues[nReturnValueIndex] = true;
			nReturnValueIndex++;
			continue;
		}

		auto pSelectedLayer =
			pNote->get_layer_selected( pCompo->get_drumkit_componentID() );

		// For round robin and random selection we will use the same
		// layer again for all other samples.
		if ( nAlreadySelectedLayer != -1 &&
			 pInstr->sample_selection_alg() != Instrument::VELOCITY ) {
			nAlreadySelectedLayer = pSelectedLayer->nSelectedLayer;
		}

		if( pSelectedLayer->nSelectedLayer == -1 ) {
			ERRORLOG( "Sample selection did not work." );
			nReturnValues[nReturnValueIndex] = true;
			nReturnValueIndex++;
			continue;
		}
		auto pLayer = pCompo->get_layer( pSelectedLayer->nSelectedLayer );
		float fLayerGain = pLayer->get_gain();
		float fLayerPitch = pLayer->get_pitch();

		if ( pSelectedLayer->fSamplePosition >= pSample->get_frames() ) {
			// Due to rounding errors in renderNoteResample() the
			// sample position can occassionaly exceed the maximum
			// frames of a sample. AFAICS this is not itself
			// harmful. So, we just log a warning if the difference is
			// larger, which might be caused by a different problem.
			if ( pSelectedLayer->fSamplePosition >= pSample->get_frames() + 3 ) {
				WARNINGLOG( QString( "sample position [%1] out of bounds [0,%2]. The layer has been resized during note play?" )
							.arg( pSelectedLayer->fSamplePosition )
							.arg( pSample->get_frames() ) );
			}
			nReturnValues[nReturnValueIndex] = true;
			nReturnValueIndex++;
			continue;
		}

		float fCost_L = 1.0f;
		float fCost_R = 1.0f;
		float fCostTrack_L = 1.0f;
		float fCostTrack_R = 1.0f;
		
		bool bIsMutedForExport = ( pHydrogen->getIsExportSessionActive() &&
								 ! pInstr->is_currently_exported() );
		bool bAnyInstrumentIsSoloed = pSong->getInstrumentList()->isAnyInstrumentSoloed();
		bool bIsMutedBecauseOfSolo = ( bAnyInstrumentIsSoloed &&
									   ! pInstr->is_soloed() );
		
		/*
		 *  Is instrument muted?
		 *
		 *  This can be the case either if: 
		 *   - the song, instrument or component is muted 
		 *   - if we're in an export session and we're doing per-instruments exports, 
		 *       but this instrument is not currently being exported.
		 *   - if at least one instrument is soloed (but not this instrument)
		 */
		if ( bIsMutedForExport || pInstr->is_muted() || pSong->getIsMuted() ||
			 pMainCompo->is_muted() || bIsMutedBecauseOfSolo) {	
			fCost_L = 0.0;
			fCost_R = 0.0;
			if ( Preferences::get_instance()->m_JackTrackOutputMode ==
				 Preferences::JackTrackOutputMode::postFader ) {
				fCostTrack_L = 0.0;
				fCostTrack_R = 0.0;
			}

		} else {
			float fMonoGain = 1.0;
			if ( pInstr->get_apply_velocity() ) {
				fMonoGain *= pNote->get_velocity();	// note velocity
			}

			fMonoGain *= fLayerGain;				// layer gain
			fMonoGain *= pInstr->get_gain();		// instrument gain
			fMonoGain *= pCompo->get_gain();		// Component gain
			fMonoGain *= pMainCompo->get_volume();	// Component volument
			fMonoGain *= pInstr->get_volume();		// instrument volume
			fMonoGain *= pSong->getVolume();		// song volume

			fCost_L = fMonoGain * fPan_L;			// pan
			fCost_R = fMonoGain * fPan_R;			// pan
			if ( Preferences::get_instance()->m_JackTrackOutputMode ==
				 Preferences::JackTrackOutputMode::postFader ) {
				fCostTrack_R = fCost_R * 2;
				fCostTrack_L = fCost_L * 2;
			}
		}

		// direct track outputs only use velocity
		if ( Preferences::get_instance()->m_JackTrackOutputMode ==
			 Preferences::JackTrackOutputMode::preFader ) {
			if ( pInstr->get_apply_velocity() ) {
				fCostTrack_L *= pNote->get_velocity();
			}
			fCostTrack_L *= fLayerGain;
			
			fCostTrack_R = fCostTrack_L;

			fCostTrack_L *= fNotePan_L;
			fCostTrack_R *= fNotePan_R;
		}

		// Se non devo fare resample (drumkit) posso evitare di utilizzare i float e gestire il tutto in
		// maniera ottimizzata
		//	constant^12 = 2, so constant = 2^(1/12) = 1.059463.
		//	float nStep = 1.0;1.0594630943593

		float fTotalPitch = pNote->get_total_pitch() + fLayerPitch;

		// Once the Sampler does start rendering a note we also push
		// it to all connected MIDI devices.
		if ( (int) pSelectedLayer->fSamplePosition == 0  && ! pInstr->is_muted() ) {
			if ( pHydrogen->getMidiOutput() != nullptr ){
				pHydrogen->getMidiOutput()->handleQueueNote( pNote );
			}
		}

		// Actual rendering.
		nReturnValues[nReturnValueIndex] = renderNoteResample( pSample, pNote, pSelectedLayer, pCompo, pMainCompo, nBufferSize, nInitialBufferPos, fCost_L, fCost_R, fCostTrack_L, fCostTrack_R, fLayerPitch );

		nReturnValueIndex++;
	}

	// Sanity check whether the note could be rendered.
	if ( ! bComponentFound ) {
		ERRORLOG( QString( "Specific note component [%1] not found in instrument associated with note: [%2]" )
				  .arg( pNote->get_specific_compo_id() )
				  .arg( pNote->toQString() ) );
		return true;
	}

	for ( const auto& bReturnValue : nReturnValues ) {
		if ( ! bReturnValue ) {
			return false;
		}
	}
	return true;
}

bool Sampler::processPlaybackTrack(int nBufferSize)
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pAudioDriver = pHydrogen->getAudioOutput();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return true;
	}

	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return true;
	}

	if ( pHydrogen->getPlaybackTrackState() != Song::PlaybackTrack::Enabled ||
		 ( pAudioEngine->getState() != AudioEngine::State::Playing ||
		   pAudioEngine->getState() == AudioEngine::State::Testing ) ||
		 pHydrogen->getMode() != Song::Mode::Song ) {
		return true;
	}

	const auto pCompo = m_pPlaybackTrackInstrument->get_components()->front();
	if ( pCompo == nullptr ) {
		ERRORLOG( "Invalid component of playback instrument" );
		return true;
	}

	auto pSample = pCompo->get_layer(0)->get_sample();
	if ( pSample == nullptr ) {
		ERRORLOG( "Unable to process playback track" );
		EventQueue::get_instance()->push_event( EVENT_ERROR,
												Hydrogen::ErrorMessages::PLAYBACK_TRACK_INVALID );
		// Disable the playback track
		pHydrogen->loadPlaybackTrack( "" );
		reinitializePlaybackTrack();
		return true;
	}

	float fVal_L;
	float fVal_R;

	auto pSample_data_L = pSample->get_data_l();
	auto pSample_data_R = pSample->get_data_r();
	
	float fInstrPeak_L = m_pPlaybackTrackInstrument->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = m_pPlaybackTrackInstrument->get_peak_r(); // this value will be reset to 0 by the mixer..

	int nAvail_bytes = 0;
	int	nInitialBufferPos = 0;

	const long long nFrame = pAudioEngine->getTransportPosition()->getFrame();
	const long long nFrameOffset =
		pAudioEngine->getTransportPosition()->getFrameOffsetTempo();

	if ( pSample->get_sample_rate() == pAudioDriver->getSampleRate() ) {
		// No resampling
		m_nPlayBackSamplePosition = nFrame - nFrameOffset;
	
		nAvail_bytes = pSample->get_frames() - m_nPlayBackSamplePosition;
		
		if ( nAvail_bytes > nBufferSize ) {
			nAvail_bytes = nBufferSize;
		}

		int nInitialSamplePos = ( int ) m_nPlayBackSamplePosition;
		int nSamplePos = nInitialSamplePos;
	
		int nFinalBufferPos = nInitialBufferPos + nAvail_bytes;
	
		if ( m_nPlayBackSamplePosition > pSample->get_frames() ) {
			//playback track has ended..
			return true;
		}
	
		for ( int nBufferPos = nInitialBufferPos; nBufferPos < nFinalBufferPos; ++nBufferPos ) {
			fVal_L = pSample_data_L[ nSamplePos ];
			fVal_R = pSample_data_R[ nSamplePos ];
	
			fVal_L = fVal_L * 1.0f * pSong->getPlaybackTrackVolume(); //costr
			fVal_R = fVal_R * 1.0f * pSong->getPlaybackTrackVolume(); //cost l
	
			//pDrumCompo->set_outs( nBufferPos, fVal_L, fVal_R );
	
			// to main mix
			if ( fVal_L > fInstrPeak_L ) {
				fInstrPeak_L = fVal_L;
			}
			if ( fVal_R > fInstrPeak_R ) {
				fInstrPeak_R = fVal_R;
			}
			
			m_pMainOut_L[nBufferPos] += fVal_L;
			m_pMainOut_R[nBufferPos] += fVal_R;
			
			++nSamplePos;
		}
	} else {
		//Perform resampling
		double	fSamplePos = 0;
		int		nSampleFrames = pSample->get_frames();
		float	fStep = 1;
		fStep *= ( float )pSample->get_sample_rate() / pAudioDriver->getSampleRate(); // Adjust for audio driver sample rate
		
		
		if ( nFrame == 0 ){
			fSamplePos = 0;
		} else {
			fSamplePos = ( nFrame - nFrameOffset ) * fStep;
		}
		
		nAvail_bytes = ( int )( ( float )( pSample->get_frames() - fSamplePos ) / fStep );
	
		if ( nAvail_bytes > nBufferSize ) {
			nAvail_bytes = nBufferSize;
		}

		int nFinalBufferPos = nInitialBufferPos + nAvail_bytes;
	
		for ( int nBufferPos = nInitialBufferPos; nBufferPos < nFinalBufferPos; ++nBufferPos ) {
			int nSamplePos = ( int ) fSamplePos;
			double fDiff = fSamplePos - nSamplePos;
			if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
				//we reach the last audioframe.
				//set this last frame to zero do nothing wrong.
							fVal_L = 0.0;
							fVal_R = 0.0;
			} else {
				// some interpolation methods need 4 frames data.
					float last_l;
					float last_r;
					if ( ( nSamplePos + 2 ) >= nSampleFrames ) {
						last_l = 0.0;
						last_r = 0.0;
					} else {
						last_l =  pSample_data_L[nSamplePos + 2];
						last_r =  pSample_data_R[nSamplePos + 2];
					}
	
					switch( m_interpolateMode ){
	
						case Interpolation::InterpolateMode::Linear:
								fVal_L = pSample_data_L[nSamplePos] * (1 - fDiff ) + pSample_data_L[nSamplePos + 1] * fDiff;
								fVal_R = pSample_data_R[nSamplePos] * (1 - fDiff ) + pSample_data_R[nSamplePos + 1] * fDiff;
								break;
						case Interpolation::InterpolateMode::Cosine:
								fVal_L = Interpolation::cosine_Interpolate( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff);
								fVal_R = Interpolation::cosine_Interpolate( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff);
								break;
						case Interpolation::InterpolateMode::Third:
								fVal_L = Interpolation::third_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
								fVal_R = Interpolation::third_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
								break;
						case Interpolation::InterpolateMode::Cubic:
								fVal_L = Interpolation::cubic_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
								fVal_R = Interpolation::cubic_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
								break;
						case Interpolation::InterpolateMode::Hermite:
								fVal_L = Interpolation::hermite_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
								fVal_R = Interpolation::hermite_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
								break;
					}
			}

			fVal_L *= pSong->getPlaybackTrackVolume();
			fVal_R *= pSong->getPlaybackTrackVolume();
			
			if ( fVal_L > fInstrPeak_L ) {
				fInstrPeak_L = fVal_L;
			}
			if ( fVal_R > fInstrPeak_R ) {
				fInstrPeak_R = fVal_R;
			}

			m_pMainOut_L[nBufferPos] += fVal_L;
			m_pMainOut_R[nBufferPos] += fVal_R;


			fSamplePos += fStep;
		} //for
	}
	
	m_pPlaybackTrackInstrument->set_peak_l( fInstrPeak_L );
	m_pPlaybackTrackInstrument->set_peak_r( fInstrPeak_R );

	return true;
}

bool Sampler::renderNoteResample(
	std::shared_ptr<Sample> pSample,
	Note *pNote,
	std::shared_ptr<SelectedLayerInfo> pSelectedLayerInfo,
	std::shared_ptr<InstrumentComponent> pCompo,
	std::shared_ptr<DrumkitComponent> pDrumCompo,
	int nBufferSize,
	int nInitialBufferPos,
	float fCost_L,
	float fCost_R,
	float fCostTrack_L,
	float fCostTrack_R,
	float fLayerPitch
)
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioDriver = pHydrogen->getAudioOutput();
	auto pSong = pHydrogen->getSong();

	if ( pSong == nullptr ) {
		ERRORLOG( "Invalid song" );
		return true;
	}

	if ( pNote == nullptr ) {
		ERRORLOG( "Invalid note" );
		return true;
	}

	if ( pAudioDriver == nullptr ) {
		ERRORLOG( "AudioDriver is not ready!" );
		return true;
	}

	auto pInstrument = pNote->get_instrument();
	if ( pInstrument == nullptr ) {
		ERRORLOG( "Invalid note instrument" );
		return true;
	}

	const float fNotePitch = pNote->get_total_pitch() + fLayerPitch;
	const bool bResample = fNotePitch != 0 ||
		pSample->get_sample_rate() != pAudioDriver->getSampleRate();

	float fStep;
	if ( bResample ){
		fStep = Note::pitchToFrequency( fNotePitch );

		// Adjust for audio driver sample rate
		fStep *= static_cast<float>(pSample->get_sample_rate()) /
			static_cast<float>(pAudioDriver->getSampleRate());
	}
	else {
		fStep = 1;
	}

	auto pSample_data_L = pSample->get_data_l();
	auto pSample_data_R = pSample->get_data_r();
	const int nSampleFrames = pSample->get_frames();
	// The number of frames of the sample left to process.
	const int nRemainingFrames = static_cast<int>(
		(static_cast<float>(nSampleFrames) - pSelectedLayerInfo->fSamplePosition) /
		fStep );

	bool bRetValue = true; // the note is ended
	int nAvail_bytes;
	if ( nRemainingFrames > nBufferSize - nInitialBufferPos ) {
		// It The number of frames of the sample left to process
		// exceeds what's available in the current process cycle of
		// the Sampler. Clip it.
		nAvail_bytes = nBufferSize - nInitialBufferPos;
		// the note is not ended yet
		bRetValue = false;
	}
	else if ( pInstrument->is_filter_active() && pNote->filter_sustain() ) {
		// If filter is causing note to ring, process more samples.
		nAvail_bytes = nBufferSize - nInitialBufferPos;
	}
	else {
		nAvail_bytes = nRemainingFrames;
	}

	double fSamplePos = pSelectedLayerInfo->fSamplePosition;
	const int nFinalBufferPos = nInitialBufferPos + nAvail_bytes;

	int nNoteEnd;
	// If the user set a custom length of the note in the PatternEditor, we will
	// use it to trigger the releases of the note. Otherwise, the whole sample
	// will be played back.
	if ( pNote->get_length() != -1 &&
		 pNote->get_adsr()->getState() != ADSR::State::Release ) {
		if ( pSelectedLayerInfo->nNoteLength == -1 ) {
			// The length of a note is only calculated once when first
			// encountering it. This makes us robust again glitches due to tempo
			// changes.
			double fTickMismatch;

			pSelectedLayerInfo->nNoteLength =
				TransportPosition::computeFrameFromTick(
					pNote->get_position() + pNote->get_length(),
					&fTickMismatch, pSample->get_sample_rate() ) -
				TransportPosition::computeFrameFromTick(
					pNote->get_position(), &fTickMismatch,
					pSample->get_sample_rate() );
		}

		nNoteEnd = std::min(nFinalBufferPos + 1, static_cast<int>(
			(static_cast<float>(pSelectedLayerInfo->nNoteLength) -
				pSelectedLayerInfo->fSamplePosition) / fStep ));

		if ( nNoteEnd < 0 ) {
			if ( ! pInstrument->is_filter_active() ) {
				// In case resonance filtering is active the sampler stops
				// rendering of the sample at the custom note length but let's
				// the filter itself ring on.
				ERRORLOG( QString( "Note end located within the previous processing cycle. nNoteEnd: %1, nNoteLength: %2, fSamplePosition: %3, nFinalBufferPos: %4, fStep: %5")
						  .arg( nNoteEnd ).arg( pSelectedLayerInfo->nNoteLength )
						  .arg( pSelectedLayerInfo->fSamplePosition )
						  .arg( nFinalBufferPos ).arg( fStep ) );
			}
			nNoteEnd = 0;
		}
	}
	else {
		// Do not apply release but play the whole sample instead. In case we
		// already released a note of custom length, we will use the whole
		// buffer to apply the release decay.
		nNoteEnd = nFinalBufferPos + 1;
	}

	float fInstrPeak_L = pInstrument->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pInstrument->get_peak_r(); // this value will be reset to 0 by the mixer..

	auto pADSR = pNote->get_adsr();
	float fVal_L;
	float fVal_R;

#ifdef H2CORE_HAVE_JACK
	float* pTrackOutL = nullptr;
	float* pTrackOutR = nullptr;

	if ( Preferences::get_instance()->m_bJackTrackOuts ) {
		auto pJackAudioDriver = dynamic_cast<JackAudioDriver*>( pAudioDriver );
		if ( pJackAudioDriver != nullptr ) {
			pTrackOutL = pJackAudioDriver->getTrackOut_L( pInstrument, pCompo );
			pTrackOutR = pJackAudioDriver->getTrackOut_R( pInstrument, pCompo );
		}
	}
#endif

	float buffer_L[MAX_BUFFER_SIZE];
	float buffer_R[MAX_BUFFER_SIZE];

	// Main rendering loop.
	// With some re-work, more of this could likely be vectorised fairly easily.
	//   - assert no buffer aliasing
	//   - template and multiple instantiations for is_filter_active x each interpolation method
	//   - iterate LP IIR filter coefficients to longer IIR filter to fit vector width
	//
	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nFinalBufferPos;
		  ++nBufferPos ) {

		int nSamplePos = static_cast<int>(fSamplePos);
		double fDiff = fSamplePos - nSamplePos;
		if ( ( nSamplePos - 1 ) >= nSampleFrames ) {
			//we reach the last audioframe.
			//set this last frame to zero do nothing wrong.
			fVal_L = 0.0;
			fVal_R = 0.0;
		} else {
			if ( ! bResample ) {
				if ( nSamplePos < nSampleFrames ) {
					fVal_L = pSample_data_L[ nSamplePos ];
					fVal_R = pSample_data_R[ nSamplePos ];
				} else {
					fVal_L = 0.0;
					fVal_R = 0.0;
				}
			}
			else {
				// Gather frame samples
				float l0, l1, l2, l3, r0, r1, r2, r3;
				// Short-circuit: the common case is that all required frames are within the sample.
				if ( nSamplePos >= 1 && nSamplePos + 2 < nSampleFrames ) {
					l0=  pSample_data_L[ nSamplePos-1 ];
					l1 = pSample_data_L[ nSamplePos ];
					l2 = pSample_data_L[ nSamplePos+1 ];
					l3 = pSample_data_L[ nSamplePos+2 ];
					r0 = pSample_data_R[ nSamplePos-1 ];
					r1 = pSample_data_R[ nSamplePos ];
					r2 = pSample_data_R[ nSamplePos+1 ];
					r3 = pSample_data_R[ nSamplePos+2 ];
				} else {
					l0 = l1 = l2 = l3 = r0 = r1 = r2 = r3 = 0.0;
					// Some required frames are off the beginning or end of the sample.
					if ( nSamplePos >= 1 && nSamplePos < nSampleFrames + 1 ) {
						l0 = pSample_data_L[ nSamplePos-1 ];
						r0 = pSample_data_R[ nSamplePos-1 ];
					}
					// Each successive frame may be past the end of the sample so check individually.
					if ( nSamplePos < nSampleFrames ) {
						l1 = pSample_data_L[ nSamplePos ];
						r1 = pSample_data_R[ nSamplePos ];
						if ( nSamplePos+1 < nSampleFrames ) {
							l2 = pSample_data_L[ nSamplePos+1 ];
							r2 = pSample_data_R[ nSamplePos+1 ];
							if ( nSamplePos+2 < nSampleFrames ) {
								l3 = pSample_data_L[ nSamplePos+2 ];
								r3 = pSample_data_R[ nSamplePos+2 ];
							}
						}
					}
				}

				// Interpolate frame values from Sample domain to audio output range
				switch ( m_interpolateMode ) {
					case Interpolation::InterpolateMode::Linear:
						fVal_L = l1 * (1 - fDiff ) + l2 * fDiff;
						fVal_R = r1 * (1 - fDiff ) + r2 * fDiff;
						break;
					case Interpolation::InterpolateMode::Cosine:
						fVal_L = Interpolation::cosine_Interpolate( l1, l2, fDiff);
						fVal_R = Interpolation::cosine_Interpolate( r1, r2, fDiff);
						break;
					case Interpolation::InterpolateMode::Third:
						fVal_L = Interpolation::third_Interpolate( l0, l1, l2, l3, fDiff);
						fVal_R = Interpolation::third_Interpolate( r0, r1, r2, r3, fDiff);
						break;
					case Interpolation::InterpolateMode::Cubic:
						fVal_L = Interpolation::cubic_Interpolate( l0, l1, l2, l3, fDiff);
						fVal_R = Interpolation::cubic_Interpolate( r0, r1, r2, r3, fDiff);
						break;
					case Interpolation::InterpolateMode::Hermite:
						fVal_L = Interpolation::hermite_Interpolate( l0, l1, l2, l3, fDiff);
						fVal_R = Interpolation::hermite_Interpolate( r0, r1, r2, r3, fDiff);
						break;
				}
			}
		}

		buffer_L[nBufferPos] = fVal_L;
		buffer_R[nBufferPos] = fVal_R;

		fSamplePos += fStep;
	}

	if ( pADSR->applyADSR( buffer_L, buffer_R, nFinalBufferPos, nNoteEnd,
						   fStep ) ) {
		bRetValue = true;
	}

	// Low pass resonant filter
	if ( pInstrument->is_filter_active() ) {
		for ( int nBufferPos = nInitialBufferPos; nBufferPos < nFinalBufferPos;
			  ++nBufferPos ) {

			fVal_L = buffer_L[ nBufferPos ];
			fVal_R = buffer_R[ nBufferPos ];

			pNote->compute_lr_values( &fVal_L, &fVal_R );

			buffer_L[ nBufferPos ] = fVal_L;
			buffer_R[ nBufferPos ] = fVal_R;

		}
	}

	// Mix rendered sample buffer to track and mixer output
	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nFinalBufferPos;
		  ++nBufferPos ) {

		fVal_L = buffer_L[ nBufferPos ];
		fVal_R = buffer_R[ nBufferPos ];

#ifdef H2CORE_HAVE_JACK
		if ( pTrackOutL ) {
			pTrackOutL[nBufferPos] += fVal_L * fCostTrack_L;
		}
		if ( pTrackOutR ) {
			pTrackOutR[nBufferPos] += fVal_R * fCostTrack_R;
		}
#endif

		fVal_L *= fCost_L;
		fVal_R *= fCost_R;

		// update instr peak
		if ( fVal_L > fInstrPeak_L ) {
			fInstrPeak_L = fVal_L;
		}
		if ( fVal_R > fInstrPeak_R ) {
			fInstrPeak_R = fVal_R;
		}

		pDrumCompo->set_outs( nBufferPos, fVal_L, fVal_R );

		// to main mix
		m_pMainOut_L[nBufferPos] += fVal_L;
		m_pMainOut_R[nBufferPos] += fVal_R;

	}

	if ( pInstrument->is_filter_active() && pNote->filter_sustain() ) {
		// Note is still ringing, do not end.
		bRetValue = false;
	}
	
	pSelectedLayerInfo->fSamplePosition += nAvail_bytes * fStep;
	pInstrument->set_peak_l( fInstrPeak_L );
	pInstrument->set_peak_r( fInstrPeak_R );


#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
	// change the below return logic if you add code after that ifdef
	if ( pInstrument->is_muted() || pSong->getIsMuted() ) {
		return bRetValue;
	}
	float masterVol = pSong->getVolume();
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		float fLevel = pInstrument->get_fx_level( nFX );
		if ( pFX != nullptr && fLevel != 0.0 ) {
			fLevel = fLevel * pFX->getVolume();

			float *pBuf_L = pFX->m_pBuffer_L;
			float *pBuf_R = pFX->m_pBuffer_R;

			float fFXCost_L = fLevel * masterVol;
			float fFXCost_R = fLevel * masterVol;

			int nBufferPos = nInitialBufferPos;
			for ( int i = 0; i < nAvail_bytes; ++i ) {

				fVal_L = buffer_L[ nBufferPos ];
				fVal_R = buffer_R[ nBufferPos ];

				pBuf_L[ nBufferPos ] += fVal_L * fFXCost_L;
				pBuf_R[ nBufferPos ] += fVal_R * fFXCost_R;
				++nBufferPos;
			}
		}
	}
#endif
	return bRetValue;
}


void Sampler::stopPlayingNotes( std::shared_ptr<Instrument> pInstr )
{
	if ( pInstr ) { // stop all notes using this instrument
		for ( unsigned i = 0; i < m_playingNotesQueue.size(); ) {
			Note *pNote = m_playingNotesQueue[ i ];
			assert( pNote );
			if ( pNote->get_instrument() == pInstr ) {
				delete pNote;
				pInstr->dequeue();
				m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
			}
			++i;
		}
	} else { // stop all notes
		// delete all copied notes in the playing notes queue
		for ( unsigned i = 0; i < m_playingNotesQueue.size(); ++i ) {
			Note *pNote = m_playingNotesQueue[i];
			pNote->get_instrument()->dequeue();
			delete pNote;
		}
		m_playingNotesQueue.clear();
	}
}



/// Preview, uses only the first layer
void Sampler::preview_sample(std::shared_ptr<Sample> pSample, int nLength )
{
	if ( m_pPreviewInstrument == nullptr ) {
		ERRORLOG( "Invalid preview instrument" );
		return;
	}

	if ( ! m_pPreviewInstrument->hasSamples() ) {
		return;
	}
	
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

	for (const auto& pComponent: *m_pPreviewInstrument->get_components()) {
		auto pLayer = pComponent->get_layer( 0 );

		pLayer->set_sample( pSample );

		Note *pPreviewNote = new Note( m_pPreviewInstrument, 0, 1.0, 0.f, nLength );

		stopPlayingNotes( m_pPreviewInstrument );
		noteOn( pPreviewNote );

	}

	Hydrogen::get_instance()->getAudioEngine()->unlock();
}



void Sampler::preview_instrument( std::shared_ptr<Instrument> pInstr )
{
	if ( pInstr == nullptr ) {
		ERRORLOG( "Invalid instrument" );
		return;
	}

	if ( ! pInstr->hasSamples() ) {
		return;
	}
	
	std::shared_ptr<Instrument> pOldPreview;
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

	stopPlayingNotes( m_pPreviewInstrument );

	pOldPreview = m_pPreviewInstrument;
	m_pPreviewInstrument = pInstr;
	pInstr->set_is_preview_instrument(true);

	Note *pPreviewNote = new Note( m_pPreviewInstrument, 0, 1.0, 0.f, MAX_NOTES );

	noteOn( pPreviewNote );	// exclusive note
	Hydrogen::get_instance()->getAudioEngine()->unlock();
}

bool Sampler::isInstrumentPlaying( std::shared_ptr<Instrument> instrument )
{
	if ( instrument ) { // stop all notes using this instrument
		for ( unsigned j = 0; j < m_playingNotesQueue.size(); j++ ) {
			if ( instrument->get_name() == m_playingNotesQueue[ j ]->get_instrument()->get_name()){
				return true;
			}
		}
	}
	return false;
}

void Sampler::reinitializePlaybackTrack()
{
	Hydrogen*	pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	std::shared_ptr<Sample>	pSample;

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	if( pHydrogen->getPlaybackTrackState() != Song::PlaybackTrack::Unavailable ){
		pSample = Sample::load( pSong->getPlaybackTrackFilename() );
	}
	
	auto  pPlaybackTrackLayer = std::make_shared<InstrumentLayer>( pSample );

	m_pPlaybackTrackInstrument->get_components()->front()->set_layer( pPlaybackTrackLayer, 0 );
	m_nPlayBackSamplePosition = 0;
}

};

