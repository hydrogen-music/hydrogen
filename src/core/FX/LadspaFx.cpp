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

#include <core/FX/LadspaFX.h>

#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_
#include <core/Hydrogen.h>
#include <core/Basics/Song.h>

#include <QDir>

#define LADSPA_IS_CONTROL_INPUT(x) (LADSPA_IS_PORT_INPUT(x) && LADSPA_IS_PORT_CONTROL(x))
#define LADSPA_IS_AUDIO_INPUT(x) (LADSPA_IS_PORT_INPUT(x) && LADSPA_IS_PORT_AUDIO(x))
#define LADSPA_IS_CONTROL_OUTPUT(x) (LADSPA_IS_PORT_OUTPUT(x) && LADSPA_IS_PORT_CONTROL(x))
#define LADSPA_IS_AUDIO_OUTPUT(x) (LADSPA_IS_PORT_OUTPUT(x) && LADSPA_IS_PORT_AUDIO(x))

namespace H2Core
{

LadspaFXGroup::LadspaFXGroup( const QString& sName )
{
//	infoLog( "INIT - " + sName );
	m_sName = sName;
}


LadspaFXGroup::~LadspaFXGroup()
{
//	infoLog( "DESTROY - " + m_sName );

	for ( int i = 0; i < ( int )m_childGroups.size(); ++i ) {
		delete m_childGroups[ i ];
	}
}



void LadspaFXGroup::clear() {
	m_childGroups.clear();
	m_ladspaList.clear();
	Hydrogen::get_instance()->setIsModified( true );
}

void LadspaFXGroup::addLadspaInfo( LadspaFXInfo *pInfo )
{
	m_ladspaList.push_back( pInfo );
	Hydrogen::get_instance()->setIsModified( true );
}


void LadspaFXGroup::addChild( LadspaFXGroup *pChild )
{
	m_childGroups.push_back( pChild );
	Hydrogen::get_instance()->setIsModified( true );
}

bool LadspaFXGroup::alphabeticOrder( LadspaFXGroup* a, LadspaFXGroup* b )
{
	return ( a->getName() < b->getName() );
}

void LadspaFXGroup::sort()
{
	std::sort( m_ladspaList.begin(), m_ladspaList.end(), LadspaFXInfo::alphabeticOrder );
	std::sort( m_childGroups.begin(), m_childGroups.end(), LadspaFXGroup::alphabeticOrder );
	Hydrogen::get_instance()->setIsModified( true );
}



////////////////


LadspaFXInfo::LadspaFXInfo( const QString& sName )
{
//	infoLog( "INIT - " + sName );
	m_sFilename = "";
	m_sLabel = "";
	m_sName = sName;
	m_nICPorts = 0;
	m_nOCPorts = 0;
	m_nIAPorts = 0;
	m_nOAPorts = 0;
}


LadspaFXInfo::~LadspaFXInfo()
{
//	infoLog( "DESTROY " + m_sName );
}

bool LadspaFXInfo::alphabeticOrder( LadspaFXInfo* a, LadspaFXInfo* b )
{
	return ( a->m_sName < b->m_sName );
}


///////////////////


// ctor
LadspaFX::LadspaFX( const QString& sLibraryPath, const QString& sPluginLabel )
//, m_nBufferSize( 0 )
		: m_pBuffer_L( nullptr )
		, m_pBuffer_R( nullptr )
		, m_pluginType( UNDEFINED )
		, m_bEnabled( false )
		, m_bActivated( false )
		, m_sLabel( sPluginLabel )
		, m_sLibraryPath( sLibraryPath )
		, m_pLibrary( nullptr )
		, m_d( nullptr )
		, m_handle( nullptr )
		, m_fVolume( 1.0f )
		, m_nICPorts( 0 )
		, m_nOCPorts( 0 )
		, m_nIAPorts( 0 )
		, m_nOAPorts( 0 )
{
	INFOLOG( QString( "INIT - %1 - %2" ).arg( sLibraryPath ).arg( sPluginLabel ) );


	m_pBuffer_L = new float[MAX_BUFFER_SIZE];
	m_pBuffer_R = new float[MAX_BUFFER_SIZE];


	// Touch all the memory (is this really necessary?)
	for ( unsigned i = 0; i < MAX_BUFFER_SIZE; ++i ) {
		m_pBuffer_L[ i ] = 0;
		m_pBuffer_R[ i ] = 0;
	}

}


// dtor
LadspaFX::~LadspaFX()
{
	// dealloca il plugin
	INFOLOG( QString( "DESTROY - %1 - %2" ).arg( m_sLibraryPath ).arg( m_sLabel ) );

	if ( m_d ) {
		/*
		if ( m_d->deactivate ) {
			if ( m_handle ) {
				INFOLOG( "deactivate" );
				m_d->deactivate( m_handle );
			}
		}*/
		deactivate();

		if ( m_d->cleanup ) {
			if ( m_handle ) {
				INFOLOG( "Cleanup" );
				Logger::CrashContext cc( &m_sLibraryPath );
				m_d->cleanup( m_handle );
			}
		}
	}
	delete m_pLibrary;

	for ( unsigned i = 0; i < inputControlPorts.size(); i++ ) {
		delete inputControlPorts[i];
	}
	for ( unsigned i = 0; i < outputControlPorts.size(); i++ ) {
		delete outputControlPorts[i];
	}

	delete[] m_pBuffer_L;
	delete[] m_pBuffer_R;
}


void LadspaFX::setPluginName( const QString& sName ) {
	m_sName = sName;
	
	if ( Hydrogen::get_instance()->getSong() != nullptr ) {
		Hydrogen::get_instance()->setIsModified( true );
	}
}
void LadspaFX::setEnabled( bool value ) {
	m_bEnabled = value;
	
	if ( Hydrogen::get_instance()->getSong() != nullptr ) {
		Hydrogen::get_instance()->setIsModified( true );
	}
}


// Static
LadspaFX* LadspaFX::load( const QString& sLibraryPath, const QString& sPluginLabel, long nSampleRate )
{
	LadspaFX* pFX = new LadspaFX( sLibraryPath, sPluginLabel );

	_INFOLOG( "INIT - " + sLibraryPath + " - " + sPluginLabel );

	Logger::CrashContext ctx( QString( "Initialising LADSPA plugin " ) + sLibraryPath + " - " + sPluginLabel);

	pFX->m_pLibrary = new QLibrary( sLibraryPath );
	LADSPA_Descriptor_Function desc_func = ( LADSPA_Descriptor_Function )pFX->m_pLibrary->resolve( "ladspa_descriptor" );
	if ( desc_func == nullptr ) {
		_ERRORLOG( "Error loading the library. (" + sLibraryPath + ")" );
		delete pFX;
		return nullptr;
	}
	if ( desc_func ) {
		for ( unsigned i = 0; ( pFX->m_d = desc_func( i ) ) != nullptr; i++ ) {
			QString sName = QString::fromLocal8Bit(pFX->m_d->Name);
			QString sLabel = QString::fromLocal8Bit(pFX->m_d->Label);

			if ( sLabel != sPluginLabel ) {
				continue;
			}
			pFX->setPluginName( sName );

			for ( unsigned j = 0; j < pFX->m_d->PortCount; j++ ) {
				LADSPA_PortDescriptor pd = pFX->m_d->PortDescriptors[j];
				if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
					pFX->m_nICPorts++;
				} else if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
					pFX->m_nIAPorts++;
				} else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
					pFX->m_nOCPorts++;
				} else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
					pFX->m_nOAPorts++;
				} else {
					_ERRORLOG( "Unknown port type" );
				}
			}
			break;
		}
	} else {
		_ERRORLOG( "Error in dlsym" );
		delete pFX;
		return nullptr;
	}

	if ( ( pFX->m_nIAPorts == 2 ) && ( pFX->m_nOAPorts == 2 ) ) {		// Stereo plugin
		pFX->m_pluginType = STEREO_FX;
	} else if ( ( pFX->m_nIAPorts == 1 ) && ( pFX->m_nOAPorts == 1 ) ) {	// Mono plugin
		pFX->m_pluginType = MONO_FX;
	} else {
		_ERRORLOG( "Wrong number of ports" );
		_ERRORLOG( QString( "in audio = %1" ).arg( pFX->m_nIAPorts ) );
		_ERRORLOG( QString( "out audio = %1" ).arg( pFX->m_nOAPorts ) );
	}

	//pFX->infoLog( "[LadspaFX::load] instantiate " + pFX->getPluginName() );
	pFX->m_handle = pFX->m_d->instantiate( pFX->m_d, nSampleRate );

	for ( unsigned nPort = 0; nPort < pFX->m_d->PortCount; nPort++ ) {
		LADSPA_PortDescriptor pd = pFX->m_d->PortDescriptors[ nPort ];

		if ( LADSPA_IS_CONTROL_INPUT( pd ) ) {
			QString sName = QString::fromLocal8Bit(pFX->m_d->PortNames[ nPort ]);
			float fMin = 0.0;
			float fMax = 0.0;
			float fDefault = 0.0;
			bool isToggle = false;
			bool isInteger = false;

			LADSPA_PortRangeHint rangeHints = pFX->m_d->PortRangeHints[ nPort ];
			if ( LADSPA_IS_HINT_BOUNDED_BELOW( rangeHints.HintDescriptor ) ) {
				fMin = ( pFX->m_d->PortRangeHints[ nPort ] ).LowerBound;
			}
			if ( LADSPA_IS_HINT_BOUNDED_ABOVE( rangeHints.HintDescriptor ) ) {
				fMax = ( pFX->m_d->PortRangeHints[ nPort ] ).UpperBound;
			}
			if ( LADSPA_IS_HINT_TOGGLED( rangeHints.HintDescriptor ) ) {
				isToggle = true;

				// this way the fader will act like a toggle (0, 1)
				isInteger = true;
				fMin = 0.0;
				fMax = 1.0;
			}
			if ( LADSPA_IS_HINT_SAMPLE_RATE( rangeHints.HintDescriptor ) ) {
				_WARNINGLOG( "samplerate hint not implemented yet" );
			}
			if ( LADSPA_IS_HINT_LOGARITHMIC( rangeHints.HintDescriptor ) ) {
				_WARNINGLOG( "logarithmic hint not implemented yet" );
			}
			if ( LADSPA_IS_HINT_INTEGER( rangeHints.HintDescriptor ) ) {
				isInteger = true;
			}
			if ( LADSPA_IS_HINT_HAS_DEFAULT( rangeHints.HintDescriptor ) ) {
				if ( LADSPA_IS_HINT_DEFAULT_MINIMUM( rangeHints.HintDescriptor ) ) {
					fDefault = fMin;
				}
				if ( LADSPA_IS_HINT_DEFAULT_LOW( rangeHints.HintDescriptor ) ) {
					// TODO: bisogna gestire diversamente se viene specificato di usare la scala logaritmica
					fDefault = ( fMin * 0.75 + fMax * 0.25 );
				}
				if ( LADSPA_IS_HINT_DEFAULT_MIDDLE( rangeHints.HintDescriptor ) ) {
					fDefault = ( fMax - fMin ) / 2.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_HIGH( rangeHints.HintDescriptor ) ) {
					// TODO: bisogna gestire diversamente se viene specificato di usare la scala logaritmica
					fDefault = ( fMin * 0.25 + fMax * 0.75 );
				}
				if ( LADSPA_IS_HINT_DEFAULT_MAXIMUM( rangeHints.HintDescriptor ) ) {
					fDefault = fMax;
				}
				if ( LADSPA_IS_HINT_DEFAULT_0( rangeHints.HintDescriptor ) ) {
					fDefault = 0.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_1( rangeHints.HintDescriptor ) ) {
					fDefault = 1.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_100( rangeHints.HintDescriptor ) ) {
					fDefault = 100.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_440( rangeHints.HintDescriptor ) ) {
					fDefault = 440.0;
				}
			}

			LadspaControlPort* pControl = new LadspaControlPort();
			pControl->sName = sName;
			pControl->fLowerBound = fMin;
			pControl->fUpperBound = fMax;
			pControl->fControlValue = fDefault;
			pControl->fDefaultValue = fDefault;
			pControl->isToggle = isToggle;
			pControl->m_bIsInteger = isInteger;

			_INFOLOG( QString( "Input control port\t[%1]\tmin=%2,\tmax=%3,\tcontrolValue=%4" ).arg( sName ).arg( fMin ).arg( fMax ).arg( pControl->fControlValue ) );

			pFX->inputControlPorts.push_back( pControl );
			pFX->m_d->connect_port( pFX->m_handle, nPort, &( pControl->fControlValue ) );
		} else if ( LADSPA_IS_CONTROL_OUTPUT( pd ) ) {
			QString sName = QString::fromLocal8Bit(pFX->m_d->PortNames[ nPort ]);
			float fMin = 0.0;
			float fMax = 0.0;
			float fDefault = 0.0;

			LADSPA_PortRangeHint rangeHints = pFX->m_d->PortRangeHints[ nPort ];
			if ( LADSPA_IS_HINT_BOUNDED_BELOW( rangeHints.HintDescriptor ) ) {
				fMin = ( pFX->m_d->PortRangeHints[ nPort ] ).LowerBound;
			}
			if ( LADSPA_IS_HINT_BOUNDED_ABOVE( rangeHints.HintDescriptor ) ) {
				fMax = ( pFX->m_d->PortRangeHints[ nPort ] ).UpperBound;
			}

			/*			LadspaControlPort* pControl = new LadspaControlPort();
						pControl->sName = pFX->m_d->PortNames[ nPort ];
						pControl->fLowerBound = ( pFX->m_d->PortRangeHints[ nPort ] ).LowerBound;
						pControl->fUpperBound = ( pFX->m_d->PortRangeHints[ nPort ] ).UpperBound;
						pControl->fControlValue = pControl->fUpperBound / 2.0;
			*/
			// always middle
			fDefault = ( fMax - fMin ) / 2.0;

			LadspaControlPort* pControl = new LadspaControlPort();
			pControl->sName = sName;
			pControl->fLowerBound = fMin;
			pControl->fUpperBound = fMax;
			pControl->fControlValue = fDefault;
			pControl->fDefaultValue = fDefault;
			//pFX->infoLog( "[LadspaFX::load] Output control port\t[" + sName + "]\tmin=" + to_string(fMin) + ",\tmax=" + to_string(fMax) + ",\tcontrolValue=" + to_string(pControl->fControlValue) );

			pFX->outputControlPorts.push_back( pControl );
			pFX->m_d->connect_port( pFX->m_handle, nPort, &( pControl->fControlValue ) );
		} else if ( LADSPA_IS_AUDIO_INPUT( pd ) ) {
		} else if ( LADSPA_IS_AUDIO_OUTPUT( pd ) ) {
		} else {
			_ERRORLOG( "unknown port" );
		}
	}
	
	if ( Hydrogen::get_instance()->getSong() != nullptr ) {
		Hydrogen::get_instance()->setIsModified( true );
	}
	return pFX;
}



