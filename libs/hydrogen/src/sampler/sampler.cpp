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

#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/JackOutput.h>

#include <hydrogen/adsr.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/data_path.h>
#include <hydrogen/globals.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/note.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/sample.h>
#include <hydrogen/Song.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/event_queue.h>

#include "gui/src/HydrogenApp.h"
#include "gui/src/PatternEditor/PatternEditorPanel.h"
#include "gui/src/PatternEditor/DrumPatternEditor.h"

#include <hydrogen/fx/Effects.h>
#include <hydrogen/sampler/Sampler.h>

#include <iostream>

namespace H2Core
{

inline static float linear_interpolation( float fVal_A, float fVal_B, float fVal )
{
	return fVal_A * ( 1 - fVal ) + fVal_B * fVal;
//	return fVal_A + fVal * ( fVal_B - fVal_A );
//	return fVal_A + ((fVal_B - fVal_A) * fVal);
}

Sampler::Sampler()
		: Object( "Sampler" )
		, __main_out_L( NULL )
		, __main_out_R( NULL )
		, __preview_instrument( NULL )
{
	INFOLOG( "INIT" );

	__main_out_L = new float[ MAX_BUFFER_SIZE ];
	__main_out_R = new float[ MAX_BUFFER_SIZE ];

	// instrument used in file preview
	QString sEmptySampleFilename = DataPath::get_data_path() + "/emptySample.wav";
	__preview_instrument = new Instrument( sEmptySampleFilename, "preview", new ADSR() );
	__preview_instrument->set_volume( 0.8 );
	__preview_instrument->set_layer( new InstrumentLayer( Sample::load( sEmptySampleFilename ) ), 0 );

}



Sampler::~Sampler()
{
	INFOLOG( "DESTROY" );

	delete[] __main_out_L;
	delete[] __main_out_R;

	delete __preview_instrument;
	__preview_instrument = NULL;
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


	// eseguo tutte le note nella lista di note in esecuzione
	unsigned i = 0;
	Note* pNote;
	while ( i < __playing_notes_queue.size() ) {
		pNote = __playing_notes_queue[ i ];		// recupero una nuova nota
		unsigned res = __render_note( pNote, nFrames, pSong );
		if ( res == 1 ) {	// la nota e' finita
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
		    midiOut->handleQueueNoteOff( pNote->get_instrument()->get_midi_out_channel(),
					        ( pNote->m_noteKey.m_nOctave +3 ) * 12 + pNote->m_noteKey.m_key +
					        ( pNote->get_instrument()->get_midi_out_note() -60 ), 
					        pNote->get_velocity() * 127 );

		}
		__queuedNoteOffs.erase( __queuedNoteOffs.begin() );
		if( pNote != NULL) delete pNote;
		pNote = NULL;
	}//while

}



void Sampler::note_on( Note *note )
{
	//infoLog( "[noteOn]" );
	assert( note );

	Instrument *pInstr = note->get_instrument();

	// mute groups	
	if ( pInstr->get_mute_group() != -1 ) {
		// remove all notes using the same mute group
		for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {	// delete older note
			Note *pNote = __playing_notes_queue[ j ];
			if ( ( pNote->get_instrument() != pInstr )  && ( pNote->get_instrument()->get_mute_group() == pInstr->get_mute_group() ) ) {
				pNote->m_adsr.release();
			}
		}
	}

	//note off notes	
	if( note->get_noteoff() ){
		for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {
			Note *pNote = __playing_notes_queue[ j ];

			if ( ( pNote->get_instrument() == pInstr ) ) {
				//ERRORLOG("note_off");
				pNote->m_adsr.release();
			}	
		}
	}
	
	pInstr->enqueue();
	if( !note->get_noteoff() ){
		__playing_notes_queue.push_back( note );
	} else {
		delete note;
	}
	
	if( Hydrogen::get_instance()->getMidiOutput() != NULL ){
		Hydrogen::get_instance()->getMidiOutput()->handleQueueNote( note );
	}
	
}

void Sampler::midi_keyboard_note_off( int key )
{
	for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {
		Note *pNote = __playing_notes_queue[ j ];

		if ( ( pNote->get_midimsg1() == key) ) {
			pNote->m_adsr.release();
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
			pNote->m_adsr.release();
		}
 	}
}


/// Render a note
/// Return 0: the note is not ended
/// Return 1: the note is ended
unsigned Sampler::__render_note( Note* pNote, unsigned nBufferSize, Song* pSong )
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

