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
#include <inttypes.h>

namespace H2Core
{
/**
 * Fake audio driver. Used only for profiling.
 */
/** \ingroup docCore docAudioDriver */
/** \ingroup docCore docMIDI */
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

	void processCallback();

private:
	audioProcessCallback m_processCallback;
	unsigned m_nBufferSize;
	unsigned m_nSampleRate;
	float* m_pOut_L;
	float* m_pOut_R;

};


};

#endif
