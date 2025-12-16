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


#ifndef SAMPLER_H
#define SAMPLER_H

#include <core/Globals.h>
#include <core/Object.h>
#include <core/Sampler/Interpolation.h>

#include <inttypes.h>
#include <vector>
#include <memory>

namespace H2Core
{

class Instrument;
class InstrumentComponent;
class InstrumentLayer;
class Note;
class Sample;
struct SelectedLayerInfo;
class Song;

///
/// Waveform based sampler.
///
/** \ingroup docCore docAudioEngine*/
class Sampler : public H2Core::Object<Sampler>
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
	
	/** default k for pan law with -4.5dB center compensation, given L^k + R^k =
	 * const it is the mean compromise between constant sum and constant power
	 */
	static constexpr float K_NORM_DEFAULT = 1.33333333333333;

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

	void process( uint32_t nFrames );

	/**
	 * @return True, if the #Sampler is still processing notes.
	 */
	bool isRenderingNotes() const;
	
	/// Start playing a note
	void noteOn( std::shared_ptr<Note> pNote );

	void midiKeyboardNoteOff( int key );

	void stopPlayingNotes( std::shared_ptr<Instrument> pInstr = nullptr );
		/** Stop playing notes gracefully by making them enter their release
		 * phase (ADSR).
		 *
		 * @param pInstr particular instrument for which notes will be release
		 *   (`nullptr` to release them all) */
		void releasePlayingNotes( std::shared_ptr<Instrument> pInstr = nullptr );

	int getPlayingNotesNumber() const {
		return m_playingNotesQueue.size();
	}

		/** Uses @a pInstr as the new preview instrument and renders it using @a
		 * pNote.
		 *
		 * This will honor all instrument, component, and layer settings. In
		 * case you want trigger just a single #H2Core::InstrumentComponent
		 * and/or a specific #H2Core::InstrumentLayer, use
		 * #H2Core::Note::setSelectedLayerInfo(). */
		void previewInstrument( std::shared_ptr<Instrument> pInstr,
								std::shared_ptr<Note> pNote );
		/** To prevent leaking other components to instrument/component/layer
		 * settings into rendering @a pSample, #m_pDefaultPreviewInstrument will
		 * be used. */
		void previewSample( std::shared_ptr<Sample> pSample, int length );

	bool isInstrumentPlaying( std::shared_ptr<Instrument> pInstr ) const;

	void setInterpolateMode( Interpolation::InterpolateMode mode ){
			 m_interpolateMode = mode;
	}
	
	std::shared_ptr<Instrument> getPlaybackTrackInstrument() const {
		return m_pPlaybackTrackInstrument;
	}

	Interpolation::InterpolateMode getInterpolateMode() const {
		return m_interpolateMode;
	}

	/**
	 * Loading of the playback track.
	 *
	 * The playback track is added to #m_pPlaybackTrackInstrument as a
	 * new InstrumentLayer containing the loaded Sample. If
	 * Song::__playback_track_filename is empty, the layer will be
	 * loaded with a nullptr instead.
	 */
	void reinitializePlaybackTrack();

	/** 
	 * Recalculates all note starts to make them valid again after a
	 * TempoMarker was added to or deleted from the #Timeline or the
	 * latter was activated.
	 */
	void handleTimelineOrTempoChange();
	/** 
	 * Recalculates all note starts and positions to make them valid
	 * again after the song size changed, e.g. a pattern was inserted
	 * or it's length was changed.
	 */
	void handleSongSizeChange();

		void clearLastUsedLayers();

	const std::vector<std::shared_ptr<Note>>& getPlayingNotesQueue() const;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
	
private:
	/** function to direct the computation to the selected pan law function
	 */
	float panLaw( float fPan, std::shared_ptr<Song> pSong );

	bool processPlaybackTrack(int nBufferSize);

    /** @return false - the note is not ended, true - the note is ended */
	bool renderNote( std::shared_ptr<Note> pNote, unsigned nBufferSize );

	bool renderNoteResample(
		std::shared_ptr<Sample> pSample,
		std::shared_ptr<Note> pNote,
		std::shared_ptr<SelectedLayerInfo> pSelectedLayerInfo,
		std::shared_ptr<InstrumentComponent> pCompo,
		int nComponentIdx,
		int nBufferSize,
		int nInitialBufferPos,
		float fGainTrack_L,
		float fGainTrack_R,
		float fGainJackTrack_L,
		float fGainJackTrack_R,
		float fLayerPitch,
		bool bIsMuted
	);

	std::vector<std::shared_ptr<Note>> m_playingNotesQueue;
	std::vector<std::shared_ptr<Note>> m_queuedNoteOffs;

	/// Instrument used for the playback track feature.
	std::shared_ptr<Instrument> m_pPlaybackTrackInstrument;

	/// Instrument used for the preview feature.
	std::shared_ptr<Instrument> m_pPreviewInstrument;

	/** Default preview instrument consisting of a single component containing a
	 * single layer. Whenever a sample is going to be preview, it will be
	 * assigned to that layer and the preview instrument will be triggered. */
	std::shared_ptr<Instrument> m_pDefaultPreviewInstrument;

	int m_nPlayBackSamplePosition;

	Interpolation::InterpolateMode m_interpolateMode;

		/** In order to allow for all layers in an #H2Core::InstrumentComponent
		 * to be selected in a round robin scheme consistently, we keep track of
		 * which layer was last used by which component. It's important to flush
		 * this map when switching drumkits in order to avoid memory leaks! */
		std::map< std::shared_ptr<InstrumentComponent>,
				  std::shared_ptr<InstrumentLayer> > m_lastUsedLayersMap;
};

inline const std::vector<std::shared_ptr<Note>>& Sampler::getPlayingNotesQueue() const {
	return m_playingNotesQueue;
}

inline void Sampler::clearLastUsedLayers() {
	m_lastUsedLayersMap.clear();
}

} // namespace

#endif

