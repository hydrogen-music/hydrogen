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

namespace H2Core
{

///////////////////

const char* LV2FXInfo::__class_name = "LV2FXInfo";

LV2FXInfo::LV2FXInfo( const QString& sName )
		: H2FXInfo( sName )
{
//	infoLog( "INIT - " + sName );
}


LV2FXInfo::~LV2FXInfo()
{
//	infoLog( "DESTROY " + m_sName );
}

LadspaFXInfo* LV2FXInfo::isLadspaFXInfo()
{
	return nullptr;
}

LV2FXInfo* LV2FXInfo::isLV2FXInfo()
{
	return this;
}

///////////////////

const char* Lv2FX::__class_name = "Lv2FX";

// ctor
Lv2FX::Lv2FX( LilvWorld* pWorld, const LilvPlugin* pPlugin, long nSampleRate)
		: H2FX( __class_name )
//, m_nBufferSize( 0 )
		, m_bActivated( false )
		, m_sURI("none")
		, m_pLilvInstance( nullptr )
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
		
	INFOLOG(QString("LV2 plugin has n ports: %1").arg( n_ports ));
	
	LilvNode* lv2_InputPort    = lilv_new_uri(pWorld, LV2_CORE__InputPort);
	LilvNode* lv2_OutputPort   = lilv_new_uri(pWorld, LV2_CORE__OutputPort);
	LilvNode* lv2_AudioPort    = lilv_new_uri(pWorld, LV2_CORE__AudioPort);
	LilvNode* lv2_ControlPort  = lilv_new_uri(pWorld, LV2_CORE__ControlPort);
	
	float* fDefaultValues = (float*)calloc(n_ports, sizeof(float));
	float* fMinValues = (float*)calloc(n_ports, sizeof(float));
	float* fMaxValues = (float*)calloc(n_ports, sizeof(float));
	
	lilv_plugin_get_port_ranges_float(pPlugin, fMinValues, fMaxValues, fDefaultValues);