void LadspaFX::connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R )
{
	INFOLOG( "[connectAudioPorts]" );
	Logger::CrashContext ctx( QString( "Connecting ports on LADSPA plugin " ) + m_sLibraryPath + " - " + m_sLabel);
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
}



void LadspaFX::processFX( unsigned nFrames )
{
//	infoLog( "[LadspaFX::applyFX()]" );
	if( m_bActivated ) {
		Logger::CrashContext cc( &m_sLibraryPath );
		m_d->run( m_handle, nFrames );
	}
}

void LadspaFX::activate()
{
	if ( m_d->activate ) {
		INFOLOG( "activate " + getPluginName() );
		m_bActivated = true;
		Logger::CrashContext cc( &m_sLibraryPath );
		m_d->activate( m_handle );
		Hydrogen::get_instance()->setIsModified( true );
	}
}


void LadspaFX::deactivate()
{
	if ( m_d->deactivate && m_bActivated ) {
		INFOLOG( "deactivate " + getPluginName() );
		m_bActivated = false;
		Logger::CrashContext cc( &m_sLibraryPath );
		m_d->deactivate( m_handle );
		Hydrogen::get_instance()->setIsModified( true );
	}
}


void LadspaFX::setVolume( float fValue )
{
	if ( fValue > 2.0 ) {
		fValue = 2.0;
	} else if ( fValue < 0.0 ) {
		fValue = 0.0;
	}
	m_fVolume = fValue;

	if ( Hydrogen::get_instance()->getSong() != nullptr ) {
		Hydrogen::get_instance()->setIsModified( true );
	}
}


};


#endif // H2CORE_HAVE_LADSPA
