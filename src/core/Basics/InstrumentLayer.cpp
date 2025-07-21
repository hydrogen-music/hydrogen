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
#include <core/License.h>
#include <core/Hydrogen.h>
#include <core/NsmClient.h>
#include <core/Preferences/Preferences.h>

namespace H2Core
{

InstrumentLayer::InstrumentLayer( std::shared_ptr<Sample> sample ) :
	__start_velocity( 0.0 ),
	__end_velocity( 1.0 ),
	__pitch( 0.0 ),
	__gain( 1.0 ),
	__sample( sample )
{
}

InstrumentLayer::InstrumentLayer( std::shared_ptr<InstrumentLayer> other ) : Object( *other ),
	__start_velocity( other->get_start_velocity() ),
	__end_velocity( other->get_end_velocity() ),
	__pitch( other->get_pitch() ),
	__gain( other->get_gain() ),
	__sample( other->get_sample() )
{
}

InstrumentLayer::InstrumentLayer( std::shared_ptr<InstrumentLayer> other, std::shared_ptr<Sample> sample ) : Object( *other ),
	__start_velocity( other->get_start_velocity() ),
	__end_velocity( other->get_end_velocity() ),
	__pitch( other->get_pitch() ),
	__gain( other->get_gain() ),
	__sample( sample )
{
}

InstrumentLayer::~InstrumentLayer()
{
}

void InstrumentLayer::set_sample( std::shared_ptr<Sample> sample )
{
	__sample = sample;
}

void InstrumentLayer::set_pitch( float fValue )
{
	if ( fValue < Instrument::fPitchMin || fValue > Instrument::fPitchMax ) {
		WARNINGLOG( QString( "Provided pitch out of bound [%1;%2]. Rounding to nearest allowed value." )
					.arg( Instrument::fPitchMin ).arg( Instrument::fPitchMax ) );
	}
	__pitch = std::clamp( fValue, Instrument::fPitchMin, Instrument::fPitchMax );
}

void InstrumentLayer::load_sample( float fBpm )
{
	if ( __sample != nullptr ) {
		__sample->load( fBpm );
	}
}

void InstrumentLayer::unload_sample()
{
	if ( __sample != nullptr ) {
		__sample->unload();
	}
}

std::shared_ptr<InstrumentLayer> InstrumentLayer::load_from( XMLNode* pNode, const QString& sDrumkitPath, const License& drumkitLicense, bool bSilent )
{
	auto pHydrogen = Hydrogen::get_instance();
	
	const QString sFileName = pNode->read_string(
		"filename", "", false, false, bSilent );
	QString sFilePath = sFileName;

	// In case just the filename is provided, like "sample.wav", the
	// corresponding sample will be searched in the folder of the corresponding
	// drumkit.
	if ( ! Filesystem::file_exists( sFileName, true ) && ! sDrumkitPath.isEmpty() &&
		 ! sFileName.startsWith( "/" ) ) {

#ifdef H2CORE_HAVE_OSC
		if ( pHydrogen->isUnderSessionManagement() ) {
			// If we use the NSM support and the sample files to save
			// are corresponding to the drumkit linked/located in the
			// session folder, we have to ensure the relative paths
			// are loaded. This is vital in order to support
			// renaming, duplicating, and porting sessions.

			// QFileInfo::isRelative() can not be used in here as
			// samples within drumkits within the user or system
			// drumkit folder are stored relatively as well (by saving
			// just the filename).
			if ( sFileName.left( 2 ) == "./" ||
				 sFileName.left( 2 ) == ".\\" ) {
				// Removing the leading "." of the relative path in
				// sFileName while still using the associated folder
				// separator.
				sFilePath = NsmClient::get_instance()->getSessionFolderPath() +
					sFileName.right( sFileName.size() - 1 );
			}
			else {
				sFilePath = sDrumkitPath + "/" + sFileName;
			}
		}
		else {
			sFilePath = sDrumkitPath + "/" + sFileName;
		}
#else
		sFilePath = sDrumkitPath + "/" + sFileName;
#endif
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

			const auto drumkitFolders = QStringList() <<
				Filesystem::usr_drumkits_dir() << Filesystem::sys_drumkits_dir();
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
	if ( Filesystem::file_exists( sFilePath, true ) ) {
		pSample = std::make_shared<Sample>( sFilePath, drumkitLicense );

		// If 'ismodified' is not present, InstrumentLayer was stored as
		// part of a drumkit. All the additional Sample info, like Loops,
		// envelopes etc., were not written to disk and we won't load the
		// sample.
		bool bIsModified = pNode->read_bool( "ismodified", false, true, false, true );
		pSample->set_is_modified( bIsModified );
	
		if ( bIsModified ) {
		
			Sample::Loops loops;
			loops.mode = Sample::parse_loop_mode( pNode->read_string( "smode", "forward", false, false, bSilent ) );
			loops.start_frame = pNode->read_int( "startframe", 0, false, false, bSilent );
			loops.loop_frame = pNode->read_int( "loopframe", 0, false, false, bSilent );
			loops.count = pNode->read_int( "loops", 0, false, false, bSilent );
			loops.end_frame = pNode->read_int( "endframe", 0, false, false, bSilent );
			pSample->set_loops( loops );
	
			Sample::Rubberband rubberband;
			rubberband.use = pNode->read_int( "userubber", 0, false, false, bSilent );
			rubberband.divider = pNode->read_float( "rubberdivider", 0.0, false, false, bSilent );
			rubberband.c_settings = pNode->read_int( "rubberCsettings", 1, false, false, bSilent );
			rubberband.pitch = pNode->read_float( "rubberPitch", 0.0, false, false, bSilent );

			// Check whether the rubberband executable is present.
			if ( ! Filesystem::file_exists( Preferences::get_instance()->
											m_rubberBandCLIexecutable ) ) {
				rubberband.use = false;
			}
			pSample->set_rubberband( rubberband );
	
			// FIXME, kill EnvelopePoint, create Envelope class
			EnvelopePoint pt;

			Sample::VelocityEnvelope velocityEnvelope;
			XMLNode volumeNode = pNode->firstChildElement( "volume" );
			while ( ! volumeNode.isNull()  ) {
				pt.frame = volumeNode.read_int( "volume-position", 0, false, false, bSilent );
				pt.value = volumeNode.read_int( "volume-value", 0, false, false , bSilent);
				velocityEnvelope.push_back( pt );
				volumeNode = volumeNode.nextSiblingElement( "volume" );
			}
			pSample->set_velocity_envelope( velocityEnvelope );

			Sample::VelocityEnvelope panEnvelope;
			XMLNode panNode = pNode->firstChildElement( "pan" );
			while ( ! panNode.isNull()  ) {
				pt.frame = panNode.read_int( "pan-position", 0, false, false, bSilent );
				pt.value = panNode.read_int( "pan-value", 0, false, false, bSilent );
				panEnvelope.push_back( pt );
				panNode = panNode.nextSiblingElement( "pan" );
			}
			pSample->set_pan_envelope( panEnvelope );
		}
	}
	else {
		WARNINGLOG( QString( "Sample file [%1] does not exist at [%2]" )
					.arg( sFileName ).arg( sFilePath ) );
	}
	
	auto pLayer = std::make_shared<InstrumentLayer>( pSample );
	pLayer->set_start_velocity( pNode->read_float( "min", 0.0,
												   true, true, bSilent  ) );
	pLayer->set_end_velocity( pNode->read_float( "max", 1.0,
												 true, true, bSilent ) );
	pLayer->set_gain( pNode->read_float( "gain", 1.0,
										 true, false, bSilent ) );
	pLayer->set_pitch( pNode->read_float( "pitch", 0.0,
										  true, false, bSilent ) );
	return pLayer;
}

void InstrumentLayer::save_to( XMLNode* node, bool bFull )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSample = get_sample();
	if ( pSample == nullptr ) {
		ERRORLOG( "No sample associated with layer. Skipping it" );
		return;
	}
	
	XMLNode layer_node = node->createNode( "layer" );

	QString sFileName;
	if ( bFull ) {

		if ( pHydrogen->isUnderSessionManagement() ) {
			// If we use the NSM support and the sample files to save
			// are corresponding to the drumkit linked/located in the
			// session folder, we have to ensure the relative paths
			// are written out. This is vital in order to support
			// renaming, duplicating, and porting sessions.
			if ( pSample->get_raw_filepath().startsWith( '.' ) ) {
				sFileName = pSample->get_raw_filepath();
			}
			else {
				sFileName = Filesystem::prepare_sample_path( pSample->get_filepath() );
			}
		}
		else {
			sFileName = Filesystem::prepare_sample_path( pSample->get_filepath() );
		}
	}
	else {
		sFileName = pSample->get_filename();
	}
	
	layer_node.write_string( "filename", sFileName );
	layer_node.write_float( "min", __start_velocity );
	layer_node.write_float( "max", __end_velocity );
	layer_node.write_float( "gain", __gain );
	layer_node.write_float( "pitch", __pitch );

	if ( bFull ) {
		layer_node.write_bool( "ismodified", pSample->get_is_modified() );
		layer_node.write_string( "smode", pSample->get_loop_mode_string() );

		Sample::Loops loops = pSample->get_loops();
		layer_node.write_int( "startframe", loops.start_frame );
		layer_node.write_int( "loopframe", loops.loop_frame );
		layer_node.write_int( "loops", loops.count );
		layer_node.write_int( "endframe", loops.end_frame );

		Sample::Rubberband rubberband = pSample->get_rubberband();
		layer_node.write_int( "userubber", static_cast<int>(rubberband.use) );
		layer_node.write_float( "rubberdivider", rubberband.divider );
		layer_node.write_int( "rubberCsettings", rubberband.c_settings );
		layer_node.write_float( "rubberPitch", rubberband.pitch );

		for ( const auto& velocity : *pSample->get_velocity_envelope() ) {
			XMLNode volumeNode = layer_node.createNode( "volume" );
			volumeNode.write_int( "volume-position", velocity.frame );
			volumeNode.write_int( "volume-value", velocity.value );
		}

		for ( const auto& pan : *pSample->get_pan_envelope() ) {
			XMLNode panNode = layer_node.createNode( "pan" );
			panNode.write_int( "pan-position", pan.frame );
			panNode.write_int( "pan-value", pan.value );
		}
	}
}

QString InstrumentLayer::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[InstrumentLayer]\n" ).arg( sPrefix )
			.append( QString( "%1%2gain: %3\n" ).arg( sPrefix ).arg( s ).arg( __gain ) )
			.append( QString( "%1%2pitch: %3\n" ).arg( sPrefix ).arg( s ).arg( __pitch ) )
			.append( QString( "%1%2start_velocity: %3\n" ).arg( sPrefix ).arg( s ).arg( __start_velocity ) )
			.append( QString( "%1%2end_velocity: %3\n" ).arg( sPrefix ).arg( s ).arg( __end_velocity ) );
		if ( __sample != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( __sample->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2sample: nullptr\n" ).arg( sPrefix ).arg( s ) );
		}
	}
	else {
		sOutput = QString( "[InstrumentLayer]" )
			.append( QString( " gain: %1" ).arg( __gain ) )
			.append( QString( ", pitch: %1" ).arg( __pitch ) )
			.append( QString( ", start_velocity: %1" ).arg( __start_velocity ) )
			.append( QString( ", end_velocity: %1" ).arg( __end_velocity ) );
		if ( __sample != nullptr ) { 
			sOutput.append( QString( ", sample: %1\n" ).arg( __sample->get_filepath() ) );
		} else {
			sOutput.append( QString( ", sample: nullptr\n" ) );
		}
	}
	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
