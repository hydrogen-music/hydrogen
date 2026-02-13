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

#include <core/Sampler/Sampler.h>

#include <cassert>
#include <cmath>
#include <cstdlib>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/Globals.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/IO/AudioDriver.h>
#include <core/IO/JackDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {

static std::shared_ptr<Instrument>
createInstrument( Instrument::Id id, const QString& sFilePath, float volume )
{
	auto pInstrument = std::make_shared<Instrument>( id, sFilePath );
	pInstrument->setVolume( volume );
	auto pLayer =
		std::make_shared<InstrumentLayer>( Sample::load( sFilePath ) );
	auto pComponent = pInstrument->getComponent( 0 );
	if ( pComponent != nullptr ) {
		pInstrument->addLayer(
			pComponent, pLayer, -1, Event::Trigger::Suppress
		);
	}
	else {
		___ERRORLOG( "Invalid default component" );
	}

	return pInstrument;
}

Sampler::Sampler()
	: m_pMainOut_L( nullptr ),
	  m_pMainOut_R( nullptr ),
	  m_pPreviewInstrument( nullptr ),
	  m_interpolateMode( Interpolation::InterpolateMode::Linear )
{
	m_pMainOut_L = new float[MAX_BUFFER_SIZE];
	m_pMainOut_R = new float[MAX_BUFFER_SIZE];

	QString sEmptySampleFileName = Filesystem::empty_sample_path();

	// instrument used in file preview
	m_pDefaultPreviewInstrument =
		createInstrument( Instrument::EmptyId, sEmptySampleFileName, 0.8 );
	m_pDefaultPreviewInstrument->setIsPreviewInstrument( true );
	m_pPreviewInstrument = m_pDefaultPreviewInstrument;

	// dummy instrument used for playback track
	m_pPlaybackTrackInstrument = createInstrument(
		Instrument::PlaybackTrackId, sEmptySampleFileName, 0.8
	);
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

	// Max notes limit
	int nMaxNotes = Preferences::get_instance()->m_nMaxNotes;
	while ( (int) m_playingNotesQueue.size() > nMaxNotes ) {
		auto pOldNote = m_playingNotesQueue[0];
		m_playingNotesQueue.erase( m_playingNotesQueue.begin() );
		if ( pOldNote->getInstrument() != nullptr ) {
			pOldNote->getInstrument()->dequeue( pOldNote );
			WARNINGLOG( QString( "Number of playing notes [%1] exceeds maximum "
								 "[%2]. Dropping note [%3]" )
							.arg( m_playingNotesQueue.size() )
							.arg( nMaxNotes )
							.arg( pOldNote->toQString() ) );
		}
		else {
			ERRORLOG( QString( "Old note in Sampler has no instrument! [%1]" )
						  .arg( pOldNote->toQString() ) );
		}
	}

	// Render next `nFrames` audio frames of all playing notes.
	unsigned i = 0;
	std::shared_ptr<Note> pNote = nullptr;
	while ( i < m_playingNotesQueue.size() ) {
		pNote = m_playingNotesQueue[i];
		if ( pNote != nullptr && handleNote( pNote, nFrames ) ) {
			// End of note was reached during rendering.
			m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
			if ( pNote->getInstrument() != nullptr ) {
				pNote->getInstrument()->dequeue( pNote );
			}
			else {
				ERRORLOG(
					QString(
						"Playing note in sampler does not have instrument! [%1]"
					)
						.arg( pNote->prettyName() )
				);
			}

			// Only send Note-Off messages in case we already sent an Note-On.
			if ( pNote->getMidiNoteOnSentFrame() != -1 ) {
				// Ensure notes of custom length result in Note-On and Note-Off
				// messages corresponding to the user-defined length (regardless
				// of the underlying sample).
				if ( pNote->getLength() != LENGTH_ENTIRE_SAMPLE ) {
					const auto nPrevStart = pNote->getNoteStart();
					pNote->setMidiNoteOffFrame(
						pNote->getMidiNoteOnSentFrame() +
						TransportPosition::computeFrame(
							pNote->getLength(), Hydrogen::get_instance()
													->getAudioEngine()
													->getTransportPosition()
													->getTickSize()
						)
					);
					m_scheduledNoteOffQueue.push( pNote );
				}
				else {
					m_queuedNoteOffs.push_back( pNote );
				}
			}
		}
		else if ( pNote == nullptr ) {
			// Pop invalid note.
			m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
		}
		else {
			// As finished notes are poped above
			++i;
		}
	}

	auto sendNote = [&]( std::shared_ptr<Note> pNote ) {
		return pNote != nullptr && pNote->getInstrument() != nullptr &&
			   pNote->getMidiNoteOnSentFrame() != -1 &&
			   ( Preferences::get_instance()->getMidiSendNoteOff() ==
					 Preferences::MidiSendNoteOff::Always ||
				 ( Preferences::get_instance()->getMidiSendNoteOff() ==
					   Preferences::MidiSendNoteOff::OnCustomLengths &&
				   pNote->getLength() != LENGTH_ENTIRE_SAMPLE ) );
	};

	// NoteOffs to be send immediately,
	if ( m_queuedNoteOffs.size() > 0 ) {
		const auto pMidiInstrumentMap =
			Preferences::get_instance()->getMidiInstrumentMap();
		auto pMidiDriver = pHydrogen->getMidiDriver();
		if ( pMidiDriver != nullptr ) {
			// Queue midi note off messages for notes that have a length
			// specified for them
			while ( !m_queuedNoteOffs.empty() ) {
				pNote = m_queuedNoteOffs[0];

				if ( sendNote( pNote ) ) {
					const auto noteRef =
						pMidiInstrumentMap->getOutputMapping( pNote );
					MidiMessage::NoteOff noteOff;
					noteOff.channel = noteRef.channel;
					noteOff.note = noteRef.note;
					noteOff.velocity = pNote->getMidiVelocity();
					if ( noteOff.channel != Midi::ChannelOff &&
						 noteOff.channel != Midi::ChannelInvalid ) {
						pMidiDriver->enqueueOutputMessage(
							MidiMessage::from( noteOff )
						);
					}
				}
				else if ( pNote == nullptr ||
						  pNote->getInstrument() == nullptr ) {
					ERRORLOG( QString( "Queued note off in sampler does not "
									   "have instrument! [%1]" )
								  .arg( pNote->toQString() ) );
				}

				m_queuedNoteOffs.erase( m_queuedNoteOffs.begin() );

				pNote = nullptr;
			}
		}
	}

	// NoteOffs to be send on a given time.
	if ( m_scheduledNoteOffQueue.size() > 0 ) {
		const auto pMidiInstrumentMap =
			Preferences::get_instance()->getMidiInstrumentMap();
		auto pMidiDriver = pHydrogen->getMidiDriver();
		while ( !m_scheduledNoteOffQueue.empty() ) {
			if ( pMidiDriver == nullptr ) {
				m_scheduledNoteOffQueue.pop();
				continue;
			}
			auto pNote = m_scheduledNoteOffQueue.top();
			if ( !sendNote( pNote ) ) {
				m_scheduledNoteOffQueue.pop();
				continue;
			}

			auto pAudioEngine = pHydrogen->getAudioEngine();
			long long nCurrentFrame;
			if ( pAudioEngine->getState() == AudioEngine::State::Playing ||
				 pAudioEngine->getState() == AudioEngine::State::Testing ) {
				// Current transport position.
				nCurrentFrame =
					pAudioEngine->getTransportPosition()->getFrame();
			}
			else {
				// In case the playback is stopped we pretend it is still
				// rolling using the realtime ticks while disregarding tempo
				// changes in the Timeline. This is important as we want to
				// continue playing back notes in the sampler and process
				// realtime events, by e.g. MIDI or Hydrogen's virtual
				// keyboard.
				nCurrentFrame = pAudioEngine->getRealtimeFrame();
			}

			if ( pNote->getMidiNoteOffFrame() <
				 nCurrentFrame + static_cast<long long>( nFrames ) ) {
				m_scheduledNoteOffQueue.pop();

				const auto noteRef =
					pMidiInstrumentMap->getOutputMapping( pNote );
				MidiMessage::NoteOff noteOff;
				noteOff.channel = noteRef.channel;
				noteOff.note = noteRef.note;
				noteOff.velocity = pNote->getMidiVelocity();
				if ( noteOff.channel != Midi::ChannelOff &&
					 noteOff.channel != Midi::ChannelInvalid ) {
					pMidiDriver->enqueueOutputMessage(
						MidiMessage::from( noteOff )
					);
				}
			}
			else {
				// Note has not been reached yet.
				break;
			}
		}
	}

	processPlaybackTrack( nFrames );
}