	float fLayerGain = 1.0;
	float fLayerPitch = 0.0;

	// scelgo il sample da usare in base alla velocity
	Sample *pSample = NULL;
	for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
		InstrumentLayer *pLayer = pInstr->get_layer( nLayer );
		if ( pLayer == NULL ) continue;

		if ( ( pNote->get_velocity() >= pLayer->get_start_velocity() ) && ( pNote->get_velocity() <= pLayer->get_end_velocity() ) ) {
			pSample = pLayer->get_sample();
			fLayerGain = pLayer->get_gain();
			fLayerPitch = pLayer->get_pitch();
			break;
		}
	}
	if ( !pSample ) {
		QString dummy = QString( "NULL sample for instrument %1. Note velocity: %2" ).arg( pInstr->get_name() ).arg( pNote->get_velocity() );
		WARNINGLOG( dummy );
		return 1;
	}

	if ( pNote->m_fSamplePosition >= pSample->get_n_frames() ) {
		WARNINGLOG( "sample position out of bounds. The layer has been resized during note play?" );
		return 1;
	}

	int noteStartInFrames = ( int ) ( pNote->get_position() * audio_output->m_transport.m_nTickSize ) + pNote->m_nHumanizeDelay;

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
				return 1;
			}
			// delay note execution
			//INFOLOG( "Delaying note execution. noteStartInFrames: " + to_string( noteStartInFrames ) + ", nFramePos: " + to_string( nFramepos ) );
			return 0;
		}
	}

	float cost_L = 1.0f;
	float cost_R = 1.0f;
	float cost_track_L = 1.0f;
        float cost_track_R = 1.0f;

	if ( pInstr->is_muted() || pSong->__is_muted ) {	// is instrument muted?
		cost_L = 0.0;
		cost_R = 0.0;
                if ( Preferences::get_instance()->m_nJackTrackOutputMode == 0 ) {
		// Post-Fader
			cost_track_L = 0.0;
			cost_track_R = 0.0;
		}

	} else {	// Precompute some values...
		cost_L = cost_L * pNote->get_velocity();		// note velocity
		cost_L = cost_L * pNote->get_pan_l();		// note pan
		cost_L = cost_L * fLayerGain;				// layer gain
		cost_L = cost_L * pInstr->get_pan_l();		// instrument pan
                cost_L = cost_L * pInstr->get_gain();		// instrument gain

		cost_L = cost_L * pInstr->get_volume();		// instrument volume
                if ( Preferences::get_instance()->m_nJackTrackOutputMode == 0 ) {
		// Post-Fader
			cost_track_L = cost_L * 2;
		}

		cost_L = cost_L * pSong->get_volume();	// song volume
		cost_L = cost_L * 2; // max pan is 0.5


		cost_R = cost_R * pNote->get_velocity();		// note velocity
		cost_R = cost_R * pNote->get_pan_r();		// note pan
		cost_R = cost_R * fLayerGain;				// layer gain
		cost_R = cost_R * pInstr->get_pan_r();		// instrument pan
                cost_R = cost_R * pInstr->get_gain();		// instrument gain

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

	float fTotalPitch = pNote->m_noteKey.m_nOctave * 12 + pNote->m_noteKey.m_key;
	fTotalPitch += pNote->get_pitch();
	fTotalPitch += fLayerPitch;

	//_INFOLOG( "total pitch: " + to_string( fTotalPitch ) );

	if ( fTotalPitch == 0.0 && pSample->get_sample_rate() == audio_output->getSampleRate() ) {	// NO RESAMPLE
                return __render_note_no_resample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, pSong );
	} else {	// RESAMPLE
                return __render_note_resample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, fLayerPitch, pSong );
	}
}




