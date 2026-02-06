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
#include <core/Basics/Note.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiMessage.h>

#include <QtGlobal>
#include "Midi/Midi.h"

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

QString MidiInstrumentMap::NoteRef::toQString(
	const QString& sPrefix,
	bool bShort
) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput = QString( "%1[NoteRef]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2note: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( static_cast<int>( note ) ) )
					  .append( QString( "%1%2channel: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( static_cast<int>( channel ) ) );
	}
	else {
		sOutput =
			QString( "[NoteRef]" )
				.append( QString( " note: %1" ).arg( static_cast<int>( note ) )
				)
				.append( QString( ", channel: %1" )
							 .arg( static_cast<int>( channel ) ) );
	}
	return sOutput;
}

MidiInstrumentMap::MidiInstrumentMap()
	: m_input( Input::AsOutput )
	, m_output( Output::Offset )
	, m_bUseGlobalInputChannel( true )
	, m_globalInputChannel( Midi::ChannelDefault )
	, m_bUseGlobalOutputChannel( false )
	, m_globalOutputChannel( Midi::ChannelDefault ) {
}

MidiInstrumentMap::MidiInstrumentMap( const std::shared_ptr<MidiInstrumentMap> pOther )
	: m_input( pOther->m_input )
	, m_output( pOther->m_output )
	, m_bUseGlobalInputChannel( pOther->m_bUseGlobalInputChannel )
	, m_globalInputChannel( pOther->m_globalInputChannel )
	, m_bUseGlobalOutputChannel( pOther->m_bUseGlobalOutputChannel )
	, m_globalOutputChannel( pOther->m_globalOutputChannel ) {
	
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
	mapNode.write_bool( "useGlobalInputChannel", getUseGlobalInputChannel() );
	// The file-based representation of the MIDI channel is zero-based (for
	// historical reasons) while we start with 1 within the application (since
	// version 2.0).
	mapNode.write_int( "globalInputChannel",
					  static_cast<int>(getGlobalInputChannel()) - 1 );
	mapNode.write_bool( "useGlobalOutputChannel",
					   getUseGlobalOutputChannel() );
	// The file-based representation of the MIDI channel is zero-based (for
	// historical reasons) while we start with 1 within the application (since
	// version 2.0).
	mapNode.write_int( "globalOutputChannel",
					  static_cast<int>(getGlobalOutputChannel()) - 1 );

	auto customInputNode = mapNode.createNode( "customMidiInputMappings" );
	for ( const auto [ssType, nnoteRef] : m_customInputMappingsType ) {
		XMLNode customInput =
			customInputNode.createNode( "customMidiInputMapping" );
		customInput.write_string( "instrumentType", ssType );
		customInput.write_int(
			"instrumentId", static_cast<int>( Instrument::EmptyId )
		);
		customInput.write_int( "note", static_cast<int>( nnoteRef.note ) );
		// The file-based representation of the MIDI channel is zero-based (for
		// historical reasons) while we start with 1 within the application
		// (since version 2.0).
		customInput.write_int(
			"channel", static_cast<int>( nnoteRef.channel ) - 1
		);
	}
	for ( const auto [iid, nnoteRef] : m_customInputMappingsId ) {
		XMLNode customInput =
			customInputNode.createNode( "customMidiInputMapping" );
		customInput.write_string( "instrumentType", "" );
		customInput.write_int( "instrumentId", static_cast<int>( iid ) );
		customInput.write_int( "note", static_cast<int>( nnoteRef.note ) );
		// The file-based representation of the MIDI channel is zero-based (for
		// historical reasons) while we start with 1 within the application
		// (since version 2.0).
		customInput.write_int(
			"channel", static_cast<int>( nnoteRef.channel ) - 1
		);
	}
}

std::shared_ptr<MidiInstrumentMap>
MidiInstrumentMap::loadFrom( const XMLNode& node, bool bSilent )
{
	auto pMidiInstrumentMap = std::make_shared<MidiInstrumentMap>();

	pMidiInstrumentMap->setInput( static_cast<Input>( node.read_int(
		"input", static_cast<int>( pMidiInstrumentMap->getInput() ), false,
		false, bSilent
	) ) );
	pMidiInstrumentMap->setOutput( static_cast<Output>( node.read_int(
		"output", static_cast<int>( pMidiInstrumentMap->getOutput() ), false,
		false, bSilent
	) ) );
	pMidiInstrumentMap->setUseGlobalInputChannel( node.read_bool(
		"useGlobalInputChannel", pMidiInstrumentMap->getUseGlobalInputChannel(),
		false, false, bSilent
	) );
	// The file-based representation of the MIDI channel is zero-based (for
	// historical reasons) while we start with 1 within the application (since
	// version 2.0).
	pMidiInstrumentMap->setGlobalInputChannel( Midi::channelFromInt(
		node.read_int(
			"globalInputChannel",
			static_cast<int>( pMidiInstrumentMap->getGlobalInputChannel() ) - 1,
			false, false, bSilent
		) +
		1
	) );
	pMidiInstrumentMap->setUseGlobalOutputChannel( node.read_bool(
		"useGlobalOutputChannel",
		pMidiInstrumentMap->getUseGlobalOutputChannel(), false, false, bSilent
	) );
	// The file-based representation of the MIDI channel is zero-based (for
	// historical reasons) while we start with 1 within the application (since
	// version 2.0).
	pMidiInstrumentMap->setGlobalOutputChannel( Midi::channelFromInt(
		node.read_int(
			"globalOutputChannel",
			static_cast<int>( pMidiInstrumentMap->getGlobalOutputChannel() ) -
				1,
			false, false, bSilent
		) +
		1
	) );

	const auto customInputMappingsNode =
		node.firstChildElement( "customMidiInputMappings" );
	if ( ! customInputMappingsNode.isNull() ) {
		XMLNode customInputMappingNode =
			customInputMappingsNode.firstChildElement( "customMidiInputMapping" );

		while ( ! customInputMappingNode.isNull() ) {
			const Instrument::Type sInstrumentType = customInputMappingNode.read_string(
				"instrumentType", "", false, false, bSilent );
			const auto id =
				static_cast<Instrument::Id>( customInputMappingNode.read_int(
					"instrumentId", static_cast<int>( Instrument::EmptyId ),
					false, false, bSilent
				) );
			const auto note =
				Midi::noteFromInt( customInputMappingNode.read_int(
					"note", static_cast<int>( Midi::NoteDefault ), false, false,
					bSilent
				) );
			// The file-based representation of the MIDI channel is zero-based
			// (for historical reasons) while we start with 1 within the
			// application (since version 2.0).
			const auto channel = Midi::channelFromInt(
				customInputMappingNode.read_int(
					"channel", static_cast<int>( Midi::ChannelDefault ) - 1,
					false, false, bSilent
				) +
				1
			);
			NoteRef noteRef;
			noteRef.note = note;
			noteRef.channel = channel;

			if ( ! sInstrumentType.isEmpty() ) {
				pMidiInstrumentMap->m_customInputMappingsType[ sInstrumentType ] =
					noteRef;
			}
			else {
				pMidiInstrumentMap->m_customInputMappingsId[ id ] =
					noteRef;
			}

			customInputMappingNode =
				customInputMappingNode.nextSiblingElement( "customMidiInputMapping" );
		}
	}

	return pMidiInstrumentMap;
}

std::vector< std::shared_ptr<Instrument> > MidiInstrumentMap::mapInput(
	Midi::Note note,
	Midi::Channel channel,
	std::shared_ptr<Drumkit> pDrumkit ) const
{
	std::vector< std::shared_ptr<Instrument> > instruments;

	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid input" );
		return instruments;
	}

	if ( channel == Midi::ChannelOff ) {
		return instruments;
	}

	// The global output channel has more weight as the per instrument
	// channel. But for inputs the global input channel always wins.
	auto getMappedChannel = [&]( std::shared_ptr<Instrument> pInstrument ) {
		auto channelMapped = m_bUseGlobalInputChannel ?
			m_globalInputChannel : pInstrument->getMidiOutChannel();
		if ( m_bUseGlobalOutputChannel && ! m_bUseGlobalInputChannel ) {
			channelMapped = m_globalOutputChannel;
		}
		return channelMapped;
	};

	switch( m_input ) {
	case Input::AsOutput: {
		for ( const auto ppInstrument : *pDrumkit->getInstruments() ) {
			if ( ppInstrument == nullptr ) {
				continue;
			}
			const auto channelMapped = getMappedChannel( ppInstrument );

			if ( ppInstrument->getMidiOutNote() == note &&
				 ( channelMapped == channel ||
				   channelMapped == Midi::ChannelAll ||
				   channel == Midi::ChannelAll ) ) {
				instruments.push_back( ppInstrument );
			}
		}
		break;
	}

	case Input::Custom: {
		for ( const auto [ ssType, nnoteRef ] : m_customInputMappingsType ) {
			const auto channelMapped = m_bUseGlobalInputChannel ?
				m_globalInputChannel : nnoteRef.channel;
			if ( nnoteRef.note == note &&
				 ( channelMapped == channel ||
				   channelMapped == Midi::ChannelAll ||
				   channel == Midi::ChannelAll ) ) {
				for ( const auto ppInstrument : *pDrumkit->getInstruments() ) {
					if ( ppInstrument != nullptr &&
						 ppInstrument->getType() == ssType ) {
						instruments.push_back( ppInstrument );
					}
				}
			}
		}
		for ( const auto [ iid, nnoteRef ] : m_customInputMappingsId ) {
			const auto channelMapped = m_bUseGlobalInputChannel ?
				m_globalInputChannel : nnoteRef.channel;
			if ( nnoteRef.note == note &&
				 ( channelMapped == channel ||
				   channelMapped == Midi::ChannelAll ||
				   channel == Midi::ChannelAll ) ) {
				const auto pInstrument = pDrumkit->getInstruments()->find( iid );
				if ( pInstrument != nullptr ) {
					instruments.push_back( pInstrument );
				}
			}
		}

		const auto fallbackMap = createFallbackMap();
		for ( const auto& [nnoteRef, ppInstrument] : fallbackMap ) {
			if ( nnoteRef.note == note &&
				 ( nnoteRef.channel == channel ||
				   nnoteRef.channel == Midi::ChannelAll ||
				   channel == Midi::ChannelAll ) ) {
				instruments.push_back( ppInstrument );
			}
		}

		break;
	}

	case Input::SelectedInstrument: {
		auto pInstrument = pDrumkit->getInstruments()->get(
			Hydrogen::get_instance()->getSelectedInstrumentNumber() );
		if ( pInstrument != nullptr ) {
			const auto channelMapped = getMappedChannel( pInstrument );
			if ( channelMapped == channel ||
				 channelMapped == Midi::ChannelAll ||
				 channel == Midi::ChannelAll ) {
				instruments.push_back( pInstrument );
			}
		}

		break;
	}

	case Input::Order: {
		const int nIndex =
			static_cast<int>( note ) - static_cast<int>( Midi::NoteOffset );
		if ( nIndex >= 0 && nIndex < pDrumkit->getInstruments()->size() ) {
			auto pInstrument = pDrumkit->getInstruments()->get( nIndex );
			if ( pInstrument != nullptr ) {
				const auto channelMapped = getMappedChannel( pInstrument );
				if ( channelMapped == channel ||
					 channelMapped == Midi::ChannelAll ||
					 channel == Midi::ChannelAll ) {
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

	const auto channelUsed = m_bUseGlobalInputChannel ?
		m_globalInputChannel : pInstrument->getMidiOutChannel();

	NoteRef noteRef;

	switch( m_input ) {
	case Input::AsOutput: {
		// The global output channel has more weight as the per instrument
		// channel. But for inputs the global input channel always wins.
		if ( m_bUseGlobalOutputChannel && ! m_bUseGlobalInputChannel ) {
			noteRef.channel = m_globalOutputChannel;
		}
		else {
			noteRef.channel = channelUsed;
		}
		noteRef.note = pInstrument->getMidiOutNote();
		break;
	}

	case Input::Custom: {
		const auto sType = pInstrument->getType();
		if ( ! sType.isEmpty() &&
			 m_customInputMappingsType.find( sType ) !=
			 m_customInputMappingsType.end() ) {
			noteRef = m_customInputMappingsType.at( sType );

			if ( m_bUseGlobalInputChannel ) {
				noteRef.channel = m_globalInputChannel;
			}
		}
		else if ( sType.isEmpty() &&
 				  m_customInputMappingsId.find( pInstrument->getId() ) !=
				  m_customInputMappingsId.end() ) {
			noteRef = m_customInputMappingsId.at( pInstrument->getId() );

			if ( m_bUseGlobalInputChannel ) {
				noteRef.channel = m_globalInputChannel;
			}
		}
		else {
			// No custom mapping. Fall back to output values.
			noteRef.channel = channelUsed;
			noteRef.note = pInstrument->getMidiOutNote();
		}
		break;
	}

	case Input::SelectedInstrument: {
		if ( Hydrogen::get_instance()->getSelectedInstrumentNumber() ==
			 pDrumkit->getInstruments()->index( pInstrument ) ) {
			noteRef.channel = channelUsed;
			// The note information is not handled by the UI
			noteRef.note = pInstrument->getMidiOutNote();
		}
		else {
			// Invalid mapping
			noteRef = NoteRef();
		}
		break;
	}

	case Input::Order: {
		noteRef.channel = channelUsed;
		const int nIndex = pDrumkit->getInstruments()->index( pInstrument );
		if ( nIndex != -1 ) {
			noteRef.note = Midi::noteFromIntClamp(
				nIndex + static_cast<int>( Midi::NoteOffset )
			);
		}
		else {
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
	std::shared_ptr<Note> pNote, std::shared_ptr<Instrument> pInstrument ) const
{
	if ( pNote != nullptr && pInstrument == nullptr ) {
		pInstrument = pNote->getInstrument();
	}
	if ( pInstrument == nullptr ) {
		ERRORLOG( "Invalid input" );
		return NoteRef();
	}

	NoteRef noteRef;

	switch( m_output ) {
	case Output::Offset: {
		noteRef.channel = pInstrument->getMidiOutChannel();
		if ( pNote != nullptr ) {
            // Within this mode the resulting MIDI note can be adjusted by
            // changing the note's pitch within the PianoRollEditor. Pitch
            // contributions from #Instrument, #InstrumentLayer, and
            // humanization are not taken into account.
			noteRef.note = Midi::noteFromIntClamp(
				static_cast<int>( pInstrument->getMidiOutNote() ) +
				static_cast<int>(
					std::round( static_cast<float>( pNote->toPitch() ) )
				)
			);
		}
		else {
			noteRef.note = pInstrument->getMidiOutNote();
		}
		break;
	}
	case Output::Constant: {
		noteRef.channel = pInstrument->getMidiOutChannel();
		noteRef.note = pInstrument->getMidiOutNote();
		break;
	}

	case Output::None:
	default:
		// The default constructor of NoteRef yields a null value.
		return noteRef;
	};

	if ( m_bUseGlobalOutputChannel ) {
		noteRef.channel = m_globalOutputChannel;
	}

	return noteRef;
}

void MidiInstrumentMap::setGlobalInputChannel( Midi::Channel channel )
{
	m_globalInputChannel = channel;
}

void MidiInstrumentMap::setGlobalOutputChannel( Midi::Channel channel )
{
	m_globalOutputChannel = channel;
}

void MidiInstrumentMap::insertCustomInputMapping(
	std::shared_ptr<Instrument> pInstrument,
	Midi::Note note,
	Midi::Channel channel
)
{
	if ( pInstrument == nullptr ) {
		return;
	}

	NoteRef noteRef;
	noteRef.note = note;
	noteRef.channel = channel;

	if ( ! pInstrument->getType().isEmpty() ) {
		m_customInputMappingsType[ pInstrument->getType() ] = noteRef;
	}
	else {
		m_customInputMappingsId[ pInstrument->getId() ] = noteRef;
	}
}

std::map<MidiInstrumentMap::NoteRef, std::shared_ptr<Instrument>>
MidiInstrumentMap::createFallbackMap() const
{
	std::map<NoteRef, std::shared_ptr<Instrument>> fallbackMap;

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return fallbackMap;
	}

	for ( const auto& ppInstrument : *pSong->getDrumkit()->getInstruments() ) {
		if ( ppInstrument == nullptr ) {
			continue;
		}

		// Is the instrument already covered by a custom mapping?
		if ( m_customInputMappingsType.find( ppInstrument->getType() ) !=
				 m_customInputMappingsType.end() ||
			 m_customInputMappingsId.find( ppInstrument->getId() ) !=
				 m_customInputMappingsId.end() ) {
			continue;
		}

		// No mapping yet. Create fallback.

		const auto channelUsed = m_bUseGlobalInputChannel
									 ? m_globalInputChannel
									 : ppInstrument->getMidiOutChannel();

		NoteRef noteRef;

		// The global output channel has more weight as the per
		// instrument channel. But for inputs the global input channel
		// always wins.
		if ( m_bUseGlobalOutputChannel && !m_bUseGlobalInputChannel ) {
			noteRef.channel = m_globalOutputChannel;
		}
		else {
			noteRef.channel = channelUsed;
		}
		noteRef.note = ppInstrument->getMidiOutNote();

		fallbackMap[noteRef] = ppInstrument;
	}
	return fallbackMap;
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
					 .arg( s ).arg( static_cast<int>(m_globalInputChannel) ) )
			.append( QString( "%1%2m_bUseGlobalOutputChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( m_bUseGlobalOutputChannel ) )
			.append( QString( "%1%2m_nGlobalOutputChannel: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( static_cast<int>(m_globalOutputChannel) ) )
			.append( QString( "%1%2m_customInputMappingsType:\n" ).arg( sPrefix )
					 .arg( s ) );
		for ( const auto [ ssType, nnoteRef ] : m_customInputMappingsType ) {
			sOutput.append( QString( "%1%2%2%3: %4" ).arg( sPrefix ).arg( s )
						   .arg( ssType ).arg( nnoteRef.toQString( "", false ) ) );
		}
		sOutput.append( QString( "%1%2m_customInputMappingsId:\n" ).arg( sPrefix )
					 .arg( s ) );
		for ( const auto [iid, nnoteRef] : m_customInputMappingsId ) {
			sOutput.append( QString( "%1%2%2%3: %4" )
								.arg( sPrefix )
								.arg( s )
								.arg( static_cast<int>( iid ) )
								.arg( nnoteRef.toQString( "", false ) ) );
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
			.append( QString( ", m_globalInputChannel: %1" )
					 .arg( static_cast<int>(m_globalInputChannel) ) )
			.append( QString( ", m_bUseGlobalOutputChannel: %1" )
					 .arg( m_bUseGlobalOutputChannel ) )
			.append( QString( ", m_globalOutputChannel: %1" )
					 .arg( static_cast<int>(m_globalOutputChannel) ) )
			.append( ", m_customInputMappingsType: [" );
		for ( const auto [ ssType, nnoteRef ] : m_customInputMappingsType ) {
			sOutput.append( QString( "%1: %2, " ).arg( ssType )
						   .arg( nnoteRef.toQString( "", false ) ) );
		}
		sOutput.append( "], m_customInputMappingsId: [" );
		for ( const auto [ iid, nnoteRef ] : m_customInputMappingsId ) {
			sOutput.append( QString( "%1: %2, " ).arg( static_cast<int>(iid) )
						   .arg( nnoteRef.toQString( "", false ) ) );
		}
		sOutput.append( "]" );
	}

	return std::move( sOutput );
}

};
