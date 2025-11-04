/*
 * Hydrogen
 * Copyright(c) 2023-2023 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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

#include "MidiInstrumentMap.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiMessage.h>

#include <QtGlobal>

namespace H2Core {

/** Translated since these are displayed in the MidiControlDialog. */
QString MidiInstrumentMap::InputToQString( Input mapping ) {
	switch ( mapping ) {
	case Input::None:
		/*: No mapping between MIDI events and instrument will be done. */
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "None" );
	case Input::AsOutput:
		/*: For mapping incoming MIDI events, the note and channel settings
            specified in the output section or instrument editor will be
            used. */
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "As Output" );
	case Input::SelectedInstrument:
		/*: Only the selected instrument will used during MIDI mapping.
            Different note values will be mapped to different instrument
            pitches. */
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "Selected Instrument" );
	case Input::Order:
		/*: Incoming MIDI notes will be mapped to instruments based on their
            order in the current drumkit. */
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "Order" );
	case Input::Custom:
		/*: The use can set arbitrary note and channel values to map incoming
            MIDI notes to instruments of the current drumkit. */
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "Custom" );
	default:
		return "Unknown input mapping";
	}
}

/** Translated since these are displayed in the MidiControlDialog. */
QString MidiInstrumentMap::OutputToQString( Output mapping ) {
	switch ( mapping ) {
	case Output::None:
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "None" );
	case Output::Offset:
		/*: The MIDI output note set does apply to a C2-pitched (pattern) note
		 * of the corresponding instrument. For notes with higher or lower
		 * pitch, the resulting MIDI event will have an offset with the same
		 * difference. */
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "Offset" );
	case Output::Constant:
		/*: All send MIDI event - regardless of the (pattern) notes' individual
		 * pitch - will have the same note and channel values. */
		return QT_TRANSLATE_NOOP( "MidiInstrumentMap", "Constant" );
	default:
		return "Unknown output mapping";
	}
}

QString MidiInstrumentMap::NoteRef::toQString( const QString& sPrefix,
  											  bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[NoteRef]\n" ).arg( sPrefix )
			.append( QString( "%1%2nNote: %3\n" ).arg( sPrefix ).arg( s )
				.arg( nNote ) )
			.append( QString( "%1%2nChannel: %3\n" ).arg( sPrefix ).arg( s )
				.arg( nChannel ) );
	}
	else {
		sOutput = QString( "[NoteRef]" )
			.append( QString( " nNote: %1" ).arg( nNote ) )
			.append( QString( ", nChannel: %1" ).arg( nChannel ) );
	}
  return sOutput;
}


MidiInstrumentMap::MidiInstrumentMap()
	: m_input( Input::AsOutput )
	, m_output( Output::Offset )
	, m_bUseGlobalInputChannel( false )
	, m_nGlobalInputChannel( MidiMessage::nChannelDefault )
	, m_bUseGlobalOutputChannel( false )
	, m_nGlobalOutputChannel( MidiMessage::nChannelDefault ) {
}

MidiInstrumentMap::MidiInstrumentMap( const std::shared_ptr<MidiInstrumentMap> pOther )
	: m_input( pOther->m_input )
	, m_output( pOther->m_output )
	, m_bUseGlobalInputChannel( pOther->m_bUseGlobalInputChannel )
	, m_nGlobalInputChannel( pOther->m_nGlobalInputChannel )
	, m_bUseGlobalOutputChannel( pOther->m_bUseGlobalOutputChannel )
	, m_nGlobalOutputChannel( pOther->m_nGlobalOutputChannel ) {
	
	for ( const auto& it : pOther->m_customInputMappingsType ) {
		m_customInputMappingsType[ it.first ] = it.second;
	}
	for ( const auto& it : pOther->m_customInputMappingsId ) {
		m_customInputMappingsId[ it.first ] = it.second;
	}
}
MidiInstrumentMap::~MidiInstrumentMap() {
}

