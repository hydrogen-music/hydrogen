/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "FilePreview.h"
#include <hydrogen/Sample.h>
#include <hydrogen/Hydrogen.h>

#include <fstream>
#include <QLabel>

FilePreview::FilePreview( QWidget *parent )
 : QWidget( parent )
 , Object( "FilePreview" )
{
	infoLog( "INIT" );
	m_pPlayBtn = new QPushButton( trUtf8( "Play sample" ), this );
	m_pPlayBtn->move( 100 - 50, 200 - 30 );
	m_pPlayBtn->resize( 100, 25 );
	m_pPlayBtn->setEnabled( false );
	connect( m_pPlayBtn, SIGNAL(clicked()), this, SLOT(playClicked()) );

	m_pNBytes = new QLabel( this );
	m_pNBytes->move( 10, 10 );
	m_pNBytes->resize( 150, 25 );

	m_pSamplerate = new QLabel( this );
	m_pSamplerate->move( 10, 50 );
	m_pSamplerate->resize( 150, 25 );

	setMinimumSize( 200, 200 );
	setMaximumSize( 200, 1000);
	resize( 200, 200 );

}

FilePreview::~FilePreview()
{
	infoLog( "DESTROY" );
}



void FilePreview::previewUrl( const Q3Url &u )
{
	m_sFilename = u.path().latin1();

	// file exists?
	std::ifstream input( m_sFilename.c_str() , std::ios::in | std::ios::binary);
	if (input){
		// the file has a supported extension?
		if 	(
			( u.path().endsWith( ".wav" ) ) ||
			( u.path().endsWith( ".WAV" ) ) ||
			( u.path().endsWith( ".au" ) ) ||
			( u.path().endsWith( ".AU" ) ) ||
			( u.path().endsWith( ".aiff" ) ) ||
			( u.path().endsWith( ".AIFF" ) ) ||
			( u.path().endsWith( ".flac" ) ) ||
			( u.path().endsWith( ".FLAC" ) )
			) {

			// FIXME: evitare di caricare il sample, visualizzare solo le info del file
			Sample *pNewSample = Sample::load( m_sFilename );
			if (pNewSample) {
				m_pNBytes->setText( trUtf8( "Size: %1 bytes" ).arg( pNewSample->getNBytes() ) );
				m_pSamplerate->setText( trUtf8( "Samplerate: %1" ).arg( pNewSample->m_nSampleRate ) );

				delete pNewSample;
				m_pPlayBtn->setEnabled( true );
				playClicked();
			}
			else {
				m_pNBytes->setText( trUtf8( "Size: - " ) );
				m_pSamplerate->setText( trUtf8( "Samplerate: - " ) );
				m_pPlayBtn->setEnabled( false );
			}
		}
		else {
			m_pNBytes->setText( trUtf8( "Size: - " ) );
			m_pSamplerate->setText( trUtf8( "Samplerate: - " ) );
			m_pPlayBtn->setEnabled( false );
		}
	}
	else {
		errorLog( string("File not found. filename = ") + m_sFilename );
	}
}



void FilePreview::playClicked()
{
	Sample *pNewSample = Sample::load( m_sFilename.toStdString() );
	if (pNewSample) {
		( Hydrogen::getInstance() )->previewSample( pNewSample );
	}
	else {
		errorLog( "[playClicked()] pNewSample = NULL" );
	}
}

