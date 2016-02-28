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


#include <QtGui>
#include <QtNetwork>

#include <hydrogen/object.h>

class Download : public QDialog, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT

public:
	Download( QWidget* parent, const QString& download_url, const QString& local_file );
	~Download();

	int get_percent_done() {	return (int)__download_percent;	}
	const QString& get_xml_content() {	return __feed_xml_string;	}
	bool get_error() { return __error; }

private slots:
	void	finished();
	void	downloadProgress( qint64 done, qint64 total );

protected:
	QNetworkAccessManager*	__http_client;
	QNetworkReply*			__reply;

	QTime					__time;

	float					__download_percent;
	int						__eta;
	qint64					__bytes_current;
	qint64					__bytes_total;


	QUrl					__redirect_url;
	QUrl					__remote_url;

	QString					__local_file;
	QString					__feed_xml_string;

	bool					__error;
};



class DownloadWidget : public Download
{
	H2_OBJECT
	Q_OBJECT

public:
	DownloadWidget( QWidget* parent, const QString& title, const QString& download_url, const QString& local_file = "" );
	~DownloadWidget();

	QUrl get_redirect_url() {	return __redirect_url;	}

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