void MidiInstrumentMap::saveTo( XMLNode& node ) const {
	auto mapNode = node.createNode( "midiInstrumentMap" );
	mapNode.write_int( "input", static_cast<int>(getInput()) );
	mapNode.write_int( "output", static_cast<int>(getOutput()) );
	mapNode.write_bool( "useGlobalInputChannel",
					   getUseGlobalInputChannel() );
	mapNode.write_int( "globalInputChannel",
					  getGlobalInputChannel() );
	mapNode.write_bool( "useGlobalOutputChannel",
					   getUseGlobalOutputChannel() );
	mapNode.write_int( "globalOutputChannel",
					  getGlobalOutputChannel() );

	auto customInputNode = mapNode.createNode( "customMidiInputMappings" );
	for ( const auto [ ssType, nnoteRef ] : m_customInputMappingsType ) {
		XMLNode customInput = customInputNode.createNode( "customMidiInputMapping" );
		customInput.write_string( "instrumentType", ssType );
		customInput.write_int( "instrumentId", EMPTY_INSTR_ID );
		customInput.write_int( "note", nnoteRef.nNote );
		customInput.write_int( "channel", nnoteRef.nChannel );
	}
	for ( const auto [ nnId, nnoteRef ] : m_customInputMappingsId ) {
		XMLNode customInput = customInputNode.createNode( "customMidiInputMapping" );
		customInput.write_string( "instrumentType", "" );
		customInput.write_int( "instrumentId", nnId );
		customInput.write_int( "note", nnoteRef.nNote );
		customInput.write_int( "channel", nnoteRef.nChannel );
	}
}

std::shared_ptr<MidiInstrumentMap> MidiInstrumentMap::loadFrom( const XMLNode& node, bool bSilent ) {
	auto pMidiInstrumentMap = std::make_shared<MidiInstrumentMap>();

	pMidiInstrumentMap->setInput(
		static_cast<Input>(
			node.read_int( "input",
							 static_cast<int>( pMidiInstrumentMap->getInput() ),
							 false, false, bSilent ) ) );
	pMidiInstrumentMap->setOutput(
		static_cast<Output>(
			node.read_int( "output",
							 static_cast<int>( pMidiInstrumentMap->getOutput() ),
							 false, false, bSilent ) ) );
	pMidiInstrumentMap->setUseGlobalInputChannel(
		node.read_bool( "useGlobalInputChannel",
						  pMidiInstrumentMap->getUseGlobalInputChannel(),
						  false, false, bSilent ) );
	pMidiInstrumentMap->setGlobalInputChannel(
		node.read_int( "globalInputChannel",
						 pMidiInstrumentMap->getGlobalInputChannel(),
						 false, false, bSilent ) );
	pMidiInstrumentMap->setUseGlobalOutputChannel(
		node.read_bool( "useGlobalOutputChannel",
						  pMidiInstrumentMap->getUseGlobalOutputChannel(),
						  false, false, bSilent ) );
	pMidiInstrumentMap->setGlobalOutputChannel(
		node.read_int( "globalOutputChannel",
						 pMidiInstrumentMap->getGlobalOutputChannel(),
						 false, false, bSilent ) );

	const auto customInputMappingsNode =
		node.firstChildElement( "customMidiInputMappings" );
	if ( ! customInputMappingsNode.isNull() ) {
		XMLNode customInputMappingNode =
			customInputMappingsNode.firstChildElement( "customMidiInputMapping" );

		while ( ! customInputMappingNode.isNull() ) {
			const QString sInstrumentType = customInputMappingNode.read_string(
				"instrumentType", "", false, false, bSilent );
			const int nInstrumentId = customInputMappingNode.read_int(
				"instrumentId", EMPTY_INSTR_ID, false, false, bSilent );
			const int nNote = customInputMappingNode.read_int(
				"note", MidiMessage::nInstrumentOffset, false, false, bSilent );
			const int nChannel = customInputMappingNode.read_int(
				"channel", MidiMessage::nChannelDefault, false, false, bSilent );
			NoteRef noteRef;
			noteRef.nNote = nNote;
			noteRef.nChannel = nChannel;

			if ( ! sInstrumentType.isEmpty() ) {
				pMidiInstrumentMap->m_customInputMappingsType[ sInstrumentType ] =
					noteRef;
			}
			else {
				pMidiInstrumentMap->m_customInputMappingsId[ nInstrumentId ] =
					noteRef;
			}

			customInputMappingNode =
				customInputMappingNode.nextSiblingElement( "customMidiInputMapping" );
		}
	}

	return pMidiInstrumentMap;
}

