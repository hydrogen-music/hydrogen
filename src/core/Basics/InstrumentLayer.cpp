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

#include <core/Basics/InstrumentLayer.h>

#include <core/Basics/Instrument.h>
#include <core/Basics/Sample.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/NsmClient.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core
{

InstrumentLayer::InstrumentLayer( std::shared_ptr<Sample> pSample ) :
	m_fStartVelocity( 0.0 ),
	m_fEndVelocity( 1.0 ),
	m_fPitch( 0.0 ),
	m_fGain( 1.0 ),
	m_bIsMuted( false ),
	m_bIsSoloed( false ),
	m_pSample( pSample )
{
	if ( pSample != nullptr ) {
		m_sFallbackSampleFileName = pSample->getFileName();
	}
}

InstrumentLayer::InstrumentLayer( std::shared_ptr<InstrumentLayer> pOther ) : Object( *pOther ),
	m_fStartVelocity( pOther->getStartVelocity() ),
	m_fEndVelocity( pOther->getEndVelocity() ),
	m_fPitch( pOther->getPitch() ),
	m_fGain( pOther->getGain() ),
	m_bIsMuted( pOther->m_bIsMuted ),
	m_bIsSoloed( pOther->m_bIsSoloed ),
	m_pSample( nullptr ),
	m_sFallbackSampleFileName( pOther->m_sFallbackSampleFileName )
{
	if ( pOther->m_pSample != nullptr ) {
		m_pSample = std::make_shared<Sample>( pOther->m_pSample );
	}
}

InstrumentLayer::InstrumentLayer( std::shared_ptr<InstrumentLayer> pOther, std::shared_ptr<Sample> pSample ) : Object( *pOther ),
	m_fStartVelocity( pOther->getStartVelocity() ),
	m_fEndVelocity( pOther->getEndVelocity() ),
	m_fPitch( pOther->getPitch() ),
	m_fGain( pOther->getGain() ),
	m_bIsMuted( pOther->m_bIsMuted ),
	m_bIsSoloed( pOther->m_bIsSoloed ),
	m_pSample( pSample )
{
	if ( pSample != nullptr ) {
		m_sFallbackSampleFileName = pSample->getFileName();
	}
}

InstrumentLayer::~InstrumentLayer()
{
}

void InstrumentLayer::setSample( std::shared_ptr<Sample> pSample )
{
	m_pSample = pSample;

	if ( pSample != nullptr ) {
		m_sFallbackSampleFileName = pSample->getFileName();
	}
}

void InstrumentLayer::setPitch( float fValue )
{
	if ( fValue < Instrument::fPitchMin || fValue > Instrument::fPitchMax ) {
		WARNINGLOG( QString( "Provided pitch out of bound [%1;%2]. Rounding to nearest allowed value." )
					.arg( Instrument::fPitchMin ).arg( Instrument::fPitchMax ) );
	}
	m_fPitch = std::clamp( fValue, Instrument::fPitchMin, Instrument::fPitchMax );
}

void InstrumentLayer::loadSample( float fBpm )
{
	if ( m_pSample != nullptr ) {
		m_pSample->load( fBpm );
	}
}

void InstrumentLayer::unloadSample()
{
	if ( m_pSample != nullptr ) {
		m_pSample->unload();
	}
}

std::shared_ptr<InstrumentLayer> InstrumentLayer::loadFrom(
	const XMLNode& node,
	const QString& sDrumkitPath,
	const QString& sSongPath,
	const License& drumkitLicense,
	bool bSilent )
{
	auto pHydrogen = Hydrogen::get_instance();
	
	const QString sFileName =
		node.read_string( "filename", "", false, false, bSilent );

	QFileInfo filenameInfo( sFileName );
	QString sFilePath;
	if ( filenameInfo.isAbsolute() ) {
		// Samples with absolute filenames are those added using the
		// InstrumentEditor.
		sFilePath = sFileName;
	}
	else {
		// QFileInfo::isRelative() can not be used in here as samples of
		// drumkits within the user or system drumkit folder are stored
		// relatively as well (by saving just the filename).
		if ( ( sFileName.contains( "\\" ) || sFileName.contains( "/" ) ) &&
			 ! sSongPath.isEmpty() ) {
			// Sample path can be stored relative to the .h2song file. This is
			// mainly present to allow for more thorough unit test. It, however,
			// has to be written manually. Hydrogen itself does not store paths
			// relatively (except when under session management) to increase
			// portability.
			QFileInfo songPathInfo( sSongPath );
			sFilePath = songPathInfo.absoluteDir().absoluteFilePath( sFileName );
		}
		else if ( ! sDrumkitPath.isEmpty() ){
			// Plain filenames of samples associated with an installed drumkit.
			QFileInfo drumkitPathInfo( sDrumkitPath );
			if ( drumkitPathInfo.isDir() ) {
				sFilePath = QDir( sDrumkitPath ).absoluteFilePath( sFileName );
			} else {
				// Path to drumkit.xml was entered. Not standard. Probably done
				// manually.
				sFilePath = drumkitPathInfo.absoluteDir().absoluteFilePath(
					sFileName );
			}
		}
	}

	// If the sample still could not be found, this could be e.g. due to an
	// absolute path referencing a sample imported from a session kit - one,
	// which was loaded by the user manually and does not reside in either user
	// or system drumkit folder - or due to a bug like #2174. We give it another
	// try by checking whether the /path/to/<drumkit>/<sample> could refer to
	// the exact same <drumkit>/<sample> in one of our drumkit folders.
	if ( ! Filesystem::file_exists( sFilePath, true ) &&
		 ( sFileName.contains( "/" ) || sFileName.contains( "\\" ) ) ) {
		// We need to ensure we work on a single set of separators without any
		// duplication. This is especially important as songs created on Windows
		// could be loaded on Linux/macOS and vice versa.
		const QString sFileNameCleaned = QString( sFileName )
			.replace( "\\", "/" ).replace( "//", "/" );

		const auto pathSegments = sFileNameCleaned.split( "/" );
		if ( pathSegments.size() > 2 ) {
			const auto sDrumkitSampleSegment = QString( "%1/%2" )
				.arg( pathSegments[ pathSegments.size() - 2 ] )
				.arg( pathSegments[ pathSegments.size() - 1 ] );

			const auto drumkitFolders = pHydrogen->getSoundLibraryDatabase()
				->getDrumkitFolders();
			for ( const auto& ssFolder : drumkitFolders ) {
				const auto sNewPath = QString( "%1/%2" )
					.arg( ssFolder ).arg( sDrumkitSampleSegment );
				if ( Filesystem::file_exists( sNewPath, true ) ) {
					WARNINGLOG( QString( "File [%1] does not exist. Loading similar file [%2] instead." )
								.arg( sFileName ).arg( sNewPath ) );
					sFilePath = sNewPath;
					break;
				}
			}
		}
	}

	std::shared_ptr<Sample> pSample = nullptr;
	if ( ! sFilePath.isEmpty() && Filesystem::file_exists( sFilePath, true ) ) {
		pSample = std::make_shared<Sample>( sFilePath, drumkitLicense );

		const bool bIsModified =
			node.read_bool( "ismodified", false, true, false, true );
		pSample->setIsModified( bIsModified );
	
		if ( bIsModified ) {
		
			Sample::Loops loops;
			loops.mode = Sample::parseLoopMode( node.read_string( "smode", "forward", false, false, bSilent ) );
			loops.start_frame = node.read_int( "startframe", 0, false, false, bSilent );
			loops.loop_frame = node.read_int( "loopframe", 0, false, false, bSilent );
			loops.count = node.read_int( "loops", 0, false, false, bSilent );
			loops.end_frame = node.read_int( "endframe", 0, false, false, bSilent );
			pSample->setLoops( loops );
	
			Sample::Rubberband rubberband;
			rubberband.use = node.read_int( "userubber", 0, false, false, bSilent );
			rubberband.divider = node.read_float( "rubberdivider", 0.0, false, false, bSilent );
			rubberband.c_settings = node.read_int( "rubberCsettings", 1, false, false, bSilent );
			rubberband.pitch = node.read_float( "rubberPitch", 0.0, false, false, bSilent );

			// Check whether the rubberband executable is present.
			if ( ! Filesystem::file_exists( Preferences::get_instance()->
											m_sRubberBandCLIexecutable ) ) {
				rubberband.use = false;
			}
			pSample->setRubberband( rubberband );
	
			// FIXME, kill EnvelopePoint, create Envelope class
			EnvelopePoint pt;

			Sample::VelocityEnvelope velocityEnvelope;
			XMLNode volumeNode = node.firstChildElement( "volume" );
			while ( ! volumeNode.isNull()  ) {
				pt.frame = volumeNode.read_int( "volume-position", 0, false, false, bSilent );
				pt.value = volumeNode.read_int( "volume-value", 0, false, false , bSilent);
				velocityEnvelope.push_back( pt );
				volumeNode = volumeNode.nextSiblingElement( "volume" );
			}
			pSample->setVelocityEnvelope( velocityEnvelope );

			Sample::VelocityEnvelope panEnvelope;
			XMLNode panNode = node.firstChildElement( "pan" );
			while ( ! panNode.isNull()  ) {
				pt.frame = panNode.read_int( "pan-position", 0, false, false, bSilent );
				pt.value = panNode.read_int( "pan-value", 0, false, false, bSilent );
				panEnvelope.push_back( pt );
				panNode = panNode.nextSiblingElement( "pan" );
			}
			pSample->setPanEnvelope( panEnvelope );
		}
	}
	else {
		if ( sFilePath.isEmpty() ) {
			ERRORLOG( QString( "Unable to find sample [%1] from sDrumkitPath [%2], sSongPath [%3]" )
					  .arg( sFileName ).arg( sDrumkitPath ).arg( sSongPath ) );
		} else {
			ERRORLOG( QString( "Unable to find sample file [%1] based on filename [%2], sDrumkitPath [%3], sSongPath [%4]" )
					  .arg( sFilePath ).arg( sFileName ).arg( sDrumkitPath )
					  .arg( sSongPath ) );
		}
	}

	auto pLayer = std::make_shared<InstrumentLayer>( pSample );
	pLayer->setStartVelocity(
		node.read_float( "min", pLayer->getStartVelocity(), true, true,
						bSilent  ) );
	pLayer->setEndVelocity(
		node.read_float( "max", pLayer->getEndVelocity(), true, true,
						bSilent ) );
	pLayer->setGain( node.read_float( "gain", pLayer->getGain(), true, false,
									 bSilent ) );
	pLayer->setPitch( node.read_float( "pitch", pLayer->getPitch(), true, false,
									  bSilent ) );
	pLayer->m_bIsMuted = node.read_bool(
		"isMuted", pLayer->m_bIsMuted, true, false, true );
	pLayer->m_bIsSoloed = node.read_bool(
		"isSoloed", pLayer->m_bIsSoloed, true, false, true );
	pLayer->m_sFallbackSampleFileName = sFileName;

	return pLayer;
}

void InstrumentLayer::saveTo(
	XMLNode& node,
	bool bSongKit,
	const QString& sDrumkitPath
) const
{
	auto pSample = getSample();

	XMLNode layer_node = node.createNode( "layer" );

	QString sFileName;
	if ( pSample != nullptr ) {
		if ( bSongKit ) {
			sFileName = Filesystem::prepare_sample_path(
				pSample->getFilePath(), sDrumkitPath
			);
		}
		else {
			sFileName = pSample->getFileName();
		}
	}
	else {
		sFileName = m_sFallbackSampleFileName;
	}

	// In case the layer does not have a proper sample, we store the values
	// corresponding to the default constructor.
	if ( pSample == nullptr ) {
		pSample = std::make_shared<Sample>( sFileName );
	}

	layer_node.write_string( "filename", sFileName );
	layer_node.write_float( "min", m_fStartVelocity );
	layer_node.write_float( "max", m_fEndVelocity );
	layer_node.write_float( "gain", m_fGain );
	layer_node.write_float( "pitch", m_fPitch );
	layer_node.write_bool( "isMuted", m_bIsMuted );
	layer_node.write_bool( "isSoloed", m_bIsSoloed );

	layer_node.write_bool( "ismodified", pSample->getIsModified() );
	layer_node.write_string( "smode", pSample->getLoopModeString() );

	Sample::Loops loops = pSample->getLoops();
	layer_node.write_int( "startframe", loops.start_frame );
	layer_node.write_int( "loopframe", loops.loop_frame );
	layer_node.write_int( "loops", loops.count );
	layer_node.write_int( "endframe", loops.end_frame );

	Sample::Rubberband rubberband = pSample->getRubberband();
	layer_node.write_int( "userubber", static_cast<int>( rubberband.use ) );
	layer_node.write_float( "rubberdivider", rubberband.divider );
	layer_node.write_int( "rubberCsettings", rubberband.c_settings );
	layer_node.write_float( "rubberPitch", rubberband.pitch );

	for ( const auto& velocity : pSample->getVelocityEnvelope() ) {
		XMLNode volumeNode = layer_node.createNode( "volume" );
		volumeNode.write_int( "volume-position", velocity.frame );
		volumeNode.write_int( "volume-value", velocity.value );
	}

	for ( const auto& pan : pSample->getPanEnvelope() ) {
		XMLNode panNode = layer_node.createNode( "pan" );
		panNode.write_int( "pan-position", pan.frame );
		panNode.write_int( "pan-value", pan.value );
	}
}

QString InstrumentLayer::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentLayer]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_fGain: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fGain ) )
			.append( QString( "%1%2m_fPitch: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fPitch ) )
			.append( QString( "%1%2m_fStartVelocity: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fStartVelocity ) )
			.append( QString( "%1%2m_fEndVelocity: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fEndVelocity ) )
			.append( QString( "%1%2m_bIsMuted: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsMuted ) )
			.append( QString( "%1%2m_bIsSoloed: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsSoloed ) );
		if ( m_pSample != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pSample->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2m_pSample: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
		sOutput.append( QString( "%1%2m_sFallbackSampleFileName: %3\n" )
						.arg( sPrefix ).arg( s ).arg( m_sFallbackSampleFileName ) );
}
	else {
		sOutput = QString( "[InstrumentLayer]" )
			.append( QString( " m_fGain: %1" ).arg( m_fGain ) )
			.append( QString( ", m_fPitch: %1" ).arg( m_fPitch ) )
			.append( QString( ", m_fStartVelocity: %1" ).arg( m_fStartVelocity ) )
			.append( QString( ", m_fEndVelocity: %1" ).arg( m_fEndVelocity ) )
			.append( QString( ", m_bIsMuted: %1" ).arg( m_bIsMuted ) )
			.append( QString( ", m_bIsSoloed: %1" ).arg( m_bIsSoloed ) );
		if ( m_pSample != nullptr ) {
			sOutput.append( QString( ", m_pSample: %1\n" ).arg( m_pSample->getFilePath() ) );
		} else {
			sOutput.append( QString( ", m_pSample: nullptr\n" ) );
		}
		sOutput.append( QString( ", m_sFallbackSampleFileName: %1\n" )
					   .arg( m_sFallbackSampleFileName ) );
	}
	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