bool Sampler::isRenderingNotes() const
{
	return m_playingNotesQueue.size() > 0 || m_scheduledNoteOffQueue.size() > 0;
}

bool Sampler::noteOn( std::shared_ptr<Note> pNote )
{
	assert( pNote );
	if ( pNote == nullptr ) {
		ERRORLOG( "Invalid note" );
		return false;
	}

	if ( pNote->getInstrument() == nullptr || pNote->getAdsr() == nullptr ) {
		ERRORLOG( QString( "Invalid note [%1]" ).arg( pNote->toQString() ) );
		return false;
	}

	pNote->getAdsr()->attack();
	auto pInstr = pNote->getInstrument();

	// Mute group handling
	//
	// For each new note all other notes associated with instruments bearing the
	// same mute group currently rendered by the Sampler will immediately
	// transit into release phase (rendering will be stopped gently).
	//
	// If there are two notes of the same mute group at the same
	// position/column, only one can be played. To allow for the user have
	// (limited) control over which one is chosen, we will render the note of
	// the bottom-most instrument according to the current instrument order in
	// the drumkit.
	const int nMuteGrp = pInstr->getMuteGroup();
	if ( nMuteGrp != -1 ) {
		const auto pSong = Hydrogen::get_instance()->getSong();

		// remove all notes using the same mute group
		for ( const auto& pOtherNote :
			  m_playingNotesQueue ) {  // delete older note
			if ( pOtherNote != nullptr &&
				 pOtherNote->getInstrument() != nullptr &&
				 pOtherNote->getAdsr() != nullptr &&
				 pOtherNote->getInstrument() != pInstr &&
				 pOtherNote->getInstrument()->getMuteGroup() == nMuteGrp ) {
				if ( pOtherNote->getPosition() == pNote->getPosition() &&
					 pSong != nullptr && pSong->getDrumkit() != nullptr &&
					 pSong->getDrumkit()->getInstruments()->index(
						 pOtherNote->getInstrument()
					 ) > pSong->getDrumkit()->getInstruments()->index( pInstr
						 ) ) {
					// There is another note of the same mute group at a lower
					// position. We keep it and discard the provided note.
					return false;
				}
				else {
					pOtherNote->getAdsr()->release();
				}
			}
		}
	}

	if ( pNote->getNoteOff() ) {
		for ( const auto& pOtherNote : m_playingNotesQueue ) {
			if ( pOtherNote != nullptr &&
				 pOtherNote->getInstrument() != nullptr &&
				 pOtherNote->getAdsr() != nullptr &&
				 pOtherNote->getInstrument() == pInstr ) {
				pOtherNote->getAdsr()->release();
			}
		}
	}

	if ( !pNote->getNoteOff() ) {
		pInstr->enqueue( pNote );
		m_playingNotesQueue.push_back( pNote );
		return true;
	}

	return false;
}

void Sampler::midiKeyboardNoteOff(
	std::shared_ptr<Instrument> pInstrument,
	Note::Key key,
	Note::Octave octave
)
{
	if ( pInstrument == nullptr ) {
		return;
	}
	for ( const auto& ppNote : m_playingNotesQueue ) {
		if ( ppNote != nullptr &&
			 ppNote->getInstrumentId() == pInstrument->getId() &&
			 ppNote->getKey() == key && ppNote->getOctave() == octave &&
			 ppNote->getAdsr() != nullptr ) {
			ppNote->getAdsr()->release();
		}
	}
}

// functions for pan parameters and laws-----------------

float Sampler::getRatioPan( float fPan_L, float fPan_R )
{
	if ( fPan_L < 0. || fPan_R < 0. ||
		 ( fPan_L == 0. && fPan_R == 0. ) ) {  // invalid input
		WARNINGLOG(
			"Invalid (panL, panR): both zero or some is negative. Pan set to "
			"center."
		);
		return 0.;	// default central value
	}
	else {
		if ( fPan_L >= fPan_R ) {
			return fPan_R / fPan_L - 1.;
		}
		else {
			return 1. - fPan_L / fPan_R;
		}
	}
}

float Sampler::ratioStraightPolygonalPanLaw( float fPan )
{
	// the straight polygonal pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1.;
	}
	else {
		return ( 1. - fPan );
	}
}

float Sampler::ratioConstPowerPanLaw( float fPan )
{
	// the constant power pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1. / sqrt( 1 + ( 1. + fPan ) * ( 1. + fPan ) );
	}
	else {
		return ( 1. - fPan ) / sqrt( 1 + ( 1. - fPan ) * ( 1. - fPan ) );
	}
}

float Sampler::ratioConstSumPanLaw( float fPan )
{
	// the constant Sum pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1. / ( 2. + fPan );
	}
	else {
		return ( 1. - fPan ) / ( 2. - fPan );
	}
}

float Sampler::linearStraightPolygonalPanLaw( float fPan )
{
	// the constant power pan law interpreting fPan as the "linear" parameter
	if ( fPan <= 0 ) {
		return 1.;
	}
	else {
		return ( 1. - fPan ) / ( 1. + fPan );
	}
}

float Sampler::linearConstPowerPanLaw( float fPan )
{
	// the constant power pan law interpreting fPan as the "linear" parameter
	return ( 1. - fPan ) / sqrt( 2. * ( 1 + fPan * fPan ) );
}

float Sampler::linearConstSumPanLaw( float fPan )
{
	// the constant Sum pan law interpreting fPan as the "linear" parameter
	return ( 1. - fPan ) * 0.5;
}

float Sampler::polarStraightPolygonalPanLaw( float fPan )
{
	// the constant power pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	if ( fPan <= 0 ) {
		return 1.;
	}
	else {
		return cos( fTheta ) / sin( fTheta );
	}
}

float Sampler::polarConstPowerPanLaw( float fPan )
{
	// the constant power pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	return cos( fTheta );
}

float Sampler::polarConstSumPanLaw( float fPan )
{
	// the constant Sum pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	return cos( fTheta ) / ( cos( fTheta ) + sin( fTheta ) );
}

float Sampler::quadraticStraightPolygonalPanLaw( float fPan )
{
	// the straight polygonal pan law interpreting fPan as the "quadratic"
	// parameter
	if ( fPan <= 0 ) {
		return 1.;
	}
	else {
		return sqrt( ( 1. - fPan ) / ( 1. + fPan ) );
	}
}

float Sampler::quadraticConstPowerPanLaw( float fPan )
{
	// the constant power pan law interpreting fPan as the "quadratic" parameter
	return sqrt( ( 1. - fPan ) * 0.5 );
}

float Sampler::quadraticConstSumPanLaw( float fPan )
{
	// the constant Sum pan law interpreting fPan as the "quadratic" parameter
	return sqrt( 1. - fPan ) / ( sqrt( 1. - fPan ) + sqrt( 1. + fPan ) );
}

float Sampler::linearConstKNormPanLaw( float fPan, float k )
{
	// the constant k norm pan law interpreting fPan as the "linear" parameter
	return ( 1. - fPan ) /
		   pow( ( pow( ( 1. - fPan ), k ) + pow( ( 1. + fPan ), k ) ), 1. / k );
}

float Sampler::quadraticConstKNormPanLaw( float fPan, float k )
{
	// the constant k norm pan law interpreting fPan as the "quadratic"
	// parameter
	return sqrt( 1. - fPan ) / pow( ( pow( ( 1. - fPan ), 0.5 * k ) +
									  pow( ( 1. + fPan ), 0.5 * k ) ),
									1. / k );
}

float Sampler::polarConstKNormPanLaw( float fPan, float k )
{
	// the constant k norm pan law interpreting fPan as the "polar" parameter
	float fTheta = 0.25 * M_PI * ( fPan + 1 );
	float cosTheta = cos( fTheta );
	return cosTheta /
		   pow( ( pow( cosTheta, k ) + pow( sin( fTheta ), k ) ), 1. / k );
}