std::vector< std::shared_ptr<Instrument> > MidiInstrumentMap::mapInput(
	int nNote,
	int nChannel,
	std::shared_ptr<Drumkit> pDrumkit ) const
{
	std::vector< std::shared_ptr<Instrument> > instruments;

	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid input" );
		return instruments;
	}

	// The global output channel has more weight as the per instrument
	// channel. But for inputs the global input channel always wins.
	auto getMappedChannel = [&]( std::shared_ptr<Instrument> pInstrument ) {
		int nChannelMapped = m_bUseGlobalInputChannel ?
			m_nGlobalInputChannel : pInstrument->getMidiOutChannel();
		if ( m_bUseGlobalOutputChannel && ! m_bUseGlobalInputChannel ) {
			nChannelMapped = m_nGlobalOutputChannel;
		}
		return nChannelMapped;
	};

	switch( m_input ) {
	case Input::AsOutput: {
		for ( const auto ppInstrument : *pDrumkit->getInstruments() ) {
			if ( ppInstrument == nullptr ) {
				continue;
			}
			const int nChannelMapped = getMappedChannel( ppInstrument );

			if ( ppInstrument->getMidiOutNote() == nNote &&
				 nChannelMapped == nChannel &&
				 nChannelMapped != MidiMessage::nChannelOff ) {
				instruments.push_back( ppInstrument );
			}
		}
		break;
	}

	case Input::Custom: {
		for ( const auto [ ssType, nnoteRef ] : m_customInputMappingsType ) {
			const int nChannelMapped = m_bUseGlobalInputChannel ?
				m_nGlobalInputChannel : nnoteRef.nChannel;
			if ( nnoteRef.nNote == nNote &&
				 nChannelMapped == nChannel &&
				 nChannelMapped != MidiMessage::nChannelOff ) {
				for ( const auto ppInstrument : *pDrumkit->getInstruments() ) {
					if ( ppInstrument != nullptr &&
						 ppInstrument->getType() == ssType ) {
						instruments.push_back( ppInstrument );
					}
				}
			}
		}
		for ( const auto [ nnId, nnoteRef ] : m_customInputMappingsId ) {
			const int nChannelMapped = m_bUseGlobalInputChannel ?
				m_nGlobalInputChannel : nnoteRef.nChannel;
			if ( nnoteRef.nNote == nNote &&
				 nChannelMapped == nChannel &&
				 nChannelMapped != MidiMessage::nChannelOff ) {
				const auto pInstrument = pDrumkit->getInstruments()->find( nnId );
				if ( pInstrument != nullptr ) {
					instruments.push_back( pInstrument );
				}
			}
		}

		break;
	}

	case Input::SelectedInstrument: {
		auto pInstrument = pDrumkit->getInstruments()->get(
			Hydrogen::get_instance()->getSelectedInstrumentNumber() );
		if ( pInstrument != nullptr ) {
			const int nChannelMapped = getMappedChannel( pInstrument );
			if ( nChannelMapped == nChannel &&
				 nChannelMapped != MidiMessage::nChannelOff ) {
				instruments.push_back( pInstrument );
			}
		}

		break;
	}

	case Input::Order: {
		const int nIndex = nNote - MidiMessage::nInstrumentOffset;
		if ( nIndex >= 0 && nIndex < pDrumkit->getInstruments()->size() ) {
			auto pInstrument = pDrumkit->getInstruments()->get( nIndex );
			if ( pInstrument != nullptr ) {
				const int nChannelMapped = getMappedChannel( pInstrument );
				if ( nChannelMapped == nChannel &&
					 nChannelMapped != MidiMessage::nChannelOff ) {
					instruments.push_back( pInstrument );
				}
			}
		}

		break;
	}

	case Input::None:
	default:
		// Nothing added
		break;
	};

	return instruments;
}

