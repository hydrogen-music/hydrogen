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

#include <hydrogen/IO/LV2AudioDriver.h>

#include <fcntl.h>
#include <hydrogen/Preferences.h>


namespace H2Core
{
const char* LV2AudioDriver::__class_name = "LV2AudioDriver";
LV2AudioDriver::LV2AudioDriver(audioProcessCallback processCallback)
	:	AudioOutput( __class_name ),
		m_outL(nullptr),
		m_outR(nullptr)
{
	___ERRORLOG("INIT");
	m_callback = processCallback;
}


LV2AudioDriver::~LV2AudioDriver()
{
	delete []m_outL;
	delete []m_outR;
}


int LV2AudioDriver::init( unsigned nBufferSize )
{
	delete []m_outL;
	delete []m_outR;
	
	m_buffer_size = nBufferSize;
	m_sample_rate = Preferences::get_instance()->m_nSampleRate;
	
	m_outL = new float[m_buffer_size];
	m_outR = new float[m_buffer_size];
	
	return 0;
}

void LV2AudioDriver::handleData( int nSamples, float* pBufferL, float* pBufferR )
{
	m_callback( nSamples, nullptr );

	for ( int i = 0; i < nSamples; ++i ) {
			pBufferL[i] = m_outL[ i ];
			pBufferR[i] = m_outR[ i ];
	}
}

int LV2AudioDriver::connect()
{
	return 0;
}


void LV2AudioDriver::disconnect()
{

}


unsigned LV2AudioDriver::getBufferSize()
{
	return m_buffer_size;
}


unsigned LV2AudioDriver::getSampleRate()
{
	return m_sample_rate;
}


float* LV2AudioDriver::getOut_L()
{
	return m_outL;
}


float* LV2AudioDriver::getOut_R()
{
	return m_outR;
}


void LV2AudioDriver::updateTransportInfo()
{
}


void LV2AudioDriver::play()
{
	m_transport.m_status = TransportInfo::ROLLING;
}


void LV2AudioDriver::stop()
{
	m_transport.m_status = TransportInfo::STOPPED;
}


void LV2AudioDriver::locate( unsigned long nFrame )
{
	m_transport.m_nFrames = nFrame;
}


void LV2AudioDriver::setBpm( float fBPM )
{
	m_transport.m_fBPM = fBPM;
}


} //namespace H2Core