float Sampler::ratioConstKNormPanLaw( float fPan, float k )
{
	// the constant k norm pan law interpreting fPan as the "ratio" parameter
	if ( fPan <= 0 ) {
		return 1. / pow( ( 1. + pow( ( 1. + fPan ), k ) ), 1. / k );
	}
	else {
		return ( 1. - fPan ) / pow( ( 1. + pow( ( 1. - fPan ), k ) ), 1. / k );
	}
}

// function to direct the computation to the selected pan law.
inline float Sampler::panLaw( float fPan, std::shared_ptr<Song> pSong )
{
	int nPanLawType = pSong->getPanLawType();
	if ( nPanLawType == RATIO_STRAIGHT_POLYGONAL ) {
		return ratioStraightPolygonalPanLaw( fPan );
	}
	else if ( nPanLawType == RATIO_CONST_POWER ) {
		return ratioConstPowerPanLaw( fPan );
	}
	else if ( nPanLawType == RATIO_CONST_SUM ) {
		return ratioConstSumPanLaw( fPan );
	}
	else if ( nPanLawType == LINEAR_STRAIGHT_POLYGONAL ) {
		return linearStraightPolygonalPanLaw( fPan );
	}
	else if ( nPanLawType == LINEAR_CONST_POWER ) {
		return linearConstPowerPanLaw( fPan );
	}
	else if ( nPanLawType == LINEAR_CONST_SUM ) {
		return linearConstSumPanLaw( fPan );
	}
	else if ( nPanLawType == POLAR_STRAIGHT_POLYGONAL ) {
		return polarStraightPolygonalPanLaw( fPan );
	}
	else if ( nPanLawType == POLAR_CONST_POWER ) {
		return polarConstPowerPanLaw( fPan );
	}
	else if ( nPanLawType == POLAR_CONST_SUM ) {
		return polarConstSumPanLaw( fPan );
	}
	else if ( nPanLawType == QUADRATIC_STRAIGHT_POLYGONAL ) {
		return quadraticStraightPolygonalPanLaw( fPan );
	}
	else if ( nPanLawType == QUADRATIC_CONST_POWER ) {
		return quadraticConstPowerPanLaw( fPan );
	}
	else if ( nPanLawType == QUADRATIC_CONST_SUM ) {
		return quadraticConstSumPanLaw( fPan );
	}
	else if ( nPanLawType == LINEAR_CONST_K_NORM ) {
		return linearConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	}
	else if ( nPanLawType == POLAR_CONST_K_NORM ) {
		return polarConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	}
	else if ( nPanLawType == RATIO_CONST_K_NORM ) {
		return ratioConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	}
	else if ( nPanLawType == QUADRATIC_CONST_K_NORM ) {
		return quadraticConstKNormPanLaw( fPan, pSong->getPanLawKNorm() );
	}
	else {
		WARNINGLOG( "Unknown pan law type. Set default." );
		pSong->setPanLawType( RATIO_STRAIGHT_POLYGONAL );
		return ratioStraightPolygonalPanLaw( fPan );
	}
}

void Sampler::handleTimelineOrTempoChange()
{
	if ( m_playingNotesQueue.size() == 0 ) {
		return;
	}

	for ( auto& ppNote : m_playingNotesQueue ) {
		if ( ppNote == nullptr || ppNote->getInstrument() == nullptr ) {
			continue;
		}

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
		if ( ppNote->isPartiallyRendered() &&
			 ppNote->getLength() != LENGTH_ENTIRE_SAMPLE &&
			 ppNote->getUsedTickSize() != -1 ) {
			double fTickMismatch;

			// Do so for all layers of all components current processed.
			for ( const auto& [_, ppSelectedLayerInfo] :
				  ppNote->getAllSelectedLayerInfos() ) {
				if ( ppSelectedLayerInfo == nullptr ||
					 ppSelectedLayerInfo->pLayer == nullptr ) {
					continue;
				}
				const auto pSample = ppSelectedLayerInfo->pLayer->getSample();
				if ( pSample == nullptr ) {
					continue;
				}
				const int nNewNoteLength =
					TransportPosition::computeFrameFromTick(
						ppNote->getPosition() + ppNote->getLength(),
						&fTickMismatch, pSample->getSampleRate()
					) -
					TransportPosition::computeFrameFromTick(
						ppNote->getPosition(), &fTickMismatch,
						pSample->getSampleRate()
					);

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
				const int nSamplePosition = static_cast<int>(
					std::floor( ppSelectedLayerInfo->fSamplePosition )
				);

				ppSelectedLayerInfo->nNoteLength =
					nSamplePosition +
					static_cast<int>( std::round(
						static_cast<float>(
							ppSelectedLayerInfo->nNoteLength - nSamplePosition
						) *
						nNewNoteLength /
						static_cast<float>( ppSelectedLayerInfo->nNoteLength )
					) );
			}
		}
	}
}

void Sampler::handleSongSizeChange()
{
	if ( m_playingNotesQueue.size() == 0 ) {
		return;
	}

	const long nTickOffset =
		static_cast<long>( std::floor( Hydrogen::get_instance()
										   ->getAudioEngine()
										   ->getTransportPosition()
										   ->getTickOffsetSongSize() ) );

	for ( auto ppNote : m_playingNotesQueue ) {
		// DEBUGLOG( QString( "pos: %1 -> %2, nTickOffset: %3, note: %4" )
		// 		  .arg( ppNote->getPosition() )
		// 		  .arg( std::max( ppNote->getPosition() + nTickOffset,
		// 						  static_cast<long>(0) ) )
		// 		  .arg( nTickOffset )
		// 		  .arg( ppNote->toQString( "", true ) ) );

		ppNote->setPosition( std::max(
			ppNote->getPosition() + nTickOffset, static_cast<long>( 0 )
		) );
		ppNote->computeNoteStart();

		// DEBUGLOG( QString( "new note: %1" )
		// 		  .arg( ppNote->toQString( "", true ) ) );
	}
}

//------------------------------------------------------------------