MidiInstrumentMap::NoteRef MidiInstrumentMap::getInputMapping(
	std::shared_ptr<Instrument> pInstrument,
	std::shared_ptr<Drumkit> pDrumkit ) const
{
	if ( pInstrument == nullptr || pDrumkit == nullptr ) {
		ERRORLOG( "Invalid input" );
		return NoteRef();
	}

	const int nChannelUsed = m_bUseGlobalInputChannel ?
		m_nGlobalInputChannel : pInstrument->getMidiOutChannel();

	NoteRef noteRef;

	switch( m_input ) {
	case Input::AsOutput: {
		// The global output channel has more weight as the per instrument
		// channel. But for inputs the global input channel always wins.
		if ( m_bUseGlobalOutputChannel && ! m_bUseGlobalInputChannel ) {
			noteRef.nChannel = m_nGlobalOutputChannel;
		}
		else {
			noteRef.nChannel = nChannelUsed;
		}
		noteRef.nNote = pInstrument->getMidiOutNote();
		break;
	}

	case Input::Custom: {
		const auto sType = pInstrument->getType();
		if ( ! sType.isEmpty() &&
			 m_customInputMappingsType.find( sType ) !=
			 m_customInputMappingsType.end() ) {
			noteRef = m_customInputMappingsType.at( sType );
		}
		else if ( sType.isEmpty() &&
 				  m_customInputMappingsId.find( pInstrument->getId() ) !=
				  m_customInputMappingsId.end() ) {
			noteRef = m_customInputMappingsId.at( pInstrument->getId() );
		}
		else {
			// No custom mapping. Fall back to output values.
			noteRef.nChannel = nChannelUsed;
			noteRef.nNote = pInstrument->getMidiOutNote();
		}
		break;
	}

	case Input::SelectedInstrument: {
		if ( Hydrogen::get_instance()->getSelectedInstrumentNumber() ==
			 pDrumkit->getInstruments()->index( pInstrument ) ) {
			noteRef.nChannel = nChannelUsed;
			// The note information is not handled by the UI
			noteRef.nNote = pInstrument->getMidiOutNote();
		}
		else {
			// Invalid mapping
			noteRef = NoteRef();
		}
		break;
	}

	case Input::Order: {
		noteRef.nChannel = nChannelUsed;
		const int nId = pDrumkit->getInstruments()->index( pInstrument );
		if ( nId != -1 ) {
			noteRef.nNote = nId + MidiMessage::nInstrumentOffset;
		} else {
			noteRef = NoteRef();
		}
		break;
	}

	case Input::None:
	default:
		// The default constructor of NoteRef yields a null value.
		break;
	};

	return noteRef;
}

MidiInstrumentMap::NoteRef MidiInstrumentMap::getOutputMapping(
	std::shared_ptr<Instrument> pInstrument ) const
{
	if ( pInstrument == nullptr ) {
		ERRORLOG( "Invalid input" );
		return NoteRef();
	}

	NoteRef noteRef;

	switch( m_output ) {
	case Output::Offset:
	case Output::Constant: {
		noteRef.nChannel = pInstrument->getMidiOutChannel();
		noteRef.nNote = pInstrument->getMidiOutNote();
		break;
	}

	case Output::None:
	default:
		// The default constructor of NoteRef yields a null value.
		break;
	};

	if ( m_bUseGlobalOutputChannel ) {
		noteRef.nChannel = m_nGlobalOutputChannel;
	}

	return noteRef;
}

