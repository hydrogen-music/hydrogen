/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/sampler/Sampler.h>
#include <hydrogen/adsr.h>
#include <hydrogen/DataPath.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/IO/JackOutput.h>

#include <hydrogen/Globals.h>
#include <hydrogen/Song.h>
#include <hydrogen/note.h>
#include <hydrogen/Instrument.h>
#include <hydrogen/Sample.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/Hydrogen.h>
#include <hydrogen/Preferences.h>

namespace H2Core {

inline static float linear_interpolation( float fVal_A, float fVal_B, float fVal)
{
	return fVal_A * ( 1 - fVal ) + fVal_B * fVal;
//	return fVal_A + fVal * ( fVal_B - fVal_A );
//	return fVal_A + ((fVal_B - fVal_A) * fVal);
}



Sampler::Sampler()
 : Object("Sampler")
 , __main_out_L( NULL )
 , __main_out_R( NULL )
 , __audio_output( NULL )
 , __preview_instrument( NULL )
{
	INFOLOG("INIT");

	__main_out_L = new float[ MAX_BUFFER_SIZE ];
	__main_out_R = new float[ MAX_BUFFER_SIZE ];

	// instrument used in file preview
	string sEmptySampleFilename = string( DataPath::getDataPath()) + "/emptySample.wav";
	__preview_instrument = new Instrument( sEmptySampleFilename, "preview", 0.8 );
	__preview_instrument->m_pADSR = new ADSR();
	__preview_instrument->setLayer( new InstrumentLayer( Sample::load( sEmptySampleFilename ) ), 0 );
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

	memset(__main_out_L, 0, nFrames * sizeof(float));
	memset(__main_out_R, 0, nFrames * sizeof(float));



	// Max notes limit
	int m_nMaxNotes = Preferences::getInstance()->m_nMaxNotes;
	while ( (int)__playing_notes_queue.size() > m_nMaxNotes ) {
		Note *oldNote = __playing_notes_queue[ 0 ];
		__playing_notes_queue.erase( __playing_notes_queue.begin() );
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
			delete pNote;
			pNote = NULL;
		}
		else {
			++i; // carico la prox nota
		}
	}
}



void Sampler::note_on(Note *note)
{
	//infoLog( "[noteOn]" );
	assert(note);

	// mute groups
	Instrument *pInstr = note->getInstrument();
	if ( pInstr->m_nMuteGroup != -1 ) {
		// remove all notes using the same mute group
		for ( unsigned j = 0; j < __playing_notes_queue.size(); j++ ) {	// delete older note
			Note *pNote = __playing_notes_queue[ j ];

			if ( ( pNote->getInstrument() != pInstr )  && ( pNote->getInstrument()->m_nMuteGroup == pInstr->m_nMuteGroup ) ) {
				//warningLog("release");
				pNote->m_adsr.release();
			}
		}
	}

	__playing_notes_queue.push_back(note);
}



void Sampler::note_off(Note* note)
{
	UNUSED(note);
	ERRORLOG( "not implemented yet" );
}



