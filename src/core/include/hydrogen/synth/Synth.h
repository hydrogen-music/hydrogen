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


#ifndef SYNTH_H
#define SYNTH_H

#include <stdint.h> // For uint32_t et al

#include <hydrogen/object.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/globals.h>


namespace H2Core
{




///
/// A simple synthetizer...
///
class Synth : public H2Core::Object
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	float *m_pOut_L;
	float *m_pOut_R;

	/**
	 * Constructor of the Synth.
	 *
	 * It is called by AudioEngine::AudioEngine() and stored in
	 * AudioEngine::__synth.
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
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;
	std::vector<Note*> m_playingNotesQueue;

	float m_fTheta;
	AudioOutput *m_pAudioOutput;


};

} // namespace H2Core

#endif


