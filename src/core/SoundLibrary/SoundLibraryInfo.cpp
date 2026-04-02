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

#include <core/SoundLibrary/SoundLibraryInfo.h>

namespace H2Core {

SoundLibraryInfo::SoundLibraryInfo() : m_context( Filesystem::Context::User )
{
}

SoundLibraryInfo::SoundLibraryInfo(
	const QString& sName,
	const QString& sURL,
	const QString& sInfo,
	const QString& sAuthor,
	const QString& sType,
	const License& license,
	const QString& sPath
)
	: m_sName( sName ),
	  m_sURL( sURL ),
	  m_sInfo( sInfo ),
	  m_sAuthor( sAuthor ),
	  m_sType( sType ),
	  m_license( license ),
	  m_sPath( sPath ),
	  m_context( Filesystem::Context::User )
{
}

SoundLibraryInfo::~SoundLibraryInfo()
{
}

QString SoundLibraryInfo::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput =
			QString( "%1[SoundLibraryInfo]\n" )
				.arg( sPrefix )
				.append( QString( "%1%2m_sName: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sName ) )
				.append( QString( "%1%2m_sURL: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sURL ) )
				.append( QString( "%1%2m_sInfo: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sInfo ) )
				.append( QString( "%1%2m_sAuthor: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sAuthor ) )
				.append( QString( "%1%2m_sType: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sType ) )
				.append(
					QString( "%1%2m_license:\n%3" )
						.arg( sPrefix )
						.arg( s )
						.arg( m_license.toQString( sPrefix + s + s, bShort ) )
				)
				.append( QString( "%1%2m_sPath: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sPath ) )
				.append( QString( "%1%2m_context: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( Filesystem::ContextToQString( m_context ) )
				);
	}
	else {
		sOutput =
			QString( "[SoundLibraryInfo]" )
				.append( QString( " m_sName: %1" ).arg( m_sName ) )
				.append( QString( ", m_sURL: %1" ).arg( m_sURL ) )
				.append( QString( ", m_sInfo: %1" ).arg( m_sInfo ) )
				.append( QString( ", m_sAuthor: %1" ).arg( m_sAuthor ) )
				.append( QString( ", m_sType: %1" ).arg( m_sType ) )
				.append( QString( ", m_license: %1" )
							 .arg( m_license.toQString( "", bShort ) ) )
				.append( QString( ", m_sPath: %1" ).arg( m_sPath ) )
				.append( QString( ", m_context: %1" )
							 .arg( Filesystem::ContextToQString( m_context ) )
				);
	}

	return sOutput;
}
};	// namespace H2Core