bool Sampler::handleNote( std::shared_ptr<Note> pNote, unsigned nBufferSize )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song" );
		return true;
	}

	if ( pNote == nullptr ) {
		return true;
	}

	auto pInstr = pNote->getInstrument();
	if ( pInstr == nullptr ) {
		ERRORLOG( "NULL instrument" );
		return true;
	}

	long long nFrame;
	auto pAudioEngine = pHydrogen->getAudioEngine();
	if ( pAudioEngine->getState() == AudioEngine::State::Playing ||
		 pAudioEngine->getState() == AudioEngine::State::Testing ) {
		nFrame = pAudioEngine->getTransportPosition()->getFrame();
	}
	else {
		// use this to support realtime events when transport is not
		// rolling.
		nFrame = pAudioEngine->getRealtimeFrame();
	}

	// Only if the Sampler has not started rendering the note yet we
	// care about its starting position. Else we would encounter
	// glitches when relocating transport during playback or starting
	// transport while using realtime playback.
	long long nInitialBufferPos = 0;
	if ( !pNote->isPartiallyRendered() ) {
		long long nNoteStartInFrames = pNote->getNoteStart();

		// DEBUGLOG(QString( "nFrame: %1, note pos: %2,
		// pAudioEngine->getTransportPosition()->getTickSize(): %3,
		// pAudioEngine->getTransportPosition()->getTick(): %4,
		// pAudioEngine->getTransportPosition()->getFrame(): %5,
		// nNoteStartInFrames: %6 ") 		 .arg( nFrame ).arg( pNote->getPosition() )
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
				// this note is not valid. it's in the future...let's skip
				// it....
				ERRORLOG(
					QString( "Note pos in the future?? nFrame: %1, note start: "
							 "%2, nInitialBufferPos: %3, nBufferSize: %4" )
						.arg( nFrame )
						.arg( pNote->getNoteStart() )
						.arg( nInitialBufferPos )
						.arg( nBufferSize )
				);

				return true;
			}
		}
	}

	// new instrument and note pan interaction--------------------------
	// notePan moves the RESULTANT pan in a smaller pan range centered at
	// instrumentPan

	/** Get the RESULTANT pan, following a "matryoshka" multi panning, like in
	 *this graphic:
	 *
	 *   L--------------instrPan---------C------------------------------>R
	 *(instrumentPan = -0.4)
	 *                     |
	 *                     V
	 *   L-----------------C---notePan-------->R
	 *(notePan = +0.3)
	 *                            |
	 *                            V
	 *   L----------------------resPan---C------------------------------>R
	 *(resultantPan = -0.22)
	 *
	 * Explanation:
	 * notePan moves the RESULTANT pan in a smaller pan range centered at
	 *instrumentPan value, whose extension depends on instrPan value: if
	 *instrPan is central, notePan moves the signal in the whole pan range
	 *(really from left to right); if instrPan is sided, notePan moves the
	 *signal in a progressively smaller pan range centered at instrPan; if
	 *instrPan is HARD-sided, notePan doesn't have any effect.
	 */
	float fPan =
		pInstr->getPan() + pNote->getPan() * ( 1 - fabs( pInstr->getPan() ) );

	// Pass fPan to the Pan Law
	float fPan_L = panLaw( fPan, pSong );
	float fPan_R = panLaw( -fPan, pSong );

	// In PreFader mode of the per track output of the JACK driver we
	// disregard the instrument pan along with all other settings
	// available in the Mixer. The Note pan, however, will be used.
	float fNotePan_L = 0;
	float fNotePan_R = 0;
	if ( pHydrogen->hasJackDriver() &&
		 Preferences::get_instance()->m_JackTrackOutputMode ==
			 Preferences::JackTrackOutputMode::preFader ) {
		fNotePan_L = panLaw( pNote->getPan(), pSong );
		fNotePan_R = panLaw( -1 * pNote->getPan(), pSong );
	}
	//---------------------------------------------------------

	// In case there were already some layers selected for specific components -
	// e.g. when clicking a layer in the ComponentEditor or when using the
	// SampleEditor - we use those. If not, we will select them right here
	// according to the sample selected algorithms.
	if ( !pNote->layersAlreadySelected() ) {
		pNote->selectLayers( m_lastUsedLayersMap );

		// Note that manually selected layers bypassing this if clause are not
		// incorporated into the round robin layer selection on purpose.
		for ( const auto& [ppComponent, ppSelectedLayerInfo] :
			  pNote->getAllSelectedLayerInfos() ) {
			if ( ppComponent != nullptr ) {
				if ( ppSelectedLayerInfo != nullptr &&
					 ppSelectedLayerInfo->pLayer != nullptr ) {
					m_lastUsedLayersMap[ppComponent] =
						ppSelectedLayerInfo->pLayer;
				}
				else if ( m_lastUsedLayersMap.find( ppComponent ) !=
						  m_lastUsedLayersMap.end() ) {
					// No layer selected and the component is already present in
					// the map. We will delete its entry.
					m_lastUsedLayersMap.erase(
						m_lastUsedLayersMap.find( ppComponent )
					);
				}
			}
		}
	}

	/** We have to ensure to only send a single MIDI NOTE_ON event. Even for
	 * instruments with more than one component. */
	bool bSendMidiNoteOn = false;

	auto pComponents = pInstr->getComponents();
	auto returnValues = std::vector<bool>( pComponents->size() );

	for ( int ii = 0; ii < pComponents->size(); ++ii ) {
		returnValues[ii] = false;
	}

	for ( int ii = 0; ii < pComponents->size(); ++ii ) {
		auto pCompo = pComponents->at( ii );
		if ( pCompo == nullptr ) {
			ERRORLOG( QString( "Component [%1] is invalid" ).arg( ii ) );
			returnValues[ii] = true;
			continue;
		}

		auto pSelectedLayerInfo = pNote->getSelecterLayerInfo( pCompo );
		if ( pSelectedLayerInfo == nullptr ) {
			// Component skipped
			returnValues[ii] = true;
			continue;
		}

		// We delay checking for valid layer and sample because we want to
		// support using Hydrogen with MIDI-only output.
		auto pLayer = pSelectedLayerInfo->pLayer;

		/*
		 *  Is instrument/component/sample muted?
		 *
		 *  This can be the case either if:
		 *   - the song, instrument, component, or layer is muted
		 *   - if we're in an export session and we're doing per-instruments
		 *     exports but this instrument is not currently being exported.
		 *   - if another instrument  or component/layer of the same
		 *     instrument is soloed.
		 */
		const bool bIsMutedForExport =
			( pHydrogen->getIsExportSessionActive() &&
			  !pInstr->isCurrentlyExported() );
		const bool bAnyInstrumentIsSoloed =
			pSong->getDrumkit()->getInstruments()->isAnyInstrumentSoloed();
		const bool bAnyComponentIsSoloed = pInstr->isAnyComponentSoloed();
		bool bAnyLayerIsSoloed = false;
		bool bIsMutedBecauseOfSolo = false;
		if ( pLayer != nullptr ) {
			bAnyLayerIsSoloed = pCompo->isAnyLayerSoloed();
			bIsMutedBecauseOfSolo =
				( bAnyInstrumentIsSoloed && !pInstr->isSoloed() ||
				  bAnyComponentIsSoloed && !pCompo->getIsSoloed() ||
				  bAnyLayerIsSoloed && !pLayer->getIsSoloed() );
		}
		else {
			// This component has no valid sample. But can we still trigger a
			// NOTE_ON MIDI message corresponding to the note.
			bIsMutedBecauseOfSolo =
				( bAnyInstrumentIsSoloed && !pInstr->isSoloed() ||
				  bAnyComponentIsSoloed && !pCompo->getIsSoloed() );
		}
		const bool bLayerMuted =
			pLayer != nullptr ? pLayer->getIsMuted() : false;

		bool bIsMuted = false;
		if ( pInstr->isPreviewInstrument() ||
			 pInstr->getId() == Instrument::MetronomeId ) {
			// Metronome and preview is done for things not related to the what
			// is happening in the song and pattern editors or drumkit. But,
			// then again, it should still be possible to prevent all audio
			// output of Hydrogen. This can be done using the master mute
			// button.
			bIsMuted = pSong->getIsMuted();
		}
		else if ( bIsMutedForExport || pInstr->isMuted() ||
				  pSong->getIsMuted() || pCompo->getIsMuted() || bLayerMuted ||
				  bIsMutedBecauseOfSolo ) {
			// Regular instruments/notes.
			bIsMuted = true;
		}

		// Once the Sampler does start rendering a note we also push
		// it to all connected MIDI devices.
		if ( static_cast<int>( pSelectedLayerInfo->fSamplePosition ) == 0 &&
			 !bIsMuted ) {
			bSendMidiNoteOn = true;
		}

		// We delay checking for the audio driver till here in order to allow
		// usign Hydrogen in "MIDI-only" mode.
		if ( pLayer == nullptr || pHydrogen->getAudioDriver() == nullptr ||
			 bIsMuted ) {
			returnValues[ii] = true;
			continue;
		}

		// Actual rendering.
		returnValues[ii] = renderNote(
			pNote, pSelectedLayerInfo, nBufferSize, nInitialBufferPos,
			pCompo->getGain(), fPan_L, fPan_R, fNotePan_L, fNotePan_R, bIsMuted
		);
	}

	if ( bSendMidiNoteOn && pHydrogen->getMidiDriver() != nullptr ) {
		auto noteOnMessage = MidiMessage::from( pNote );
		noteOnMessage.setFrameOffset( nInitialBufferPos );

		if ( noteOnMessage.getChannel() != Midi::ChannelInvalid &&
			 noteOnMessage.getChannel() != Midi::ChannelOff ) {
			// Due to historical reasons Hydrogen is sending a MIDI NOTE_OFF
			// messages right before a NOTE_ON one.
			if ( pInstr->isStopNotes() &&
				 ( Preferences::get_instance()->getMidiSendNoteOff() ==
					   Preferences::MidiSendNoteOff::Always ||
				   ( Preferences::get_instance()->getMidiSendNoteOff() ==
						 Preferences::MidiSendNoteOff::OnCustomLengths &&
					 pNote->getLength() != LENGTH_ENTIRE_SAMPLE ) ) ) {
				auto noteOffMessage = MidiMessage::from( noteOnMessage );
				noteOffMessage.setType( MidiMessage::Type::NoteOff );
				pHydrogen->getMidiDriver()->enqueueOutputMessage( noteOffMessage
				);
			}
			pHydrogen->getMidiDriver()->enqueueOutputMessage( noteOnMessage );
			pNote->setMidiNoteOnSentFrame( nFrame );
		}
	}

	for ( const auto& bReturnValue : returnValues ) {
		if ( !bReturnValue ) {
			return false;
		}
	}
	return true;
}

