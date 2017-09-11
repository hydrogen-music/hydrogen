/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <cassert>
#include <cmath>
#include <cstdlib>

#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/JackAudioDriver.h>

#include <hydrogen/basics/adsr.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/globals.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/event_queue.h>

#include <hydrogen/fx/Effects.h>
#include <hydrogen/sampler/Sampler.h>

#include <iostream>
#include <QDebug>

namespace H2Core
{

const char* Sampler::__class_name = "Sampler";

Sampler::Sampler()
		: Object( __class_name )
		, __main_out_L( NULL )
		, __main_out_R( NULL )
		, __preview_instrument( NULL )
{
	INFOLOG( "INIT" );
		__interpolateMode = LINEAR;
	__main_out_L = new float[ MAX_BUFFER_SIZE ];
	__main_out_R = new float[ MAX_BUFFER_SIZE ];

	// instrument used in file preview
	QString sEmptySampleFilename = Filesystem::empty_sample();
	__preview_instrument = new Instrument( EMPTY_INSTR_ID, sEmptySampleFilename );
	__preview_instrument->set_is_preview_instrument(true);
	__preview_instrument->set_volume( 0.8 );

	InstrumentLayer* pLayer = new InstrumentLayer( Sample::load( sEmptySampleFilename ) );
	InstrumentComponent* pComponent = new InstrumentComponent( 0 );

	pComponent->set_layer( pLayer, 0 );
	__preview_instrument->get_components()->push_back( pComponent );

	// dummy instrument used for playback track
	__playback_instrument = new Instrument( PLAYBACK_INSTR_ID, sEmptySampleFilename );
	__playback_instrument->set_volume( 0.8 );

	InstrumentLayer* pPlaybackTrackLayer = new InstrumentLayer( Sample::load( sEmptySampleFilename ) );
	InstrumentComponent* pPlaybackTrackComponent = new InstrumentComponent( 0 );
	pPlaybackTrackComponent->set_layer( pPlaybackTrackLayer, 0 );

	__playback_instrument->get_components()->push_back( pPlaybackTrackComponent );
	__playBackSamplePosition = 0;
}


Sampler::~Sampler()
{
	INFOLOG( "DESTROY" );

	delete[] __main_out_L;
	delete[] __main_out_R;

	delete __preview_instrument;
	__preview_instrument = NULL;


	delete __playback_instrument;
	__playback_instrument = NULL;
}

// perche' viene passata anche la canzone? E' davvero necessaria?
void Sampler::process( uint32_t nFrames, Song* pSong )
{
	//infoLog( "[process]" );
	AudioOutput* audio_output = Hydrogen::get_instance()->getAudioOutput();
	assert( audio_output );

	memset( __main_out_L, 0, nFrames * sizeof( float ) );
	memset( __main_out_R, 0, nFrames * sizeof( float ) );

	// Track output queues are zeroed by
	// audioEngine_process_clearAudioBuffers()

	// Max notes limit
	int m_nMaxNotes = Preferences::get_instance()->m_nMaxNotes;
	while ( ( int )__playing_notes_queue.size() > m_nMaxNotes ) {
		Note *oldNote = __playing_notes_queue[ 0 ];
		__playing_notes_queue.erase( __playing_notes_queue.begin() );
		oldNote->get_instrument()->dequeue();
		delete oldNote;	// FIXME: send note-off instead of removing the note from the list?
	}

	for (std::vector<DrumkitComponent*>::iterator it = pSong->get_components()->begin() ; it != pSong->get_components()->end(); ++it) {
		DrumkitComponent* component = *it;
		component->reset_outs(nFrames);
	}


	// eseguo tutte le note nella lista di note in esecuzione
	unsigned i = 0;
	Note* pNote;
	while ( i < __playing_notes_queue.size() ) {
		pNote = __playing_notes_queue[ i ];		// recupero una nuova nota
		if ( __render_note( pNote, nFrames, pSong ) ) {	// la nota e' finita
			__playing_notes_queue.erase( __playing_notes_queue.begin() + i );
			pNote->get_instrument()->dequeue();
			__queuedNoteOffs.push_back( pNote );
//			delete pNote;
//			pNote = NULL;
		} else {
			++i; // carico la prox nota
		}
	}

	//Queue midi note off messages for notes that have a length specified for them

	while ( !__queuedNoteOffs.empty() ) {
		pNote =  __queuedNoteOffs[0];
		MidiOutput* midiOut = Hydrogen::get_instance()->getMidiOutput();
		if( midiOut != NULL ){
			midiOut->handleQueueNoteOff( pNote->get_instrument()->get_midi_out_channel(), pNote->get_midi_key(),  pNote->get_midi_velocity() );

		}
		__queuedNoteOffs.erase( __queuedNoteOffs.begin() );
		if( pNote != NULL) delete pNote;
		pNote = NULL;
	}//while

	processPlaybackTrack(nFrames);
}



void Sampler::note_on( Note *note )
{
	//infoLog( "[noteOn]" );
	assert( note );

	note->get_adsr()->attack();
	Instrument *pInstr = note->get_instrument();

	// mute group
	int mute_grp = pInstr->get_mute_group();
	if ( mute_grp != -1 ) {
		// remove all notes using the same mute group
		for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {	// delete older note
			Note *pNote = __playing_notes_queue[ j ];
			if ( ( pNote->get_instrument() != pInstr )  && ( pNote->get_instrument()->get_mute_group() == mute_grp ) ) {
				pNote->get_adsr()->release();
			}
		}
	}

	//note off notes
	if( note->get_note_off() ){
		for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {
			Note *pNote = __playing_notes_queue[ j ];

			if ( ( pNote->get_instrument() == pInstr ) ) {
				//ERRORLOG("note_off");
				pNote->get_adsr()->release();
			}
		}
	}

	pInstr->enqueue();
	if( !note->get_note_off() ){
		__playing_notes_queue.push_back( note );
	}
}

void Sampler::midi_keyboard_note_off( int key )
{
	for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {
		Note *pNote = __playing_notes_queue[ j ];

		if ( ( pNote->get_midi_msg() == key) ) {
			pNote->get_adsr()->release();
		}
	}
}



void Sampler::note_off( Note* note )
	/*
	* this old note_off function is only used by right click on mixer channel strip play button
	* all other note_off stuff will handle in midi_keyboard_note_off() and note_on()
	*/
{

	Instrument *pInstr = note->get_instrument();
	// find the notes using the same instrument, and release them
	for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {
		Note *pNote = __playing_notes_queue[ j ];
		if ( pNote->get_instrument() == pInstr ) {
			pNote->get_adsr()->release();
		}
	}
	delete note;
}


/// Render a note
/// Return false: the note is not ended
/// Return true: the note is ended
bool Sampler::__render_note( Note* pNote, unsigned nBufferSize, Song* pSong )
{
	//infoLog( "[renderNote] instr: " + pNote->getInstrument()->m_sName );
	assert( pSong );

	unsigned int nFramepos;
	Hydrogen* pEngine = Hydrogen::get_instance();
	AudioOutput* audio_output = pEngine->getAudioOutput();
	if (  pEngine->getState() == STATE_PLAYING ) {
		nFramepos = audio_output->m_transport.m_nFrames;
	} else {
		// use this to support realtime events when not playing
		nFramepos = pEngine->getRealtimeFrames();
	}

	Instrument *pInstr = pNote->get_instrument();
	if ( !pInstr ) {
		ERRORLOG( "NULL instrument" );
		return 1;
	}

	bool nReturnValues [pInstr->get_components()->size()];

	for(int i = 0; i < pInstr->get_components()->size(); i++){
		nReturnValues[i] = false;
	}

	int nReturnValueIndex = 0;
	int nAlreadySelectedLayer = -1;

	for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it !=pInstr->get_components()->end(); ++it) {
		nReturnValues[nReturnValueIndex] = false;
		InstrumentComponent *pCompo = *it;
		DrumkitComponent* pMainCompo = pEngine->getSong()->get_component( pCompo->get_drumkit_componentID() );

		if( pNote->get_specific_compo_id() != -1 && pNote->get_specific_compo_id() != pCompo->get_drumkit_componentID() )
			continue;

		if(		pInstr->is_preview_instrument()
			||	pInstr->is_metronome_instrument()){
			pMainCompo = pEngine->getSong()->get_components()->front();
		} else {
			pMainCompo = pEngine->getSong()->get_component( pCompo->get_drumkit_componentID() );
		}

		assert(pMainCompo);

		float fLayerGain = 1.0;
		float fLayerPitch = 0.0;

		// scelgo il sample da usare in base alla velocity
		Sample *pSample = NULL;
		SelectedLayerInfo *pSelectedLayer = pNote->get_layer_selected( pCompo->get_drumkit_componentID() );

		if ( !pSelectedLayer ) {
			QString dummy = QString( "NULL Layer Informationfor instrument %1. Component: %2" ).arg( pInstr->get_name() ).arg( pCompo->get_drumkit_componentID() );
			WARNINGLOG( dummy );
			nReturnValues[nReturnValueIndex] = true;
			continue;
		}

		if( pSelectedLayer->SelectedLayer != -1 ) {
			InstrumentLayer *pLayer = pCompo->get_layer( pSelectedLayer->SelectedLayer );

			pSample = pLayer->get_sample();
			fLayerGain = pLayer->get_gain();
			fLayerPitch = pLayer->get_pitch();
		}
		else {
			switch ( pInstr->sample_selection_alg() ) {
				case Instrument::VELOCITY:
					for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
						InstrumentLayer *pLayer = pCompo->get_layer( nLayer );
						if ( pLayer == NULL ) continue;

						if ( ( pNote->get_velocity() >= pLayer->get_start_velocity() ) && ( pNote->get_velocity() <= pLayer->get_end_velocity() ) ) {
							pSelectedLayer->SelectedLayer = nLayer;

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
							break;
						}
					}
					break;

				case Instrument::RANDOM:
					if( nAlreadySelectedLayer != -1 ) {
						InstrumentLayer *pLayer = pCompo->get_layer( nAlreadySelectedLayer );
						if ( pLayer != NULL ) {
							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					if( pSample == NULL ) {
						int __possibleIndex[MAX_LAYERS];
						int __poundSamples = 0;
						for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
							InstrumentLayer *pLayer = pCompo->get_layer( nLayer );
							if ( pLayer == NULL ) continue;

							if ( ( pNote->get_velocity() >= pLayer->get_start_velocity() ) && ( pNote->get_velocity() <= pLayer->get_end_velocity() ) ) {
								__possibleIndex[__poundSamples] = nLayer;
								__poundSamples++;
							}
						}

						if( __poundSamples > 0 ) {
							nAlreadySelectedLayer = __possibleIndex[rand() % __poundSamples];
							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							InstrumentLayer *pLayer = pCompo->get_layer( nAlreadySelectedLayer );

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					break;

				case Instrument::ROUND_ROBIN:
					if( nAlreadySelectedLayer != -1 ) {
						InstrumentLayer *pLayer = pCompo->get_layer( nAlreadySelectedLayer );
						if ( pLayer != NULL ) {
							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							pSample = pLayer->get_sample();
							fLayerGain = pLayer->get_gain();
							fLayerPitch = pLayer->get_pitch();
						}
					}
					if( !pSample ) {
						int __possibleIndex[MAX_LAYERS];
						int __foundSamples = 0;
						float __roundRobinID;
						for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
							InstrumentLayer *pLayer = pCompo->get_layer( nLayer );
							if ( pLayer == NULL ) continue;

							if ( ( pNote->get_velocity() >= pLayer->get_start_velocity() ) && ( pNote->get_velocity() <= pLayer->get_end_velocity() ) ) {
								__possibleIndex[__foundSamples] = nLayer;
								__roundRobinID = pLayer->get_start_velocity();
								__foundSamples++;
							}
						}

						if( __foundSamples > 0 ) {
							__roundRobinID = pInstr->get_id() * 10 + __roundRobinID;
							int p_indexToUse = pSong->get_latest_round_robin(__roundRobinID)+1;
							if( p_indexToUse > __foundSamples - 1)
								p_indexToUse = 0;

							pSong->set_latest_round_robin(__roundRobinID, p_indexToUse);
							nAlreadySelectedLayer = __possibleIndex[p_indexToUse];

							pSelectedLayer->SelectedLayer = nAlreadySelectedLayer;

							InstrumentLayer *pLayer = pCompo->get_layer( nAlreadySelectedLayer );
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

		int noteStartInFrames = ( int ) ( pNote->get_position() * audio_output->m_transport.m_nTickSize ) + pNote->get_humanize_delay();

		int nInitialSilence = 0;
		if ( noteStartInFrames > ( int ) nFramepos ) {	// scrivo silenzio prima dell'inizio della nota
			nInitialSilence = noteStartInFrames - nFramepos;
			int nFrames = nBufferSize - nInitialSilence;
			if ( nFrames < 0 ) {
				int noteStartInFramesNoHumanize = ( int )pNote->get_position() * audio_output->m_transport.m_nTickSize;
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

		bool isMutedForExport = (pEngine->getIsExportSessionActive() && !pInstr->is_currently_exported());

		/*
		 *  Is instrument muted?
		 *
		 *  This can be the case either if the song, instrument or component is muted or if we're in an
		 *  export session and we're doing per-instruments exports, but this instrument is not currently
		 *  beeing exported.
		 */
		if ( isMutedForExport || pInstr->is_muted() || pSong->__is_muted || pMainCompo->is_muted() ) {
			cost_L = 0.0;
			cost_R = 0.0;
			if ( Preferences::get_instance()->m_nJackTrackOutputMode == 0 ) {
				// Post-Fader
				cost_track_L = 0.0;
				cost_track_R = 0.0;
			}

		} else {	// Precompute some values...
			if ( pInstr->get_apply_velocity() ) {
				cost_L = cost_L * pNote->get_velocity();		// note velocity
				cost_R = cost_R * pNote->get_velocity();		// note velocity
			}
			cost_L = cost_L * pNote->get_pan_l();		// note pan
			cost_L = cost_L * fLayerGain;				// layer gain
			cost_L = cost_L * pInstr->get_pan_l();		// instrument pan
			cost_L = cost_L * pInstr->get_gain();		// instrument gain

			cost_L = cost_L * pCompo->get_gain();       // Component gain
			cost_L = cost_L * pMainCompo->get_volume(); // Component volument

			cost_L = cost_L * pInstr->get_volume();		// instrument volume
			if ( Preferences::get_instance()->m_nJackTrackOutputMode == 0 ) {
			// Post-Fader
			cost_track_L = cost_L * 2;
			}
			cost_L = cost_L * pSong->get_volume();	// song volume
			cost_L = cost_L * 2; // max pan is 0.5

			cost_R = cost_R * pNote->get_pan_r();		// note pan
			cost_R = cost_R * fLayerGain;				// layer gain
			cost_R = cost_R * pInstr->get_pan_r();		// instrument pan
			cost_R = cost_R * pInstr->get_gain();		// instrument gain

			cost_R = cost_R * pCompo->get_gain();       // Component gain
			cost_R = cost_R * pMainCompo->get_volume(); // Component volument

			cost_R = cost_R * pInstr->get_volume();		// instrument volume
			if ( Preferences::get_instance()->m_nJackTrackOutputMode == 0 ) {
			// Post-Fader
			cost_track_R = cost_R * 2;
			}
			cost_R = cost_R * pSong->get_volume();	// song pan
			cost_R = cost_R * 2; // max pan is 0.5
		}

		// direct track outputs only use velocity
		if ( Preferences::get_instance()->m_nJackTrackOutputMode == 1 ) {
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
		if( ( int )pSelectedLayer->SamplePosition == 0 )
		{
			if( Hydrogen::get_instance()->getMidiOutput() != NULL ){
			Hydrogen::get_instance()->getMidiOutput()->handleQueueNote( pNote );
			}
		}

		if ( fTotalPitch == 0.0 && pSample->get_sample_rate() == audio_output->getSampleRate() ) // NO RESAMPLE
			nReturnValues[nReturnValueIndex] = __render_note_no_resample( pSample, pNote, pSelectedLayer, pCompo, pMainCompo, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, pSong );
		else // RESAMPLE
			nReturnValues[nReturnValueIndex] = __render_note_resample( pSample, pNote, pSelectedLayer, pCompo, pMainCompo, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, fLayerPitch, pSong );

		nReturnValueIndex++;
	}
	for ( unsigned i = 0 ; i < pInstr->get_components()->size() ; i++ )
		if ( !nReturnValues[i] ) return false;
	return true;
}

bool Sampler::processPlaybackTrack(int nBufferSize)
{
	Hydrogen* pEngine = Hydrogen::get_instance();
	AudioOutput* pAudioOutput = Hydrogen::get_instance()->getAudioOutput();
	Song* pSong = pEngine->getSong();

	if(   !pSong->get_playback_track_enabled()
	   || pEngine->getState() != STATE_PLAYING
	   || pSong->get_mode() != Song::SONG_MODE)
	{
		return false;
	}

	InstrumentComponent *pCompo = __playback_instrument->get_components()->front();
	Sample *pSample = pCompo->get_layer(0)->get_sample();

	float fVal_L;
	float fVal_R;

	float *pSample_data_L = pSample->get_data_l();
	float *pSample_data_R = pSample->get_data_r();

	float fInstrPeak_L = __playback_instrument->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = __playback_instrument->get_peak_r(); // this value will be reset to 0 by the mixer..

	assert(pSample);

	int nAvail_bytes = 0;
	int	nInitialBufferPos = 0;

	if(pSample->get_sample_rate() == pAudioOutput->getSampleRate()){
		//No resampling
		__playBackSamplePosition = pAudioOutput->m_transport.m_nFrames;

		nAvail_bytes = pSample->get_frames() - ( int )__playBackSamplePosition;

		if ( nAvail_bytes > nBufferSize ) {
			nAvail_bytes = nBufferSize;
		}

		int nInitialSamplePos = ( int ) __playBackSamplePosition;
		int nSamplePos = nInitialSamplePos;

		int nTimes = nInitialBufferPos + nAvail_bytes;

		if(__playBackSamplePosition > pSample->get_frames()){
			//playback track has ended..
			return true;
		}

		for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
			fVal_L = pSample_data_L[ nSamplePos ];
			fVal_R = pSample_data_R[ nSamplePos ];

			fVal_L = fVal_L * 1.0f * pSong->get_playback_track_volume(); //costr
			fVal_R = fVal_R * 1.0f * pSong->get_playback_track_volume(); //cost l

			//pDrumCompo->set_outs( nBufferPos, fVal_L, fVal_R );

			// to main mix
			if ( fVal_L > fInstrPeak_L ) {
				fInstrPeak_L = fVal_L;
			}
			if ( fVal_R > fInstrPeak_R ) {
				fInstrPeak_R = fVal_R;
			}

			__main_out_L[nBufferPos] += fVal_L;
			__main_out_R[nBufferPos] += fVal_R;

			++nSamplePos;
		}
	} else {
		//Perform resampling
		double	fSamplePos = 0;
		int		nSampleFrames = pSample->get_frames();
		float	fStep = 1.0594630943593;
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
				//set this last frame to zero do nothin wrong.
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

					switch( __interpolateMode ){

							case LINEAR:
									fVal_L = pSample_data_L[nSamplePos] * (1 - fDiff ) + pSample_data_L[nSamplePos + 1] * fDiff;
									fVal_R = pSample_data_R[nSamplePos] * (1 - fDiff ) + pSample_data_R[nSamplePos + 1] * fDiff;
									break;
							case COSINE:
									fVal_L = cosine_Interpolate( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff);
									fVal_R = cosine_Interpolate( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff);
									break;
							case THIRD:
									fVal_L = third_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
									fVal_R = third_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
									break;
							case CUBIC:
									fVal_L = cubic_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
									fVal_R = cubic_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
									break;
							case HERMITE:
									fVal_L = hermite_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
									fVal_R = hermite_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
									break;
					}
			}

			if ( fVal_L > fInstrPeak_L ) {
				fInstrPeak_L = fVal_L;
			}
			if ( fVal_R > fInstrPeak_R ) {
				fInstrPeak_R = fVal_R;
			}

			__main_out_L[nBufferPos] += fVal_L;
			__main_out_R[nBufferPos] += fVal_R;


			fSamplePos += fStep;
		} //for
	}

	__playback_instrument->set_peak_l( fInstrPeak_L );
	__playback_instrument->set_peak_r( fInstrPeak_R );

	return true;
}

bool Sampler::__render_note_no_resample(
	Sample *pSample,
	Note *pNote,
	SelectedLayerInfo *pSelectedLayerInfo,
	InstrumentComponent *pCompo,
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
		nNoteLength = ( int )( pNote->get_length() * pAudioOutput->m_transport.m_nTickSize );
	}

	int nAvail_bytes = pSample->get_frames() - ( int )pSelectedLayerInfo->SamplePosition;	// verifico il numero di frame disponibili ancora da eseguire

	if ( nAvail_bytes > nBufferSize - nInitialSilence ) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = false; // the note is not ended yet
	}


	//ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	int nInitialSamplePos = ( int )pSelectedLayerInfo->SamplePosition;
	int nSamplePos = nInitialSamplePos;
	int nTimes = nInitialBufferPos + nAvail_bytes;

	float *pSample_data_L = pSample->get_data_l();
	float *pSample_data_R = pSample->get_data_r();

	float fInstrPeak_L = pNote->get_instrument()->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->get_instrument()->get_peak_r(); // this value will be reset to 0 by the mixer..

	float fADSRValue;
	float fVal_L;
	float fVal_R;


#ifdef H2CORE_HAVE_JACK
	JackAudioDriver* pJackAudioDriver = 0;
	float *		pTrackOutL = 0;
	float *		pTrackOutR = 0;

	if( pAudioOutput->has_track_outs()
	&& (pJackAudioDriver = dynamic_cast<JackAudioDriver*>(pAudioOutput)) ) {
		 pTrackOutL = pJackAudioDriver->getTrackOut_L( pNote->get_instrument(), pCompo );
		pTrackOutR = pJackAudioDriver->getTrackOut_R( pNote->get_instrument(), pCompo );
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
		__main_out_L[nBufferPos] += fVal_L;
		__main_out_R[nBufferPos] += fVal_R;

		++nSamplePos;
	}
	pSelectedLayerInfo->SamplePosition += nAvail_bytes;
	pNote->get_instrument()->set_peak_l( fInstrPeak_L );
	pNote->get_instrument()->set_peak_r( fInstrPeak_R );


#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
    // change the below return logic if you add code after that ifdef
    if (pNote->get_instrument()->is_muted()) return retValue;
    float masterVol =  pSong->get_volume();
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );

		float fLevel;
		if ( pNote->get_instrument()->is_muted() ) {
			fLevel = 0;
		}
		else {
			fLevel = pNote->get_instrument()->get_fx_level( nFX );
		}

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



bool Sampler::__render_note_resample(
	Sample *pSample,
	Note *pNote,
	SelectedLayerInfo *pSelectedLayerInfo,
	InstrumentComponent *pCompo,
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
		nNoteLength = ( int )( pNote->get_length() * pAudioOutput->m_transport.m_nTickSize );
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

	//	ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	//float fInitialSamplePos = pNote->get_sample_position( pCompo->get_drumkit_componentID() );
	double fSamplePos = pSelectedLayerInfo->SamplePosition;
	int nTimes = nInitialBufferPos + nAvail_bytes;

	float *pSample_data_L = pSample->get_data_l();
	float *pSample_data_R = pSample->get_data_r();

	float fInstrPeak_L = pNote->get_instrument()->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->get_instrument()->get_peak_r(); // this value will be reset to 0 by the mixer..

	float fADSRValue = 1.0;
	float fVal_L;
	float fVal_R;
	int nSampleFrames = pSample->get_frames();


#ifdef H2CORE_HAVE_JACK
	JackAudioDriver* pJackAudioDriver = 0;
	float *		pTrackOutL = 0;
	float *		pTrackOutR = 0;

	if( pAudioOutput->has_track_outs()
	&& (pJackAudioDriver = dynamic_cast<JackAudioDriver*>(pAudioOutput)) ) {
				pTrackOutL = pJackAudioDriver->getTrackOut_L( pNote->get_instrument(), pCompo );
				pTrackOutR = pJackAudioDriver->getTrackOut_R( pNote->get_instrument(), pCompo );
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
			//set this last frame to zero do nothin wrong.
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

				switch( __interpolateMode ){

						case LINEAR:
								fVal_L = pSample_data_L[nSamplePos] * (1 - fDiff ) + pSample_data_L[nSamplePos + 1] * fDiff;
								fVal_R = pSample_data_R[nSamplePos] * (1 - fDiff ) + pSample_data_R[nSamplePos + 1] * fDiff;
								//fVal_L = linear_Interpolate( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff);
								//fVal_R = linear_Interpolate( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff);
								break;
						case COSINE:
								fVal_L = cosine_Interpolate( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff);
								fVal_R = cosine_Interpolate( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff);
								break;
						case THIRD:
								fVal_L = third_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
								fVal_R = third_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
								break;
						case CUBIC:
								fVal_L = cubic_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
								fVal_R = cubic_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
								break;
						case HERMITE:
								fVal_L = hermite_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
								fVal_R = hermite_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
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
		__main_out_L[nBufferPos] += fVal_L;
		__main_out_R[nBufferPos] += fVal_R;

		fSamplePos += fStep;
	}
	pSelectedLayerInfo->SamplePosition += nAvail_bytes * fStep;
	pNote->get_instrument()->set_peak_l( fInstrPeak_L );
	pNote->get_instrument()->set_peak_r( fInstrPeak_R );



#ifdef H2CORE_HAVE_LADSPA
	// LADSPA
    // change the below return logic if you add code after that ifdef
    if (pNote->get_instrument()->is_muted()) return retValue;
	float masterVol = pSong->get_volume();
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
					//set this last frame to zero do nothin wrong.
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

					switch( __interpolateMode ){

					case LINEAR:
						fVal_L = pSample_data_L[nSamplePos] * (1 - fDiff ) + pSample_data_L[nSamplePos + 1] * fDiff;
						fVal_R = pSample_data_R[nSamplePos] * (1 - fDiff ) + pSample_data_R[nSamplePos + 1] * fDiff;
						break;
					case COSINE:
						fVal_L = cosine_Interpolate( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff);
						fVal_R = cosine_Interpolate( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff);
						break;
					case THIRD:
						fVal_L = third_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
						fVal_R = third_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
						break;
					case CUBIC:
						fVal_L = cubic_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
						fVal_R = cubic_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
						break;
					case HERMITE:
						fVal_L = hermite_Interpolate( pSample_data_L[ nSamplePos -1], pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], last_l, fDiff);
						fVal_R = hermite_Interpolate( pSample_data_R[ nSamplePos -1], pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], last_r, fDiff);
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


void Sampler::stop_playing_notes( Instrument* instrument )
{
	if ( instrument ) { // stop all notes using this instrument
		for ( unsigned i = 0; i < __playing_notes_queue.size(); ) {
			Note *pNote = __playing_notes_queue[ i ];
			assert( pNote );
			if ( pNote->get_instrument() == instrument ) {
				delete pNote;
				instrument->dequeue();
				__playing_notes_queue.erase( __playing_notes_queue.begin() + i );
			}
			++i;
		}
	} else { // stop all notes
		// delete all copied notes in the playing notes queue
		for ( unsigned i = 0; i < __playing_notes_queue.size(); ++i ) {
			Note *pNote = __playing_notes_queue[i];
			pNote->get_instrument()->dequeue();
			delete pNote;
		}
		__playing_notes_queue.clear();
	}
}



/// Preview, uses only the first layer
void Sampler::preview_sample( Sample* sample, int length )
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	for (std::vector<InstrumentComponent*>::iterator it = __preview_instrument->get_components()->begin() ; it != __preview_instrument->get_components()->end(); ++it) {
		InstrumentComponent* pComponent = *it;
		InstrumentLayer *pLayer = pComponent->get_layer( 0 );


		Sample *pOldSample = pLayer->get_sample();
		pLayer->set_sample( sample );

		Note *pPreviewNote = new Note( __preview_instrument, 0, 1.0, 0.5, 0.5, length, 0 );

		stop_playing_notes( __preview_instrument );
		note_on( pPreviewNote );
		delete pOldSample;
	}

	AudioEngine::get_instance()->unlock();
}



void Sampler::preview_instrument( Instrument* instr )
{
	Instrument * pOldPreview;
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	stop_playing_notes( __preview_instrument );

	pOldPreview = __preview_instrument;
	__preview_instrument = instr;
	instr->set_is_preview_instrument(true);

	Note *pPreviewNote = new Note( __preview_instrument, 0, 1.0, 0.5, 0.5, MAX_NOTES, 0 );

	note_on( pPreviewNote );	// exclusive note
	AudioEngine::get_instance()->unlock();
	delete pOldPreview;
}



void Sampler::setPlayingNotelength( Instrument* instrument, unsigned long ticks, unsigned long noteOnTick )
{
	if ( instrument ) { // stop all notes using this instrument
		Hydrogen *pEngine = Hydrogen::get_instance();
		Song* pSong = pEngine->getSong();
		int selectedpattern = pEngine->__get_selected_PatterNumber();
		Pattern* pCurrentPattern = NULL;


		if ( pSong->get_mode() == Song::PATTERN_MODE ||
		( pEngine->getState() != STATE_PLAYING )){
			PatternList *pPatternList = pSong->get_pattern_list();
			if ( ( selectedpattern != -1 )
			&& ( selectedpattern < ( int )pPatternList->size() ) ) {
				pCurrentPattern = pPatternList->get( selectedpattern );
			}
		}else
		{
			std::vector<PatternList*> *pColumns = pSong->get_pattern_group_vector();
//			Pattern *pPattern = NULL;
			int pos = pEngine->getPatternPos() +1;
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
						if ( pNote!=NULL ) {
							if( !Preferences::get_instance()->__playselectedinstrument ){
								if ( pNote->get_instrument() == instrument
								&& pNote->get_position() == noteOnTick ) {
									AudioEngine::get_instance()->lock( RIGHT_HERE );

									if ( ticks >  patternsize )
										ticks = patternsize - noteOnTick;
									pNote->set_length( ticks );
									Hydrogen::get_instance()->getSong()->set_is_modified( true );
									AudioEngine::get_instance()->unlock(); // unlock the audio engine
								}
							}else
							{
								if ( pNote->get_instrument() == pEngine->getSong()->get_instrument_list()->get( pEngine->getSelectedInstrumentNumber())
								&& pNote->get_position() == noteOnTick ) {
									AudioEngine::get_instance()->lock( RIGHT_HERE );
									if ( ticks >  patternsize )
										ticks = patternsize - noteOnTick;
									pNote->set_length( ticks );
									Hydrogen::get_instance()->getSong()->set_is_modified( true );
									AudioEngine::get_instance()->unlock(); // unlock the audio engine
								}
							}
						}
					}
				}
			}
		}

	EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, -1 );
}

bool Sampler::is_instrument_playing( Instrument* instrument )
{
	if ( instrument ) { // stop all notes using this instrument
		for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {
			if ( instrument->get_name() == __playing_notes_queue[ j ]->get_instrument()->get_name()){
				return true;
			}
		}
	}
	return false;
}

void Sampler::reinitialize_playback_track()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song* pSong = pEngine->getSong();

	InstrumentLayer* pPlaybackTrackLayer = new InstrumentLayer( Sample::load( pSong->get_playback_track_filename() ) );

	__playback_instrument->get_components()->front()->set_layer(pPlaybackTrackLayer, 0);
	__playBackSamplePosition = 0;
}

};
