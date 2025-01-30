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


#ifndef SYNTH_H
#define SYNTH_H

#include <cstdint>
#include <vector>

#include <core/Object.h>


namespace H2Core
{
class Note;
class AudioOutput;

///
/// A simple synthetizer...
///
/** \ingroup docCore docAudioEngine*/
class Synth : public H2Core::Object<Synth>
{
	H2_OBJECT(Synth)
public:
	float *m_pOut_L;
	float *m_pOut_R;

	/**
	 * Constructor of the Synth.
	 *
	 * It is called by AudioEngine::AudioEngine() and stored in
	 * AudioEngine::m_pSynth.
	 */
	Synth();
	~Synth();

	/// Start playing a note
	void noteOn( Note* pNote );

	/// Stop playing a note.
	void noteOff( Note* pNote );

	void process( uint32_t nFrames );
	void setAudioOutput( AudioOutput* pAudioOutput );

	int getPlayingNotesNumber() {
		return m_playingNotesQueue.size();
	}


private:
	std::vector<Note*> m_playingNotesQueue;

	float m_fTheta;
	AudioOutput *m_pAudioOutput;


};

} // namespace H2Core

#endif


