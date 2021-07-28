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

#ifndef FAKE_DRIVER_H
#define FAKE_DRIVER_H

#include <core/IO/AudioOutput.h>
#include <inttypes.h>

namespace H2Core
{
/**
 * Fake audio driver. Used only for profiling.
 */
class FakeDriver : public AudioOutput
{
	H2_OBJECT
public:
	FakeDriver( audioProcessCallback processCallback );
	~FakeDriver();

	int init( unsigned nBufferSize );
	int connect();
	void disconnect();
	unsigned getBufferSize() {
		return m_nBufferSize;
	}
	unsigned getSampleRate();

	float* getOut_L();
	float* getOut_R();

	void processCallback();

private:
	audioProcessCallback m_processCallback;
	unsigned m_nBufferSize;
	float* m_pOut_L;
	float* m_pOut_R;

};


};

#endif
