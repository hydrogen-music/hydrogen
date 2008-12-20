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

#include "gui/src/HydrogenApp.h"
#include "gui/src/PatternEditor/PatternEditorPanel.h"
#include "gui/src/PatternEditor/DrumPatternEditor.h"

#include <hydrogen/fx/Effects.h>
#include <hydrogen/sampler/Sampler.h>

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
		, __audio_output( NULL )
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
	assert( __audio_output );

	memset( __main_out_L, 0, nFrames * sizeof( float ) );
	memset( __main_out_R, 0, nFrames * sizeof( float ) );


#ifdef JACK_SUPPORT
	JackOutput* jao;
	jao = dynamic_cast<JackOutput*>(__audio_output);
	if (jao) {
		int numtracks = jao->getNumTracks();

		if ( jao->has_track_outs() ) {
			for(int nTrack = 0; nTrack < numtracks; nTrack++) {
				memset( __track_out_L[nTrack],
					0,
					jao->getBufferSize( ) * sizeof( float ) );
				memset( __track_out_R[nTrack],
					0,
					jao->getBufferSize( ) * sizeof( float ) );
			}
		}
	}
#endif // JACK_SUPPORT

	// Max notes limit
	int m_nMaxNotes = Preferences::getInstance()->m_nMaxNotes;
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
			delete pNote;
			pNote = NULL;
		} else {
			++i; // carico la prox nota
		}
	}
}



void Sampler::note_on( Note *note )
{
	//infoLog( "[noteOn]" );
	assert( note );

	// mute groups
	Instrument *pInstr = note->get_instrument();
	if ( pInstr->get_mute_group() != -1 ) {
		// remove all notes using the same mute group
		for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {	// delete older note
			Note *pNote = __playing_notes_queue[ j ];

			if ( ( pNote->get_instrument() != pInstr )  && ( pNote->get_instrument()->get_mute_group() == pInstr->get_mute_group() ) ) {
				//warningLog("release");
				pNote->m_adsr.release();
			}
		}
	}
	
	pInstr->enqueue();
	__playing_notes_queue.push_back( note );
}



void Sampler::note_off( Note* note )
{
//note_off has change to add_note_off
	stop_playing_notes( note->get_instrument() );

}



