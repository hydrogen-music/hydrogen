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


#ifndef SAMPLER_H
#define SAMPLER_H

#include <core/Object.h>
#include <core/Globals.h>
#include <core/Sampler/Interpolation.h>

#include <inttypes.h>
#include <vector>
#include <memory>

namespace H2Core
{

class Note;
class Song;
class Sample;
class DrumkitComponent;
class Instrument;
struct SelectedLayerInfo;
class InstrumentComponent;
class AudioOutput;

///
/// Waveform based sampler.
///
class Sampler : public H2Core::Object
{
	H2_OBJECT
public:
	float* m_pMainOut_L;	///< sampler main out (left channel)
	float* m_pMainOut_R;	///< sampler main out (right channel)

	/**
	 * Constructor of the Sampler.
	 *
	 * It is called by AudioEngine::AudioEngine() and stored in
	 * AudioEngine::__sampler.
	 */
	Sampler();
	~Sampler();

	void process( uint32_t nFrames, Song* pSong );

	/// Start playing a note
	void noteOn( Note * pNote );

	/// Stop playing a note.
	void noteOff( Note *pNote );
	void midiKeyboardNoteOff( int key );

	void stopPlayingNotes( Instrument* pInstr = nullptr );

	int getPlayingNotesNumber() {
		return m_playingNotesQueue.size();
	}

	void preview_sample( std::shared_ptr<Sample> pSample, int length );
	void preview_instrument( Instrument* pInstr );

	void setPlayingNotelength( Instrument* pInstrument, unsigned long ticks, unsigned long noteOnTick );
	bool isInstrumentPlaying( Instrument* pInstr );

	void setInterpolateMode( Interpolation::InterpolateMode mode ){
			 m_interpolateMode = mode;
	}
	
	Instrument* getPreviewInstrument() const {
		return m_pPreviewInstrument;
	}
	
	Instrument* getPlaybackTrackInstrument() const {
		return m_pPlaybackTrackInstrument;
	}

	Interpolation::InterpolateMode getInterpolateMode(){ return m_interpolateMode; }

	/**
	 * Loading of the playback track.
	 *
	 * The playback track is added to
	 * #__playback_instrument as a new InstrumentLayer
	 * containing the loaded Sample. If
	 * Song::__playback_track_filename is empty, the
	 * layer will be loaded with a nullptr instead.
	 */
	void reinitializePlaybackTrack();


private:
	std::vector<Note*> m_playingNotesQueue;
	std::vector<Note*> m_queuedNoteOffs;
	
	/// Instrument used for the playback track feature.
	Instrument* m_pPlaybackTrackInstrument;

	/// Instrument used for the preview feature.
	Instrument* m_pPreviewInstrument;

	/** Maximum number of layers to be used in the Instrument
	    editor. It will be inferred from
	    InstrumentComponent::m_nMaxLayers, which itself is
	    inferred from Preferences::m_nMaxLayers. Default value
	    assigned in Preferences::Preferences(): 16.*/
	int m_nMaxLayers;
	
	int m_nPlayBackSamplePosition;
	

	bool processPlaybackTrack(int nBufferSize);
	
	bool isAnyInstrumentSoloed() const;
	
	bool renderNote( Note* pNote, unsigned nBufferSize, Song* pSong );

	Interpolation::InterpolateMode m_interpolateMode;

	bool renderNoteNoResample(
		std::shared_ptr<Sample> pSample,
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
	);

	bool renderNoteResample(
		std::shared_ptr<Sample> pSample,
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
	);
};

} // namespace

#endif