int Sampler::__render_note_no_resample(
    Sample *pSample,
    Note *pNote,
    int nBufferSize,
    int nInitialSilence,
    float cost_L,
    float cost_R,
    float cost_track_L,
    float cost_track_R,
    Song* pSong
)
{
	AudioOutput* audio_output = Hydrogen::get_instance()->getAudioOutput();
	int retValue = 1; // the note is ended

	int nNoteLength = -1;
	if ( pNote->get_length() != -1 ) {
		nNoteLength = ( int )( pNote->get_length() * audio_output->m_transport.m_nTickSize );
	}

	int nAvail_bytes = pSample->get_n_frames() - ( int )pNote->m_fSamplePosition;	// verifico il numero di frame disponibili ancora da eseguire

	if ( nAvail_bytes > nBufferSize - nInitialSilence ) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = 0; // the note is not ended yet
	}


	//ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	int nInitialSamplePos = ( int )pNote->m_fSamplePosition;
	int nSamplePos = nInitialSamplePos;
	int nTimes = nInitialBufferPos + nAvail_bytes;
	int nInstrument = pSong->get_instrument_list()->get_pos( pNote->get_instrument() );

	// filter
	bool bUseLPF = pNote->get_instrument()->is_filter_active();
	float fResonance = pNote->get_instrument()->get_filter_resonance();
	float fCutoff = pNote->get_instrument()->get_filter_cutoff();

	float *pSample_data_L = pSample->get_data_l();
	float *pSample_data_R = pSample->get_data_r();

	float fInstrPeak_L = pNote->get_instrument()->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->get_instrument()->get_peak_r(); // this value will be reset to 0 by the mixer..

	float fADSRValue;
	float fVal_L;
	float fVal_R;

	/*
	 * nInstrument could be -1 if the instrument is not found in the current drumset.
	 * This happens when someone is using the prelistening function of the soundlibrary.
	 */

	if( nInstrument < 0 ) {
		nInstrument = 0;
	}

#ifdef JACK_SUPPORT 
	JackOutput* jao = 0; 
	float *track_out_L = 0; 
	float *track_out_R = 0; 
	if( audio_output->has_track_outs() 
	&& (jao = dynamic_cast<JackOutput*>(audio_output)) ) { 
		track_out_L = jao->getTrackOut_L( nInstrument ); 
		track_out_R = jao->getTrackOut_R( nInstrument ); 
	} 
#endif 
	
	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pNote->m_fSamplePosition )  ) {
			if ( pNote->m_adsr.release() == 0 ) {
				retValue = 1;	// the note is ended
			}
		}

		fADSRValue = pNote->m_adsr.get_value( 1 );
		fVal_L = pSample_data_L[ nSamplePos ] * fADSRValue;
		fVal_R = pSample_data_R[ nSamplePos ] * fADSRValue;

		// Low pass resonant filter
		if ( bUseLPF ) {
			pNote->m_fBandPassFilterBuffer_L = fResonance * pNote->m_fBandPassFilterBuffer_L + fCutoff * ( fVal_L - pNote->m_fLowPassFilterBuffer_L );
			pNote->m_fLowPassFilterBuffer_L += fCutoff * pNote->m_fBandPassFilterBuffer_L;
			fVal_L = pNote->m_fLowPassFilterBuffer_L;

			pNote->m_fBandPassFilterBuffer_R = fResonance * pNote->m_fBandPassFilterBuffer_R + fCutoff * ( fVal_R - pNote->m_fLowPassFilterBuffer_R );
			pNote->m_fLowPassFilterBuffer_R += fCutoff * pNote->m_fBandPassFilterBuffer_R;
			fVal_R = pNote->m_fLowPassFilterBuffer_R;
		}

#ifdef JACK_SUPPORT
	if( track_out_L ) { 
		track_out_L[nBufferPos] += fVal_L * cost_track_L; 
	} 
	if( track_out_R ) { 
		track_out_R[nBufferPos] += fVal_R * cost_track_R; 
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

		// to main mix
		__main_out_L[nBufferPos] += fVal_L;
		__main_out_R[nBufferPos] += fVal_R;

		++nSamplePos;
	}
	pNote->m_fSamplePosition += nAvail_bytes;
	pNote->get_instrument()->set_peak_l( fInstrPeak_L );
	pNote->get_instrument()->set_peak_r( fInstrPeak_R );

#ifdef LADSPA_SUPPORT
	// LADSPA

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