void MidiInstrumentMap::setGlobalInputChannel( int nValue ) {
	m_nGlobalInputChannel = std::clamp( nValue, MidiMessage::nChannelMinimum,
									   MidiMessage::nChannelMaximum );
}

void MidiInstrumentMap::setGlobalOutputChannel( int nValue ) {
	m_nGlobalOutputChannel = std::clamp( nValue, MidiMessage::nChannelMinimum,
										 MidiMessage::nChannelMaximum );
}

void MidiInstrumentMap::insertCustomInputMapping(
	std::shared_ptr<Instrument> pInstrument, int nNote, int nChannel )
{
	if ( pInstrument == nullptr ) {
		return;
	}

	NoteRef noteRef;
	noteRef.nNote = nNote;
	noteRef.nChannel = nChannel;

	if ( ! pInstrument->getType().isEmpty() ) {
		m_customInputMappingsType[ pInstrument->getType() ] = noteRef;
	}
	else {
		m_customInputMappingsId[ pInstrument->getId() ] = noteRef;
	}
}

QString MidiInstrumentMap::toQString( const QString& sPrefix, bool bShort ) const {

	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[MidiInstrumentMap]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_input: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( InputToQString( m_input ) ) )
			.append( QString( "%1%2m_output: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( OutputToQString( m_output ) ) )
			.append( QString( "%1%2m_bUseGlobalInputChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_bUseGlobalInputChannel ) )
			.append( QString( "%1%2m_nGlobalInputChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nGlobalInputChannel ) )
			.append( QString( "%1%2m_bUseGlobalOutputChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_bUseGlobalOutputChannel ) )
			.append( QString( "%1%2m_nGlobalOutputChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_nGlobalOutputChannel ) )
			.append( QString( "%1%2m_customInputMappingsType:\n" ).arg( sPrefix )
					 .arg( s ) );
		for ( const auto [ ssType, nnoteRef ] : m_customInputMappingsType ) {
			sOutput.append( QString( "%1%2%2%3: %4" ).arg( sPrefix ).arg( s )
						   .arg( ssType ).arg( nnoteRef.toQString( "", false ) ) );
		}
		sOutput.append( QString( "%1%2m_customInputMappingsId:\n" ).arg( sPrefix )
					 .arg( s ) );
		for ( const auto [ nnId, nnoteRef ] : m_customInputMappingsId ) {
			sOutput.append( QString( "%1%2%2%3: %4" ).arg( sPrefix ).arg( s )
						   .arg( nnId ).arg( nnoteRef.toQString( "", false ) ) );
		}
}
	else {
		sOutput = QString( "[MidiInstrumentMap]" )
			.append( QString( " m_input: %1" )
					 .arg( InputToQString( m_input ) ) )
			.append( QString( ", m_output: %1" )
					 .arg( OutputToQString( m_output ) ) )
			.append( QString( ", m_bUseGlobalInputChannel: %1" )
					 .arg( m_bUseGlobalInputChannel ) )
			.append( QString( ", m_nGlobalInputChannel: %1" )
					 .arg( m_nGlobalInputChannel ) )
			.append( QString( ", m_bUseGlobalOutputChannel: %1" )
					 .arg( m_bUseGlobalOutputChannel ) )
			.append( QString( ", m_nGlobalOutputChannel: %1" )
					 .arg( m_nGlobalOutputChannel ) )
			.append( ", m_customInputMappingsType: [" );
		for ( const auto [ ssType, nnoteRef ] : m_customInputMappingsType ) {
			sOutput.append( QString( "%1: %2, " ).arg( ssType )
						   .arg( nnoteRef.toQString( "", false ) ) );
		}
		sOutput.append( "], m_customInputMappingsId: [" );
		for ( const auto [ nnId, nnoteRef ] : m_customInputMappingsId ) {
			sOutput.append( QString( "%1: %2, " ).arg( nnId )
						   .arg( nnoteRef.toQString( "", false ) ) );
		}
		sOutput.append( "]" );
	}

	return std::move( sOutput );
}

};