/// Render a note
/// Return 0: the note is not ended
/// Return 1: the note is ended
unsigned Sampler::__render_note( Note* pNote, unsigned nBufferSize, Song* pSong )
{
	//infoLog( "[renderNote] instr: " + pNote->getInstrument()->m_sName );
	assert( pSong );

	unsigned int nFramepos;
	Hydrogen* pEngine = Hydrogen::getInstance();
	if (  pEngine->getState() == STATE_PLAYING ) {
		nFramepos = __audio_output->m_transport.m_nFrames;
	}
	else {
		// use this to support realtime events when not playing
		nFramepos = pEngine->getRealtimeFrames();
	}


	Instrument *pInstr = pNote->getInstrument();
	if ( !pInstr ) {
		ERRORLOG( "NULL instrument" );
		return 1;
	}

	float fLayerGain = 1.0;
	float fLayerPitch = 0.0;

	// scelgo il sample da usare in base alla velocity
	Sample *pSample = NULL;
	for (unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer) {
		InstrumentLayer *pLayer = pInstr->getLayer( nLayer );
		if ( pLayer == NULL ) continue;

		if ( ( pNote->m_fVelocity >= pLayer->m_fStartVelocity ) && ( pNote->m_fVelocity <= pLayer->m_fEndVelocity ) ) {
			pSample = pLayer->m_pSample;
			fLayerGain = pLayer->m_fGain;
			fLayerPitch = pLayer->m_fPitch;
			break;
		}
	}
	if ( !pSample ) {
		WARNINGLOG( "NULL sample for instrument " + pInstr->m_sName + ". Note velocity: " + toString( pNote->m_fVelocity ) );
		return 1;
	}

	if ( pNote->m_fSamplePosition >= pSample->m_nFrames ) {
		WARNINGLOG( "sample position out of bounds. The layer has been resized during note play?" );
		return 1;
	}

	int noteStartInFrames = (int) ( pNote->m_nPosition * __audio_output->m_transport.m_nTickSize ) + pNote->m_nHumanizeDelay;

	int nInitialSilence = 0;
	if (noteStartInFrames > (int) nFramepos) {	// scrivo silenzio prima dell'inizio della nota
		nInitialSilence = noteStartInFrames - nFramepos;
		int nFrames = nBufferSize - nInitialSilence;
		if ( nFrames < 0 ) {
			int noteStartInFramesNoHumanize = (int)pNote->m_nPosition * __audio_output->m_transport.m_nTickSize;
			if ( noteStartInFramesNoHumanize > (int)( nFramepos + nBufferSize ) ) {
				// this note is not valid. it's in the future...let's skip it....
				ERRORLOG( "Note pos in the future?? Current frames: " + toString( nFramepos ) + ", note frame pos: " + toString( noteStartInFramesNoHumanize ) );
				//pNote->dumpInfo();
				return 1;
			}
			// delay note execution
			//INFOLOG( "Delaying note execution. noteStartInFrames: " + toString( noteStartInFrames ) + ", nFramePos: " + toString( nFramepos ) );
			return 0;
		}
	}

	float cost_L = 1.0f;
	float cost_R = 1.0f;
	float cost_track = 1.0f;
	float fSendFXLevel_L = 1.0f;
	float fSendFXLevel_R = 1.0f;

	if ( pInstr->m_bIsMuted || pSong->m_bIsMuted ) {	// is instrument muted?
		cost_L = 0.0;
		cost_R = 0.0;

		fSendFXLevel_L = 0.0f;
		fSendFXLevel_R = 0.0f;
	}
	else {	// Precompute some values...
		cost_L = cost_L * pNote->m_fVelocity;		// note velocity
		cost_L = cost_L * pNote->m_fPan_L;		// note pan
		cost_L = cost_L * fLayerGain;				// layer gain
		cost_L = cost_L * pInstr->m_fPan_L;		// instrument pan
		cost_L = cost_L * pInstr->m_fGain;		// instrument gain
		fSendFXLevel_L = cost_L;

		cost_L = cost_L * pInstr->m_fVolume;		// instrument volume
		cost_L = cost_L * pSong->getVolume();	// song volume
		cost_L = cost_L * 2; // max pan is 0.5


		cost_R = cost_R * pNote->m_fVelocity;		// note velocity
		cost_R = cost_R * pNote->m_fPan_R;		// note pan
		cost_R = cost_R * fLayerGain;				// layer gain
		cost_R = cost_R * pInstr->m_fPan_R;		// instrument pan
		cost_R = cost_R * pInstr->m_fGain;		// instrument gain
		fSendFXLevel_R = cost_R;

		cost_R = cost_R * pInstr->m_fVolume;		// instrument volume
		cost_R = cost_R * pSong->getVolume();	// song pan
		cost_R = cost_R * 2; // max pan is 0.5
	}

	// direct track outputs only use velocity
	cost_track = cost_track * pNote->m_fVelocity;
	cost_track = cost_track * fLayerGain;

	// Se non devo fare resample (drumkit) posso evitare di utilizzare i float e gestire il tutto in
	// maniera ottimizzata
	//	constant^12 = 2, so constant = 2^(1/12) = 1.059463.
	//	float nStep = 1.0;1.0594630943593

	float fTotalPitch = pNote->m_noteKey.m_nOctave * 12 + pNote->m_noteKey.m_key;
	fTotalPitch += pNote->m_fPitch;
	fTotalPitch += fLayerPitch;

	//_INFOLOG( "total pitch: " + toString( fTotalPitch ) );

	if ( fTotalPitch == 0.0 && pSample->m_nSampleRate == __audio_output->getSampleRate() ) {	// NO RESAMPLE
		return __render_note_no_resample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track, fSendFXLevel_L, fSendFXLevel_R, pSong );
	}
	else {	// RESAMPLE
		return __render_note_resample( pSample, pNote, nBufferSize, nInitialSilence, cost_L, cost_R, cost_track, fLayerPitch, fSendFXLevel_L, fSendFXLevel_R, pSong );
	}
}