int Sampler::__render_note_resample(
    Sample *pSample,
    Note *pNote,
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
	AudioOutput* audio_output = Hydrogen::get_instance()->getAudioOutput();
	int nNoteLength = -1;
	if ( pNote->get_length() != -1 ) {
		nNoteLength = ( int )( pNote->get_length() * audio_output->m_transport.m_nTickSize );
	}
	float fNotePitch = pNote->get_pitch() + fLayerPitch;
	fNotePitch += pNote->m_noteKey.m_nOctave * 12 + pNote->m_noteKey.m_key;

	float fStep = pow( 1.0594630943593, ( double )fNotePitch );
//	_ERRORLOG( QString("pitch: %1, step: %2" ).arg(fNotePitch).arg( fStep) );
	fStep *= ( float )pSample->get_sample_rate() / audio_output->getSampleRate(); // Adjust for audio driver sample rate

	int nAvail_bytes = ( int )( ( float )( pSample->get_n_frames() - pNote->m_fSamplePosition ) / fStep );	// verifico il numero di frame disponibili ancora da eseguire

	int retValue = 1; // the note is ended
	if ( nAvail_bytes > nBufferSize - nInitialSilence ) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = 0; // the note is not ended yet
	}

//	ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	float fInitialSamplePos = pNote->m_fSamplePosition;
	double fSamplePos = pNote->m_fSamplePosition;
	int nTimes = nInitialBufferPos + nAvail_bytes;
	int nInstrument = pSong->get_instrument_list()->get_pos( pNote->get_instrument() );

	// filter
	bool bUseLPF = pNote->get_instrument()->is_filter_active();
	float fResonance = pNote->get_instrument()->get_filter_resonance();
	float fCutoff = pNote->get_instrument()->get_filter_cutoff();

	float *pSample_data_L = pSample->get_data_l();
	float *pSample_data_R = pSample->get_data_r();

	float fInstrPeak_L = pNote->get_instrument()->get_peak_l(); // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->get_instrument()->get_peak_r(); // this value will be reset to 0 by the mixer..

	float fADSRValue = 1.0;
	float fVal_L;
	float fVal_R;
	int nSampleFrames = pSample->get_n_frames();

	/*
	 * nInstrument could be -1 if the instrument is not found in the current drumset.
	 * This happens when someone is using the prelistening function of the soundlibrary.
	 */

	if( nInstrument < 0 ) {
		nInstrument = 0;
	}

#ifdef JACK_SUPPORT 
	JackOutput* jao = 0; 
	float *track_out_L = 0; 
	float *track_out_R = 0; 
	if( audio_output->has_track_outs() 
	&& (jao = dynamic_cast<JackOutput*>(audio_output)) ) { 
		track_out_L = jao->getTrackOut_L( nInstrument ); 
		track_out_R = jao->getTrackOut_R( nInstrument ); 
	} 
