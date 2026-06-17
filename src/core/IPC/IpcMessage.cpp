/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/IPC/IpcMessage.h>

namespace H2Core {

QByteArray IpcMessage::encode() const {
	// Body: opcode + args + payload.
	QByteArray body;
	{
		QDataStream ds( &body, QIODevice::WriteOnly );
		ds.setVersion( IPC_DATASTREAM_VERSION );
		ds << static_cast<quint16>( m_opcode );
		ds << m_args;
		ds << m_payload;
	}

	// Frame: [u32 bodyLength][body].
	QByteArray frame;
	{
		QDataStream ds( &frame, QIODevice::WriteOnly );
		ds.setVersion( IPC_DATASTREAM_VERSION );
		ds << static_cast<quint32>( body.size() );
	}
	frame.append( body );
	return frame;
}

bool IpcMessage::decodeBody( const QByteArray& body, IpcMessage& out ) {
	QDataStream ds( body );
	ds.setVersion( IPC_DATASTREAM_VERSION );

	quint16 nOpcode = 0;
	QVector<QVariant> args;
	QByteArray payload;
	ds >> nOpcode >> args >> payload;
	if ( ds.status() != QDataStream::Ok ) {
		return false;
	}
	if ( nOpcode >= static_cast<quint16>( IpcOpcode::OpcodeCount ) ) {
		return false;
	}

	out.m_opcode = static_cast<IpcOpcode>( nOpcode );
	out.m_args = args;
	out.m_payload = payload;
	return true;
}

IpcMessage IpcMessage::fromEvent( Event::Type type, int nValue, long nId ) {
	IpcMessage msg( IpcOpcode::Event );
	msg.arg( static_cast<int>( type ) )
		.arg( nValue )
		.arg( static_cast<qlonglong>( nId ) );
	return msg;
}

bool IpcMessage::toEventFields( Event::Type& type, int& nValue, long& nId ) const {
	if ( m_opcode != IpcOpcode::Event || m_args.size() < 3 ) {
		return false;
	}
	type = static_cast<Event::Type>( m_args[0].toInt() );
	nValue = m_args[1].toInt();
	nId = static_cast<long>( m_args[2].toLongLong() );
	return true;
}

IpcMessage IpcMessage::hello( quint16 nProtocolVersion ) {
	IpcMessage msg( IpcOpcode::Hello );
	msg.arg( static_cast<int>( nProtocolVersion ) );
	return msg;
}

quint16 IpcMessage::helloProtocolVersion() const {
	if ( m_opcode != IpcOpcode::Hello || m_args.isEmpty() ) {
		return 0;
	}
	return static_cast<quint16>( m_args[0].toInt() );
}

bool IpcFrameReader::next( IpcMessage& out ) {
	// Need the 4-byte length prefix first.
	if ( m_buffer.size() < static_cast<int>( sizeof( quint32 ) ) ) {
		return false;
	}

	quint32 nBodyLength = 0;
	{
		QDataStream ds( m_buffer );
		ds.setVersion( IPC_DATASTREAM_VERSION );
		ds >> nBodyLength;
	}

	const int nFrameSize =
		static_cast<int>( sizeof( quint32 ) ) + static_cast<int>( nBodyLength );
	if ( m_buffer.size() < nFrameSize ) {
		return false; // frame not fully received yet
	}

	const QByteArray body =
		m_buffer.mid( sizeof( quint32 ), static_cast<int>( nBodyLength ) );
	m_buffer.remove( 0, nFrameSize );

	return IpcMessage::decodeBody( body, out );
}

bool isEngineOriginEvent( Event::Type type ) {
	// Editor-internal events originate and are consumed inside the editor
	// process and must not cross IPC (ADR 0016/0018). OnlineImportProgress is
	// the worked example: online library import is editor-side (OnlineImporter),
	// so its progress events stay local. Everything else is engine-origin.
	switch ( type ) {
	case Event::Type::OnlineImportProgress:
		return false;
	default:
		return true;
	}
}

};
