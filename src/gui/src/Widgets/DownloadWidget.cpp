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

#include "DownloadWidget.h"

#include <cmath>
#include <cstdlib>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtNetwork>

Download::Download(
	QWidget* pParent,
	const QString& download_url,
	const QString& local_file
)
	: QDialog( pParent ),
	  __download_percent( 0 ),
	  __eta( 0 ),
	  __bytes_current( 0 ),
	  __bytes_total( 0 ),
	  __remote_url( download_url ),
	  __local_file( local_file ),
	  __reply( nullptr ),
	  __error( "" )
{
	if ( !__local_file.isEmpty() ) {
		INFOLOG( QString( "Downloading '%1' in '%2'" )
					 .arg( __remote_url.toString() )
					 .arg( __local_file ) );
	}
	else {
		INFOLOG( QString( "Downloading '%1'" ).arg( __remote_url.toString() ) );
	}

	__http_client = new QNetworkAccessManager( this );
	__http_client->setRedirectPolicy(
		QNetworkRequest::ManualRedirectPolicy
	);

	QString sEnvHttpProxy = QString( getenv( "http_proxy" ) );
	int nEnvHttpPort = 0;
	QString sEnvHttpUser = QString( getenv( "http_user" ) );
	QString sEnvHttpPassword = QString( getenv( "http_password" ) );

	nEnvHttpPort =
		sEnvHttpProxy
			.right( sEnvHttpProxy.length() - sEnvHttpProxy.indexOf( ':' ) - 1 )
			.toInt();
	sEnvHttpProxy = sEnvHttpProxy.left( sEnvHttpProxy.indexOf( ':' ) );

	__time.start();

	if ( ( !sEnvHttpProxy.isNull() ) && ( nEnvHttpPort != 0 ) ) {
		QNetworkProxy proxy;
		proxy.setType( QNetworkProxy::DefaultProxy );
		proxy.setHostName( sEnvHttpProxy );
		proxy.setPort( nEnvHttpPort );
		proxy.setUser( sEnvHttpUser );
		proxy.setPassword( sEnvHttpPassword );
		__http_client->setProxy( proxy );
	}

	QNetworkRequest getReq;
	getReq.setUrl( __remote_url );
	getReq.setRawHeader( "User-Agent", "Hydrogen" );

	__reply = __http_client->get( getReq );

	connect( __reply, SIGNAL( finished() ), this, SLOT( finished() ) );
	connect(
		__reply, SIGNAL( downloadProgress( qint64, qint64 ) ), this,
		SLOT( downloadProgress( qint64, qint64 ) )
	);
}

Download::~Download()
{
}

/// TODO: I have to save the file to disk on a temporary dir and then move it if everything is ok.
void Download::finished()
{
	if ( __reply->error() ) {
		__error = QString( tr( "Importing item failed: %1" ) ).arg( __reply->errorString() );
		ERRORLOG( __error );
		reject();
		return;
	}


	int StatusAttribute = __reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if(StatusAttribute >= 200 && StatusAttribute < 300){
		//do nothing, handling will be done later..
	} else if(StatusAttribute >= 300 && StatusAttribute < 400){
		QVariant RedirectAttribute = __reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

		if ( RedirectAttribute != 0 ) {

			__redirect_url = __remote_url.resolved(RedirectAttribute.toUrl());
			__reply->deleteLater();
			INFOLOG( QString( "Download redirected to '%1'" ).arg( __redirect_url.toString() ) );

			reject();
			return;
		}
	}

	INFOLOG( "Download completed. " );

	if ( __local_file.isEmpty() ) {
		// store the text received only when not using the file.
		__feed_xml_string = QString( __reply->readAll() );
	} else {
		QFile file( __local_file );

		if ( !file.open( QIODevice::WriteOnly ) ) {
			ERRORLOG( QString( "Unable to save %1" ).arg( __local_file ) );
		} else {
			file.write(__reply->readAll());
			file.flush();
			file.close();
		}
	}
	accept();
}



void Download::downloadProgress( qint64 done, qint64 total )
{
	__bytes_current = done;
	__bytes_total = total;

	__download_percent = ( float )done / ( float )total * 100.0;
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

	__url_label = new QLabel( nullptr );
	__url_label->setFont( boldFont );
	__url_label->setAlignment( Qt::AlignCenter );
	__url_label->setText( QFileInfo( __remote_url ).fileName() );

	__progress_bar = new QProgressBar( nullptr );

	__progress_bar->setMinimum( 0 );
	__progress_bar->setMaximum( 100 );

	__eta_label = new QLabel( nullptr );
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

	__eta_label->setText( tr( "(%1/%2 KiB) - ETA %3" ).arg( __bytes_current / 1024 ).arg( __bytes_total / 1024 ).arg( sETA ) );

	if ( __download_percent == 100 ) {
		__update_timer->stop();

		__close_timer->start( 1000 ); // close the window after 1 second
	}
}

