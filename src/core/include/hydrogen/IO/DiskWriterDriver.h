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

#ifndef DISK_WRITER_DRIVER_H
#define DISK_WRITER_DRIVER_H

#include <sndfile.h>

#include <inttypes.h>

#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/object.h>

namespace H2Core
{

typedef int  ( *audioProcessCallback )( uint32_t, void * );

///
/// Driver for export audio to disk
///
class DiskWriterDriver : public AudioOutput
{
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }

		unsigned				m_nSampleRate;
		QString					m_sFilename;
		unsigned				m_nBufferSize;
		int						m_nSampleDepth;
		audioProcessCallback	m_processCallback;
		float*					m_pOut_L;
		float*					m_pOut_R;

		DiskWriterDriver( audioProcessCallback processCallback, unsigned nSamplerate, int nSampleDepth );
		~DiskWriterDriver();

		int init( unsigned nBufferSize );

		int connect();
		void disconnect();

		void write( float* buffer_L, float* buffer_R, unsigned int bufferSize );

		void audioEngine_process_checkBPMChanged();

		unsigned getBufferSize() {
			return m_nBufferSize;
		}

		unsigned getSampleRate();
		
		float* getOut_L() {
			return m_pOut_L;
		}
		float* getOut_R() {
			return m_pOut_R;
		}
		
		void  setFileName( const QString& sFilename ){
			m_sFilename = sFilename;
		}

		virtual void play();
		virtual void stop();
		virtual void locate( unsigned long nFrame );
		virtual void updateTransportInfo();
		virtual void setBpm( float fBPM );
		

		

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;


};

};

#endif
