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

#include <core/Basics/Instrument.h>

#include <cassert>
#include <memory>

#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>
#include <core/EventQueue.h>
#include <core/Helpers/Legacy.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiMessage.h>
#include <core/Sampler/Sampler.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core
{

Instrument::Instrument( const int id, const QString& name, std::shared_ptr<ADSR> adsr )
	: m_nId( id )
	, m_sName( name )
	, m_type( "" )
	, m_sDrumkitPath( "" )
	, m_sDrumkitName( "" )
	, m_fGain( 1.0 )
	, m_fVolume( 1.0 )
	, m_fPan( PAN_DEFAULT )
	, m_fPeak_L( 0.0 )
	, m_fPeak_R( 0.0 )
	, m_pAdsr( adsr )
	, m_bFilterActive( false )
	, m_fFilterCutoff( 1.0 )
	, m_fFilterResonance( 0.0 )
	, m_fRandomPitchFactor( 0.0 )
	, m_fPitchOffset( 0.0 )
	, m_nMidiOutNote( MidiMessage::nInstrumentOffset + id )
	, m_nMidiOutChannel( -1 )
	, m_bStopNotes( false )
	, m_bSoloed( false )
	, m_bMuted( false )
	, m_nMuteGroup( -1 )
	, m_nQueued( 0 )
	, m_enqueuedBy( QStringList() )
	, m_nHihatGrp( -1 )
	, m_nLowerCc( 0 )
	, m_nHigherCc( 127 )
	, m_bIsPreviewInstrument(false)
	, m_bApplyVelocity( true )
	, m_bCurrentInstrForExport(false)
	, m_bHasMissingSamples( false )
	, m_pComponents( std::make_shared<std::vector<std::shared_ptr<InstrumentComponent>>>() )
{
	/*: Name assigned to an Instrument created either as part of a fresh kit
	 *  created via the Main Menu > Drumkit > New or via the "Add Instrument"
	 *  action. */
	const QString sInstrumentName = QT_TRANSLATE_NOOP( "Instrument", "New Instrument");

	if ( name.isEmpty() ) {
		m_sName = sInstrumentName;
	}

	if ( m_pAdsr == nullptr ) {
		m_pAdsr = std::make_shared<ADSR>();
	}

    if( m_nMidiOutNote < MIDI_OUT_NOTE_MIN ){
		m_nMidiOutNote = MIDI_OUT_NOTE_MIN;
	}

	if( m_nMidiOutNote > MIDI_OUT_NOTE_MAX ){
		m_nMidiOutNote = MIDI_OUT_NOTE_MAX;
	}

	for ( int i=0; i<MAX_FX; i++ ) {
		m_fxLevel[i] = 0.0;
	}

	m_pComponents->push_back( std::make_shared<InstrumentComponent>() );
}

Instrument::Instrument( std::shared_ptr<Instrument> other )
	: m_nId( other->getId() )
	, m_sName( other->getName() )
	, m_type( other->m_type )
	, m_sDrumkitPath( other->getDrumkitPath() )
	, m_sDrumkitName( other->m_sDrumkitName )
	, m_fGain( other->m_fGain )
	, m_fVolume( other->getVolume() )
	, m_fPan( other->getPan() )
	, m_fPeak_L( other->getPeak_L() )
	, m_fPeak_R( other->getPeak_R() )
	, m_pAdsr( std::make_shared<ADSR>( *( other->getAdsr() ) ) )
	, m_bFilterActive( other->isFilterActive() )
	, m_fFilterCutoff( other->getFilterCutoff() )
	, m_fFilterResonance( other->getFilterResonance() )
	, m_fRandomPitchFactor( other->getRandomPitchFactor() )
	, m_fPitchOffset( other->getPitchOffset() )
	, m_nMidiOutNote( other->getMidiOutNote() )
	, m_nMidiOutChannel( other->getMidiOutChannel() )
	, m_bStopNotes( other->isStopNotes() )
	, m_bSoloed( other->isSoloed() )
	, m_bMuted( other->isMuted() )
	, m_nMuteGroup( other->getMuteGroup() )
	, m_nQueued( 0 )
	, m_enqueuedBy( QStringList() )
	, m_nHihatGrp( other->getHihatGrp() )
	, m_nLowerCc( other->getLowerCc() )
	, m_nHigherCc( other->getHigherCc() )
	, m_bIsPreviewInstrument(false)
	, m_bApplyVelocity( other->getApplyVelocity() )
	, m_bCurrentInstrForExport(false)
	, m_bHasMissingSamples(other->hasMissingSamples())
	, m_pComponents( nullptr )
{
	for ( int i=0; i<MAX_FX; i++ ) {
		m_fxLevel[i] = other->getFxLevel( i );
	}

	m_pComponents = std::make_shared<std::vector<std::shared_ptr<InstrumentComponent>>>();
	for ( const auto& pComponent : *other->getComponents() ) {
		m_pComponents->push_back( std::make_shared<InstrumentComponent>( pComponent ) );
	}
}

Instrument::~Instrument() {
	if ( m_nQueued > 0 ) {
		WARNINGLOG( QString( "Instrument [%1] is destroyed while still being enqueued! m_nQueued: %2,\nm_enqueuedNotes:\n\t%3" )
					.arg( m_sName ).arg( m_nQueued )
					.arg( m_enqueuedBy.join( "\n\t" ) ) );
	}
}

std::shared_ptr<Instrument> Instrument::loadFrom( const XMLNode& node,
												  const QString& sDrumkitPath,
												  const QString& sDrumkitName,
												  const QString& sSongPath,
												  const License& license,
												  bool bSongKit,
												  bool* pLegacyFormatEncountered,
												  bool bSilent )
{
	// We use -2 instead of EMPTY_INSTR_ID (-1) to allow for loading
	// empty instruments as well (e.g. during unit tests or as part of
	// dummy kits)
	int nId = node.read_int( "id", -2, false, false, bSilent );
	if ( nId == -2 ) {
		if ( pLegacyFormatEncountered != nullptr ) {
			*pLegacyFormatEncountered = true;
		}
		return nullptr;
	}

	auto pInstrument =
		std::make_shared<Instrument>(
			nId,
			node.read_string( "name", "", false, false, bSilent ),
			std::make_shared<ADSR>( node.read_int( "Attack", 0, true, false, bSilent ),
									node.read_int( "Decay", 0, true, false, bSilent  ),
									node.read_float( "Sustain", 1.0f, true, false, bSilent ),
									node.read_int( "Release", 1000, true, false, bSilent ) ) );

	pInstrument->setType( node.read_string( "type", "", true, true, bSilent ) );

	QString sInstrumentDrumkitPath, sInstrumentDrumkitName;
	if ( bSongKit ) {

		// Instrument is not read as part of a plain Drumkit but as part of a
		// Song.
		sInstrumentDrumkitName = node.read_string( "drumkit", "", false,
													 true, bSilent );
		
		if ( ! node.firstChildElement( "drumkitPath" ).isNull() ) {
			// Current format
			sInstrumentDrumkitPath = node.read_string( "drumkitPath", "",
														 false, true, bSilent  );

			if ( ! sInstrumentDrumkitPath.isEmpty() ) {
#ifdef H2CORE_HAVE_APPIMAGE
				sInstrumentDrumkitPath =
					Filesystem::rerouteDrumkitPath( sInstrumentDrumkitPath );
#endif

				// Check whether corresponding drumkit exist. When tweaking or
				// assembling drumkits locally their absolute paths serve as
				// unique identifiers to keep them apart. But in terms of
				// portability (and to assure backward compatibility) paths are
				// bad and we will use the drumkit name and check whether we can
				// find the kit on the local system.
				if ( ! Filesystem::drumkit_valid( sInstrumentDrumkitPath ) ) {
					WARNINGLOG( QString( "Couldn't find drumkit at [%1]. Searching for [%2] instead." )
								.arg( sInstrumentDrumkitPath )
								.arg( sInstrumentDrumkitName ) );
					sInstrumentDrumkitPath = "";
				}
			}
		}

		// Both empty drumkit path and name indicate that the instrument was
		// added as a new one to the drumkit instead of importing it from
		// another kit. It must only hold absolute paths for samples.
		if ( sInstrumentDrumkitPath.isEmpty() &&
			 ! sInstrumentDrumkitName.isEmpty() ) {
			if ( ! node.firstChildElement( "drumkitLookup" ).isNull() ) {
				// Format introduced in #1f2a06b and used in (at least)
				// releases 1.1.0-beta1, 1.1.0, and 1.1.1.
				//
				// Using the additional lookup variable two drumkits holding
				// the same name but one of the residing in user-space and
				// the other one in system-space can be distinguished.
				Filesystem::Lookup lookup = static_cast<Filesystem::Lookup>(
					node.read_int( "drumkitLookup",
								   static_cast<int>(Filesystem::Lookup::stacked),
								   false, false, bSilent ) );

				sInstrumentDrumkitPath =
					Filesystem::drumkit_path_search( sInstrumentDrumkitName,
													 lookup, true );

				if ( sInstrumentDrumkitPath.isEmpty() &&
					 lookup != Filesystem::Lookup::stacked ) {
					// Drumkit could not be found.
					//
					// It's possible the song was composed with a
					// custom version of a system-level drumkit stored
					// in user space. When loaded again in a fresh
					// installed Hydrogen the custom user-level one
					// will not be present anymore but it's plausible
					// to fall back to the system-level one. (The
					// other way around is also possible but much more
					// unlikely. Nevertheless we will use the stacked
					// search in one final effort)
					sInstrumentDrumkitPath =
						Filesystem::drumkit_path_search( sInstrumentDrumkitName,
														 Filesystem::Lookup::stacked,
														 true );

					if ( sInstrumentDrumkitPath.isEmpty() ) {
						ERRORLOG( QString( "Drumkit [%1] could neither found at system nor at user level." )
								  .arg( sInstrumentDrumkitName ) );
					}
					else if ( ! bSilent ) {
						WARNINGLOG( QString( "Drumkit [%1] could not found using lookup type [%2]. Falling back to [%3] found using stacked search" )
									.arg( sInstrumentDrumkitName )
									.arg( static_cast<int>(lookup) )
									.arg( sInstrumentDrumkitPath ) );
					}
				}

				if ( pLegacyFormatEncountered != nullptr ) {
					*pLegacyFormatEncountered = true;
				}
			}
			else if ( ! node.firstChildElement( "drumkit" ).isNull() ) {
				// Format used from version 0.9.7 till 1.1.0.
				//
				// It features just the name of the drumkit an relies on
				// it being unique throught the entire search path.
				sInstrumentDrumkitPath =
					Filesystem::drumkit_path_search( sInstrumentDrumkitName,
													 Filesystem::Lookup::stacked,
													 bSilent );

				if ( pLegacyFormatEncountered != nullptr ) {
					*pLegacyFormatEncountered = true;
				}
			}
			else {
				// Format used prior to 0.9.7 which worked with absolute
				// paths for the samples instead of relative ones.
				sInstrumentDrumkitPath = "";

				if ( pLegacyFormatEncountered != nullptr ) {
					*pLegacyFormatEncountered = true;
				}
			}
		}
	}
	else {
		sInstrumentDrumkitPath = sDrumkitPath;
		sInstrumentDrumkitName = sDrumkitName;
	}
	
	pInstrument->setDrumkitPath( sInstrumentDrumkitPath );
	pInstrument->m_sDrumkitName = sInstrumentDrumkitName;

	pInstrument->setVolume( node.read_float( "volume", 1.0f,
											   true, true, bSilent ) );
	pInstrument->setMuted( node.read_bool( "isMuted", false,
											 true, true, bSilent ) );
	pInstrument->setSoloed( node.read_bool( "isSoloed", false,
											  true, false, true ) );
	bool bFound, bFound2;
	float fPan = node.read_float( "pan", PAN_DEFAULT, &bFound,
								   true, true, true );
	if ( !bFound ) {
		// check if pan is expressed in the old fashion (version <=
		// 1.1 ) with the pair (pan_L, pan_R)
		float fPanL = node.read_float( "pan_L", 1.f, &bFound,
										true, true, bSilent );
		float fPanR = node.read_float( "pan_R", 1.f, &bFound2,
										true, true, bSilent );
		if ( bFound == true && bFound2 == true ) { // found nodes pan_L and pan_R
			fPan = Sampler::getRatioPan( fPanL, fPanR );  // convert to single pan parameter
		}
	}
	pInstrument->setPan( fPan );
	
	pInstrument->setApplyVelocity( node.read_bool( "applyVelocity", true,
													  false, true, bSilent ) );
	pInstrument->setFilterActive( node.read_bool( "filterActive", true,
													 false, true, bSilent ) );
	pInstrument->setFilterCutoff( node.read_float( "filterCutoff", 1.0f,
													  true, false, bSilent ) );
	pInstrument->setFilterResonance( node.read_float( "filterResonance", 0.0f,
														 true, false, bSilent ) );
	pInstrument->setPitchOffset( node.read_float( "pitchOffset", 0.0f,
													 true, false, true ) );
	pInstrument->setRandomPitchFactor( node.read_float( "randomPitchFactor", 0.0f,
															true, false, bSilent ) );
	pInstrument->setGain( node.read_float( "gain", 1.0f,
											 true, false, bSilent ) );
	pInstrument->setMuteGroup( node.read_int( "muteGroup", -1,
												 true, false, bSilent ) );
	pInstrument->setMidiOutChannel( node.read_int( "midiOutChannel", -1,
													   true, false, bSilent ) );
	pInstrument->setMidiOutNote( node.read_int( "midiOutNote", pInstrument->m_nMidiOutNote,
													true, false, bSilent ) );
	pInstrument->setStopNotes( node.read_bool( "isStopNote", true,
												  false, true, bSilent ) );
	pInstrument->setHihatGrp( node.read_int( "isHihat", -1,
											 true, true, bSilent ) );
	pInstrument->setLowerCc( node.read_int( "lower_cc", 0,
											true, true, bSilent ) );
	pInstrument->setHigherCc( node.read_int( "higher_cc", 127,
											 true, true, bSilent ) );

	for ( int i=0; i<MAX_FX; i++ ) {
		pInstrument->setFxLevel( node.read_float( QString( "FX%1Level" ).arg( i+1 ), 0.0,
													 true, true, bSilent ), i );
	}

	// This license will be applied to all samples contained in this
	// instrument.
	License instrumentLicense = License();
	if ( license == License() ) {
		// No/empty license supplied. We will use the license stored
		// in the drumkit.xml file found in m_sDrumkitName. But since
		// loading it from file is a rather expensive action, we will
		// query it from the Drumkit database. If, for some reasons,
		// the drumkit is not present yet, the License will be loaded
		// directly.
		auto pSoundLibraryDatabase = Hydrogen::get_instance()->getSoundLibraryDatabase();
		if ( pSoundLibraryDatabase != nullptr &&
			 ! pInstrument->getDrumkitPath().isEmpty() ) {

			// It is important to _not_ load the drumkit into the
			// database as this code is part of the drumkit load
			// itself. In case two drumkits contain an instrument from
			// each other an infinite loop would be created.
			auto pDrumkit = pSoundLibraryDatabase->getDrumkit(
				pInstrument->getDrumkitPath() );
			if ( pDrumkit != nullptr ) {
				instrumentLicense = pDrumkit->getLicense();
			}
		}
	}

	std::vector<std::shared_ptr<InstrumentComponent>> componentsLoaded;
	if ( ! node.firstChildElement( "instrumentComponent" ).isNull() ) {
		// current format
		XMLNode componentNode = node.firstChildElement( "instrumentComponent" );
		while ( ! componentNode.isNull() ) {
			auto ppComponent = InstrumentComponent::loadFrom(
				componentNode, pInstrument->getDrumkitPath(),
				sSongPath, instrumentLicense, bSilent );
			if ( ppComponent != nullptr ) {
				componentsLoaded.push_back( ppComponent );
			}
			componentNode = componentNode.nextSiblingElement( "instrumentComponent" ) ;
		}
	}
	else {
		// back compatibility code
		auto pCompo = Legacy::loadInstrumentComponent(
			node, pInstrument->getDrumkitPath(), sSongPath, instrumentLicense,
			bSilent );
		if ( pCompo == nullptr ) {
			ERRORLOG( QString( "Unable to load component for instrument [%1]. Aborting." )
					  .arg( pInstrument->getName() ) );
			return nullptr;
		}
		componentsLoaded.push_back( pCompo );
	}

	// Each new instrument comes with a fallback/default component. Only discard
	// it in case we did successfully loaded components from file.
	if ( componentsLoaded.size() > 0 ) {
		pInstrument->m_pComponents->clear();
		for ( const auto& ppComponent : componentsLoaded ) {
			pInstrument->getComponents()->push_back( ppComponent );
		}
	}

	// Backward compatibility.
	//
	// In versions prior to 2.0 the sample selection algorithm was stored on
	// instrument level. If found there, we need to propagate it the component
	// level.
	XMLNode selectionNode = node.firstChildElement( "sampleSelectionAlgo" );
	if ( ! selectionNode.isNull() ) {
		const QString sSelection = node.read_string(
			"sampleSelectionAlgo", "VELOCITY", true, true, bSilent );
		auto selection = InstrumentComponent::Selection::Velocity;
		if ( sSelection.compare("VELOCITY") == 0 ) {
			selection = InstrumentComponent::Selection::Velocity;
		}
		else if ( sSelection.compare("ROUND_ROBIN") == 0 ) {
			selection = InstrumentComponent::Selection::RoundRobin;
		}
		else if ( sSelection.compare("RANDOM") == 0 ) {
			selection = InstrumentComponent::Selection::Random;
		}

		for ( auto& ppComponent : *pInstrument->getComponents() ) {
			ppComponent->setSelection( selection );
		}
	}

	// Sanity checks

	// There has to be at least one InstrumentComponent
	if ( pInstrument->getComponents()->size() == 0 ) {
		pInstrument->getComponents()->push_back(
			std::make_shared<InstrumentComponent>() );
	}

	pInstrument->checkForMissingSamples( Event::Trigger::Suppress );

	return pInstrument;
}

void Instrument::loadSamples( float fBpm )
{
	for ( auto& ppComponent : *m_pComponents ) {
		if ( ppComponent == nullptr ) {
			continue;
		}
		for ( auto& ppLayer : *ppComponent ) {
			if ( ppLayer != nullptr ) {
				ppLayer->loadSample( fBpm );
			}
		}
	}
}

void Instrument::unloadSamples()
{
	for ( auto& ppComponent : *m_pComponents ) {
		if ( ppComponent == nullptr ) {
			continue;
		}
		for ( auto& ppLayer : *ppComponent ) {
			if ( ppLayer != nullptr ) {
				ppLayer->unloadSample();
			}
		}
	}
}

void Instrument::saveTo( XMLNode& node, bool bSongKit, bool bKeepMissingSamples,
						bool bSilent )
{
	XMLNode InstrumentNode = node.createNode( "instrument" );
	InstrumentNode.write_int( "id", m_nId );
	InstrumentNode.write_string( "name", m_sName );

	InstrumentNode.write_string( "type", m_type );

	if ( bSongKit ) {
		InstrumentNode.write_string( "drumkitPath", m_sDrumkitPath );
		InstrumentNode.write_string( "drumkit", m_sDrumkitName );
	}
	
	InstrumentNode.write_float( "volume", m_fVolume );
	InstrumentNode.write_bool( "isMuted", m_bMuted );
	InstrumentNode.write_bool( "isSoloed", m_bSoloed );

	// We still store the pan using the old format to allow drumkits
	// being created with Hydrogen versions v1.2 to be valid for prior
	// versions too. After a couple of years and when all major Linux
	// distributions ship a version >= 1.2 we can drop this part and
	// just store the plain pan.
	if ( getPan() >= 0.0 ) {
		InstrumentNode.write_float( "pan_L", 1.0 - getPan() );
		InstrumentNode.write_float( "pan_R", 1.0 );
	}
	else {
		InstrumentNode.write_float( "pan_L", 1.0 );
		InstrumentNode.write_float( "pan_R", getPan() + 1.0 );
	}
		
	InstrumentNode.write_float( "pitchOffset", m_fPitchOffset );
	InstrumentNode.write_float( "randomPitchFactor", m_fRandomPitchFactor );
	InstrumentNode.write_float( "gain", m_fGain );
	InstrumentNode.write_bool( "applyVelocity", m_bApplyVelocity );
	InstrumentNode.write_bool( "filterActive", m_bFilterActive );
	InstrumentNode.write_float( "filterCutoff", m_fFilterCutoff );
	InstrumentNode.write_float( "filterResonance", m_fFilterResonance );
	InstrumentNode.write_int( "Attack", m_pAdsr->getAttack() );
	InstrumentNode.write_int( "Decay", m_pAdsr->getDecay() );
	InstrumentNode.write_float( "Sustain", m_pAdsr->getSustain() );
	InstrumentNode.write_int( "Release", m_pAdsr->getRelease() );
	InstrumentNode.write_int( "muteGroup", m_nMuteGroup );
	InstrumentNode.write_int( "midiOutChannel", m_nMidiOutChannel );
	InstrumentNode.write_int( "midiOutNote", m_nMidiOutNote );
	InstrumentNode.write_bool( "isStopNote", m_bStopNotes );
	InstrumentNode.write_int( "isHihat", m_nHihatGrp );
	InstrumentNode.write_int( "lower_cc", m_nLowerCc );
	InstrumentNode.write_int( "higher_cc", m_nHigherCc );

	for ( int i=0; i<MAX_FX; i++ ) {
		InstrumentNode.write_float( QString( "FX%1Level" )
									.arg( i+1 ), m_fxLevel[i] );
	}

	for ( const auto& pComponent : *m_pComponents ) {
		if ( pComponent != nullptr ) {
			pComponent->saveTo( InstrumentNode, bSongKit, bKeepMissingSamples,
							   bSilent );
		} else {
			ERRORLOG( "Invalid component!" );
		}
	}

	// Instrument layers with missing samples will be discarded during saving.
	if ( m_bHasMissingSamples ) {
		checkForMissingSamples( Event::Trigger::Suppress );
	}
}

bool Instrument::isAnyComponentSoloed() const {
	for ( const auto& ppComponent : *m_pComponents ) {
		if ( ppComponent != nullptr && ppComponent->getIsSoloed() ) {
			return true;
		}
	}

	return false;

}

void Instrument::enqueue( std::shared_ptr<Note> pNote ) {
	m_nQueued++;

	m_enqueuedBy.push_back( pNote->prettyName() );
}

void Instrument::dequeue( std::shared_ptr<Note> pNote ) {
	if ( m_nQueued <= 0 ) {
		ERRORLOG( QString( "[%1] is not queued!" ).arg( m_sName ) );
		return;
	}

	m_nQueued--;

	if ( m_nQueued > 0 ) {
		m_enqueuedBy.removeOne( pNote->prettyName() );
	} else {
		m_enqueuedBy.clear();
	}
}

void Instrument::setPitchOffset( float fValue )
{
	if ( fValue < fPitchMin || fValue > fPitchMax ) {
		WARNINGLOG( QString( "Provided pitch out of bound [%1;%2]. Rounding to nearest allowed value." )
					.arg( fPitchMin ).arg( fPitchMax ) );
	}
	m_fPitchOffset = std::clamp( fValue, fPitchMin, fPitchMax );
}

void Instrument::setPanWithRangeFrom0To1( float fVal ) {
	// scale and translate into [-1;1]
	setPan( PAN_MIN + ( PAN_MAX - PAN_MIN ) * fVal );
}

std::shared_ptr<InstrumentComponent> Instrument::getComponent( int nIdx ) const
{
	if ( nIdx < 0 || nIdx >= m_pComponents->size() ) {
		ERRORLOG( QString( "Provided index [%1] out of bound [0,%2)" )
				  .arg( nIdx ).arg( m_pComponents->size() ) );
		return nullptr;
	}

	return m_pComponents->at( nIdx );
}

int Instrument::index( std::shared_ptr<InstrumentComponent> pComponent ) const {
	for( int ii = 0; ii < m_pComponents->size(); ii++ ) {
		if ( m_pComponents->at( ii ) == pComponent ) {
			return ii;
		}
	}
	return -1;
}

void Instrument::addComponent( std::shared_ptr<InstrumentComponent> pComponent ) {
	m_pComponents->push_back( pComponent );
}

void Instrument::removeComponent( int nIdx ) {
	if ( nIdx < 0 || nIdx >= m_pComponents->size() ) {
		ERRORLOG( QString( "Provided index [%1] out of bound [0,%2)" )
				  .arg( nIdx ).arg( m_pComponents->size() ) );
		return;
	}

	m_pComponents->erase( m_pComponents->begin() + nIdx );
}

const QString& Instrument::getDrumkitPath() const
{
	return m_sDrumkitPath;
}

void Instrument::addLayer(
	std::shared_ptr<InstrumentComponent> pComponent,
	std::shared_ptr<InstrumentLayer> pLayer,
	int nIndex,
	Event::Trigger trigger
)
{
	if ( pComponent == nullptr ) {
		// The provided layer is allowed to be nullptr. This will be used to
		// remove it from the component.
		ERRORLOG( "Invalid input" );
		return;
	}

	for ( auto& ppComponent : *m_pComponents ) {
		if ( pComponent == ppComponent ) {
			ppComponent->addLayer( pLayer, nIndex );
		}
	}

	checkForMissingSamples( trigger );
}

void Instrument::moveLayer(
	std::shared_ptr<InstrumentComponent> pComponent,
	int nOldIndex,
	int nNewIndex,
	Event::Trigger trigger
)
{
	if ( pComponent == nullptr ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	for ( auto& ppComponent : *m_pComponents ) {
		if ( pComponent == ppComponent ) {
			ppComponent->moveLayer( nOldIndex, nNewIndex );
		}
	}
}

void Instrument::setLayer(
	std::shared_ptr<InstrumentComponent> pComponent,
	std::shared_ptr<InstrumentLayer> pLayer,
	int nIndex,
	Event::Trigger trigger
)
{
	if ( pComponent == nullptr || pLayer == nullptr ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	for ( auto& ppComponent : *m_pComponents ) {
		if ( pComponent == ppComponent ) {
			ppComponent->setLayer( pLayer, nIndex );
		}
	}

	checkForMissingSamples( trigger );
}

void Instrument::removeLayer(
	std::shared_ptr<InstrumentComponent> pComponent,
	int nIndex,
	Event::Trigger trigger
)
{
	if ( pComponent == nullptr ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	for ( auto& ppComponent : *m_pComponents ) {
		if ( pComponent == ppComponent ) {
			ppComponent->removeLayer( nIndex );
		}
	}

	checkForMissingSamples( trigger );
}

bool Instrument::hasSamples() const
{
	for ( const auto& pComponent : *m_pComponents ) {
		if ( pComponent != nullptr && pComponent->hasSamples() ) {
			return true;
		}
	}

	return false;
}

void Instrument::setSample(
	std::shared_ptr<InstrumentComponent> pComponent,
	std::shared_ptr<InstrumentLayer> pLayer,
	std::shared_ptr<Sample> pSample,
	Event::Trigger trigger
)
{
	if ( pComponent == nullptr || pLayer == nullptr || pSample == nullptr ) {
		ERRORLOG( "Invalid input" );
		return;
	}

	for ( const auto& ppComponent : *m_pComponents ) {
		if ( pComponent == ppComponent ) {
			for ( auto& ppLayer : *pComponent ) {
				if ( ppLayer == pLayer ) {
					ppLayer->setSample( pSample );
				}
			}
		}
	}

	checkForMissingSamples( trigger );
}

int Instrument::getLongestSampleFrames() const
{
	int nLongestFrames = 0;

	for ( const auto& pComponent : *m_pComponents ) {
		if ( pComponent != nullptr ) {
			for ( const auto& pLayer : *pComponent ) {
				if ( pLayer != nullptr ) {
					if ( pLayer->getSample() != nullptr &&
						 pLayer->getSample()->getFrames() > nLongestFrames ) {
						nLongestFrames = pLayer->getSample()->getFrames();
					}
				}
			}
		}
	}

	return nLongestFrames;
}

void Instrument::checkForMissingSamples( Event::Trigger trigger )
{
	const bool bPreviousValue = m_bHasMissingSamples;

	m_bHasMissingSamples = false;

	for ( const auto& pComponent : *getComponents() ) {
		if ( pComponent == nullptr ) {
			ERRORLOG(
				"Invalid component. Something went wrong loading the instrument"
			);
			m_bHasMissingSamples = true;
			return;
		}

		for ( const auto& pLayer : *pComponent ) {
			if ( pLayer == nullptr ) {
				continue;
			}

			if ( pLayer->getSample() == nullptr ) {
				m_bHasMissingSamples = true;
			}
		}
	}

	if ( m_bHasMissingSamples != bPreviousValue &&
		 trigger != Event::Trigger::Suppress ) {
		EventQueue::get_instance()->pushEvent(
			Event::Type::InstrumentLayerChanged, m_nId
		);
	}
}

std::vector<std::shared_ptr<InstrumentComponent>>::iterator Instrument::begin()
{
	return m_pComponents->begin();
}

std::vector<std::shared_ptr<InstrumentComponent>>::iterator Instrument::end()
{
	return m_pComponents->end();
}

QString Instrument::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Instrument]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_nId: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nId ) )
			.append( QString( "%1%2m_sName: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sName ) )
			.append( QString( "%1%2m_type: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_type ) )
			.append( QString( "%1%2m_sDrumkitPath: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sDrumkitPath ) )
			.append( QString( "%1%2m_sDrumkitName: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sDrumkitName ) )
			.append( QString( "%1%2m_fGain: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fGain ) )
			.append( QString( "%1%2m_fVolume: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fVolume ) )
			.append( QString( "%1%2m_fPan: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPan ) )
			.append( QString( "%1%2m_fPeak_L: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPeak_L ) )
			.append( QString( "%1%2m_fPeak_R: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPeak_R ) )
			.append( QString( "%1" ).arg( m_pAdsr->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_bFilterActive: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bFilterActive ) )
			.append( QString( "%1%2m_fFilterCutoff: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fFilterCutoff ) )
			.append( QString( "%1%2m_fFilterResonance: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fFilterResonance ) )
			.append( QString( "%1%2m_fRandomPitchFactor: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fRandomPitchFactor ) )
			.append( QString( "%1%2m_fPitchOffset: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPitchOffset ) )
			.append( QString( "%1%2m_nMidiOutNote: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nMidiOutNote ) )
			.append( QString( "%1%2m_nMidiOutChannel: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nMidiOutChannel ) )
			.append( QString( "%1%2m_bStopNotes: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bStopNotes ) )
			.append( QString( "%1%2m_bSoloed: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bSoloed ) )
			.append( QString( "%1%2m_bMuted: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bMuted ) )
			.append( QString( "%1%2m_nMuteGroup: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nMuteGroup ) )
			.append( QString( "%1%2m_nQueued: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nQueued ) )
			.append( QString( "%1%2m_enqueuedBy: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_enqueuedBy.join( "\n" + sPrefix + s + s  ) ) );
		sOutput.append( QString( "%1%2m_fxLevel: [ " ).arg( sPrefix ).arg( s ) );
		for ( const auto& ff : m_fxLevel ) {
			sOutput.append( QString( "%1 " ).arg( ff ) );
		}
		sOutput.append( QString( "]\n" ) )
			.append( QString( "%1%2m_nHihatGrp: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nHihatGrp ) )
			.append( QString( "%1%2m_nLowerCc: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nLowerCc ) )
			.append( QString( "%1%2m_nHigherCc: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nHigherCc ) )
			.append( QString( "%1%2m_bIsPreviewInstrument: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsPreviewInstrument ) )
			.append( QString( "%1%2m_bApplyVelocity: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bApplyVelocity ) )
			.append( QString( "%1%2m_bCurrentInstrForExport: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bCurrentInstrForExport ) )
			.append( QString( "%1%2m_bHasMissingSamples: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bHasMissingSamples ) )
			.append( QString( "%1%2m_pComponents:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& cc : *m_pComponents ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( "%1" ).arg( cc->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	} else {
		
		sOutput = QString( "[Instrument]" )
			.append( QString( " m_nId: %1" ).arg( m_nId ) )
			.append( QString( ", m_sName: %1" ).arg( m_sName ) )
			.append( QString( ", m_type: %1" ).arg( m_type ) )
			.append( QString( ", m_sDrumkitPath: %1" ).arg( m_sDrumkitPath ) )
			.append( QString( ", m_sDrumkitName: %1" ).arg( m_sDrumkitName ) )
			.append( QString( ", m_fGain: %1" ).arg( m_fGain ) )
			.append( QString( ", m_fVolume: %1" ).arg( m_fVolume ) )
			.append( QString( ", m_fPan: %1" ).arg( m_fPan ) )
			.append( QString( ", m_fPeak_L: %1" ).arg( m_fPeak_L ) )
			.append( QString( ", m_fPeak_R: %1" ).arg( m_fPeak_R ) )
			.append( QString( ", [%1" ).arg(
						 m_pAdsr->toQString( sPrefix + s, bShort )
						 .replace( "\n", "]" ) ) )
			.append( QString( ", m_bFilterActive: %1" ).arg( m_bFilterActive ) )
			.append( QString( ", m_fFilterCutoff: %1" ).arg( m_fFilterCutoff ) )
			.append( QString( ", m_fFilterResonance: %1" ).arg( m_fFilterResonance ) )
			.append( QString( ", m_fRandomPitchFactor: %1" ).arg( m_fRandomPitchFactor ) )
			.append( QString( ", m_fPitchOffset: %1" ).arg( m_fPitchOffset ) )
			.append( QString( ", m_nMidiOutNote: %1" ).arg( m_nMidiOutNote ) )
			.append( QString( ", m_nMidiOutChannel: %1" ).arg( m_nMidiOutChannel ) )
			.append( QString( ", m_bStopNotes: %1" ).arg( m_bStopNotes ) )
			.append( QString( ", m_bSoloed: %1" ).arg( m_bSoloed ) )
			.append( QString( ", m_bMuted: %1" ).arg( m_bMuted ) )
			.append( QString( ", m_nMuteGroup: %1" ).arg( m_nMuteGroup ) )
			.append( QString( ", m_nQueued: %1" ).arg( m_nQueued ) )
			.append( QString( ", m_enqueuedBy: [%1]" )
					 .arg( m_enqueuedBy.join( " ; " ) ) );
		sOutput.append( QString( ", m_fxLevel: [ " ) );
		for ( const auto& ff : m_fxLevel ) {
			sOutput.append( QString( "%1 " ).arg( ff ) );
		}
		sOutput.append( QString( "]" ) )
			.append( QString( ", m_nHihatGrp: %1" ).arg( m_nHihatGrp ) )
			.append( QString( ", m_nLowerCc: %1" ).arg( m_nLowerCc ) )
			.append( QString( ", m_nHigherCc: %1" ).arg( m_nHigherCc ) )
			.append( QString( ", m_bIsPreviewInstrument: %1" ).arg( m_bIsPreviewInstrument ) )
			.append( QString( ", m_bApplyVelocity: %1" ).arg( m_bApplyVelocity ) )
			.append( QString( ", m_bCurrentInstrForExport: %1" ).arg( m_bCurrentInstrForExport ) )
			.append( QString( ", m_bHasMissingSamples: %1" ).arg( m_bHasMissingSamples ) )
			.append( QString( ", m_pComponents: [" ) );
		for ( const auto& cc : *m_pComponents ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( " %1" ).arg( cc->getName() ) );
			}
		}
		sOutput.append("]\n");
	}
		
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