/// Copy sample data to buffer, filling buffer with trailing silence at end of
/// sample data.
void copySample(
	float* __restrict__ pBuffer_L,
	float* __restrict__ pBuffer_R,
	float* __restrict__ pSample_data_L,
	float* __restrict__ pSample_data_R,
	int nFrames,
	double fSamplePos,
	int nSampleFrames
)
{
	int nSamplePos = static_cast<int>( fSamplePos );
	int nFramesFromSample = std::min( nFrames, nSampleFrames - nSamplePos );

	memcpy(
		pBuffer_L, &pSample_data_L[nSamplePos],
		nFramesFromSample * sizeof( float )
	);
	memcpy(
		pBuffer_R, &pSample_data_R[nSamplePos],
		nFramesFromSample * sizeof( float )
	);

	if ( nFramesFromSample < nFrames ) {
		memset(
			&pBuffer_L[nFramesFromSample], '0',
			( nFrames - nFramesFromSample ) * sizeof( float )
		);
		memset(
			&pBuffer_R[nFramesFromSample], '0',
			( nFrames - nFramesFromSample ) * sizeof( float )
		);
	}
}

/// Interpolate stereo samples into audio buffer of different frame rate.
///
/// Acquiring the frames to interpolate from the input sample data in a safe
/// manner is surprisingly costly since up to 4 input frames must be fetched
/// for each output frame, and each must be bounds-checked.
///
/// To handle this efficiently, we define a "safe" frame acquisition method
/// with bounds checking on each frame and providing a silent frame outside
/// the sample data boundaries, as well as a "fast" path which assumes
/// it can read all the necessary samples without bounds checking or flow
/// control.
///
/// The output frames are partitioned into three ranges, corresponding to the
/// beginning, "middle" and end of the input sample data, so that the fast
/// method can be used for the majority of the sample, and the safe method for
/// the beginning and ends.
///
/// Although not all input frames are needed for each interpolation method
/// (linear requires only two), the interpolation mode is a constant parameter
/// and so the compiler wil remove accesses and some unnecessary bounds
/// checking where it's not needed, without having to hand-write
/// specialisations for each.
///
template <Interpolation::InterpolateMode mode>
void resample(
	float* __restrict__ pBuffer_L,
	float* __restrict__ pBuffer_R,
	float* __restrict__ pSample_data_L,
	float* __restrict__ pSample_data_R,
	int nFrames,
	double& fSamplePos,
	float fStep,
	int nSampleFrames
)
{
	auto getSampleFrames = [&]( int nSamplePos, float& l0, float& l1, float& l2,
								float& l3, float& r0, float& r1, float& r2,
								float& r3 ) {
		l0 = l1 = l2 = l3 = r0 = r1 = r2 = r3 = 0.0;
		// Some required frames are off the beginning or end of the sample.
		if ( nSamplePos >= 1 && nSamplePos < nSampleFrames + 1 ) {
			l0 = pSample_data_L[nSamplePos - 1];
			r0 = pSample_data_R[nSamplePos - 1];
		}
		// Each successive frame may be past the end of the sample so check
		// individually.
		if ( nSamplePos < nSampleFrames ) {
			l1 = pSample_data_L[nSamplePos];
			r1 = pSample_data_R[nSamplePos];
			if ( nSamplePos + 1 < nSampleFrames ) {
				l2 = pSample_data_L[nSamplePos + 1];
				r2 = pSample_data_R[nSamplePos + 1];
				if ( nSamplePos + 2 < nSampleFrames ) {
					l3 = pSample_data_L[nSamplePos + 2];
					r3 = pSample_data_R[nSamplePos + 2];
				}
			}
		}
	};

	float fVal_L, fVal_R;
	int nFrame;
	float l0, l1, l2, l3, r0, r1, r2, r3;

	// Initial safe iterations to avoid reading off the beginning of the sample
	for ( nFrame = 0; nFrame < nFrames; nFrame++ ) {
		int nSamplePos = static_cast<int>( fSamplePos );
		if ( nSamplePos >= 1 ) {
			break;
		}
		double fDiff = fSamplePos - nSamplePos;
		getSampleFrames( 0, l0, l1, l2, l3, r0, r1, r2, r3 );

		fVal_L = Interpolation::interpolate<mode>( l0, l1, l2, l3, fDiff );
		fVal_R = Interpolation::interpolate<mode>( r0, r1, r2, r3, fDiff );
		pBuffer_L[nFrame] = fVal_L;
		pBuffer_R[nFrame] = fVal_R;
		fSamplePos += fStep;
	}

	// Fast iterations for main body of sample, with unconditional sample lookup
	int nFastFrames = std::min(
		nFrames, static_cast<int>( ( nSampleFrames - 2 - fSamplePos ) / fStep )
	);
	for ( ; nFrame < nFastFrames; nFrame++ ) {
		int nSamplePos = static_cast<int>( fSamplePos );
		double fDiff = fSamplePos - nSamplePos;
		// Gather frame samples
		l0 = pSample_data_L[nSamplePos - 1];
		l1 = pSample_data_L[nSamplePos];
		l2 = pSample_data_L[nSamplePos + 1];
		l3 = pSample_data_L[nSamplePos + 2];
		r0 = pSample_data_R[nSamplePos - 1];
		r1 = pSample_data_R[nSamplePos];
		r2 = pSample_data_R[nSamplePos + 1];
		r3 = pSample_data_R[nSamplePos + 2];
		fVal_L = Interpolation::interpolate<mode>( l0, l1, l2, l3, fDiff );
		fVal_R = Interpolation::interpolate<mode>( r0, r1, r2, r3, fDiff );
		pBuffer_L[nFrame] = fVal_L;
		pBuffer_R[nFrame] = fVal_R;
		fSamplePos += fStep;
	}

	for ( ; nFrame < nFrames; nFrame++ ) {
		int nSamplePos = static_cast<int>( fSamplePos );
		double fDiff = fSamplePos - nSamplePos;
		getSampleFrames( nSamplePos, l0, l1, l2, l3, r0, r1, r2, r3 );
		fVal_L = Interpolation::interpolate<mode>( l0, l1, l2, l3, fDiff );
		fVal_R = Interpolation::interpolate<mode>( r0, r1, r2, r3, fDiff );
		pBuffer_L[nFrame] = fVal_L;
		pBuffer_R[nFrame] = fVal_R;
		fSamplePos += fStep;
	}
}

/// Resample with runtime-selection of interpolation mode
void resample(
	Interpolation::InterpolateMode mode,
	float* __restrict__ pBuffer_L,
	float* __restrict__ pBuffer_R,
	float* __restrict__ pSample_data_L,
	float* __restrict__ pSample_data_R,
	int nFrames,
	double& fSamplePos,
	float fStep,
	int nSampleFrames
)
{
	switch ( mode ) {
		case Interpolation::InterpolateMode::Linear:
			resample<Interpolation::InterpolateMode::Linear>(
				pBuffer_L, pBuffer_R, pSample_data_L, pSample_data_R, nFrames,
				fSamplePos, fStep, nSampleFrames
			);
			break;
		case Interpolation::InterpolateMode::Cosine:
			resample<Interpolation::InterpolateMode::Cosine>(
				pBuffer_L, pBuffer_R, pSample_data_L, pSample_data_R, nFrames,
				fSamplePos, fStep, nSampleFrames
			);
			break;
		case Interpolation::InterpolateMode::Third:
			resample<Interpolation::InterpolateMode::Third>(
				pBuffer_L, pBuffer_R, pSample_data_L, pSample_data_R, nFrames,
				fSamplePos, fStep, nSampleFrames
			);
			break;
		case Interpolation::InterpolateMode::Cubic:
			resample<Interpolation::InterpolateMode::Cubic>(
				pBuffer_L, pBuffer_R, pSample_data_L, pSample_data_R, nFrames,
				fSamplePos, fStep, nSampleFrames
			);
			break;
		case Interpolation::InterpolateMode::Hermite:
			resample<Interpolation::InterpolateMode::Hermite>(
				pBuffer_L, pBuffer_R, pSample_data_L, pSample_data_R, nFrames,
				fSamplePos, fStep, nSampleFrames
			);
			break;
	}
}

