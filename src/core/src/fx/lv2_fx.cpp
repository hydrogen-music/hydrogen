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

#include <hydrogen/fx/LV2FX.h>

#if defined(H2CORE_HAVE_LILV) || _DOXYGEN_
#include <hydrogen/Preferences.h>

#include <QDir>

#include <algorithm>
#include <cstdio>
#include <vector>
using namespace std;

namespace H2Core
{

///////////////////


const char* Lv2FX::__class_name = "Lv2FX";

// ctor
Lv2FX::Lv2FX( LilvWorld* pWorld, const LilvPlugin* pPlugin, long nSampleRate)
		: H2FX( __class_name )
//, m_nBufferSize( 0 )
		, m_bActivated( false )
		, m_sURI("none")
		, m_pLilvInstance( nullptr )
		, m_handle( nullptr )
		, m_nICPorts( 0 )
		, m_nOCPorts( 0 )
		, m_nIAPorts( 0 )
		, m_nOAPorts( 0 )
		, m_nAudioInput1Idx( -1 )
		, m_nAudioInput2Idx( -1 )
		, m_nAudioOutput1Idx( -1 )
		, m_nAudioOutput2Idx( -1 )
		, m_pPlugin( pPlugin )
		, m_pWorld( pWorld )
{
	const uint32_t n_ports = lilv_plugin_get_num_ports(pPlugin);
	
	m_fDefaultValues.resize( n_ports );
	
	std::cout << "LV2 plugin has n ports" << n_ports <<  std::endl;
	
	LilvNode* lv2_InputPort    = lilv_new_uri(pWorld, LV2_CORE__InputPort);
	LilvNode* lv2_OutputPort   = lilv_new_uri(pWorld, LV2_CORE__OutputPort);
	LilvNode* lv2_AudioPort    = lilv_new_uri(pWorld, LV2_CORE__AudioPort);
	LilvNode* lv2_ControlPort  = lilv_new_uri(pWorld, LV2_CORE__ControlPort);
	
	for (uint32_t i = 0; i < n_ports; ++i) {
		const LilvPort* lport = lilv_plugin_get_port_by_index(pPlugin, i);
		
		/* Check if port is an input or output */
		if (lilv_port_is_a(pPlugin, lport, lv2_InputPort)) {
			std::cout << "Port " << i << " is a input"<< std::endl;
		} else if (lilv_port_is_a(pPlugin, lport, lv2_OutputPort)) {
			std::cout << "Port " << i << " is a output"<< std::endl;
		}
		
		if(lilv_port_is_a(pPlugin, lport, lv2_ControlPort)) {
			std::cout << "Port " << i << " is a control port"<< std::endl;
		} else if(lilv_port_is_a(pPlugin, lport, lv2_AudioPort)) {
			std::cout << "Port " << i << " is a audio port"<< std::endl;
			/* Check if port is an input or output */
			if (lilv_port_is_a(pPlugin, lport, lv2_InputPort)) {
				if( m_nIAPorts == 0) {
					m_nAudioInput1Idx = i;
				} else if( m_nIAPorts == 1 ) {
					m_nAudioInput2Idx = i;
				} else {
					//This should never happen! 
					assert(nullptr);
				}
				
				m_nIAPorts++;
			} else if (lilv_port_is_a(pPlugin, lport, lv2_OutputPort)) {
				if( m_nOAPorts == 0) {
					m_nAudioOutput1Idx = i;
				} else if( m_nOAPorts == 1 ) {
					m_nAudioOutput2Idx = i;
				} else {
					//This should never happen! 
					assert(nullptr);
				}
				m_nOAPorts++;
			}
		}
	}
	
	lilv_node_free(lv2_ControlPort);
	lilv_node_free(lv2_AudioPort);
	lilv_node_free(lv2_OutputPort);
	lilv_node_free(lv2_InputPort);
	
	m_pLilvInstance = lilv_plugin_instantiate(pPlugin, nSampleRate, nullptr);

	m_pBuffer_L = new float[MAX_BUFFER_SIZE] ;
	m_pBuffer_R = new float[MAX_BUFFER_SIZE];

	// Touch all the memory (is this really necessary?)
	for ( unsigned i = 0; i < MAX_BUFFER_SIZE; ++i ) {
		m_pBuffer_L[ i ] = 0;
		m_pBuffer_R[ i ] = 0;
	}

}


// dtor
Lv2FX::~Lv2FX()
{
	ERRORLOG(QString("Destroy plugin").arg(m_sURI));
	
	lilv_instance_free( m_pLilvInstance );
	
	delete[] m_pBuffer_L;
	delete[] m_pBuffer_R;
}

Lv2FX* Lv2FX::isLv2FX() {
	return this;
}


// Static
Lv2FX* Lv2FX::load( const QString& sPluginURI, long nSampleRate )
{
	LilvWorld*         world;
	const LilvPlugin*  plugin;
	Lv2FX*             pFx;


	world =	lilv_world_new();
	lilv_world_load_all(world);
	const LilvPlugins* allPlugins = lilv_world_get_all_plugins(world);
	std::cout << "LV2 plugins: " << (int) lilv_plugins_size(allPlugins) << (int) 5 << std::endl;
	
	// Stereo: http://gareus.org/oss/lv2/meters#VUstereo
	// "http://plugin.org.uk/swh-plugins/plate"
	QByteArray array = sPluginURI.toLocal8Bit();
	char* buffer = array.data();
	LilvNode* uri = lilv_new_uri(world, buffer);
	
	if (!uri) {
		std::cout << "LV2 plugins: load failed" << std::endl;
		return nullptr;
	}
	

	plugin = lilv_plugins_get_by_uri (allPlugins, uri);
	lilv_node_free(uri);
	assert(plugin);
	
	pFx = new Lv2FX(world, plugin, nSampleRate);
	
	return pFx;
}



void Lv2FX::connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R )
{
	std::cout <<  "[connectAudioPorts]" << std::endl;
	const uint32_t n_ports = lilv_plugin_get_num_ports(m_pPlugin);
	
	LilvNode* lv2_InputPort    = lilv_new_uri(m_pWorld, LV2_CORE__InputPort);
	LilvNode* lv2_OutputPort   = lilv_new_uri(m_pWorld, LV2_CORE__OutputPort);
	LilvNode* lv2_AudioPort    = lilv_new_uri(m_pWorld, LV2_CORE__AudioPort);
	LilvNode* lv2_ControlPort  = lilv_new_uri(m_pWorld, LV2_CORE__ControlPort);
	
	float * a = new float;
	float * b = new float;
	float * c = new float;
	*a = 0.0f;
	*b = 0.1f;
	*c = 0.1f;
	
	lilv_instance_connect_port( m_pLilvInstance, 0, a);
	lilv_instance_connect_port( m_pLilvInstance, 1, b);
	lilv_instance_connect_port( m_pLilvInstance, 2, c);
	
	

	std::cout << "Audio in1 is " << m_nAudioInput1Idx << std::endl;
	std::cout << "Audio in2 is " << m_nAudioInput1Idx << std::endl;
	std::cout << "Audio out1 is " << m_nAudioOutput1Idx << std::endl;
	std::cout << "Audio out2 is " << m_nAudioOutput1Idx << std::endl;
	
	lilv_instance_connect_port( m_pLilvInstance, 3, pIn_L);
	lilv_instance_connect_port( m_pLilvInstance, 4, pOut_L);
	
	/*
	float OutBuf = 0.0;
	lilv_instance_connect_port( m_pLilvInstance, 4, &OutBuf);
	*/
	
	

/*
	unsigned nAIConn = 0;
	unsigned nAOConn = 0;
	for ( unsigned nPort = 0; nPort < m_d->PortCount; nPort++ ) {
		LADSPA_PortDescriptor pd = m_d->PortDescriptors[ nPort ];
		if ( LADSPA_IS_CONTROL_INPUT( pd ) ) {
		} else if ( LADSPA_IS_CONTROL_OUTPUT( pd ) ) {
		} else if ( LADSPA_IS_AUDIO_INPUT( pd ) ) {
			if ( nAIConn == 0 ) {
				m_d->connect_port( m_handle, nPort, pIn_L );
				//infoLog( "connect input port (L): " + string( m_d->PortNames[ nPort ] ) );
			} else if ( nAIConn == 1 ) {
				m_d->connect_port( m_handle, nPort, pIn_R );
				//infoLog( "connect input port (R): " + string( m_d->PortNames[ nPort ] ) );
			} else {
				ERRORLOG( "too many input ports.." );
			}
			nAIConn++;
		} else if ( LADSPA_IS_AUDIO_OUTPUT( pd ) ) {
			if ( nAOConn == 0 ) {
				m_d->connect_port( m_handle, nPort, pOut_L );
				//infoLog( "connect output port (L): " + string( m_d->PortNames[ nPort ] ) );
			} else if ( nAOConn == 1 ) {
				m_d->connect_port( m_handle, nPort, pOut_R );
				//infoLog( "connect output port (R): " + string( m_d->PortNames[ nPort ] ) );
			} else {
				ERRORLOG( "too many output ports.." );
			}
			nAOConn++;
		} else {
			ERRORLOG( "unknown port" );
		}
	}
	*/
}



void Lv2FX::processFX( unsigned nFrames )
{
//	infoLog( "[LadspaFX::applyFX()]" );
	
	if( m_bActivated ) {
		lilv_instance_run( m_pLilvInstance, nFrames );
		//	std::cout <<  "[ProcessFX: ]" << nFrames << ":" << MAX_BUFFER_SIZE <<   std::endl;	
	}
}

void Lv2FX::activate()
{
	lilv_instance_activate( m_pLilvInstance );
	m_bActivated = true;
}


void Lv2FX::deactivate()
{
	lilv_instance_deactivate( m_pLilvInstance );
	m_bActivated = false;
}

};


#endif // H2CORE_HAVE_LADSPA