void Sampler::add_note_off( QString id )
{
	__stop_notes_intrument_ids_queue.push_back( id );
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
	if (  pEngine->getState() == STATE_PLAYING ) {
		nFramepos = __audio_output->m_transport.m_nFrames;
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

	int noteStartInFrames = ( int ) ( pNote->get_position() * __audio_output->m_transport.m_nTickSize ) + pNote->m_nHumanizeDelay;

	int nInitialSilence = 0;
	if ( noteStartInFrames > ( int ) nFramepos ) {	// scrivo silenzio prima dell'inizio della nota
		nInitialSilence = noteStartInFrames - nFramepos;
		int nFrames = nBufferSize - nInitialSilence;
		if ( nFrames < 0 ) {
			int noteStartInFramesNoHumanize = ( int )pNote->get_position() * __audio_output->m_transport.m_nTickSize;
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
	float fSendFXLevel_L = 1.0f;
	float fSendFXLevel_R = 1.0f;

	if ( pInstr->is_muted() || pSong->__is_muted ) {	// is instrument muted?
		cost_L = 0.0;
		cost_R = 0.0;
                if ( Preferences::getInstance()->m_nJackTrackOutputMode == 0 ) {
		// Post-Fader
			cost_track_L = 0.0;
			cost_track_R = 0.0;
		}

		fSendFXLevel_L = 0.0f;
		fSendFXLevel_R = 0.0f;
	} else {	// Precompute some values...
		cost_L = cost_L * pNote->get_velocity();		// note velocity
		cost_L = cost_L * pNote->get_pan_l();		// note pan
		cost_L = cost_L * fLayerGain;				// layer gain
		cost_L = cost_L * pInstr->get_pan_l();		// instrument pan
		cost_L = cost_L * pInstr->get_gain();		// instrument gain
		fSendFXLevel_L = cost_L;

		cost_L = cost_L * pInstr->get_volume();		// instrument volume
                if ( Preferences::getInstance()->m_nJackTrackOutputMode == 0 ) {
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
		fSendFXLevel_R = cost_R;

		cost_R = cost_R * pInstr->get_volume();		// instrument volume
                if ( Preferences::getInstance()->m_nJackTrackOutputMode == 0 ) {
		// Post-Fader
			cost_track_R = cost_R * 2;
		}
		cost_R = cost_R * pSong->get_volume();	// song pan
		cost_R = cost_R * 2; // max pan is 0.5
	}

	// direct track outputs only use velocity
	if ( Preferences::getInstance()->m_nJackTrackOutputMode == 1 ) {
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

	if ( fTotalPitch == 0.0 && pSample->get_sample_rate() == __audio_output->getSampleRate() ) {	// NO RESAMPLE
		return __render_note_no_resample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, fSendFXLevel_L, fSendFXLevel_R, pSong );
	} else {	// RESAMPLE
		return __render_note_resample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track_L, cost_track_R, fLayerPitch, fSendFXLevel_L, fSendFXLevel_R, pSong );
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
    float fSendFXLevel_L,
    float fSendFXLevel_R,
    Song* pSong
)
{
	int retValue = 1; // the note is ended

	int nNoteLength = -1;
	if ( pNote->get_lenght() != -1 ) {
		nNoteLength = ( int )( pNote->get_lenght() * __audio_output->m_transport.m_nTickSize );
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

	float fadeout = 1.0F;
	int steps = __audio_output->getSampleRate() / 50; // 1/50 sec
	float substract = fadeout / steps;
	bool fade_note_out = false;
	
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

		if ( __stop_notes_intrument_ids_queue.size() >0 ){
			//ERRORLOG(QString("noteoff-queue: %1").arg(__stop_notes_intrument_ids_queue.size()));
			for ( unsigned i = 0; i < __stop_notes_intrument_ids_queue.size(); ) {
				QString id = __stop_notes_intrument_ids_queue[ i ];
				//assert( id );
				if ( pNote->get_instrument()->get_id() == id ) {
					fade_note_out = true;
					if ( __stop_notes_intrument_ids_queue.size() == 1){
						__stop_notes_intrument_ids_queue.clear();
					}else
					{
						__stop_notes_intrument_ids_queue.erase( __stop_notes_intrument_ids_queue.begin() + i );
					}
				}
				++i;
			}
		}
		if(fade_note_out){
			fadeout -= substract;
			steps--;
			if (steps <= 0 || fadeout <= 0.0F ){
				fadeout = 0.0F;
				retValue = 1;
			}
//			ERRORLOG(QString("true %1, steps %2").arg(fadeout).arg(steps));		
		}

#ifdef JACK_SUPPORT
		if ( __audio_output->has_track_outs()
		     && dynamic_cast<JackOutput*>(__audio_output) ) {
                        assert( __track_out_L[ nInstrument ] );
                        assert( __track_out_R[ nInstrument ] );
			__track_out_L[ nInstrument ][nBufferPos] += fVal_L * cost_track_L * fadeout;
			__track_out_R[ nInstrument ][nBufferPos] += fVal_R * cost_track_R * fadeout;
		}
#endif

                fVal_L = fVal_L * cost_L * fadeout;
		fVal_R = fVal_R * cost_R * fadeout;

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
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::getInstance()->getLadspaFX( nFX );

		float fLevel = pNote->get_instrument()->get_fx_level( nFX );

		if ( ( pFX ) && ( fLevel != 0.0 ) ) {
			fLevel = fLevel * pFX->getVolume();
			float *pBuf_L = pFX->m_pBuffer_L;
			float *pBuf_R = pFX->m_pBuffer_R;

//			float fFXCost_L = cost_L * fLevel;
//			float fFXCost_R = cost_R * fLevel;
			float fFXCost_L = fLevel * fSendFXLevel_L;
			float fFXCost_R = fLevel * fSendFXLevel_R;

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
    float fSendFXLevel_L,
    float fSendFXLevel_R,
    Song* pSong
)
{
	int nNoteLength = -1;
	if ( pNote->get_lenght() != -1 ) {
		nNoteLength = ( int )( pNote->get_lenght() * __audio_output->m_transport.m_nTickSize );
	}
	float fNotePitch = pNote->get_pitch() + fLayerPitch;
	fNotePitch += pNote->m_noteKey.m_nOctave * 12 + pNote->m_noteKey.m_key;

	//_INFOLOG( "pitch: " + to_string( fNotePitch ) );

	float fStep = pow( 1.0594630943593, ( double )fNotePitch );
	fStep *= ( float )pSample->get_sample_rate() / __audio_output->getSampleRate(); // Adjust for audio driver sample rate

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
	float fSamplePos = pNote->m_fSamplePosition;
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

	float fadeout = 1.0F;
	int steps = __audio_output->getSampleRate() / 50; // 1/50 sec
	float substract = fadeout / steps;
	bool fade_note_out = false;

	for ( int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pNote->m_fSamplePosition )  ) {
			if ( pNote->m_adsr.release() == 0 ) {
				retValue = 1;	// the note is ended
			}
		}

		int nSamplePos = ( int )fSamplePos;
		float fDiff = fSamplePos - nSamplePos;
		if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
			fVal_L = linear_interpolation( pSample_data_L[ nSampleFrames ], 0, fDiff );
			fVal_R = linear_interpolation( pSample_data_R[ nSampleFrames ], 0, fDiff );
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

		if ( __stop_notes_intrument_ids_queue.size() >0 ){
			//ERRORLOG(QString("resample noteoff-queue: %1").arg(__stop_notes_intrument_ids_queue.size()));
			for ( unsigned i = 0; i < __stop_notes_intrument_ids_queue.size(); ) {
				QString id = __stop_notes_intrument_ids_queue[ i ];
				//assert( id );
				if ( pNote->get_instrument()->get_id() == id ) {
					fade_note_out = true;
					if ( __stop_notes_intrument_ids_queue.size() == 1){
						__stop_notes_intrument_ids_queue.clear();
					}else
					{
						__stop_notes_intrument_ids_queue.erase( __stop_notes_intrument_ids_queue.begin() + i );
					}
				}
				++i;
			}
		}

		if(fade_note_out){
			fadeout -= substract;
			steps--;
			if (steps <= 0 || fadeout <= 0.0F ){
				fadeout = 0.0F;
				retValue = 1;
			}
			//ERRORLOG(QString("resample true %1").arg(fadeout));		
		}

#ifdef JACK_SUPPORT
		if ( __audio_output->has_track_outs()
			&& dynamic_cast<JackOutput*>(__audio_output) ) {
			assert( __track_out_L[ nInstrument ] );
                        assert( __track_out_R[ nInstrument ] );
			__track_out_L[ nInstrument ][nBufferPos] += (fVal_L * cost_track_L * fadeout);
			__track_out_R[ nInstrument ][nBufferPos] += (fVal_R * cost_track_R * fadeout);
		}
#endif

		fVal_L = fVal_L * cost_L * fadeout;
		fVal_R = fVal_R * cost_R * fadeout;

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
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::getInstance()->getLadspaFX( nFX );
		float fLevel = pNote->get_instrument()->get_fx_level( nFX );
		if ( ( pFX ) && ( fLevel != 0.0 ) ) {
			fLevel = fLevel * pFX->getVolume();

			float *pBuf_L = pFX->m_pBuffer_L;
			float *pBuf_R = pFX->m_pBuffer_R;

//			float fFXCost_L = cost_L * fLevel;
//			float fFXCost_R = cost_R * fLevel;
			float fFXCost_L = fLevel * fSendFXLevel_L;
			float fFXCost_R = fLevel * fSendFXLevel_R;

			int nBufferPos = nInitialBufferPos;
			float fSamplePos = fInitialSamplePos;
			for ( int i = 0; i < nAvail_bytes; ++i ) {
				int nSamplePos = ( int )fSamplePos;
				float fDiff = fSamplePos - nSamplePos;

				if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
					fVal_L = linear_interpolation( pSample_data_L[nSamplePos], 0, fDiff );
					fVal_R = linear_interpolation( pSample_data_R[nSamplePos], 0, fDiff );
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
	/*
	// send a note-off event to all notes present in the playing note queue
	for ( int i = 0; i < __playing_notes_queue.size(); ++i ) {
		Note *pNote = __playing_notes_queue[ i ];
		pNote->m_pADSR->release();
	}
	*/

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
	AudioEngine::get_instance()->lock( "Sampler::previewSample" );

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
	AudioEngine::get_instance()->lock( "Sampler::previewInstrument" );

	stop_playing_notes( __preview_instrument );

	old_preview = __preview_instrument;
	__preview_instrument = instr;

	Note *previewNote = new Note( __preview_instrument, 0, 1.0, 0.5, 0.5, MAX_NOTES, 0 );

	note_on( previewNote );	// exclusive note
	AudioEngine::get_instance()->unlock();
	delete old_preview;
}



void Sampler::set_audio_output( AudioOutput* audio_output )
{
	__audio_output = audio_output;
}

void Sampler::makeTrackOutputQueues( )
{
	INFOLOG( "Making Output Queues" );

#ifdef JACK_SUPPORT
	JackOutput* jao = 0;
	if (__audio_output && __audio_output->has_track_outs() ) {
		jao = dynamic_cast<JackOutput*>(__audio_output);
	}
	if ( jao ) {
		for (int nTrack = 0; nTrack < jao->getNumTracks( ); nTrack++) {
			__track_out_L[nTrack] = jao->getTrackOut_L( nTrack );
			assert( __track_out_L[ nTrack ] );
			__track_out_R[nTrack] = jao->getTrackOut_R( nTrack );
			assert( __track_out_R[ nTrack ] );
		}
	}
#endif // JACK_SUPPORT

}



void Sampler::setPlayingNotelenght( Instrument* instrument, unsigned long ticks, unsigned long noteOnTick )
{

	if ( instrument ) { // stop all notes using this instrument
		Hydrogen *pEngine = Hydrogen::get_instance();	
		Song* mSong = pEngine->getSong();
		int selectedpattern = pEngine->__get_selected_PatterNumber();
		Pattern* currentPattern = NULL;


		if ( mSong->get_mode() == Song::PATTERN_MODE ||
		( !Preferences::getInstance()->__recordsong && pEngine->getState() != STATE_PLAYING )){
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
				int patternsize = currentPattern->get_lenght();
	
				for ( unsigned nNote = 0 ;
				nNote < currentPattern->get_lenght() ;
				nNote++ ) {
					std::multimap <int, Note*>::iterator pos;
					for ( pos = currentPattern->note_map.lower_bound( nNote ) ;
					pos != currentPattern->note_map.upper_bound( nNote ) ;
					++pos ) {
						Note *pNote = pos->second;
						if ( pNote!=NULL ) {
							if( !Preferences::getInstance()->__playselectedinstrument ){
								if ( pNote->get_instrument() == instrument
								&& pNote->get_position() == noteOnTick ) {
									AudioEngine::get_instance()->lock("Sample::setnotelenght_event");
					
									if ( ticks >  patternsize ) 
										ticks = patternsize - noteOnTick;
									pNote->set_lenght( ticks );
									Hydrogen::get_instance()->getSong()->__is_modified = true;
									AudioEngine::get_instance()->unlock(); // unlock the audio engine
								}
							}else
							{
								if ( pNote->get_instrument() == pEngine->getSong()->get_instrument_list()->get( pEngine->getSelectedInstrumentNumber())
								&& pNote->get_position() == noteOnTick ) {
									AudioEngine::get_instance()->lock("Sample::setnotelenght_event");
									if ( ticks >  patternsize ) 
										ticks = patternsize - noteOnTick;
									pNote->set_lenght( ticks );
									Hydrogen::get_instance()->getSong()->__is_modified = true;
									AudioEngine::get_instance()->unlock(); // unlock the audio engine	
								}	
							}
						}
					}
				}
			}	
		}

	HydrogenApp::getInstance()->getPatternEditorPanel()->getDrumPatternEditor()->updateEditor();

}
};