	bool bIsInputPort = false;
	for (uint i = 0; i < n_ports; ++i) {
		const LilvPort* lport = lilv_plugin_get_port_by_index(pPlugin, i);
		
		/* Check if port is an input or output */
		if (lilv_port_is_a(pPlugin, lport, lv2_InputPort)) {
			bIsInputPort = true;
		} else if (lilv_port_is_a(pPlugin, lport, lv2_OutputPort)) {
			bIsInputPort =  false;
		}
		
		if(lilv_port_is_a(pPlugin, lport, lv2_ControlPort)) {
			LilvNode* pName = lilv_port_get_name(pPlugin, lport);
			
			LadspaControlPort* pControl = new LadspaControlPort();
			pControl->sName = lilv_node_as_string(pName);
			pControl->fLowerBound = fMinValues[i];
			pControl->fUpperBound = fMaxValues[i];
			pControl->fControlValue = fDefaultValues[i];
			pControl->fDefaultValue = fDefaultValues[i];
			pControl->isToggle = false;
			pControl->m_bIsInteger = false;
			pControl->nPortIndex = i;
			
			if( bIsInputPort ) {
				inputControlPorts.push_back( pControl );
			} else {
				outputControlPorts.push_back( pControl );
			}
			
			m_fDefaultValues.emplace_back(std::make_pair(i, fDefaultValues[i]));
			
			lilv_node_free(pName);
		} else if(lilv_port_is_a(pPlugin, lport, lv2_AudioPort)) {
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
	
	assert(m_pLilvInstance);

	m_pBuffer_L = new float[MAX_BUFFER_SIZE] ;
	m_pBuffer_R = new float[MAX_BUFFER_SIZE];

	// Touch all the memory (is this really necessary?)
	for ( unsigned i = 0; i < MAX_BUFFER_SIZE; ++i ) {
		m_pBuffer_L[ i ] = 0;
		m_pBuffer_R[ i ] = 0;
	}
	
	free(fMinValues);
	free(fMaxValues);
	free(fDefaultValues);
}


// dtor
Lv2FX::~Lv2FX()
{
	ERRORLOG(QString("Destroy plugin %1").arg(m_sURI));
	
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
	LilvWorld*			pLilvWorld;
	const LilvPlugin*	pLilvPlugin;
	Lv2FX*				pFx;

	pLilvWorld =	lilv_world_new();
	lilv_world_load_all(pLilvWorld);
	const LilvPlugins* allPlugins = lilv_world_get_all_plugins(pLilvWorld);
	
	QByteArray array = sPluginURI.toLocal8Bit();
	char* pBuffer = array.data();
	LilvNode* pLilvUri = lilv_new_uri(pLilvWorld, pBuffer);
	
	if (!pLilvUri) {
		ERRORLOG( "Failed to load LV2 environment" )
		return nullptr;
	}

	pLilvPlugin = lilv_plugins_get_by_uri (allPlugins, pLilvUri);
	assert(pLilvPlugin);
	
	pFx = new Lv2FX(pLilvWorld, pLilvPlugin, nSampleRate);
	
	LilvNode* pPluginNameNode = lilv_plugin_get_name(pLilvPlugin);
	pFx->setPluginName( QString::fromLocal8Bit( lilv_node_as_string(pPluginNameNode) ));
	pFx->setPluginLabel( sPluginURI );
	
	lilv_node_free(pLilvUri);
	lilv_node_free(pPluginNameNode);
	
	return pFx;
}



void Lv2FX::connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R )
{
	std::cout <<  "[connectAudioPorts]" << std::endl;
	
	std::cout << "Audio in1 is " << m_nAudioInput1Idx << std::endl;
	std::cout << "Audio in2 is " << m_nAudioInput2Idx << std::endl;
	std::cout << "Audio out1 is " << m_nAudioOutput1Idx << std::endl;
	std::cout << "Audio out2 is " << m_nAudioOutput2Idx << std::endl;
	
	if( m_nAudioInput1Idx >= 0 && m_nAudioInput2Idx < 0 ) {
		//mono
		lilv_instance_connect_port( m_pLilvInstance, m_nAudioInput1Idx, pIn_L );
		lilv_instance_connect_port( m_pLilvInstance, m_nAudioOutput1Idx, pOut_L );
		
		m_pluginType = MONO_FX;
	} else if( m_nAudioInput1Idx >= 0 && m_nAudioInput2Idx >= 0 ) {
		//stereo
		lilv_instance_connect_port( m_pLilvInstance, m_nAudioInput1Idx, pIn_L );
		lilv_instance_connect_port( m_pLilvInstance, m_nAudioOutput1Idx, pOut_L );
		
		lilv_instance_connect_port( m_pLilvInstance, m_nAudioInput2Idx, pIn_R );
		lilv_instance_connect_port( m_pLilvInstance, m_nAudioOutput2Idx, pOut_R );
		
		m_pluginType = STEREO_FX;
	} else {
		//should never happen..		
		assert(nullptr);
	}
	
	for( auto& indexPair : m_fDefaultValues ) {
		INFOLOG(QString("Connecting port %1 to control port value: %2").arg(indexPair.first).arg(indexPair.second));
		lilv_instance_connect_port( m_pLilvInstance, indexPair.first, &(indexPair.second) );
	}
	
	for( auto& controlPort : inputControlPorts) {
		lilv_instance_connect_port( m_pLilvInstance, controlPort->nPortIndex, &( controlPort->fControlValue ));
	}
	
	for( auto& controlPort : outputControlPorts) {
		lilv_instance_connect_port( m_pLilvInstance, controlPort->nPortIndex, &( controlPort->fControlValue ));
	}
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
	if( m_bActivated ) {
		lilv_instance_deactivate( m_pLilvInstance );
		m_bActivated = false;	
	}
}

};


#endif // H2CORE_HAVE_LADSPA