int Sampler::__render_note_no_resample(
		Sample *pSample,
		Note *pNote,
		int nBufferSize,
		int nInitialSilence,
		float cost_L,
		float cost_R,
		float cost_track,
		float fSendFXLevel_L,
		float fSendFXLevel_R,
		Song* pSong
)
{
	int retValue = 1; // the note is ended

	int nNoteLength = -1;
	if ( pNote->m_nLength != -1) {
		nNoteLength = (int)( pNote->m_nLength * __audio_output->m_transport.m_nTickSize );
	}

	int nAvail_bytes = pSample->m_nFrames - (int)pNote->m_fSamplePosition;	// verifico il numero di frame disponibili ancora da eseguire

	if ( nAvail_bytes > nBufferSize - nInitialSilence) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = 0; // the note is not ended yet
	}

	//ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	int nInitialSamplePos = (int)pNote->m_fSamplePosition;
	int nSamplePos = nInitialSamplePos;
	int nTimes = nInitialBufferPos + nAvail_bytes;
	int nInstrument = pSong->getInstrumentList()->getPos( pNote->getInstrument() );

	// filter
	bool bUseLPF = pNote->getInstrument()->m_bFilterActive;
	float fResonance = pNote->getInstrument()->m_fResonance;
	float fCutoff = pNote->getInstrument()->m_fCutoff;

	float *pSample_data_L = pSample->getData_L();
	float *pSample_data_R = pSample->getData_R();

	float fInstrPeak_L = pNote->getInstrument()->m_fPeak_L; // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->getInstrument()->m_fPeak_R; // this value will be reset to 0 by the mixer..

	float fADSRValue;
	float fVal_L;
	float fVal_R;
	for (int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pNote->m_fSamplePosition)  ) {
			if ( pNote->m_adsr.release() == 0 ) {
				retValue = 1;	// the note is ended
			}
		}

		fADSRValue = pNote->m_adsr.getValue( 1 );
		fVal_L = pSample_data_L[ nSamplePos ] * cost_L * fADSRValue;
		fVal_R = pSample_data_R[ nSamplePos ] * cost_R * fADSRValue;

		if (__audio_output->has_track_outs()) {
			// hack hack hack: cast to JackOutput
#ifdef JACK_SUPPORT
			float* track_out_L = ((JackOutput*)__audio_output)->getTrackOut_L(nInstrument);
			float* track_out_R = ((JackOutput*)__audio_output)->getTrackOut_L(nInstrument);
			assert(track_out_L);
			assert(track_out_R);
			track_out_L[nBufferPos] = pSample_data_L[nSamplePos] * cost_track * fADSRValue;
			track_out_R[nBufferPos] = pSample_data_R[nSamplePos] * cost_track * fADSRValue;
#endif
		}

 		// Low pass resonant filter
 		if ( bUseLPF ) {
 			pNote->m_fBandPassFilterBuffer_L = fResonance * pNote->m_fBandPassFilterBuffer_L + fCutoff * (fVal_L - pNote->m_fLowPassFilterBuffer_L);
 			pNote->m_fLowPassFilterBuffer_L += fCutoff * pNote->m_fBandPassFilterBuffer_L;
 			fVal_L = pNote->m_fLowPassFilterBuffer_L;

 			pNote->m_fBandPassFilterBuffer_R = fResonance * pNote->m_fBandPassFilterBuffer_R + fCutoff * (fVal_R - pNote->m_fLowPassFilterBuffer_R);
 			pNote->m_fLowPassFilterBuffer_R += fCutoff * pNote->m_fBandPassFilterBuffer_R;
 			fVal_R = pNote->m_fLowPassFilterBuffer_R;
 		}

		// update instr peak
		if (fVal_L > fInstrPeak_L) {	fInstrPeak_L = fVal_L;	}
		if (fVal_R > fInstrPeak_R) {	fInstrPeak_R = fVal_R;	}

		// to main mix
		__main_out_L[nBufferPos] += fVal_L;
		__main_out_R[nBufferPos] += fVal_R;

		++nSamplePos;
	}
	pNote->m_fSamplePosition += nAvail_bytes;
	pNote->getInstrument()->m_fPeak_L = fInstrPeak_L;
	pNote->getInstrument()->m_fPeak_R = fInstrPeak_R;


