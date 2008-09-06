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


RedirectHttp::RedirectHttp( QObject* parent )
		: QHttp( parent )
		, m_device( 0 )
		, m_to( 0 )
		, m_lastRequest( 0 )
{
	connect( this, SIGNAL( responseHeaderReceived( const QHttpResponseHeader& ) ), SLOT( onHeaderReceived( const QHttpResponseHeader& ) ) );
	connect( this, SIGNAL( requestFinished( int , bool ) ), SLOT( onRequestFinished( int , bool ) ) );
	connect( this, SIGNAL( requestStarted( int ) ), SLOT( onRequestStarted( int ) ) );
}


RedirectHttp::~RedirectHttp()
{
}


int RedirectHttp::get( const QString& path, QIODevice* to )
{
	m_mode = GET;
	m_data = QByteArray();
	m_to = to;

	m_lastRequest = QHttp::get( path, to );
	return m_lastRequest;
}


int RedirectHttp::post( const QString& path, QIODevice* data, QIODevice* to )
{
	m_mode = POSTIO;
	m_data = QByteArray();
	m_device = data;
	m_to = to;

	m_lastRequest = QHttp::post( path, data );
	return m_lastRequest;
}


int RedirectHttp::post( const QString& path, const QByteArray& data, QIODevice* to )
{
	m_mode = POST;
	m_data = data;
	m_to = to;

	m_lastRequest = QHttp::post( path, data );
	return m_lastRequest;
}


int RedirectHttp::request( const QHttpRequestHeader& header, QIODevice* data, QIODevice* to )
{
	m_mode = REQUESTIO;
	m_data = QByteArray();
	m_device = data;
	m_header = header;
	m_to = to;

	m_lastRequest = QHttp::request( header, data, to );
	return m_lastRequest;
}


int RedirectHttp::request( const QHttpRequestHeader& header, const QByteArray& data, QIODevice* to )
{
	m_mode = REQUEST;
	m_data = data;
	m_header = header;
	m_to = to;

	m_lastRequest = QHttp::request( header, data, to );
	return m_lastRequest;
}


void RedirectHttp::onHeaderReceived( const QHttpResponseHeader& resp )
{
	switch ( resp.statusCode() ) {
		case 301:   //Moved Permanently
		case 302:
		case 307: { //Temporary Redirect
			QString redirectUrl = resp.value( "location" );
			_INFOLOG( "Http request returned redirect (301, 302 or 307): " + redirectUrl );

			blockSignals( true );

			abort();
			close();

			QUrl url( redirectUrl );

			if ( !url.isValid() )
				return;

			setHost( url.host(), url.port() > 0 ? url.port() : 80 );

			int oldId = m_lastRequest;

			int id;

			switch ( m_mode ) {
				case GET:
					id = get( url.path(), m_to );
					break;

				case POST:
					id = post( url.path(), m_data, m_to );
					break;

				case POSTIO:
					id = post( url.path(), m_device, m_to );
					break;

				case REQUEST:
					m_header.setRequest( "GET", url.path() );
					m_header.setValue( "Host", url.host() );

					id = request( m_header, m_data, m_to );
					break;

				case REQUESTIO:
					m_header.setRequest( "GET", url.path() );
					m_header.setValue( "Host", url.host() );

					id = request( m_header, m_device, m_to );
					break;
			}

			m_idTrans.insert( id, oldId );

			blockSignals( false );
		}

		break;
	}
}


void RedirectHttp::onRequestFinished( int id, bool error )
{
	int tId = id;

	if ( m_idTrans.contains( id ) ) {
		tId = m_idTrans.value( id );
	}

	if ( id != tId )
		emit requestFinished( tId, error );
}


void RedirectHttp::onRequestStarted( int id )
{
	int tId = id;

	if ( m_idTrans.contains( id ) ) {
		tId = m_idTrans.value( id );
	}

	if ( id != tId )
		emit requestStarted( tId );
}




////////




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

	} else {
		INFOLOG( QString( "Downloading %1" ).arg( sRemoteURL ) );
	}

	QUrl url( sRemoteURL );

	// download del feed...
	connect( &m_httpClient, SIGNAL( done( bool ) ), this, SLOT( fetchDone( bool ) ) );
	connect( &m_httpClient, SIGNAL( dataReadProgress( int, int ) ), this, SLOT( fetchProgress( int, int ) ) );
	connect( &m_httpClient, SIGNAL( requestFinished( int, bool ) ), this, SLOT( httpRequestFinished( int, bool ) ) );

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

	INFOLOG( "Download completed. " );

	INFOLOG( m_httpClient.errorString().toStdString().c_str() );

	if ( m_sLocalFile == "" ) {
		// store the text received only when not using the file.
		m_sFeedXML = m_httpClient.readAll();
		//INFOLOG( m_sFeedXML.toStdString() );

	} else {
		QFile file( m_sLocalFile );

		if ( !file.open( QIODevice::WriteOnly ) ) {
			ERRORLOG( QString( "Unable to save %1" ).arg( m_sLocalFile ) );

		} else {
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

	m_fPercDownload = ( float )done / ( float )total * 100.0;
}



void Download::httpRequestFinished( int requestId, bool error )
{
	if ( error ) {
		INFOLOG( "Error" );
		ERRORLOG( m_httpClient.errorString() );
	}
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
// m_pETALabel->setFont( boldFont );
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
		m_nETA = ( int )( round( ( m_time.elapsed() / m_fPercDownload * ( 100 - m_fPercDownload ) ) / 1000 ) );
	}

	m_pProgressBar->setValue( getPercentDone() );

	int hours = m_nETA / 60 / 60;
	int minutes = ( m_nETA / 60 ) % 60;
	int seconds = m_nETA % 60;

	QString m_sHours = QString( "%1" ).arg( hours );
	QString m_sMinutes = QString( "%1" ).arg( minutes );
	QString m_sSeconds = QString( "%1" ).arg( seconds );

	m_sHours = m_sHours.rightJustified( 2, '0' );
	m_sMinutes = m_sMinutes.rightJustified( 2, '0' );
	m_sSeconds = m_sSeconds.rightJustified( 2, '0' );

	QString sETA = m_sHours + ":" + m_sMinutes + ":" +  m_sSeconds;

	//QString sETA = QString( "%1:%2:%3" ).arg( 24 , 3 ).arg( minutes , 2 ).arg( seconds );

	m_pETALabel->setText( trUtf8( "(%1KiB/%2KiB) - ETA %3" ).arg( m_nBytesCurrent / 1024 ).arg( m_nBytesTotal / 1024 ).arg( sETA ) );

	if ( m_fPercDownload == 100 ) {
		m_pUpdateTimer->stop();

		m_pCloseTimer->start( 1000 ); // close the window after 1 second
	}
}



