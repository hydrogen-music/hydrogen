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

#include "DownloadWidget.h"

#include <cmath>

#include <hydrogen/globals.h>

Download::Download( QWidget* pParent, const QString& sRemoteURL, const QString& sLocalFile )
 : QDialog( pParent )
 , Object( "Download" )
 , m_fPercDownload( 0 )
 , m_nETA( 0 )
 , m_nBytesCurrent( 0 )
 , m_nBytesTotal( 0 )
 , m_sRemoteURL( sRemoteURL )
 , m_sLocalFile( sLocalFile )
{
	if ( m_sLocalFile != "" ) {
		INFOLOG( QString( "Downloading %1 in %2" ).arg( sRemoteURL ).arg( sLocalFile ) );
	}
	else {
		INFOLOG( QString( "Downloading %1" ).arg( sRemoteURL ) );
	}

	QUrl url( sRemoteURL );

	// download del feed...
	connect( &m_httpClient, SIGNAL( done(bool) ), this, SLOT( fetchDone(bool) ) );
	connect( &m_httpClient, SIGNAL( dataReadProgress( int, int ) ), this, SLOT( fetchProgress( int, int ) ) );
	connect( &m_httpClient, SIGNAL( requestFinished( int, bool ) ), this, SLOT( httpRequestFinished( int, bool ) ) );
	connect( &m_httpClient, SIGNAL( responseHeaderReceived( const QHttpResponseHeader & ) ),this, SLOT( readResponseHeader( const QHttpResponseHeader & ) ) );

	QString sPath = url.path();
	sPath = sPath.replace( " ", "%20" );

	QHttpRequestHeader header( "GET", sPath );
	header.setValue( "Host", url.host() );

	m_time.start();
	m_httpClient.setHost( url.host() );
	m_httpClient.request( header );
}



Download::~Download()
{

	//INFOLOG( "DESTROY" );
}



/// TODO: devo salvare il file sul disco su una dir temporanea e poi spostarlo se e' tutto ok.
void Download::fetchDone( bool bError )
{
	if ( bError ) {
		ERRORLOG( "Error retrieving the resource." );
		return;
	}
	INFOLOG( "Download completed" );

	if ( m_sLocalFile == "" ) {
		// store the text received only when not using the file.
		m_sFeedXML = m_httpClient.readAll();
		//INFOLOG( m_sFeedXML.toStdString() );
	}
	else {
		QFile file( m_sLocalFile );
		if ( !file.open( QIODevice::WriteOnly) ) {
			ERRORLOG( QString( "Unable to save %1" ).arg( m_sLocalFile ) );
        	}
		else {
			file.write( m_httpClient.readAll() );
			file.flush();
			file.close();
		}
	}
}



void Download::fetchProgress ( int done, int total )
{
	m_nBytesCurrent = done;
	m_nBytesTotal = total;

	m_fPercDownload = (float)done / (float)total * 100.0;
}



void Download::httpRequestFinished(int requestId, bool error)
{
	UNUSED( requestId );
	UNUSED( error );
	INFOLOG( "httpRequestFinished" );
	//m_bFinished = true;
}



void Download::readResponseHeader( const QHttpResponseHeader & )
{
	INFOLOG( "readResponseHeader" );
}



// :::::::::::::::::::..



DownloadWidget::DownloadWidget( QWidget* pParent, const QString& sTitleText, const QString& sRemoteURL, const QString& sLocalFile )
 : Download( pParent, sRemoteURL, sLocalFile )
{
	setWindowTitle( sTitleText );
	setModal( true );

	setFixedSize( 500, 100 );

	QFont boldFont;
	boldFont.setBold( true );

	m_pURLLabel = new QLabel( NULL );
	m_pURLLabel->setFont( boldFont );
	m_pURLLabel->setAlignment( Qt::AlignCenter );
	m_pURLLabel->setText( QFileInfo( sRemoteURL ).fileName() );

	m_pProgressBar = new QProgressBar( NULL );

	m_pProgressBar->setMinimum( 0 );
	m_pProgressBar->setMaximum( 100 );

	m_pETALabel = new QLabel( NULL );
//	m_pETALabel->setFont( boldFont );
	m_pETALabel->setAlignment( Qt::AlignHCenter );


	QVBoxLayout* pVBox = new QVBoxLayout();
	pVBox->addWidget( m_pURLLabel );
	pVBox->addWidget( m_pProgressBar );
	pVBox->addWidget( m_pETALabel );


	setLayout( pVBox );

	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateStats() ) );

	m_pCloseTimer = new QTimer( this );
	connect( m_pCloseTimer, SIGNAL( timeout() ), this, SLOT( close() ) );

	m_pUpdateTimer->start( 100 );
}



DownloadWidget::~DownloadWidget()
{
	m_pUpdateTimer->stop();
	m_pCloseTimer->stop();
}



void DownloadWidget::updateStats()
{
	if ( m_fPercDownload > 0 ) {
		m_nETA = (int)( round( ( m_time.elapsed() / m_fPercDownload * ( 100 - m_fPercDownload ) ) / 1000 ) );
	}

	m_pProgressBar->setValue( getPercentDone() );
	int hours = m_nETA / 60 / 60;
	int minutes = ( m_nETA / 60 ) % 60;
	int seconds = m_nETA % 60;
	
	QString m_sHours = QString( "%1" ).arg( hours );
	QString m_sMinutes = QString( "%1" ).arg( minutes );
	QString m_sSeconds = QString( "%1" ).arg( seconds );

	m_sHours = m_sHours.rightJustified(2,'0');
	m_sMinutes = m_sMinutes.rightJustified(2,'0');
	m_sSeconds = m_sSeconds.rightJustified(2,'0');

	QString sETA = m_sHours + ":" + m_sMinutes + ":" +  m_sSeconds;
	
	//QString sETA = QString( "%1:%2:%3" ).arg( 24 , 3 ).arg( minutes , 2 ).arg( seconds );

	m_pETALabel->setText( trUtf8( "(%1K/%2K) - ETA %3" ).arg( m_nBytesCurrent / 1024 ).arg( m_nBytesTotal / 1024 ).arg( sETA ) );

	if ( m_fPercDownload == 100 ){
		m_pUpdateTimer->stop();

		m_pCloseTimer->start( 1000 );	// close the window after 1 second
	}
}