#ifdef LADSPA_SUPPORT
	// LADSPA
	for ( unsigned nFX = 0; nFX < MAX_FX; ++nFX ) {
		LadspaFX *pFX = Effects::getInstance()->getLadspaFX( nFX );

		float fLevel = pNote->getInstrument()->m_fFXLevel[nFX];

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
			for (int i = 0; i < nAvail_bytes; ++i) {
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
		float cost_track,
		float fLayerPitch,
		float fSendFXLevel_L,
		float fSendFXLevel_R,
		Song* pSong
)
{
	int nNoteLength = -1;
	if ( pNote->m_nLength != -1) {
		nNoteLength = (int)( pNote->m_nLength * __audio_output->m_transport.m_nTickSize );
	}
	float fNotePitch = pNote->m_fPitch + fLayerPitch;
	fNotePitch += pNote->m_noteKey.m_nOctave * 12 + pNote->m_noteKey.m_key;

	//_INFOLOG( "pitch: " + toString( fNotePitch ) );

	float fStep = pow( 1.0594630943593, (double)fNotePitch );
	fStep *= (float)pSample->m_nSampleRate/__audio_output->getSampleRate(); // Adjust for audio driver sample rate

	int nAvail_bytes = (int)( (float)(pSample->m_nFrames - pNote->m_fSamplePosition) / fStep );	// verifico il numero di frame disponibili ancora da eseguire

	int retValue = 1; // the note is ended
	if (nAvail_bytes > nBufferSize - nInitialSilence ) {	// il sample e' piu' grande del buffersize
		// imposto il numero dei bytes disponibili uguale al buffersize
		nAvail_bytes = nBufferSize - nInitialSilence;
		retValue = 0; // the note is not ended yet
	}

//	ADSR *pADSR = pNote->m_pADSR;

	int nInitialBufferPos = nInitialSilence;
	float fInitialSamplePos = pNote->m_fSamplePosition;
	float fSamplePos = pNote->m_fSamplePosition;
	int nTimes = nInitialBufferPos + nAvail_bytes;
	int nInstrument = pSong->getInstrumentList()->getPos( pNote->getInstrument() );

	// filter
	bool bUseLPF = pNote->getInstrument()->m_bFilterActive;
	float fResonance = pNote->getInstrument()->m_fResonance;
	float fCutoff = pNote->getInstrument()->m_fCutoff;

	float *pSample_data_L = pSample->getData_L();
	float *pSample_data_R = pSample->getData_R();

	float fInstrPeak_L = pNote->getInstrument()->m_fPeak_L; // this value will be reset to 0 by the mixer..
	float fInstrPeak_R = pNote->getInstrument()->m_fPeak_R; // this value will be reset to 0 by the mixer..

	float fADSRValue = 1.0;
	float fVal_L;
	float fVal_R;
	int nSampleFrames = pSample->m_nFrames;

	for (int nBufferPos = nInitialBufferPos; nBufferPos < nTimes; ++nBufferPos ) {
		if ( ( nNoteLength != -1 ) && ( nNoteLength <= pNote->m_fSamplePosition)  ) {
			if ( pNote->m_adsr.release() == 0 ) {
				retValue = 1;	// the note is ended
			}
		}

		int nSamplePos = (int)fSamplePos;
		float fDiff = fSamplePos - nSamplePos;
		if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
			fVal_L = linear_interpolation( pSample_data_L[ nSampleFrames ], 0, fDiff );
			fVal_R = linear_interpolation( pSample_data_R[ nSampleFrames ], 0, fDiff );
		}
		else {
			fVal_L = linear_interpolation( pSample_data_L[nSamplePos], pSample_data_L[nSamplePos + 1], fDiff );
			fVal_R = linear_interpolation( pSample_data_R[nSamplePos], pSample_data_R[nSamplePos + 1], fDiff );
		}

		if (__audio_output->has_track_outs()) {
#ifdef JACK_SUPPORT
			// hack hack hack: cast to JackOutput
			float* track_out_L = ((JackOutput*)__audio_output)->getTrackOut_L(nInstrument);
			float* track_out_R = ((JackOutput*)__audio_output)->getTrackOut_L(nInstrument);
			assert(track_out_L);
			assert(track_out_R);
			track_out_L[nBufferPos] = fVal_L * cost_track * fADSRValue;
			track_out_R[nBufferPos] = fVal_R * cost_track * fADSRValue;
#endif
		}

		// ADSR envelope
		fADSRValue = pNote->m_adsr.getValue( fStep );
		fVal_L = fVal_L * cost_L * fADSRValue;
		fVal_R = fVal_R * cost_R * fADSRValue;

		// Low pass resonant filter
		if ( bUseLPF ) {
			pNote->m_fBandPassFilterBuffer_L = fResonance * pNote->m_fBandPassFilterBuffer_L + fCutoff * (fVal_L - pNote->m_fLowPassFilterBuffer_L);
			pNote->m_fLowPassFilterBuffer_L += fCutoff * pNote->m_fBandPassFilterBuffer_L;
			fVal_L = pNote->m_fLowPassFilterBuffer_L;

			pNote->m_fBandPassFilterBuffer_R = fResonance * pNote->m_fBandPassFilterBuffer_R + fCutoff * (fVal_R - pNote->m_fLowPassFilterBuffer_R);
			pNote->m_fLowPassFilterBuffer_R += fCutoff * pNote->m_fBandPassFilterBuffer_R;
			fVal_R = pNote->m_fLowPassFilterBuffer_R;
		}

		// update instr peak
		if (fVal_L > fInstrPeak_L) {	fInstrPeak_L = fVal_L;	}
		if (fVal_R > fInstrPeak_R) {	fInstrPeak_R = fVal_R;	}

		// to main mix
		__main_out_L[nBufferPos] += fVal_L;
		__main_out_R[nBufferPos] += fVal_R;

		fSamplePos += fStep;
	}
	pNote->m_fSamplePosition += nAvail_bytes * fStep;
	pNote->getInstrument()->m_fPeak_L = fInstrPeak_L;
	pNote->getInstrument()->m_fPeak_R = fInstrPeak_R;



#ifdef LADSPA_SUPPORT
	// LADSPA
	for (unsigned nFX = 0; nFX < MAX_FX; ++nFX) {
		LadspaFX *pFX = Effects::getInstance()->getLadspaFX( nFX );
		float fLevel = pNote->getInstrument()->m_fFXLevel[nFX];
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
			for (int i = 0; i < nAvail_bytes; ++i) {
				int nSamplePos = (int)fSamplePos;
				float fDiff = fSamplePos - nSamplePos;

				if ( ( nSamplePos + 1 ) >= nSampleFrames ) {
					fVal_L = linear_interpolation( pSample_data_L[nSamplePos], 0, fDiff );
					fVal_R = linear_interpolation( pSample_data_R[nSamplePos], 0, fDiff );
				}
				else{
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


void Sampler::stop_playing_notes(Instrument* instrument)
{
	/*
	// send a note-off event to all notes present in the playing note queue
	for ( int i = 0; i < __playing_notes_queue.size(); ++i ) {
		Note *pNote = __playing_notes_queue[ i ];
		pNote->m_pADSR->release();
	}
	*/

	if (instrument) { // stop all notes using this instrument
		for ( unsigned i = 0; i < __playing_notes_queue.size(); ) {
			Note *pNote = __playing_notes_queue[ i ];
			assert( pNote );
			if (pNote->getInstrument() == instrument) {
				delete pNote;
				__playing_notes_queue.erase( __playing_notes_queue.begin() + i );
			}
			++i;
		}
	}
	else { // stop all notes
		// delete all copied notes in the playing notes queue
		for (unsigned i = 0; i < __playing_notes_queue.size(); ++i) {
			Note *pNote = __playing_notes_queue[i];
			delete pNote;
		}
		__playing_notes_queue.clear();
	}
}



/// Preview, usa solo il primo layer
void Sampler::preview_sample(Sample* sample)
{
	AudioEngine::getInstance()->lock( "Sampler::previewSample" );

	InstrumentLayer *pLayer = __preview_instrument->getLayer(0);

	Sample *pOldSample = pLayer->m_pSample;
	pLayer->m_pSample = sample;
	delete pOldSample;

	Note *previewNote = new Note( __preview_instrument, 0, 1.0, 0.5, 0.5, MAX_NOTES, 0 );

	stop_playing_notes(__preview_instrument);
	note_on(previewNote);

	AudioEngine::getInstance()->unlock();
}



void Sampler::preview_instrument(Instrument* instr)
{
	AudioEngine::getInstance()->lock( "Sampler::previewInstrument" );

	stop_playing_notes(__preview_instrument);

	delete __preview_instrument;
	__preview_instrument = instr;

	Note *previewNote = new Note( __preview_instrument, 0, 1.0, 0.5, 0.5, MAX_NOTES, 0 );

	note_on(previewNote);	// exclusive note
	AudioEngine::getInstance()->unlock();
}



void Sampler::set_audio_output(AudioOutput* audio_output)
{
	__audio_output = audio_output;
}




};

