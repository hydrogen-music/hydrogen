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

#include <hydrogen/object.h>
#include <hydrogen/globals.h>

#include <inttypes.h>
#include <vector>


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
	float *__main_out_L;	///< sampler main out (left channel)
	float *__main_out_R;	///< sampler main out (right channel)

	Sampler();
	~Sampler();

	void process( uint32_t nFrames, Song* pSong );

	/// Start playing a note
	void note_on( Note *note );

	/// Stop playing a note.
	void note_off( Note *note );
	void midi_keyboard_note_off( int key );

	void stop_playing_notes( Instrument *instr = NULL );

	int get_playing_notes_number() {
		return __playing_notes_queue.size();
	}

	void preview_sample( Sample* sample, int length );
	void preview_instrument( Instrument* instr );

	void setPlayingNotelength( Instrument* instrument, unsigned long ticks, unsigned long noteOnTick );
	bool is_instrument_playing( Instrument* pInstr );

		enum InterpolateMode { LINEAR,
							   COSINE,
							   THIRD,
							   CUBIC,
							   HERMITE };

		void setInterpolateMode( InterpolateMode mode ){
				 __interpolateMode = mode;
		}

		InterpolateMode getInterpolateMode(){ return __interpolateMode; }

		/// Instrument used for the playback track feature.
		Instrument* __playback_instrument;


		/// Instrument used for the preview feature.
		Instrument* __preview_instrument;

		void reinitialize_playback_track();

private:
	std::vector<Note*> __playing_notes_queue;
	std::vector<Note*> __queuedNoteOffs;


	int __maxLayers;

	bool processPlaybackTrack(int nBufferSize);

	int __playBackSamplePosition;

	bool __render_note( Note* pNote, unsigned nBufferSize, Song* pSong );

		InterpolateMode __interpolateMode;

		/*
		double Interpolate( float y0, float y1, float y2, float y3, double mu )
		{
				switch( __interpolateMode ){

				case LINEAR:
						return linear_Interpolate( y1, y2, (float) mu );
				case COSINE:
						return cosine_Interpolate( y1, y2, mu );
				case THIRD:
						return third_Interpolate( y0, y1, y2, y3, mu );
				case CUBIC:
						return cubic_Interpolate( y0, y1, y2, y3, mu );
				case HERMITE:
						return hermite_Interpolate( y0, y1, y2, y3, mu );
				}
		};*/

		inline static float linear_Interpolate( float y1, float y2, float mu )
		{
				/*
				 * mu defines where to estimate the value on the interpolated line
				 * y1 = buffervalue on position
				 * y2 = buffervalue on position +1
				 */
				return y1 * ( 1 - mu ) + y2 * mu;
		};

		inline static float cosine_Interpolate( float y1, float y2, double mu )
		{
				/*
				 * mu defines where to estimate the value on the interpolated line
				 * y1 = buffervalue on position
				 * y2 = buffervalue on position +1
				 */
				double mu2;

				mu2 = ( 1 - cos ( mu * 3.14159 ) ) / 2;
				return( y1 * (1 - mu2 ) + y2 * mu2 );
		};

		inline static float third_Interpolate( float y0, float y1, float y2, float y3, double mu )
		{
				/*
				 * mu defines where to estimate the value on the interpolated line
				 * y0 = buffervalue on position -1
				 * y1 = buffervalue on position
				 * y2 = buffervalue on position +1
				 * y3 = buffervalue on position +2
				 */

				float c0 = y1;
				float c1 = 0.5f * ( y2 - y0 );
				float c3 = 1.5f * ( y1 - y2 ) + 0.5f * ( y3 - y0 );
				float c2 = y0 - y1 + c1 - c3;
				return ( ( c3 * mu + c2 ) * mu + c1 ) * mu + c0;
		};

		inline static float cubic_Interpolate( float y0, float y1, float y2, float y3, double mu)
		{
				/*
				 * mu defines where to estimate the value on the interpolated line
				 * y0 = buffervalue on position -1
				 * y1 = buffervalue on position
				 * y2 = buffervalue on position +1
				 * y3 = buffervalue on position +2
				 */

				double a0, a1, a2, a3, mu2;

				mu2 = mu * mu;
				a0 = y3 - y2 - y0 + y1;
				a1 = y0 - y1 - a0;
				a2 = y2 - y0;
				a3 = y1;

				return( a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3 );
		};

		inline static float hermite_Interpolate( float y0, float y1, float y2, float y3, double mu )
		{
				/*
				 * mu defines where to estimate the value on the interpolated line
				 * y0 = buffervalue on position -1
				 * y1 = buffervalue on position
				 * y2 = buffervalue on position +1
				 * y3 = buffervalue on position +2
				 */

				double a0, a1, a2, a3, mu2;

				mu2 = mu * mu;
				a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
				a1 = y0 - 2.5 * y1 + 2 * y2 - 0.5 * y3;
				a2 = -0.5 * y0 + 0.5 * y2;
				a3 = y1;

				return( a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3 );
		};

	bool __render_note_no_resample(
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
	);

	bool __render_note_resample(
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
	);
};

} // namespace

#endif