bool Sampler::processPlaybackTrack( int nBufferSize )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	auto pAudioDriver = pHydrogen->getAudioDriver();
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

	const auto pCompo = m_pPlaybackTrackInstrument->getComponents()->front();
	if ( pCompo == nullptr || pCompo->getLayer( 0 ) == nullptr ||
		 pCompo->getLayer( 0 )->getSample() == nullptr ) {
		ERRORLOG( "Invalid playback instrument" );
		EventQueue::get_instance()->pushEvent(
			Event::Type::Error, Hydrogen::ErrorMessages::PLAYBACK_TRACK_INVALID
		);
		// Disable the playback track
		pHydrogen->loadPlaybackTrack( "" );
		reinitializePlaybackTrack();
		return true;
	}

	auto pSample = pCompo->getLayer( 0 )->getSample();

	auto pSample_data_L = pSample->getData_L();
	auto pSample_data_R = pSample->getData_R();

	int nAvail_bytes = 0;
	int nInitialBufferPos = 0;

	const long long nFrame = pAudioEngine->getTransportPosition()->getFrame() -
							 pAudioEngine->getLastLoopFrame();
	const long long nFrameOffset =
		pAudioEngine->getTransportPosition()->getFrameOffsetTempo();

	int nSampleFrames = pSample->getFrames();
	float fStep =
		(float) pSample->getSampleRate() /
		pAudioDriver->getSampleRate();	// Adjust for audio driver sample rate
	double fSamplePos = ( nFrame - nFrameOffset ) * fStep;

	nAvail_bytes = std::min(
		(int) ( (float) ( pSample->getFrames() - fSamplePos ) / fStep ),
		nBufferSize
	);

	int nFinalBufferPos = nInitialBufferPos + nAvail_bytes;
	if ( nFinalBufferPos < 0 ) {
		// end of sample reached.
		return true;
	}

	// Output-rate buffer as temporary storage for sample data, resampled to
	// output rate
	float buffer_L[nBufferSize];
	float buffer_R[nBufferSize];

#ifdef H2CORE_HAVE_JACK
	float* pTrackOutL = nullptr;
	float* pTrackOutR = nullptr;

	if ( Preferences::get_instance()->m_bJackTrackOuts ) {
		auto pJackDriver = std::dynamic_pointer_cast<JackDriver>( pAudioDriver );
		if ( pJackDriver != nullptr ) {
			pTrackOutL = pJackDriver->getTrackBuffer(
				m_pPlaybackTrackInstrument, JackDriver::Channel::Left
			);
			pTrackOutR = pJackDriver->getTrackBuffer(
				m_pPlaybackTrackInstrument, JackDriver::Channel::Right
			);
		}
	}
#endif

	if ( pSample->getSampleRate() == pAudioDriver->getSampleRate() ) {
		copySample(
			&buffer_L[nInitialBufferPos], &buffer_R[nInitialBufferPos],
			pSample_data_L, pSample_data_R, nBufferSize, fSamplePos,
			nSampleFrames
		);
	}
	else {
		resample(
			m_interpolateMode, &buffer_L[nInitialBufferPos],
			&buffer_R[nInitialBufferPos], pSample_data_L, pSample_data_R,
			nBufferSize, fSamplePos, fStep, nSampleFrames
		);
	}

	// Track peaks and mix in to main output
	float fInstrPeak_L = m_pPlaybackTrackInstrument->getPeak_L();
	float fInstrPeak_R = m_pPlaybackTrackInstrument->getPeak_R();

	const float fGain = pSong->getPlaybackTrackVolume() * pSong->getVolume();
	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nFinalBufferPos;
		  ++nBufferPos ) {
		float fVal_L = buffer_L[nBufferPos] * fGain,
			  fVal_R = buffer_R[nBufferPos] * fGain;

#ifdef H2CORE_HAVE_JACK
		if ( pTrackOutL ) {
			pTrackOutL[nBufferPos] += fVal_L;
		}
		if ( pTrackOutR ) {
			pTrackOutR[nBufferPos] += fVal_R;
		}
#endif

		fInstrPeak_L = std::max( fInstrPeak_L, buffer_L[nBufferPos] );
		fInstrPeak_R = std::max( fInstrPeak_R, buffer_R[nBufferPos] );
		m_pMainOut_L[nBufferPos] += fVal_L;
		m_pMainOut_R[nBufferPos] += fVal_R;
	}

	m_pPlaybackTrackInstrument->setPeak_L( fInstrPeak_L );
	m_pPlaybackTrackInstrument->setPeak_R( fInstrPeak_R );

	return true;
}

