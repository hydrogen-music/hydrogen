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

#ifndef AUDIO_ENGINE_TESTS_H
#define AUDIO_ENGINE_TESTS_H

#include <core/Object.h>
#include <core/Basics/Note.h>

#include <memory>
#include <vector>

namespace H2Core
{

	class TransportPosition;

class AudioEngineTests : public H2Core::Object<AudioEngineTests>
{
	H2_OBJECT(AudioEngineTests)
public:
	/** 
	 * Unit test checking for consistency when converting frames to
	 * ticks and back.
	 *
	 * @return true on success.
	 */
	static bool testFrameToTickConversion();
	/** 
	 * Unit test checking the incremental update of the transport
	 * position in audioEngine_process().
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	static bool testTransportProcessing();
	/** 
	 * Unit test checking the relocation of the transport
	 * position in audioEngine_process().
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	static bool testTransportRelocation();
	/** 
	 * Unit test checking consistency of tick intervals processed in
	 * updateNoteQueue() (no overlap and no holes).
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	static bool testComputeTickInterval();
	/** 
	 * Unit test checking consistency of transport position when
	 * playback was looped at least once and the song size is changed
	 * by toggling a pattern.
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	static bool testSongSizeChange();
	/** 
	 * Unit test checking consistency of transport position when
	 * playback was looped at least once and the song size is changed
	 * by toggling a pattern.
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	static bool testSongSizeChangeInLoopMode();
	/** 
	 * Unit test checking that all notes in a song are picked up once.
	 *
	 * Defined in here since it requires access to methods and
	 * variables private to the #AudioEngine class.
	 *
	 * @return true on success.
	 */
	static bool testNoteEnqueuing();
	
private:	
	/**
	 * Checks the consistency of the transport position @a pPos by
	 * converting the current tick, frame, column, pattern start tick
	 * etc. into each other and comparing the results. 
	 *
	 * \param pPos Transport position to check
	 * \param sContext String identifying the calling function and
	 * used for logging
	 */
	static bool checkTransportPosition( std::shared_ptr<TransportPosition> pPos, const QString& sContext );
	/**
	 * Takes two instances of Sampler::m_playingNotesQueue and checks
	 * whether matching notes have exactly @a nPassedFrames difference
	 * in their SelectedLayerInfo::SamplePosition.
	 */
	static bool checkAudioConsistency( const std::vector<std::shared_ptr<Note>> oldNotes,
									   const std::vector<std::shared_ptr<Note>> newNotes,
									   const QString& sContext,
									   int nPassedFrames,
									   bool bTestAudio = true,
									   float fPassedTicks = 0.0 );
	/**
	 * Toggles the grid cell defined by @a nToggleColumn and @a
	 * nToggleRow twice and checks whether the transport position and
	 * the audio processing remains consistent.
	 */
	static bool toggleAndCheckConsistency( int nToggleColumn, int nToggleRow, const QString& sContext );
	
	static std::vector<std::shared_ptr<Note>> copySongNoteQueue();
	/**
	 * Add every Note in @a newNotes not yet contained in @a noteList
	 * to the latter.
	 */
	static void mergeQueues( std::vector<std::shared_ptr<Note>>* noteList,
							 std::vector<std::shared_ptr<Note>> newNotes );
	static void mergeQueues( std::vector<std::shared_ptr<Note>>* noteList,
							 std::vector<Note*> newNotes );
};
};

#endif
