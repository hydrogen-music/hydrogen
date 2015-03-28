#include "FileDownloader.h"
 
FileDownloader::FileDownloader(QUrl imageUrl, QObject *parent) :
	QObject(parent)
	{
		connect(&m_WebCtrl, SIGNAL(finished(QNetworkReply*)),
		SLOT(fileDownloaded(QNetworkReply*)));
 
		QNetworkRequest request(imageUrl);
		// FIXME: The header below caused a 406 error on my server - need to pick a good header
		//   request.setRawHeader( "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
		//request.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.76 Safari/537.36");
		request.setRawHeader("User-Agent", "Hydrogen/1.0 (Nokia; Qt)");
		
		m_WebCtrl.get(request);
}
 
FileDownloader::~FileDownloader()
{
 
}
 
void FileDownloader::fileDownloaded(QNetworkReply* pReply)
{
	m_DownloadedData = pReply->readAll();
	//emit a signal
	pReply->deleteLater();
	emit imageDownloaded();
}
 
QByteArray FileDownloader::downloadedData() const
{
	return m_DownloadedData;
}