bool Sampler::renderNote(
	std::shared_ptr<Note> pNote,
	std::shared_ptr<SelectedLayerInfo> pSelectedLayerInfo,
	int nBufferSize,
	int nInitialBufferPos,
    float fComponentGain,
    float fPan_L,
    float fPan_R,
    float fNotePan_L,
    float fNotePan_R,
	bool bIsMuted
)
{
	if ( pSelectedLayerInfo == nullptr ) {
		ERRORLOG( "Invalid input" );
		return true;
	}
	const auto pLayer = pSelectedLayerInfo->pLayer;
	if ( pLayer == nullptr ) {
		ERRORLOG( "Invalid input layer" );
        return true;
	}
	const auto pSample = pLayer->getSample();
	if ( pSample == nullptr ) {
		ERRORLOG( "Invalid input sample" );
		return true;
    }

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioDriver = pHydrogen->getAudioDriver();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pAudioDriver == nullptr ) {
		return true;
	}

	auto pInstrument = pNote->getInstrument();
	if ( pInstrument == nullptr || pNote->getAdsr() == nullptr ) {
		ERRORLOG( "Invalid note instrument" );
		return true;
	}
	if ( !pSample->isLoaded() ) {
		WARNINGLOG( QString( "Sample [%1] of instrument [%2] was not loaded." )
						.arg( pSample->getFilePath() )
						.arg( pInstrument->getName() ) );
		return true;
	}

	if ( pSelectedLayerInfo->fSamplePosition >= pSample->getFrames() ) {
		// Due to rounding errors in renderNote() the
		// sample position can occassionaly exceed the maximum
		// frames of a sample. AFAICS this is not itself
		// harmful. So, we just log a warning if the difference is
		// larger, which might be caused by a different problem.
		if ( pSelectedLayerInfo->fSamplePosition >= pSample->getFrames() + 3 ) {
			WARNINGLOG(
				QString( "sample position [%1] out of bounds [0,%2]. The "
						 "layer has been resized during note play?" )
					.arg( pSelectedLayerInfo->fSamplePosition )
					.arg( pSample->getFrames() )
			);
		}
        return true;
	}

	const float fLayerGain = pLayer->getGain();
	const float fLayerPitch = pLayer->getPitchOffset();

	float fGainTrack_L = 1.0f;
	float fGainTrack_R = 1.0f;
	float fGainJackTrack_L = 1.0f;
	float fGainJackTrack_R = 1.0f;

	if ( bIsMuted ) {
		fGainTrack_L = 0.0;
		fGainTrack_R = 0.0;
		if ( Preferences::get_instance()->m_JackTrackOutputMode ==
			 Preferences::JackTrackOutputMode::postFader ) {
			fGainJackTrack_L = 0.0;
			fGainJackTrack_R = 0.0;
		}
	}
	else {
		float fMonoGain = 1.0;
		if ( pInstrument->getApplyVelocity() ) {
			fMonoGain *= pNote->getVelocity();
		}

		fMonoGain *= fLayerGain;
		fMonoGain *= pInstrument->getGain();
		fMonoGain *= fComponentGain;
		fMonoGain *= pInstrument->getVolume();

		fGainTrack_L = fMonoGain * fPan_L;
		fGainTrack_R = fMonoGain * fPan_R;
		if ( Preferences::get_instance()->m_JackTrackOutputMode ==
			 Preferences::JackTrackOutputMode::postFader ) {
			fGainJackTrack_R = fGainTrack_R * 2;
			fGainJackTrack_L = fGainTrack_L * 2;
		}
	}

	// direct track outputs only use velocity
	if ( Preferences::get_instance()->m_JackTrackOutputMode ==
		 Preferences::JackTrackOutputMode::preFader ) {
		if ( pInstrument->getApplyVelocity() ) {
			fGainJackTrack_L *= pNote->getVelocity();
		}
		fGainJackTrack_L *= fLayerGain;
		fGainJackTrack_L *= fComponentGain;

		fGainJackTrack_R = fGainJackTrack_L;

		fGainJackTrack_L *= fNotePan_L;
		fGainJackTrack_R *= fNotePan_R;
	}

	const auto pitch = Note::Pitch::fromFloatClamp(
		static_cast<float>( pNote->toPitch() ) + pNote->getPitchHumanization() +
		pInstrument->getPitchOffset() + fLayerPitch
	);
	const bool bResample =
		pitch != Note::Pitch::Default ||
		pSample->getSampleRate() != pAudioDriver->getSampleRate();

	// Ratio of target to source frequency.
	float fFrequencyRatio = 1.0;
	if ( bResample ) {
		fFrequencyRatio = pitch.toFrequencyRatio();

		// Adjust for audio driver sample rate
		fFrequencyRatio *= static_cast<float>( pSample->getSampleRate() ) /
						   static_cast<float>( pAudioDriver->getSampleRate() );
	}

	auto pSample_data_L = pSample->getData_L();
	auto pSample_data_R = pSample->getData_R();
	const int nSampleFrames = pSample->getFrames();
	// The number of frames of the sample left to process.
	const int nRemainingFrames = static_cast<int>(
		( static_cast<float>( nSampleFrames ) -
		  pSelectedLayerInfo->fSamplePosition ) /
		fFrequencyRatio
	);

	bool bRetValue = true;	// the note is ended
	int nAvail_bytes;
	if ( nRemainingFrames > nBufferSize - nInitialBufferPos ) {
		// It The number of frames of the sample left to process
		// exceeds what's available in the current process cycle of
		// the Sampler. Clip it.
		nAvail_bytes = nBufferSize - nInitialBufferPos;
		// the note is not ended yet
		bRetValue = false;
	}
	else if ( pInstrument->isFilterActive() && pNote->filterSustain() ) {
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
	if ( pNote->getLength() != LENGTH_ENTIRE_SAMPLE &&
		 pNote->getAdsr()->getState() != ADSR::State::Release ) {
		if ( pSelectedLayerInfo->nNoteLength == LENGTH_ENTIRE_SAMPLE ) {
			// The length of a note is only calculated once when first
			// encountering it. This makes us robust again glitches due to tempo
			// changes.
			double fTickMismatch;

			pSelectedLayerInfo->nNoteLength =
				(TransportPosition::computeFrameFromTick(
					pNote->getPosition() + pNote->getLength(), &fTickMismatch,
					pSample->getSampleRate()
				) -
				TransportPosition::computeFrameFromTick(
					pNote->getPosition(), &fTickMismatch,
					pSample->getSampleRate()
				)) * fFrequencyRatio;
		}

		nNoteEnd = std::min(
			nFinalBufferPos + 1,
            static_cast<int>((pSelectedLayerInfo->nNoteLength -
				  static_cast<int>(pSelectedLayerInfo->fSamplePosition)) / fFrequencyRatio
			)
		);

		if ( nNoteEnd < 0 ) {
			if ( !pInstrument->isFilterActive() ) {
				// In case resonance filtering is active the sampler stops
				// rendering of the sample at the custom note length but lets
				// the filter itself ring on.
				ERRORLOG( QString( "Note end located within the previous "
								   "processing cycle. nNoteEnd: %1, "
								   "nNoteLength: %2, fSamplePosition: %3, "
								   "nFinalBufferPos: %4, fFrequencyRatio: %5" )
							  .arg( nNoteEnd )
							  .arg( pSelectedLayerInfo->nNoteLength )
							  .arg( pSelectedLayerInfo->fSamplePosition )
							  .arg( nFinalBufferPos )
							  .arg( fFrequencyRatio ) );
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

	auto pADSR = pNote->getAdsr();
	float fVal_L;
	float fVal_R;

#ifdef H2CORE_HAVE_JACK
	float* pTrackOutL = nullptr;
	float* pTrackOutR = nullptr;

	if ( Preferences::get_instance()->m_bJackTrackOuts ) {
		auto pJackDriver = std::dynamic_pointer_cast<JackDriver>( pAudioDriver );
		if ( pJackDriver != nullptr ) {
			pTrackOutL = pJackDriver->getTrackBuffer(
				pInstrument, JackDriver::Channel::Left
			);
			pTrackOutR = pJackDriver->getTrackBuffer(
				pInstrument, JackDriver::Channel::Right
			);
		}
	}
#endif

	float buffer_L[nBufferSize];
	float buffer_R[nBufferSize];

	if ( bResample ) {
		resample(
			m_interpolateMode, &buffer_L[nInitialBufferPos],
			&buffer_R[nInitialBufferPos], pSample_data_L, pSample_data_R,
			nFinalBufferPos - nInitialBufferPos, fSamplePos, fFrequencyRatio,
			nSampleFrames
		);
	}
	else {
		copySample(
			&buffer_L[nInitialBufferPos], &buffer_R[nInitialBufferPos],
			pSample_data_L, pSample_data_R, nFinalBufferPos - nInitialBufferPos,
			fSamplePos, nSampleFrames
		);
	}

	if ( pADSR->applyADSR(
			 buffer_L, buffer_R, nFinalBufferPos, nNoteEnd, fFrequencyRatio
		 ) ) {
		bRetValue = true;
	}

	float fSamplePeak_L = 0.0, fSamplePeak_R = 0.0;
	// Only process audio in case we do actually plan to use it.
	if ( !bIsMuted ) {
		// Low pass resonant filter
		if ( pInstrument->isFilterActive() ) {
			for ( int nBufferPos = nInitialBufferPos;
				  nBufferPos < nFinalBufferPos; ++nBufferPos ) {
				fVal_L = buffer_L[nBufferPos];
				fVal_R = buffer_R[nBufferPos];

				pNote->computeLrValues( &fVal_L, &fVal_R );

				buffer_L[nBufferPos] = fVal_L;
				buffer_R[nBufferPos] = fVal_R;
			}
		}

		const auto fMainVolume = pSong->getVolume();
		// Mix rendered sample buffer to track and mixer output
		for ( int nBufferPos = nInitialBufferPos; nBufferPos < nFinalBufferPos;
			  ++nBufferPos ) {
			fVal_L = buffer_L[nBufferPos];
			fVal_R = buffer_R[nBufferPos];

#ifdef H2CORE_HAVE_JACK
			if ( pTrackOutL != nullptr ) {
				pTrackOutL[nBufferPos] += fVal_L * fGainJackTrack_L;
			}
			if ( pTrackOutR != nullptr ) {
				pTrackOutR[nBufferPos] += fVal_R * fGainJackTrack_R;
			}
#endif

			// The meters and peaks of a instrument strip in the mixer do
			// represent post-fader values and include the overall track gain.
			fVal_L *= fGainTrack_L;
			fVal_R *= fGainTrack_R;

			fSamplePeak_L = std::max( fSamplePeak_L, fVal_L );
			fSamplePeak_R = std::max( fSamplePeak_R, fVal_R );

			// But the peaks are _not_ affected by the main volume.
			fVal_L *= fMainVolume;
			fVal_R *= fMainVolume;

			// to main mix
			m_pMainOut_L[nBufferPos] += fVal_L;
			m_pMainOut_R[nBufferPos] += fVal_R;
		}
	}

	// update instr peak
	pInstrument->setPeak_L( std::max( pInstrument->getPeak_L(), fSamplePeak_L )
	);
	pInstrument->setPeak_R( std::max( pInstrument->getPeak_R(), fSamplePeak_R )
	);

	if ( pInstrument->isFilterActive() && pNote->filterSustain() ) {
		// Note is still ringing, do not end.
		bRetValue = false;
	}

	pSelectedLayerInfo->fSamplePosition += nAvail_bytes * fFrequencyRatio;

#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
	// change the below return logic if you add code after that ifdef
	if ( bIsMuted ) {
		return bRetValue;
	}
	float masterVol = pSong->getVolume();
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		auto pFX = Effects::get_instance()->getLadspaFX( nFX );
		float fLevel = pInstrument->getFxLevel( nFX );
		if ( pFX != nullptr && fLevel != 0.0 ) {
			fLevel = fLevel * pFX->getVolume();

			float* pBuf_L = pFX->m_pBuffer_L;
			float* pBuf_R = pFX->m_pBuffer_R;

			float fFXCost_L = fLevel * masterVol;
			float fFXCost_R = fLevel * masterVol;

			int nBufferPos = nInitialBufferPos;
			for ( int i = 0; i < nAvail_bytes; ++i ) {
				fVal_L = buffer_L[nBufferPos];
				fVal_R = buffer_R[nBufferPos];

				pBuf_L[nBufferPos] += fVal_L * fFXCost_L;
				pBuf_R[nBufferPos] += fVal_R * fFXCost_R;
				++nBufferPos;
			}
		}
	}
#endif
	return bRetValue;
}

void Sampler::stopPlayingNotes( std::shared_ptr<Instrument> pInstr )
{
	if ( pInstr != nullptr ) {	// stop all notes using this instrument
		for ( unsigned i = 0; i < m_playingNotesQueue.size(); ) {
			auto pNote = m_playingNotesQueue[i];
			assert( pNote );
			if ( pNote != nullptr && pNote->getInstrument() == pInstr ) {
				pInstr->dequeue( pNote );
				m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
			}
			++i;
		}
	}
	else {	// stop all notes
		// delete all copied notes in the playing notes queue
		for ( unsigned i = 0; i < m_playingNotesQueue.size(); ++i ) {
			auto pNote = m_playingNotesQueue[i];
			if ( pNote != nullptr && pNote->getInstrument() != nullptr ) {
				pNote->getInstrument()->dequeue( pNote );
			}
		}
		m_playingNotesQueue.clear();
	}
}

void Sampler::releasePlayingNotes( std::shared_ptr<Instrument> pInstr )
{
	for ( auto ppNote : m_playingNotesQueue ) {
		if ( ppNote == nullptr || ppNote->getInstrument() == nullptr ||
			 ( pInstr == nullptr ||
			   ( pInstr != nullptr && pInstr == ppNote->getInstrument() ) ) ) {
			ppNote->getAdsr()->release();
		}
	}
}

void Sampler::previewInstrument(
	std::shared_ptr<Instrument> pInstr,
	std::shared_ptr<Note> pNote
)
{
	if ( pInstr == nullptr || pNote == nullptr ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	if ( !pInstr->hasSamples() ) {
		ERRORLOG( "Provided instrument not meant for playback" )
		return;
	}

	if ( pNote->getInstrument() == nullptr ||
		 pNote->getInstrument() != pInstr ) {
		ERRORLOG( "Provided note not associated with the provided instrument" );
		return;
	}

	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

	stopPlayingNotes( m_pPreviewInstrument );

	m_pPreviewInstrument = pInstr;
	pInstr->setIsPreviewInstrument( true );
	noteOn( pNote );

	Hydrogen::get_instance()->getAudioEngine()->unlock();
}

void Sampler::previewSample( std::shared_ptr<Sample> pSample, int nLength )
{
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

	stopPlayingNotes( m_pPreviewInstrument );

	if ( m_pPreviewInstrument != m_pDefaultPreviewInstrument ) {
		m_pPreviewInstrument = m_pDefaultPreviewInstrument;
	}

	// The default preview instrument has a single component with a single
	// layer. This is where we assign the new sample to.
	if ( m_pPreviewInstrument == nullptr ||
		 m_pPreviewInstrument->getComponent( 0 ) == nullptr ||
		 m_pPreviewInstrument->getComponent( 0 )->getLayer( 0 ) == nullptr ) {
		Hydrogen::get_instance()->getAudioEngine()->unlock();
		ERRORLOG( "Invalid preview instrument" );
		return;
	}

	const auto pComponent = m_pPreviewInstrument->getComponent( 0 );
	const auto pLayer = pComponent->getLayer( 0 );

	m_pPreviewInstrument->setSample(
		pComponent, pLayer, pSample, Event::Trigger::Suppress
	);

	auto pPreviewNote = std::make_shared<Note>(
		m_pPreviewInstrument, 0, VELOCITY_MAX, PAN_DEFAULT, nLength
	);

	noteOn( pPreviewNote );

	Hydrogen::get_instance()->getAudioEngine()->unlock();
}

bool Sampler::isInstrumentPlaying( std::shared_ptr<Instrument> pInstrument
) const
{
	if ( pInstrument != nullptr ) {	 // stop all notes using this instrument
		for ( unsigned j = 0; j < m_playingNotesQueue.size(); j++ ) {
			if ( m_playingNotesQueue[j]->getInstrument() != nullptr &&
				 pInstrument->getName() ==
					 m_playingNotesQueue[j]->getInstrument()->getName() ) {
				return true;
			}
		}
	}
	return false;
}

void Sampler::reinitializePlaybackTrack()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	std::shared_ptr<Sample> pSample;

	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}

	if ( pHydrogen->getPlaybackTrackState() !=
		 Song::PlaybackTrack::Unavailable ) {
		pSample = Sample::load( pSong->getPlaybackTrackFileName() );
	}

	auto pPlaybackTrackLayer = std::make_shared<InstrumentLayer>( pSample );

	m_pPlaybackTrackInstrument->setLayer(
		m_pPlaybackTrackInstrument->getComponents()->front(),
		pPlaybackTrackLayer, 0, Event::Trigger::Suppress
	);
	m_nPlayBackSamplePosition = 0;
}

QString Sampler::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput = QString( "%1[Sampler]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2m_playingNotesQueue: [\n" )
								   .arg( sPrefix )
								   .arg( s ) );
		for ( const auto& ppNote : m_playingNotesQueue ) {
			sOutput.append( ppNote->toQString( sPrefix + s, bShort ) );
		}
		sOutput.append(
			QString( "]\n%1%2m_queuedNoteOffs: [\n" ).arg( sPrefix ).arg( s )
		);
		for ( const auto& ppNote : m_queuedNoteOffs ) {
			sOutput.append( ppNote->toQString( sPrefix + s, bShort ) );
		}
		sOutput
			.append( QString( "]\n%1%2m_scheduledNoteOffQueue: size = %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_scheduledNoteOffQueue.size() ) )
			.append( QString( "]\n%1%2m_pPlaybackTrackInstrument: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg(
							 m_pPlaybackTrackInstrument == nullptr
								 ? "nullptr"
								 : m_pPlaybackTrackInstrument->toQString(
									   sPrefix + s, bShort
								   )
						 ) )
			.append( QString( "%1%2m_pPreviewInstrument: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg(
							 m_pPreviewInstrument == nullptr
								 ? "nullptr"
								 : m_pPreviewInstrument->toQString(
									   sPrefix + s, bShort
								   )
						 ) )
			.append( QString( "%1%2m_nPlayBackSamplePosition: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nPlayBackSamplePosition ) )
			.append( QString( "%1%2m_interpolateMode: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( Interpolation::ModeToQString( m_interpolateMode )
						 ) );
	}
	else {
		sOutput = QString( "[Sampler] " ).append( "m_playingNotesQueue: [" );
		for ( const auto& ppNote : m_playingNotesQueue ) {
			sOutput.append( QString( "[%1] " ).arg( ppNote->prettyName() ) );
		}
		sOutput.append( QString( "], m_queuedNoteOffs: [" ) );
		for ( const auto& ppNote : m_queuedNoteOffs ) {
			sOutput.append( QString( "[%1] " ).arg( ppNote->prettyName() ) );
		}
		sOutput
			.append( QString( "], m_scheduledNoteOffQueue: size = %1" )
						 .arg( m_scheduledNoteOffQueue.size() ) )
			.append( QString( "], m_pPlaybackTrackInstrument: %1" )
						 .arg(
							 m_pPlaybackTrackInstrument == nullptr
								 ? "nullptr"
								 : m_pPlaybackTrackInstrument->toQString(
									   "", bShort
								   )
						 ) )
			.append( QString( ", m_pPreviewInstrument: %1" )
						 .arg(
							 m_pPreviewInstrument == nullptr
								 ? "nullptr"
								 : m_pPreviewInstrument->toQString( "", bShort )
						 ) )
			.append( QString( ", m_nPlayBackSamplePosition: %1" )
						 .arg( m_nPlayBackSamplePosition ) )
			.append( QString( ", m_interpolateMode: %1" )
						 .arg( Interpolation::ModeToQString( m_interpolateMode )
						 ) );
	}

	return sOutput;
}
};	// namespace H2Core
