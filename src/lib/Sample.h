/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Sample.h,v 1.6 2005/05/01 19:51:40 comix Exp $
 *
 */

#ifndef SAMPLE_BUFFER_H
#define SAMPLE_BUFFER_H

#include <string>
using std::string;

#include "Globals.h"
#include "Object.h"

class Sample : public Object
{
	public:
		float *m_pData_L;		///< Left channel data
		float *m_pData_R;		///< Right channel data
		unsigned m_nFrames;		///< Total number of frames in this sample.
		string m_sFilename;	///< filename associated with this sample
		unsigned m_nSampleRate;	///< samplerate for this sample

		Sample(unsigned nFrames, const string& sFilename);
		~Sample();


		/// Returns the bytes number ( 2 channels )
		unsigned getNBytes(){	return m_nFrames * sizeof(float) * STEREO;	}

		/// Loads a sample from disk
		static Sample* load(const string& sFilename);

	private:

		/// loads a wave file
		static Sample* loadWave( const string& sFilename );

		/// loads a FLAC file
		static Sample* loadFLAC( const string& sFilename );
};

#endif
