/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: HelpBrowser.cpp,v 1.10 2005/06/19 08:27:44 comix Exp $
 *
 */


#include "HelpBrowser.h"
#include "Skin.h"

#include "qstringlist.h"
#include "qapplication.h"
#include "qrect.h"

SimpleHTMLBrowser::SimpleHTMLBrowser( QWidget *pParent, string sDataPath, string sFilename, SimpleHTMLBrowserType type )
 : QDialog( pParent, "SimpleHTMLBrowser" )
 , Object( "SimpleHTMLBrowser" )
 , m_type( type )
 , m_sDataPath( sDataPath )
 , m_sFilename( sFilename )
{
//	infoLog( "INIT" );
//	infoLog( "Data path:" + m_sDataPath );
//	infoLog( "Filename:" + m_sFilename );

	if (m_type == MANUAL ) {
		setCaption( trUtf8( "Manual" ) );
		resize( 800, 600 );
		setMinimumSize( 300, 200 );
	}
	else {
		setCaption( trUtf8( "Welcome to Hydrogen" ) );
		resize( 800, 650 );
		setMinimumSize( width(), height() );
		setMaximumSize( width(), height() );
		// is not present in qt 3.0?
		//setModal( true );
	}

	m_pDontShowAnymoreBtn = new QPushButton( trUtf8( "Don't show this message anymore"), this );
	connect( m_pDontShowAnymoreBtn, SIGNAL( clicked() ), this, SLOT( dontShowAnymoreBtnClicked() ) );
	m_pDontShowAnymoreBtn->resize( 300, 25 );
	m_pDontShowAnymoreBtn->hide();

	m_pCloseWindowBtn = new QPushButton( trUtf8( "Ok" ), this );
	connect( m_pCloseWindowBtn, SIGNAL( clicked() ), this, SLOT( closeWindow() ) );
	m_pCloseWindowBtn->resize( 100, 25 );
	m_pCloseWindowBtn->hide();

	m_pDocHomeBtn = new QPushButton( trUtf8( "Documentation index" ), this );
	connect( m_pDocHomeBtn, SIGNAL( clicked() ), this, SLOT( docIndex() ) );
	m_pDocHomeBtn->resize( 300, 25 );
	m_pDocHomeBtn->hide();

	QMimeSourceFactory * bodyformat = new QMimeSourceFactory;
	bodyformat->setExtensionType("html", "text/html");
	bodyformat->setExtensionType( "png", "image/png" );
	bodyformat->setFilePath( QStringList( m_sDataPath.c_str() ) );

	m_pBrowser = new QTextBrowser( this );
//	m_pBrowser->mimeSourceFactory()->setExtensionType( "html", "text/html" );
//	m_pBrowser->mimeSourceFactory()->setExtensionType( "png", "image/png" );
	m_pBrowser->setMimeSourceFactory( bodyformat );
	m_pBrowser->setReadOnly( true );

	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	QFile file( m_sFilename.c_str() ); // Read the text from a file
	if ( file.open( IO_ReadOnly ) ) {
		QTextStream stream( &file );
		m_pBrowser->setText( stream.read() );
	}

	// center...
	QRect rect = QApplication::desktop()->geometry();
	this->move( rect.center() - this->rect().center() );
}



SimpleHTMLBrowser::~SimpleHTMLBrowser()
{
//	infoLog( "DESTROY" );
}



void SimpleHTMLBrowser::showEvent ( QShowEvent *ev )
{
//	infoLog( "[showEvent]" );
}



void SimpleHTMLBrowser::resizeEvent( QResizeEvent *ev )
{

	if ( m_type == MANUAL ) {
		m_pBrowser->move( 0, 29 );
		m_pBrowser->resize( width(), height() - 29 );

		m_pDocHomeBtn->move( 5, 3 );
		m_pDocHomeBtn->show();

		m_pDontShowAnymoreBtn->hide();
		m_pCloseWindowBtn->hide();
	}
	else if ( m_type == WELCOME ) {
		m_pBrowser->move( 0, 0 );
		m_pBrowser->resize( width(), height() - 29 );

		m_pDontShowAnymoreBtn->move( width() - m_pDontShowAnymoreBtn->width() - m_pCloseWindowBtn->width() - 5 - 5, height() - 27 );
		m_pDontShowAnymoreBtn->show();

		m_pCloseWindowBtn->move( width() - m_pCloseWindowBtn->width() - 5, height() - 27 );
		m_pCloseWindowBtn->show();
		m_pCloseWindowBtn->setDefault(true);

		m_pDocHomeBtn->hide();
	}
}


void SimpleHTMLBrowser::dontShowAnymoreBtnClicked()
{
	accept();
}

void SimpleHTMLBrowser::closeWindow()
{
	reject();
}

void SimpleHTMLBrowser::docIndex()
{
	infoLog( "[docIndex]" );

	QFile file( m_sFilename.c_str() ); // Read the text from a file
	if ( file.open( IO_ReadOnly ) ) {
		QTextStream stream( &file );
		m_pBrowser->setText( stream.read() );
	}

}


