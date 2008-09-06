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

#ifndef DOWNLOAD_WIDGET_H
#define DOWNLOAD_WIDGET_H

#include "config.h"

#include <QtGui>
#include <QtNetwork>

#include <hydrogen/Object.h>

/*
The RedirectHttp class is borrowed from AmaroK.

Copyright (C) 2005 - 2007 by
    Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>
    Erik Jaelevik, Last.fm Ltd <erik@last.fm>
    Jono Cole, Last.fm Ltd <jono@last.fm>
*/
class RedirectHttp : public QHttp
{
    Q_OBJECT

    public:

        RedirectHttp( QObject* parent = 0 );
        ~RedirectHttp();

        int get( const QString& path, QIODevice* to = 0 );
        int post( const QString& path, QIODevice* data = 0, QIODevice* to = 0 );
        int post( const QString& path, const QByteArray& data, QIODevice* to = 0 );
        int request( const QHttpRequestHeader& header, QIODevice* data = 0, QIODevice* to = 0 );
        int request( const QHttpRequestHeader& header, const QByteArray& data, QIODevice* to = 0 );

    private slots:
        void onHeaderReceived( const QHttpResponseHeader& resp );

        void onRequestFinished( int id, bool error );
        void onRequestStarted( int id );

    private:

        enum RequestMode
        {
            GET = 0,
            POST,
            POSTIO,
            REQUEST,
            REQUESTIO
        };

        QByteArray m_data;
        QIODevice* m_device;
        QIODevice* m_to;
        QHttpRequestHeader m_header;

        QHash<int,int> m_idTrans;
        int m_mode;
        int m_lastRequest;
};


class Download : public QDialog, public Object
{
	Q_OBJECT

	public:
		Download( QWidget* pParent, const QString& sRemoteURL, const QString& sLocalFile );
		~Download();

		int getPercentDone() {	return (int)m_fPercDownload;	}
		const QString& getXMLContent() {	return m_sFeedXML;	}

	private slots:
		void fetchDone( bool bError );
		void fetchProgress( int done, int total );
		void httpRequestFinished(int requestId, bool error);

	protected:
		//QHttp m_httpClient;
		RedirectHttp m_httpClient;
		QTime m_time;

		float m_fPercDownload;
		int m_nETA;
		int m_nBytesCurrent;
		int m_nBytesTotal;
		QString m_sRemoteURL;
		QString m_sLocalFile;
		QString m_sFeedXML;
};



class DownloadWidget : public Download
{
	Q_OBJECT

	public:
		DownloadWidget( QWidget* pParent, const QString& sTitleText, const QString& sRemoteURL, const QString& sLocalFile = "" );
		~DownloadWidget();

	private slots:
		void updateStats();

	private:
 		QTimer *m_pUpdateTimer;
		QTimer *m_pCloseTimer;
		QLabel *m_pURLLabel;
		QLabel *m_pETALabel;
		QProgressBar* m_pProgressBar;
};

#endif

