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

#include <cassert>
#include <cmath>
#include <cstdlib>

#include <core/IO/AudioOutput.h>
#include <core/IO/JackAudioDriver.h>

#include <core/Basics/Adsr.h>
#include <core/AudioEngine.h>
#include <core/Globals.h>
#include <core/Hydrogen.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Note.h>
#include <core/Preferences.h>
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

const char* Sampler::__class_name = "Sampler";

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
		: Object( __class_name )
		, m_pMainOut_L( nullptr )
		, m_pMainOut_R( nullptr )
		, m_pPreviewInstrument( nullptr )
		, m_interpolateMode( Interpolation::InterpolateMode::Linear )
{
	INFOLOG( "INIT" );
	
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

/** set default k for pan law with -4.5dB center compensation, given L^k + R^k = const
 * it is the mean compromise between constant sum and constant power
 */
float const Sampler::K_NORM_DEFAULT = 1.33333333333333;

void Sampler::process( uint32_t nFrames, Song* pSong )
{
	//infoLog( "[process]" );
	AudioOutput* pAudioOutpout = Hydrogen::get_instance()->getAudioOutput();
	assert( pAudioOutpout );

	memset( m_pMainOut_L, 0, nFrames * sizeof( float ) );
	memset( m_pMainOut_R, 0, nFrames * sizeof( float ) );

	// Track output queues are zeroed by
	// audioEngine_process_clearAudioBuffers()

	// Max notes limit
	int m_nMaxNotes = Preferences::get_instance()->m_nMaxNotes;
	while ( ( int )m_playingNotesQueue.size() > m_nMaxNotes ) {
		Note * pOldNote = m_playingNotesQueue[ 0 ];
		m_playingNotesQueue.erase( m_playingNotesQueue.begin() );
		 pOldNote->get_instrument()->dequeue();
		delete  pOldNote;	// FIXME: send note-off instead of removing the note from the list?
	}

	for ( auto& pComponent : *pSong->getComponents() ) {
		pComponent->reset_outs(nFrames);
	}

	// eseguo tutte le note nella lista di note in esecuzione
	unsigned i = 0;
	Note* pNote;
	while ( i < m_playingNotesQueue.size() ) {
		pNote = m_playingNotesQueue[ i ];		// recupero una nuova nota
		if ( renderNote( pNote, nFrames, pSong ) ) {	// la nota e' finita
			m_playingNotesQueue.erase( m_playingNotesQueue.begin() + i );
			pNote->get_instrument()->dequeue();
			m_queuedNoteOffs.push_back( pNote );
		} else {
			++i; // carico la prox nota
		}
	}

	//Queue midi note off messages for notes that have a length specified for them
	while ( !m_queuedNoteOffs.empty() ) {
		pNote =  m_queuedNoteOffs[0];
		MidiOutput* pMidiOut = Hydrogen::get_instance()->getMidiOutput();
		
		if( pMidiOut != nullptr && !pNote->get_instrument()->is_muted() ){
			pMidiOut->handleQueueNoteOff(	pNote->get_instrument()->get_midi_out_channel(), 
											pNote->get_midi_key(),
											pNote->get_midi_velocity() );
		}
		
		m_queuedNoteOffs.erase( m_queuedNoteOffs.begin() );
		
		if( pNote != nullptr ){
			delete pNote;
		}
		
		pNote = nullptr;
	}//while

	processPlaybackTrack(nFrames);
}



void Sampler::noteOn(Note *pNote )
{
	//infoLog( "[noteOn]" );
	assert( pNote );

	pNote->get_adsr()->attack();
	auto pInstr = pNote->get_instrument();

	// mute group
	int nMuteGrp = pInstr->get_mute_group();
	if ( nMuteGrp != -1 ) {
		// remove all notes using the same mute group
		for ( const auto& pNote: m_playingNotesQueue ) {	// delete older note
			if ( ( pNote->get_instrument() != pInstr )  && ( pNote->get_instrument()->get_mute_group() == nMuteGrp ) ) {
				pNote->get_adsr()->release();
			}
		}
	}

	//note off notes
	if( pNote->get_note_off() ){
		for ( const auto& pNote: m_playingNotesQueue ) {
			if ( ( pNote->get_instrument() == pInstr ) ) {
				//ERRORLOG("note_off");
				pNote->get_adsr()->release();
			}
		}
	}

	pInstr->enqueue();
	if( !pNote->get_note_off() ){
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
inline float Sampler::panLaw( float fPan, Song* pSong ) {
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

//------------------------------------------------------------------

/// Render a note
/// Return false: the note is not ended
/// Return true: the note is ended
bool Sampler::renderNote( Note* pNote, unsigned nBufferSize, Song* pSong )
{
	//infoLog( "[renderNote] instr: " + pNote->getInstrument()->m_sName );
	assert( pSong );

	unsigned int nFramepos;
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	AudioOutput* pAudioOutput = pHydrogen->getAudioOutput();
	if ( pHydrogen->getState() == STATE_PLAYING ) {
		nFramepos = pAudioOutput->m_transport.m_nFrames;
	} else {
		// use this to support realtime events when not playing
		nFramepos = pHydrogen->getRealtimeFrames();
	}

	auto pInstr = pNote->get_instrument();
	if ( !pInstr ) {
		ERRORLOG( "NULL instrument" );
		return 1;
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
	//---------------------------------------------------------

	bool nReturnValues [pInstr->get_components()->size()];
	
	for(int i = 0; i < pInstr->get_components()->size(); i++){
		nReturnValues[i] = false;
	}
	
	int nReturnValueIndex = 0;
	int nAlreadySelectedLayer = -1;

	for (const auto& pCompo : *pInstr->get_components()) {
		nReturnValues[nReturnValueIndex] = false;
		DrumkitComponent* pMainCompo = nullptr;

		if( pNote->get_specific_compo_id() != -1 && pNote->get_specific_compo_id() != pCompo->get_drumkit_componentID() ) {
			continue;
		}

		if(		pInstr->is_preview_instrument()
			||	pInstr->is_metronome_instrument()){
			pMainCompo = pHydrogen->getSong()->getComponents()->front();
		} else {
			int nComponentID = pCompo->get_drumkit_componentID();
			if ( nComponentID >= 0 ) {
				pMainCompo = pHydrogen->getSong()->getComponent( nComponentID );
			} else {
				/* Invalid component found. This is possible on loading older or broken song files. */
				pMainCompo = pHydrogen->getSong()->getComponents()->front();
			}
		}

		assert(pMainCompo);

		float fLayerGain = 1.0;
		float fLayerPitch = 0.0;

		// scelgo il sample da usare in base alla velocity
		std::shared_ptr<Sample> pSample;
		SelectedLayerInfo *pSelectedLayer = pNote->get_layer_selected( pCompo->get_drumkit_componentID() );

		if ( !pSelectedLayer ) {
			QString dummy = QString( "NULL Layer Information for instrument %1. Component: %2" ).arg( pInstr->get_name() ).arg( pCompo->get_drumkit_componentID() );
			WARNINGLOG( dummy );
			nReturnValues[nReturnValueIndex] = true;
			continue;
		}

		if( pSelectedLayer->SelectedLayer != -1 ) {
			auto pLayer = pCompo->get_layer( pSelectedLayer->SelectedLayer );

			if( pLayer )
			{
				pSample = pLayer->get_sample();
				fLayerGain = pLayer->get_gain();
				fLayerPitch = pLayer->get_pitch();
			}
			
		}
		else {
			switch ( pInstr->sample_selection_alg() ) {
				case Instrument::VELOCITY:
					for ( unsigned nLayer = 0; nLayer < m_nMaxLayers; ++nLayer ) {
						auto pLayer = pCompo->get_layer( nLayer );
						if ( pLayer == nullptr ) continue;

						if ( ( pNote->get_velocity() >= pLayer->get_start_velocity() ) && ( pNote->get_velocity() <= pLayer->get_end_velocity() ) ) {
							pSelectedLayer->SelectedLayer = nLayer;

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
							break;
						}
					}

					if ( !pSample ){
						WARNINGLOG( "Velocity did fall into a hole between the instrument layers." );
						// There are a small distance between the
						// layers of the instruments the velocity of
						// the pNote has fallen into. This can if the
						// drumkits weren't written with enough care.
						// To fix this rare problem, we have to search
						// for the nearest layer and use its sample.
						float shortestDistance = 1.0f;
						int nearestLayer = -1;
						for ( unsigned nLayer = 0; nLayer < m_nMaxLayers; ++nLayer ){
							auto pLayer = pCompo->get_layer( nLayer );
							if ( pLayer == nullptr ) continue;
							
							if ( std::min( abs( pLayer->get_start_velocity() - pNote->get_velocity() ),
								  abs( pLayer->get_start_velocity() - pNote->get_velocity() ) ) <
							     shortestDistance ){
								shortestDistance = std::min( abs( pLayer->get_start_velocity() - pNote->get_velocity() ),
											abs( pLayer->get_start_velocity() - pNote->get_velocity() ) );
								nearestLayer = nLayer;
							}
						}

						// Check whether the search was successful and assign the results.
						if ( nearestLayer > -1 ){
							auto pLayer = pCompo->get_layer( nearestLayer );
							pSelectedLayer->SelectedLayer = nearestLayer;
						
							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					break;

				case Instrument::RANDOM:
					if( nAlreadySelectedLayer != -1 ) {
						auto pLayer = pCompo->get_layer( nAlreadySelectedLayer );
						if ( pLayer != nullptr ) {
							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					if( pSample == nullptr ) {
						int __possibleIndex[ m_nMaxLayers ];
						int __foundSamples = 0;
						for ( unsigned nLayer = 0; nLayer < m_nMaxLayers; ++nLayer ) {
							auto pLayer = pCompo->get_layer( nLayer );
							if ( pLayer == nullptr ) continue;

							if ( ( pNote->get_velocity() >= pLayer->get_start_velocity() ) && ( pNote->get_velocity() <= pLayer->get_end_velocity() ) ) {
								__possibleIndex[__foundSamples] = nLayer;
								__foundSamples++;
							}
						}

						

						// In some instruments the start and end
						// velocities of a layer are not set
						// perfectly giving rise to some 'holes'.
						// Occasionally the velocity of a note
						// can fall into it causing the sampler
						// to just skip it. Instead, we will search
						// for the nearest sample and play this
						// one instead.
						if ( __foundSamples == 0 ){
							WARNINGLOG( "Velocity did fall into a hole between the instrument layers." );
							float shortestDistance = 1.0f;
							int nearestLayer = -1;
							for ( unsigned nLayer = 0; nLayer < m_nMaxLayers; ++nLayer ){
								auto pLayer = pCompo->get_layer( nLayer );
								if ( pLayer == nullptr ) continue;
								
								if ( std::min( abs( pLayer->get_start_velocity() - pNote->get_velocity() ),
									  abs( pLayer->get_start_velocity() - pNote->get_velocity() ) ) <
								     shortestDistance ){
									shortestDistance = std::min( abs( pLayer->get_start_velocity() - pNote->get_velocity() ),
												abs( pLayer->get_start_velocity() - pNote->get_velocity() ) );
									nearestLayer = nLayer;
								}
							}
							// Check whether the search was
							// successful and assign the
							// results.
							if ( nearestLayer > -1 ){
								pSelectedLayer->SelectedLayer = nearestLayer;

								// No loop needed in here.
								// Since the note was in
								// no layer in the first
								// place, only one is
								// sufficient.
								__possibleIndex[__foundSamples] = nearestLayer;
								__foundSamples++;
							}
						}

						if( __foundSamples > 0 ) {
							nAlreadySelectedLayer = __possibleIndex[rand() % __foundSamples];
							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							auto pLayer = pCompo->get_layer( nAlreadySelectedLayer );

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					break;

				case Instrument::ROUND_ROBIN:
					if( nAlreadySelectedLayer != -1 ) {
						auto pLayer = pCompo->get_layer( nAlreadySelectedLayer );
						if ( pLayer != nullptr ) {
							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					if( !pSample ) {
						int __possibleIndex[ m_nMaxLayers ];
						int __foundSamples = 0;
						float __roundRobinID;
						for ( unsigned nLayer = 0; nLayer < m_nMaxLayers; ++nLayer ) {
							auto pLayer = pCompo->get_layer( nLayer );
							if ( pLayer == nullptr ) continue;

							if ( ( pNote->get_velocity() >= pLayer->get_start_velocity() ) && ( pNote->get_velocity() <= pLayer->get_end_velocity() ) ) {
								__possibleIndex[__foundSamples] = nLayer;
								__roundRobinID = pLayer->get_start_velocity();
								__foundSamples++;
							}
						}

						// In some instruments the start and end
						// velocities of a layer are not set
						// perfectly giving rise to some 'holes'.
						// Occasionally the velocity of a note
						// can fall into it causing the sampler
						// to just skip it. Instead, we will search
						// for the nearest sample and play this
						// one instead.
						if ( __foundSamples == 0 ){
							WARNINGLOG( "Velocity did fall into a hole between the instrument layers." );
							float shortestDistance = 1.0f;
							int nearestLayer = -1;
							for ( unsigned nLayer = 0; nLayer < m_nMaxLayers; ++nLayer ){
								auto pLayer = pCompo->get_layer( nLayer );
								if ( pLayer == nullptr ) {
									continue;
								}
								
								
								if ( std::min( abs( pLayer->get_start_velocity() - pNote->get_velocity() ),
									  abs( pLayer->get_start_velocity() - pNote->get_velocity() ) ) <
								     shortestDistance ){
									shortestDistance = std::min( abs( pLayer->get_start_velocity() - pNote->get_velocity() ),
												abs( pLayer->get_start_velocity() - pNote->get_velocity() ) );
									nearestLayer = nLayer;
								}
							}
							// Check whether the search was
							// successful and assign the
							// results.
							if ( nearestLayer > -1 ){
								auto pLayer = pCompo->get_layer( nearestLayer );
								pSelectedLayer->SelectedLayer = nearestLayer;

								// No loop needed in here.
								// Since the note was in
								// no layer in the first
								// place, only one is
								// sufficient.
								__possibleIndex[__foundSamples] = nearestLayer;
								__roundRobinID = pLayer->get_start_velocity();
								__foundSamples++;
							}
						}

						if( __foundSamples > 0 ) {
							__roundRobinID = pInstr->get_id() * 10 + __roundRobinID;
							int p_indexToUse = pSong->getLatestRoundRobin(__roundRobinID)+1;
							if( p_indexToUse > __foundSamples - 1) {
								p_indexToUse = 0;
							}

							pSong->setLatestRoundRobin(__roundRobinID, p_indexToUse);
							nAlreadySelectedLayer = __possibleIndex[p_indexToUse];

							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							auto pLayer = pCompo->get_layer( nAlreadySelectedLayer );
							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					break;
			}
		}
		if ( !pSample ) {
			QString dummy = QString( "NULL sample for instrument %1. Note velocity: %2" ).arg( pInstr->get_name() ).arg( pNote->get_velocity() );
			WARNINGLOG( dummy );
			nReturnValues[nReturnValueIndex] = true;
			continue;
		}

		if ( pSelectedLayer->SamplePosition >= pSample->get_frames() ) {
			WARNINGLOG( "sample position out of bounds. The layer has been resized during note play?" );
			nReturnValues[nReturnValueIndex] = true;
			continue;
		}

		int noteStartInFrames = ( int ) ( pNote->get_position() * pAudioOutput->m_transport.m_fTickSize ) + pNote->get_humanize_delay();

		int nInitialSilence = 0;
		if ( noteStartInFrames > ( int ) nFramepos ) {	// scrivo silenzio prima dell'inizio della nota
			nInitialSilence = noteStartInFrames - nFramepos;
			int nFrames = nBufferSize - nInitialSilence;
			if ( nFrames < 0 ) {
				int noteStartInFramesNoHumanize = ( int )pNote->get_position() * pAudioOutput->m_transport.m_fTickSize;
				if ( noteStartInFramesNoHumanize > ( int )( nFramepos + nBufferSize ) ) {
					// this note is not valid. it's in the future...let's skip it....
					ERRORLOG( QString( "Note pos in the future?? Current frames: %1, note frame pos: %2" ).arg( nFramepos ).arg(noteStartInFramesNoHumanize ) );
					//pNote->dumpInfo();
					nReturnValues[nReturnValueIndex] = true;
					continue;
				}
				// delay note execution
				//INFOLOG( "Delaying note execution. noteStartInFrames: " + to_string( noteStartInFrames ) + ", nFramePos: " + to_string( nFramepos ) );
				//return 0;
				continue;
			}
		}

		float cost_L = 1.0f;
		float cost_R = 1.0f;
		float cost_track_L = 1.0f;
		float cost_track_R = 1.0f;

		assert(pMainCompo);
		
		bool isMutedForExport = (pHydrogen->getIsExportSessionActive() && !pInstr->is_currently_exported());
		bool isMutedBecauseOfSolo = (isAnyInstrumentSoloed() && !pInstr->is_soloed());
		
		/*
		 *  Is instrument muted?
		 *
		 *  This can be the case either if: 
		 *   - the song, instrument or component is muted 
		 *   - if we're in an export session and we're doing per-instruments exports, 
		 *       but this instrument is not currently being exported.
		 *   - if at least one instrument is soloed (but not this instrument)
		 */
		if ( isMutedForExport || pInstr->is_muted() || pSong->getIsMuted() || pMainCompo->is_muted() || isMutedBecauseOfSolo) {	
			cost_L = 0.0;
			cost_R = 0.0;
			if ( Preferences::get_instance()->m_JackTrackOutputMode == Preferences::JackTrackOutputMode::postFader ) {
				cost_track_L = 0.0;
				cost_track_R = 0.0;
			}

		} else {	// Precompute some values...
			if ( pInstr->get_apply_velocity() ) {
				cost_L = cost_L * pNote->get_velocity();		// note velocity
				cost_R = cost_R * pNote->get_velocity();		// note velocity
			}


			cost_L *= fPan_L;							// pan
			cost_L = cost_L * fLayerGain;				// layer gain
			cost_L = cost_L * pInstr->get_gain();		// instrument gain

			cost_L = cost_L * pCompo->get_gain();		// Component gain
			cost_L = cost_L * pMainCompo->get_volume(); // Component volument

			cost_L = cost_L * pInstr->get_volume();		// instrument volume
			if ( Preferences::get_instance()->m_JackTrackOutputMode == Preferences::JackTrackOutputMode::postFader ) {
				cost_track_L = cost_L * 2;
			}
			cost_L = cost_L * pSong->getVolume();	// song volume

			cost_R *= fPan_R;							// pan
			cost_R = cost_R * fLayerGain;				// layer gain
			cost_R = cost_R * pInstr->get_gain();		// instrument gain

			cost_R = cost_R * pCompo->get_gain();		// Component gain
			cost_R = cost_R * pMainCompo->get_volume(); // Component volument

			cost_R = cost_R * pInstr->get_volume();		// instrument volume
			if ( Preferences::get_instance()->m_JackTrackOutputMode == Preferences::JackTrackOutputMode::postFader ) {
				cost_track_R = cost_R * 2;
			}
			cost_R = cost_R * pSong->getVolume();	// song pan
		}

		// direct track outputs only use velocity
		if ( Preferences::get_instance()->m_JackTrackOutputMode == Preferences::JackTrackOutputMode::preFader ) {
			cost_track_L = cost_track_L * pNote->get_velocity();
			cost_track_L = cost_track_L * fLayerGain;
			cost_track_R = cost_track_L;
		}

		// Se non devo fare resample (drumkit) posso evitare di utilizzare i float e gestire il tutto in
		// maniera ottimizzata
		//	constant^12 = 2, so constant = 2^(1/12) = 1.059463.
		//	float nStep = 1.0;1.0594630943593

		float fTotalPitch = pNote->get_total_pitch() + fLayerPitch;

		//_INFOLOG( "total pitch: " + to_string( fTotalPitch ) );
		if( (int) pSelectedLayer->SamplePosition == 0  && !pInstr->is_muted() )
		{
			if( Hydrogen::get_instance()->getMidiOutput() != nullptr ){
				Hydrogen::get_instance()->getMidiOutput()->handleQueueNote( pNote );
			}
		}

		if ( fTotalPitch == 0.0 && pSample->get_sample_rate() == pAudioOutput->getSampleRate() ) { // NO RESAMPLE
			nReturnValues[nReturnValueIndex] = renderNoteNoResample( pSample, pNote, pSelectedLayer, pCompo, pMainCompo, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, pSong );
		}
		else { // RESAMPLE
			nReturnValues[nReturnValueIndex] = renderNoteResample( pSample, pNote, pSelectedLayer, pCompo, pMainCompo, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, fLayerPitch, pSong );
		}

		nReturnValueIndex++;
	}
	for ( unsigned i = 0 ; i < pInstr->get_components()->size() ; i++ ) {
		if ( !nReturnValues[i] ) {
			return false;
		}
	}
	return true;
}

bool Sampler::processPlaybackTrack(int nBufferSize)
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	AudioOutput* pAudioOutput = Hydrogen::get_instance()->getAudioOutput();
	Song* pSong = pHydrogen->getSong();


	if(   !pSong->getPlaybackTrackEnabled()
	   || pHydrogen->getState() != STATE_PLAYING
	   || pSong->getMode() != Song::SONG_MODE)
	{
		return false;
	}

	auto pCompo = m_pPlaybackTrackInstrument->get_components()->front();
	auto pSample = pCompo->get_layer(0)->get_sample();

	assert(pSample);

	float fVal_L;
	float fVal_R;

	auto pSample_data_L = pSample->get_data_l();
	auto pSample_data_R = pSample->get_data_r();
	
	float fInstrPeak_L = m_pPlaybackTrackInstrument->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = m_pPlaybackTrackInstrument->get_peak_r(); // this value will be reset to 0 by the mixer..

	int nAvail_bytes = 0;
	int	nInitialBufferPos = 0;

	if(pSample->get_sample_rate() == pAudioOutput->getSampleRate()){
		//No resampling	
		m_nPlayBackSamplePosition = pAudioOutput->m_transport.m_nFrames;
	
		nAvail_bytes = pSample->get_frames() - ( int )m_nPlayBackSamplePosition;
		
		if ( nAvail_bytes > nBufferSize ) {
			nAvail_bytes = nBufferSize;
		}

		int nInitialSamplePos = ( int ) m_nPlayBackSamplePosition;
		int nSamplePos = nInitialSamplePos;
	
		int nTimes = nInitialBufferPos + nAvail_bytes;
	
		if(m_nPlayBackSamplePosition > pSample->get_frames()){
			//playback track has ended..
			return true;
		}
	
		for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
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
		fStep *= ( float )pSample->get_sample_rate() / pAudioOutput->getSampleRate(); // Adjust for audio driver sample rate
		
		
		if(pAudioOutput->m_transport.m_nFrames == 0){
			fSamplePos = 0;
		} else {
			fSamplePos = ( (pAudioOutput->m_transport.m_nFrames/nBufferSize) * (nBufferSize * fStep));
		}
		
		nAvail_bytes = ( int )( ( float )( pSample->get_frames() - fSamplePos ) / fStep );
	
		if ( nAvail_bytes > nBufferSize ) {
			nAvail_bytes = nBufferSize;
		}

		int nTimes = nInitialBufferPos + nAvail_bytes;
	
		for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
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

bool Sampler::renderNoteNoResample(
	std::shared_ptr<Sample> pSample,
	Note *pNote,
	SelectedLayerInfo *pSelectedLayerInfo,
	std::shared_ptr<InstrumentComponent> pCompo,
	DrumkitComponent *pDrumCompo,
	int nBufferSize,
	int nInitialSilence,
	float cost_L,
	float cost_R,
	float cost_track_L,
	float cost_track_R,
	Song* pSong
)
{
	AudioOutput* pAudioOutput = Hydrogen::get_instance()->getAudioOutput();
	bool retValue = true; // the note is ended

	int nNoteLength = -1;
	if ( pNote->get_length() != -1 ) {
		nNoteLength = ( int )( pNote->get_length() * pAudioOutput->m_transport.m_fTickSize );
	}

	int nAvail_bytes = pSample->get_frames() - ( int )pSelectedLayerInfo->SamplePosition;	// verifico il numero di frame disponibili ancora da eseguire

	if ( nAvail_bytes > nBufferSize - nInitialSilence ) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = false; // the note is not ended yet
	}

	int nInitialBufferPos = nInitialSilence;
	int nInitialSamplePos = ( int )pSelectedLayerInfo->SamplePosition;
	int nSamplePos = nInitialSamplePos;
	int nTimes = nInitialBufferPos + nAvail_bytes;

	auto pSample_data_L = pSample->get_data_l();
	auto pSample_data_R = pSample->get_data_r();

	float fInstrPeak_L = pNote->get_instrument()->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->get_instrument()->get_peak_r(); // this value will be reset to 0 by the mixer..

	float fADSRValue;
	float fVal_L;
	float fVal_R;


#ifdef H2CORE_HAVE_JACK
	float *		pTrackOutL = nullptr;
	float *		pTrackOutR = nullptr;

	if ( Preferences::get_instance()->m_bJackTrackOuts ) {
		auto pJackAudioDriver = dynamic_cast<JackAudioDriver*>(pAudioOutput);
		if( pJackAudioDriver ) {
			pTrackOutL = pJackAudioDriver->getTrackOut_L( pNote->get_instrument(), pCompo );
			pTrackOutR = pJackAudioDriver->getTrackOut_R( pNote->get_instrument(), pCompo );
		}
	}
#endif

	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pSelectedLayerInfo->SamplePosition ) ) {
						if ( pNote->get_adsr()->release() == 0 ) {
				retValue = true;	// the note is ended
			}
		}

		fADSRValue = pNote->get_adsr()->get_value( 1 );
		fVal_L = pSample_data_L[ nSamplePos ] * fADSRValue;
		fVal_R = pSample_data_R[ nSamplePos ] * fADSRValue;

		// Low pass resonant filter
		if ( pNote->get_instrument()->is_filter_active() ) {
			pNote->compute_lr_values( &fVal_L, &fVal_R );
		}

#ifdef H2CORE_HAVE_JACK
		if(  pTrackOutL ) {
			 pTrackOutL[nBufferPos] += fVal_L * cost_track_L;
		}
		if( pTrackOutR ) {
			pTrackOutR[nBufferPos] += fVal_R * cost_track_R;
		}
#endif

		fVal_L = fVal_L * cost_L;
		fVal_R = fVal_R * cost_R;

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

		++nSamplePos;
	}
	pSelectedLayerInfo->SamplePosition += nAvail_bytes;
	pNote->get_instrument()->set_peak_l( fInstrPeak_L );
	pNote->get_instrument()->set_peak_r( fInstrPeak_R );


#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
	// change the below return logic if you add code after that ifdef
	if (pNote->get_instrument()->is_muted() || pSong->getIsMuted() ) return retValue;
	float masterVol =  pSong->getVolume();
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );

		float fLevel = pNote->get_instrument()->get_fx_level( nFX );

		if ( ( pFX ) && ( fLevel != 0.0 ) ) {
			fLevel = fLevel * pFX->getVolume();
			float *pBuf_L = pFX->m_pBuffer_L;
			float *pBuf_R = pFX->m_pBuffer_R;

			float fFXCost_L = fLevel * masterVol;
			float fFXCost_R = fLevel * masterVol;

			int nBufferPos = nInitialBufferPos;
			int nSamplePos = nInitialSamplePos;
			for ( int i = 0; i < nAvail_bytes; ++i ) {
				pBuf_L[ nBufferPos ] += pSample_data_L[ nSamplePos ] * fFXCost_L;
				pBuf_R[ nBufferPos ] += pSample_data_R[ nSamplePos ] * fFXCost_R;
				++nSamplePos;
				++nBufferPos;
			}
		}
	}
	// ~LADSPA
#endif

	return retValue;
}



bool Sampler::renderNoteResample(
	std::shared_ptr<Sample> pSample,
	Note *pNote,
	SelectedLayerInfo *pSelectedLayerInfo,
	std::shared_ptr<InstrumentComponent> pCompo,
	DrumkitComponent *pDrumCompo,
	int nBufferSize,
	int nInitialSilence,
	float cost_L,
	float cost_R,
	float cost_track_L,
	float cost_track_R,
	float fLayerPitch,
	Song* pSong
)
{
	AudioOutput* pAudioOutput = Hydrogen::get_instance()->getAudioOutput();

	int nNoteLength = -1;
	if ( pNote->get_length() != -1 ) {
		float resampledTickSize = AudioEngine::computeTickSize( pSample->get_sample_rate(),
		                                                          pAudioOutput->m_transport.m_fBPM,
		                                                          pSong->getResolution() );
		
		nNoteLength = ( int )( pNote->get_length() * resampledTickSize);
	}
	float fNotePitch = pNote->get_total_pitch() + fLayerPitch;

	float fStep = pow( 1.0594630943593, ( double )fNotePitch );
//	_ERRORLOG( QString("pitch: %1, step: %2" ).arg(fNotePitch).arg( fStep) );
	fStep *= ( float )pSample->get_sample_rate() / pAudioOutput->getSampleRate(); // Adjust for audio driver sample rate

	// verifico il numero di frame disponibili ancora da eseguire
	int nAvail_bytes = ( int )( ( float )( pSample->get_frames() - pSelectedLayerInfo->SamplePosition ) / fStep );


	bool retValue = true; // the note is ended
	if ( nAvail_bytes > nBufferSize - nInitialSilence ) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = false; // the note is not ended yet
	}

	int nInitialBufferPos = nInitialSilence;
	//float fInitialSamplePos = pNote->get_sample_position( pCompo->get_drumkit_componentID() );
	double fSamplePos = pSelectedLayerInfo->SamplePosition;
	int nTimes = nInitialBufferPos + nAvail_bytes;

	auto pSample_data_L = pSample->get_data_l();
	auto pSample_data_R = pSample->get_data_r();

	float fInstrPeak_L = pNote->get_instrument()->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->get_instrument()->get_peak_r(); // this value will be reset to 0 by the mixer..

	float fADSRValue = 1.0;
	float fVal_L;
	float fVal_R;
	int nSampleFrames = pSample->get_frames();


#ifdef H2CORE_HAVE_JACK
	float *		pTrackOutL = nullptr;
	float *		pTrackOutR = nullptr;

	if ( Preferences::get_instance()->m_bJackTrackOuts ) {
		auto pJackAudioDriver = dynamic_cast<JackAudioDriver*>(pAudioOutput);
		if( pJackAudioDriver ) {
			pTrackOutL = pJackAudioDriver->getTrackOut_L( pNote->get_instrument(), pCompo );
			pTrackOutR = pJackAudioDriver->getTrackOut_R( pNote->get_instrument(), pCompo );
		}
	}
#endif

	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pSelectedLayerInfo->SamplePosition ) ) {
						if ( pNote->get_adsr()->release() == 0 ) {
				retValue = 1;	// the note is ended
			}
		}

		int nSamplePos = ( int )fSamplePos;
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
								//fVal_L = linear_Interpolate( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff);
								//fVal_R = linear_Interpolate( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff);
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

		// ADSR envelope
		fADSRValue = pNote->get_adsr()->get_value( fStep );
		fVal_L = fVal_L * fADSRValue;
		fVal_R = fVal_R * fADSRValue;
		// Low pass resonant filter
		if ( pNote->get_instrument()->is_filter_active() ) {
			pNote->compute_lr_values( &fVal_L, &fVal_R );
		}



#ifdef H2CORE_HAVE_JACK
		if( 		pTrackOutL ) {
					pTrackOutL[nBufferPos] += fVal_L * cost_track_L;
		}
		if( 		pTrackOutR ) {
					pTrackOutR[nBufferPos] += fVal_R * cost_track_R;
		}
#endif

		fVal_L = fVal_L * cost_L;
		fVal_R = fVal_R * cost_R;

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

		fSamplePos += fStep;
	}
	pSelectedLayerInfo->SamplePosition += nAvail_bytes * fStep;
	pNote->get_instrument()->set_peak_l( fInstrPeak_L );
	pNote->get_instrument()->set_peak_r( fInstrPeak_R );



#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
	// change the below return logic if you add code after that ifdef
	if (pNote->get_instrument()->is_muted() || pSong->getIsMuted() ) return retValue;
	float masterVol = pSong->getVolume();
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		float fLevel = pNote->get_instrument()->get_fx_level( nFX );
		if ( ( pFX ) && ( fLevel != 0.0 ) ) {
			fLevel = fLevel * pFX->getVolume();

			float *pBuf_L = pFX->m_pBuffer_L;
			float *pBuf_R = pFX->m_pBuffer_R;

//			float fFXCost_L = cost_L * fLevel;
//			float fFXCost_R = cost_R * fLevel;
			float fFXCost_L = fLevel * masterVol;
			float fFXCost_R = fLevel * masterVol;

			int nBufferPos = nInitialBufferPos;
			float fSamplePos = pSelectedLayerInfo->SamplePosition;
			for ( int i = 0; i < nAvail_bytes; ++i ) {
				int nSamplePos = ( int )fSamplePos;
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
					}else
					{
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

				pBuf_L[ nBufferPos ] += fVal_L * fFXCost_L;
								pBuf_R[ nBufferPos ] += fVal_R * fFXCost_R;
				fSamplePos += fStep;
				++nBufferPos;
			}
		}
	}
#endif

	return retValue;
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
void Sampler::preview_sample(std::shared_ptr<Sample> pSample, int length )
{
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

	for (const auto& pComponent: *m_pPreviewInstrument->get_components()) {
		auto pLayer = pComponent->get_layer( 0 );

		pLayer->set_sample( pSample );

		Note *pPreviewNote = new Note( m_pPreviewInstrument, 0, 1.0, 0.f, length, 0 );

		stopPlayingNotes( m_pPreviewInstrument );
		noteOn( pPreviewNote );

	}

	Hydrogen::get_instance()->getAudioEngine()->unlock();
}



void Sampler::preview_instrument( std::shared_ptr<Instrument> pInstr )
{
	std::shared_ptr<Instrument> pOldPreview;
	Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );

	stopPlayingNotes( m_pPreviewInstrument );

	pOldPreview = m_pPreviewInstrument;
	m_pPreviewInstrument = pInstr;
	pInstr->set_is_preview_instrument(true);

	Note *pPreviewNote = new Note( m_pPreviewInstrument, 0, 1.0, 0.f, MAX_NOTES, 0 );

	noteOn( pPreviewNote );	// exclusive note
	Hydrogen::get_instance()->getAudioEngine()->unlock();
}



void Sampler::setPlayingNotelength( std::shared_ptr<Instrument> pInstrument, unsigned long ticks, unsigned long noteOnTick )
{
	if ( pInstrument ) { // stop all notes using this instrument
		Hydrogen *pHydrogen = Hydrogen::get_instance();
		Song* pSong = pHydrogen->getSong();
		int nSelectedpattern = pHydrogen->getSelectedPatternNumber();
		Pattern* pCurrentPattern = nullptr;


		if ( pSong->getMode() == Song::PATTERN_MODE ||
		( pHydrogen->getState() != STATE_PLAYING )){
			PatternList *pPatternList = pSong->getPatternList();
			if ( ( nSelectedpattern != -1 )
			&& ( nSelectedpattern < ( int )pPatternList->size() ) ) {
				pCurrentPattern = pPatternList->get( nSelectedpattern );
			}
		}else
		{
			std::vector<PatternList*> *pColumns = pSong->getPatternGroupVector();
//			Pattern *pPattern = NULL;
			int pos = pHydrogen->getPatternPos() +1;
			for ( int i = 0; i < pos; ++i ) {
				PatternList *pColumn = ( *pColumns )[i];
				pCurrentPattern = pColumn->get( 0 );
			}
		}


		if ( pCurrentPattern ) {
				int patternsize = pCurrentPattern->get_length();

				for ( unsigned nNote = 0; nNote < pCurrentPattern->get_length(); nNote++ ) {
					const Pattern::notes_t* notes = pCurrentPattern->get_notes();
					FOREACH_NOTE_CST_IT_BOUND(notes,it,nNote) {
						Note *pNote = it->second;
						if ( pNote!=nullptr ) {
							if( !Preferences::get_instance()->__playselectedinstrument ){
								if ( pNote->get_instrument() == pInstrument
								&& pNote->get_position() == noteOnTick ) {
									pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

									if ( ticks >  patternsize ) {
										ticks = patternsize - noteOnTick;
									}
									pNote->set_length( ticks );
									Hydrogen::get_instance()->getSong()->setIsModified( true );
									pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
								}
							}else
							{
								if ( pNote->get_instrument() == pHydrogen->getSong()->getInstrumentList()->get( pHydrogen->getSelectedInstrumentNumber())
								&& pNote->get_position() == noteOnTick ) {
									Hydrogen::get_instance()->getAudioEngine()->lock( RIGHT_HERE );
									if ( ticks >  patternsize ) {
										ticks = patternsize - noteOnTick;
									}
									pNote->set_length( ticks );
									pHydrogen->getSong()->setIsModified( true );
									pHydrogen->getAudioEngine()->unlock(); // unlock the audio engine
								}
							}
						}
					}
				}
			}
		}

	EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, -1 );
}

bool Sampler::isAnyInstrumentSoloed() const
{
	Hydrogen*		pHydrogen = Hydrogen::get_instance();
	Song*			pSong = pHydrogen->getSong();
	InstrumentList* pInstrList = pSong->getInstrumentList();
	bool			bAnyInstrumentIsSoloed = false;
	
	for(int i=0; i < pInstrList->size(); i++) {
		std::shared_ptr<Instrument> pInstr = pInstrList->get( i );
		
		if( pInstr->is_soloed() )	{
			bAnyInstrumentIsSoloed = true;
		}
	}
	
	return bAnyInstrumentIsSoloed;
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
	Song*		pSong = pHydrogen->getSong();
	std::shared_ptr<Sample>	pSample;

	if(!pSong->getPlaybackTrackFilename().isEmpty()){
		pSample = Sample::load( pSong->getPlaybackTrackFilename() );
	}
	
	auto  pPlaybackTrackLayer = std::make_shared<InstrumentLayer>( pSample );

	m_pPlaybackTrackInstrument->get_components()->front()->set_layer( pPlaybackTrackLayer, 0 );
	m_nPlayBackSamplePosition = 0;
}

};

