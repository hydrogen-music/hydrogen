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
#include <cstdlib>


Download::Download( QWidget* pParent, const QString& download_url, const QString& local_file )
		: QDialog( pParent )
		, Object( "Download" )
		, __download_percent( 0 )
		, __eta( 0 )
		, __bytes_current( 0 )
		, __bytes_total( 0 )
		, __remote_url( download_url )
		, __local_file( local_file )
{
	if ( !__local_file.isEmpty() ) {
		INFOLOG( QString( "Downloading '%1' in '%2'" ).arg( __remote_url ).arg( __local_file ) );

	} else {
		INFOLOG( QString( "Downloading '%1'" ).arg( __remote_url ) );
	}

	QUrl url( __remote_url );

	QString sEnvHttpProxy		= QString( getenv( "http_proxy" ) );
	int     nEnvHttpPort		= 0;
	QString sEnvHttpUser		= QString( getenv( "http_user" ) );
	QString sEnvHttpPassword	= QString( getenv( "http_password" ) );

	nEnvHttpPort	= sEnvHttpProxy.right( sEnvHttpProxy.length() - sEnvHttpProxy.indexOf(':') - 1 ).toInt();
	sEnvHttpProxy	= sEnvHttpProxy.left( sEnvHttpProxy.indexOf(':') );

	connect( &__http_client, SIGNAL( done( bool ) ), this, SLOT( __fetch_done( bool ) ) );
	connect( &__http_client, SIGNAL( dataReadProgress( int, int ) ), this, SLOT( __fetch_progress( int, int ) ) );
	connect( &__http_client, SIGNAL( requestFinished( int, bool ) ), this, SLOT( __http_request_finished( int, bool ) ) );
	connect( &__http_client, SIGNAL( responseHeaderReceived( const QHttpResponseHeader& ) ), SLOT( __header_received( const QHttpResponseHeader& ) ) );

	QString sPath = url.path();
	sPath = sPath.replace( " ", "%20" );

	QHttpRequestHeader header( "GET", sPath );
	header.setValue( "Host", url.host() );

	__time.start();

	if ( ( !sEnvHttpProxy.isNull() ) && ( nEnvHttpPort != 0 ) ) {
		__http_client.setProxy( sEnvHttpProxy, nEnvHttpPort, sEnvHttpUser, sEnvHttpPassword );
	}

	__http_client.setHost( url.host() );
	__http_client.request( header );
}



Download::~Download()
{
}



/// TODO: devo salvare il file sul disco su una dir temporanea e poi spostarlo se e' tutto ok.
void Download::__fetch_done( bool bError )
{
	if ( bError ) {
		ERRORLOG( "Error retrieving the resource." );
		reject();
		return;
	}

	if ( !__redirect_url.isEmpty() ) {
		reject();
		return;
	}

	INFOLOG( "Download completed. " );

	if ( __local_file.isEmpty() ) {
		// store the text received only when not using the file.
		__feed_xml_string = __http_client.readAll();
	} else {
		QFile file( __local_file );

		if ( !file.open( QIODevice::WriteOnly ) ) {
			ERRORLOG( QString( "Unable to save %1" ).arg( __local_file ) );

		} else {
			file.write( __http_client.readAll() );
			file.flush();
			file.close();
		}
	}
	accept();
}



void Download::__fetch_progress ( int done, int total )
{
	__bytes_current = done;
	__bytes_total = total;

	__download_percent = ( float )done / ( float )total * 100.0;
}



void Download::__http_request_finished( int requestId, bool error )
{
	if ( error ) {
		ERRORLOG( "Error: " + __http_client.errorString() );
		return;
	}
}



void Download::__header_received( const QHttpResponseHeader& res )
{
	//INFOLOG( "Header received: " + to_string( res.statusCode() ) );
	if ( ( res.statusCode() == 301 ) || ( res.statusCode() == 302 ) || ( res.statusCode() == 307 ) ) {
		__redirect_url = res.value( "location" );
		INFOLOG( "Received redirect to: " + __redirect_url );
		//__http_client.abort();
	}
}


// :::::::::::::::::::..



DownloadWidget::DownloadWidget( QWidget* parent, const QString& title, const QString& __remote_url, const QString& local_file )
		: Download( parent, __remote_url, local_file )
{
	setWindowTitle( title );
	setModal( true );

	setFixedSize( 500, 100 );

	QFont boldFont;
	boldFont.setBold( true );

	__url_label = new QLabel( NULL );
	__url_label->setFont( boldFont );
	__url_label->setAlignment( Qt::AlignCenter );
	__url_label->setText( QFileInfo( __remote_url ).fileName() );

	__progress_bar = new QProgressBar( NULL );

	__progress_bar->setMinimum( 0 );
	__progress_bar->setMaximum( 100 );

	__eta_label = new QLabel( NULL );
// __eta_label->setFont( boldFont );
	__eta_label->setAlignment( Qt::AlignHCenter );


	QVBoxLayout* pVBox = new QVBoxLayout();
	pVBox->addWidget( __url_label );
	pVBox->addWidget( __progress_bar );
	pVBox->addWidget( __eta_label );


	setLayout( pVBox );

	__update_timer = new QTimer( this );
	connect( __update_timer, SIGNAL( timeout() ), this, SLOT( updateStats() ) );

	__close_timer = new QTimer( this );
	connect( __close_timer, SIGNAL( timeout() ), this, SLOT( close() ) );

	__update_timer->start( 100 );
}



DownloadWidget::~DownloadWidget()
{
	__update_timer->stop();
	__close_timer->stop();
}



void DownloadWidget::updateStats()
{
	if ( __download_percent > 0 ) {
		__eta = ( int )( round( ( __time.elapsed() / __download_percent * ( 100 - __download_percent ) ) / 1000 ) );
	}

	__progress_bar->setValue( get_percent_done() );

	QString hours = QString( "%1" ).arg( __eta / 60 / 60 );
	QString minutes = QString( "%1" ).arg( ( __eta / 60 ) % 60 );
	QString seconds = QString( "%1" ).arg( __eta % 60 );

	hours = hours.rightJustified( 2, '0' );
	minutes = minutes.rightJustified( 2, '0' );
	seconds = seconds.rightJustified( 2, '0' );

	QString sETA = hours + ":" + minutes + ":" + seconds;

	__eta_label->setText( trUtf8( "(%1/%2 KiB) - ETA %3" ).arg( __bytes_current / 1024 ).arg( __bytes_total / 1024 ).arg( sETA ) );

	if ( __download_percent == 100 ) {
		__update_timer->stop();

		__close_timer->start( 1000 ); // close the window after 1 second
	}
}