#endif

	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pNote->m_fSamplePosition )  ) {
			if ( pNote->m_adsr.release() == 0 ) {
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
                        fVal_L = linear_interpolation( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff );
                        fVal_R = linear_interpolation( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff );
		}

		// ADSR envelope
		fADSRValue = pNote->m_adsr.get_value( fStep );
		fVal_L = fVal_L * fADSRValue;
		fVal_R = fVal_R * fADSRValue;

		// Low pass resonant filter
		if ( bUseLPF ) {
			pNote->m_fBandPassFilterBuffer_L = fResonance * pNote->m_fBandPassFilterBuffer_L + fCutoff * ( fVal_L - pNote->m_fLowPassFilterBuffer_L );
			pNote->m_fLowPassFilterBuffer_L += fCutoff * pNote->m_fBandPassFilterBuffer_L;
			fVal_L = pNote->m_fLowPassFilterBuffer_L;

			pNote->m_fBandPassFilterBuffer_R = fResonance * pNote->m_fBandPassFilterBuffer_R + fCutoff * ( fVal_R - pNote->m_fLowPassFilterBuffer_R );
			pNote->m_fLowPassFilterBuffer_R += fCutoff * pNote->m_fBandPassFilterBuffer_R;
			fVal_R = pNote->m_fLowPassFilterBuffer_R;
		}


#ifdef JACK_SUPPORT
	if( track_out_L ) { 
		track_out_L[nBufferPos] += fVal_L * cost_track_L; 
	} 
	if( track_out_R ) { 
		track_out_R[nBufferPos] += fVal_R * cost_track_R; 
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

		// to main mix
		__main_out_L[nBufferPos] += fVal_L;
		__main_out_R[nBufferPos] += fVal_R;

		fSamplePos += fStep;
	}
	pNote->m_fSamplePosition += nAvail_bytes * fStep;
	pNote->get_instrument()->set_peak_l( fInstrPeak_L );
	pNote->get_instrument()->set_peak_r( fInstrPeak_R );



#ifdef LADSPA_SUPPORT
	// LADSPA
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
			float fSamplePos = fInitialSamplePos;
			for ( int i = 0; i < nAvail_bytes; ++i ) {
				int nSamplePos = ( int )fSamplePos;
				double fDiff = fSamplePos - nSamplePos;

                                if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
                                        //we reach the last audioframe.
					//set this last frame to zero do nothin wrong.
					fVal_L = 0.0;
					fVal_R = 0.0;
				} else {
					fVal_L = linear_interpolation( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff );
                                        fVal_R = linear_interpolation( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff );
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

	InstrumentLayer *pLayer = __preview_instrument->get_layer( 0 );

	Sample *pOldSample = pLayer->get_sample();
	pLayer->set_sample( sample );

	Note *previewNote = new Note( __preview_instrument, 0, 1.0, 0.5, 0.5, length, 0 );

	stop_playing_notes( __preview_instrument );
	note_on( previewNote );
	delete pOldSample;

	AudioEngine::get_instance()->unlock();
}



void Sampler::preview_instrument( Instrument* instr )
{
	Instrument * old_preview;
	AudioEngine::get_instance()->lock( RIGHT_HERE );

	stop_playing_notes( __preview_instrument );

	old_preview = __preview_instrument;
	__preview_instrument = instr;

	Note *previewNote = new Note( __preview_instrument, 0, 1.0, 0.5, 0.5, MAX_NOTES, 0 );

	note_on( previewNote );	// exclusive note
	AudioEngine::get_instance()->unlock();
	delete old_preview;
}



void Sampler::setPlayingNotelength( Instrument* instrument, unsigned long ticks, unsigned long noteOnTick )
{
	if ( instrument ) { // stop all notes using this instrument
		Hydrogen *pEngine = Hydrogen::get_instance();	
		Song* mSong = pEngine->getSong();
		int selectedpattern = pEngine->__get_selected_PatterNumber();
		Pattern* currentPattern = NULL;


		if ( mSong->get_mode() == Song::PATTERN_MODE ||
		( pEngine->getState() != STATE_PLAYING )){
			PatternList *pPatternList = mSong->get_pattern_list();
			if ( ( selectedpattern != -1 )
			&& ( selectedpattern < ( int )pPatternList->get_size() ) ) {
				currentPattern = pPatternList->get( selectedpattern );
			}
		}else
		{
			std::vector<PatternList*> *pColumns = mSong->get_pattern_group_vector();
//			Pattern *pPattern = NULL;
			int pos = pEngine->getPatternPos() +1;
			for ( int i = 0; i < pos; ++i ) {
				PatternList *pColumn = ( *pColumns )[i];
				currentPattern = pColumn->get( 0 );	
			}
		}

		
		if ( currentPattern ) {
				int patternsize = currentPattern->get_length();
	
				for ( unsigned nNote = 0 ;
				nNote < currentPattern->get_length() ;
				nNote++ ) {
					std::multimap <int, Note*>::iterator pos;
					for ( pos = currentPattern->note_map.lower_bound( nNote ) ;
					pos != currentPattern->note_map.upper_bound( nNote ) ;
					++pos ) {
						Note *pNote = pos->second;
						if ( pNote!=NULL ) {
							if( !Preferences::get_instance()->__playselectedinstrument ){
								if ( pNote->get_instrument() == instrument
								&& pNote->get_position() == noteOnTick ) {
									AudioEngine::get_instance()->lock( RIGHT_HERE );
					
									if ( ticks >  patternsize ) 
										ticks = patternsize - noteOnTick;
									pNote->set_length( ticks );
									Hydrogen::get_instance()->getSong()->__is_modified = true;
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
									Hydrogen::get_instance()->getSong()->__is_modified = true;
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
}

};
