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

class Download : public QDialog, public Object
{
Q_OBJECT

public:
	Download( QWidget* parent, const QString& download_url, const QString& local_file );
	~Download();

	int get_percent_done() {	return (int)__download_percent;	}
	const QString& get_xml_content() {	return __feed_xml_string;	}

private slots:
	void __fetch_done( bool bError );
	void __fetch_progress( int done, int total );
	void __http_request_finished( int requestId, bool error );
	void __header_received( const QHttpResponseHeader& res );

protected:
	QHttp __http_client;
	QTime __time;

	float __download_percent;
	int __eta;
	int __bytes_current;
	int __bytes_total;
	QString __remote_url;
	QString __local_file;
	QString __feed_xml_string;

	QString __redirect_url;
};



class DownloadWidget : public Download
{
Q_OBJECT

public:
	DownloadWidget( QWidget* parent, const QString& title, const QString& download_url, const QString& local_file = "" );
	~DownloadWidget();

	QString get_redirect_url() {	return __redirect_url;	}

private slots:
	void updateStats();

private:
	QTimer* __update_timer;
	QTimer* __close_timer;
	QLabel* __url_label;
	QLabel* __eta_label;
	QProgressBar* __progress_bar;
};

#endif

