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
	H2_OBJECT(Sampler)
public:

   /** PAN LAWS
	* The following pan law functions return pan_L (==L, which is the gain for Left channel).
	* They assume the fPan argument domain = [-1;1], which is used in Note and Instrument classes.
	*----------------------------
	* For the right channel use: R(p) == pan_R(p) = pan_L(-p) == L(-p)
	* thanks to the Left-Right symmetry.
	*--------------------------------------
	* The prefix of the function name tells the interpretation of the fPan argument:
	*
	* "ratio" parameter:
	* 	 fPan = R/L - 1	if panned to the left,
	* 	 fPan = 1 - L/R	if panned to the right.
	*
	* "linear" parameter (arithmetic mean with linear weights):
	*	 fPan = ( R - L ) / ( R + L ).
	*
	* "polar" parameter (polar coordinate in LR plane):
	*    fPan = 4 / pi * arctan( R/L ) - 1	if L != 0,
	*    fPan = 1	if L == 0.
	*
	* "quadratic" parameter (arithmetic mean with squared weights):
	*	 fPan = ( R^2 - L^2 ) / ( R^2 + L^2 ).
	*
	* Note: using a different fPan interpretation makes the output signal more central or more lateral.
	* From more central to more lateral:
	* "quadratic" ---> "ratio" ---> "polar" ---> "linear"
	*---------------------------------------------
	* After the prefix, the name describes the Image of the pan law in the LR plane.
	* (remember that each pan law is a parametrized curve in LR plane.
	* E.g.:
	*	"ConstantSum":
	*		it's properly used in an anechoic room, where there are no reflections.
	*		Ensures uniform volumes in MONO export,
	*		has -6.02 dB center compensation.
	*	"ConstantPower":
	*		probably makes uniform volumes in a common room,
	*		has -3.01 dB center compensation.
	*	"ConstantKNorm":
	*		L^k + R^k = const
	*		generalises constant sum (k = 1) and constant power (k = 2)
	* 	"StraightPolygonal":
	*		one gain is constant while the other varies.
	*		It's ideal as BALANCE law of DUAL-channel track,
	*		has 0 dB center compensation.
	*------------------------------------------------
	* Some pan laws use expensive math functions like pow() and sqrt().
	* Pan laws can be approximated by polynomials, e.g. with degree = 2, to adjust the center compensation,
	* but then you cannot control the interpretation of the fPan argument exactly.
	*/
	enum PAN_LAW_TYPES {
		RATIO_STRAIGHT_POLYGONAL = 0,
		RATIO_CONST_POWER,
		RATIO_CONST_SUM,
		LINEAR_STRAIGHT_POLYGONAL,
		LINEAR_CONST_POWER,
		LINEAR_CONST_SUM,
		POLAR_STRAIGHT_POLYGONAL,
		POLAR_CONST_POWER,
		POLAR_CONST_SUM,
		QUADRATIC_STRAIGHT_POLYGONAL,
		QUADRATIC_CONST_POWER,
		QUADRATIC_CONST_SUM,
		LINEAR_CONST_K_NORM,
		RATIO_CONST_K_NORM,
		POLAR_CONST_K_NORM,
		QUADRATIC_CONST_K_NORM
	};
	
	/** default k for pan law with such that L^k + R^k = const
	 * must be initialised in Sampler.cpp
	 */
	static const float K_NORM_DEFAULT;
	

	// pan law functions
	static float ratioStraightPolygonalPanLaw( float fPan );
	static float ratioConstPowerPanLaw( float fPan );
	static float ratioConstSumPanLaw( float fPan );
	static float linearStraightPolygonalPanLaw( float fPan );
	static float linearConstPowerPanLaw( float fPan );
	static float linearConstSumPanLaw( float fPan );
	static float polarStraightPolygonalPanLaw( float fPan );
	static float polarConstPowerPanLaw( float fPan );
	static float polarConstSumPanLaw( float fPan );
	static float quadraticStraightPolygonalPanLaw( float fPan );
	static float quadraticConstPowerPanLaw( float fPan );
	static float quadraticConstSumPanLaw( float fPan );
	// customly compensated	
	static float linearConstKNormPanLaw( float fPan, float k );
	static float polarConstKNormPanLaw( float fPan, float k );
	static float ratioConstKNormPanLaw( float fPan, float k );
	static float quadraticConstKNormPanLaw( float fPan, float k );

   /** This function is used to load old version files (v<=1.1).
	* It returns the single pan parameter in [-1,1] from the L,R gains
	* as it was input from the GUI (up to scale and translation, which is arbitrary).
	* Default output is 0 (=central pan) if arguments are invalid.
	*-----Historical Note-----
	* Originally (version <= 1.0) pan_L,pan_R were actually gains for each channel;
	*	"instrument" and "note" pans were multiplied as in a gain CHAIN in each separate channel,
	*	so the chain killed the signal if instrument and note pans were hard-sided to opposites sides!
	* In v1.1, pan_L and pan_R were still the members of Note/Instrument representing the pan knob position,
	*	still using the ratioStraightPolygonalPanLaw() for the correspondence (up to constant multiplication),
	*	but pan_L,pan_R were reconverted to single parameter in the Sampler, and fPan was used in the selected pan law.
	*/
	static float getRatioPan( float fPan_L, float fPan_R );
	

	float* m_pMainOut_L;	///< sampler main out (left channel)
	float* m_pMainOut_R;	///< sampler main out (right channel)

	/**
	 * Constructor of the Sampler.
	 *
	 * It is called by AudioEngine::AudioEngine() and stored in
	 * AudioEngine::m_pSampler.
	 */
	Sampler();
	~Sampler();

	void process( uint32_t nFrames, std::shared_ptr<Song> pSong );

	/// Start playing a note
	void noteOn( Note * pNote );

	/// Stop playing a note.
	void noteOff( Note *pNote );
	void midiKeyboardNoteOff( int key );

	void stopPlayingNotes( std::shared_ptr<Instrument> pInstr = nullptr );

	int getPlayingNotesNumber() {
		return m_playingNotesQueue.size();
	}

	void preview_sample( std::shared_ptr<Sample> pSample, int length );
	void preview_instrument( std::shared_ptr<Instrument> pInstr );

	void setPlayingNotelength( std::shared_ptr<Instrument> pInstrument, unsigned long ticks, unsigned long noteOnTick );
	bool isInstrumentPlaying( std::shared_ptr<Instrument> pInstr );

	void setInterpolateMode( Interpolation::InterpolateMode mode ){
			 m_interpolateMode = mode;
	}
	
	std::shared_ptr<Instrument> getPreviewInstrument() const {
		return m_pPreviewInstrument;
	}
	
	std::shared_ptr<Instrument> getPlaybackTrackInstrument() const {
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
	std::shared_ptr<Instrument> m_pPlaybackTrackInstrument;

	/// Instrument used for the preview feature.
	std::shared_ptr<Instrument> m_pPreviewInstrument;

	/** Maximum number of layers to be used in the Instrument
	    editor. It will be inferred from
	    InstrumentComponent::m_nMaxLayers, which itself is
	    inferred from Preferences::m_nMaxLayers. Default value
	    assigned in Preferences::Preferences(): 16.*/
	int m_nMaxLayers;
	
	int m_nPlayBackSamplePosition;
	
	/** function to direct the computation to the selected pan law function
	 */
	float panLaw( float fPan, std::shared_ptr<Song> pSong );



	bool processPlaybackTrack(int nBufferSize);
	
	bool isAnyInstrumentSoloed() const;
	
	bool renderNote( Note* pNote, unsigned nBufferSize, std::shared_ptr<Song> pSong );

	Interpolation::InterpolateMode m_interpolateMode;

	bool renderNoteNoResample(
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
		std::shared_ptr<Song> pSong
	);

	bool renderNoteResample(
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
		std::shared_ptr<Song> pSong
	);
};


} // namespace

#endif

