/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef H2C_DISK_WRITER_DRIVER_H
#define H2C_DISK_WRITER_DRIVER_H

#include <sndfile.h>

#include <atomic>
#include <inttypes.h>
#include <thread>

#include <core/IO/AudioDriver.h>
#include <core/Object.h>

namespace H2Core
{

	class DiskWriterDriver;
	void diskWriterDriver_thread( DiskWriterDriver *pDriver );
///
/// Driver for export audio to disk
///
/** \ingroup docCore docAudioDriver */
class DiskWriterDriver : public Object<DiskWriterDriver>, public AudioDriver
{
	H2_OBJECT(DiskWriterDriver)
	public:

		unsigned				m_nSampleRate;
		QString					m_sFileName;
		unsigned				m_nBufferSize;
		int						m_nSampleDepth;
		/** A value between 0.0 (maximum quality) and 1.0 (maximum
		 * compression). */
		double					m_fCompressionLevel;
		audioProcessCallback	m_processCallback;
		float*					m_pOut_L;
		float*					m_pOut_R;
		bool					 m_bIsRunning;

		DiskWriterDriver( Hydrogen* pHydrogen, audioProcessCallback processCallback );
		~DiskWriterDriver();

		virtual int init( unsigned nBufferSize ) override;

		virtual int connect() override;
		virtual void disconnect() override;

		void write();
		bool isDoneWriting() const {
			return m_bDoneWriting;
		}
		bool writingFailed() const {
			return m_bWritingFailed;
		}
		bool m_bDoneWriting;
		bool m_bWritingFailed;
		// Cooperative cancel: disconnect() raises it before joining;
		// the render loop checks it every buffer and tears down, so a
		// mid-render stop joins within one buffer instead of waiting
		// out the song. Atomic: written under the engine lock on the
		// teardown path, read on the writer thread. Public like the
		// sibling flags — the writer thread function is a free
		// function that accesses the driver's state directly.
		std::atomic<bool> m_bCancelRequested;

		virtual unsigned getBufferSize() override {
			return m_nBufferSize;
		}

		virtual unsigned getSampleRate() override;
	void setSampleRate( unsigned nNewRate ) {
		m_nSampleRate = nNewRate;
	}
	void setSampleDepth( int nNewDepth ) {
		m_nSampleDepth = nNewDepth;
	}
		void setCompressionLevel( double fCompressionLevel );

		virtual float* getOut_L() override {
			return m_pOut_L;
		}
		virtual float* getOut_R() override {
			return m_pOut_R;
		}

		void  setFileName( const QString& sFileName ){
			m_sFileName = sFileName;
		}

		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
	private:
		// Writer thread of the most recent write() call. A session
		// stopped before its first render has no thread to join —
		// joinable() is false both before the first write() and after
		// a join. Not synchronised: write() and disconnect() must stay
		// serialized (same thread, or via the export plan join).
		std::thread m_writerThread;
};

};

#endif
