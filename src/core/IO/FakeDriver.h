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

#ifndef FAKE_DRIVER_H
#define FAKE_DRIVER_H

#include <core/IO/AudioOutput.h>

#include <chrono>
#include <memory>
#include <thread>

namespace H2Core {

/** Fake audio driver. Used only for profiling and unit tests.
 *
 * \ingroup docCore docAudioDriver docMIDI */
class FakeDriver : Object<FakeDriver>, public AudioOutput
{
	H2_OBJECT(FakeDriver)
public:
	FakeDriver( audioProcessCallback processCallback );
	~FakeDriver();

	virtual int init( unsigned nBufferSize ) override;
	virtual int connect() override;
	virtual void disconnect() override;
	virtual unsigned getBufferSize() override {
		return m_nBufferSize;
	}
	virtual unsigned getSampleRate() override;

	virtual float* getOut_L() override;
	virtual float* getOut_R() override;

		const std::chrono::duration<float>& getProcessInterval() const;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
		static void processCallback( void* pInstance );

		bool m_bActive;
		std::shared_ptr< std::thread > m_pCallbackHandler;

		/** Time that needs to elapse between two runs of the processCallback in
		 * order to properly simulate an audio driver. */
		std::chrono::duration<float> m_processInterval;
		/** Point in time the last run of the process callback did finish. */
		std::chrono::time_point< std::chrono::high_resolution_clock > m_lastRun;

		audioProcessCallback m_processCallback;
		unsigned m_nBufferSize;
		unsigned m_nSampleRate;
		float* m_pOut_L;
		float* m_pOut_R;

};

inline const std::chrono::duration<float>& FakeDriver::getProcessInterval() const {
	return m_processInterval;
}

};

#endif
