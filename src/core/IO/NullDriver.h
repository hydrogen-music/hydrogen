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


#ifndef NULL_AUDIO_DRIVER_H
#define NULL_AUDIO_DRIVER_H

#include <core/IO/AudioDriver.h>

#include <inttypes.h>

namespace H2Core
{

typedef int  ( *audioProcessCallback )( uint32_t, void * );

/** \ingroup docCore docAudioDriver */
class NullDriver : public Object<NullDriver>, public AudioDriver
{
	H2_OBJECT(NullDriver)
public:
	NullDriver( audioProcessCallback processCallback );
	~NullDriver();

	int init( unsigned nBufferSize ) override; 
	int connect() override;
	void disconnect() override;
	unsigned getBufferSize() override;
	unsigned getSampleRate() override;

	float* getOut_L() override;
	float* getOut_R() override;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
};

};

#endif

