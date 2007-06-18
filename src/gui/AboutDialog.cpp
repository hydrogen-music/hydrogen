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
 * $Id: AboutDialog.cpp,v 1.16 2005/07/09 14:01:32 comix Exp $
 *
 */

#include "AboutDialog.h"
#include "Skin.h"
#include "config.h"

#include <qpixmap.h>
#include <qlabel.h>
#include <qtextbrowser.h>

#include <vector>


class Author {
	public:
		QString m_sName;
		QString m_sEmail;
		QString m_sInfo;
		
		Author( QString sName, QString sEmail, QString sInfo ) : m_sName( sName ), m_sEmail( sEmail ), m_sInfo( sInfo ) {}
};


AboutDialog::AboutDialog(QWidget* parent) : AboutDialog_UI(parent, 0, false) {
	setCaption( tr( "About" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );
	move( 240, 100 );

	QString about;
	about += "<center><b>Hydrogen Drum Machine " + QString(VERSION) + "</b><br>";
	about += tr( "<i>Compiled modules: %1</i></center>").arg(COMPILED_FEATURES);
	about += "<b>HomePage</b><br>";
	about += "<a href=\"http://www.hydrogen-music.org\">http://www.hydrogen-music.org</a><br><br>";
	about += tr( "<b>Project page</b><br>");
	about += "<a href=\"http://sourceforge.net/projects/hydrogen\">http://sourceforge.net/projects/hydrogen</a><br><br>";
	about += tr( "<b>Mailing lists:</b><br>");
	about += "<a href=\"http://lists.sourceforge.net/lists/listinfo/hydrogen-announce\">http://lists.sourceforge.net/lists/listinfo/hydrogen-announce</a><br>";
	about += "<a href=\"http://lists.sourceforge.net/lists/listinfo/hydrogen-devel\">http://lists.sourceforge.net/lists/listinfo/hydrogen-devel</a><br>";
	aboutTxt->setText( about );

	std::vector<Author> authorList;
	authorList.push_back( Author( "Antonio Piraino", "", "Italian manual" ) );
	authorList.push_back( Author( "Artemiy Pavlov (aka Artemio)", "www.artemiolabs.com", "Drum kits, demo patterns, web site" ) );
	authorList.push_back( Author( "Alexandre Prokoudine", "", "Russian translation" ) );
	authorList.push_back( Author( "Ben Powers", "", "Docs" ) );
	authorList.push_back( Author( "Benjamin Flaming", "", "Jack patches, bug fix" ) );
	authorList.push_back( Author( "Carlo Impagliazzo (aka Truijllo)", "", "Testing, ideas.." ) );
	authorList.push_back( Author( "Chris Wareham", "", "NetBSD patch" ) );
	authorList.push_back( Author( "Christian Vorhof", "", "Interface design concept" ) );
	authorList.push_back( Author( "Daniil Kolpakov", "", "" ) );
	authorList.push_back( Author( "Daniel Tonda Castillo", "", "Spanish manual" ) );
	authorList.push_back( Author( "Dave Fancella", "", "" ) );
	authorList.push_back( Author( "Dave Phillips", "", "Bug reports, ideas" ) );
	authorList.push_back( Author( "Derrick Karpo", "", "Patches, testing" ) );
	authorList.push_back( Author( "Dmitry Ivanov", "", "" ) );
	authorList.push_back( Author( "Ede Wolf", "", "Faq, testing" ) );
	authorList.push_back( Author( "Elizeu Santos-Neto", "", "Portuguese(Brazil) translation" ) );
	authorList.push_back( Author( "Emiliano Grilli (aka Emillo)", "www.emillo.net", "Drum kits, demo patterns" ) );
	authorList.push_back( Author( "Esben Stien", "", "" ) );
	authorList.push_back( Author( "Francesco Cabras", "", "Patches, testing" ) );
	authorList.push_back( Author( "Gene", "", "Patches, testing" ) );
	authorList.push_back( Author( "Jesse Chappel", "", "Jack patches" ) );
	authorList.push_back( Author( "Jonas Melzer", "", "German manual" ) );
	authorList.push_back( Author( "Jonathan Dempsey", "jonathandempsey@fastmail.fm", "Mac OSX port" ) );
	authorList.push_back( Author( "Kevin Dahan (aka Unet)", "", "French translation" ) );
	authorList.push_back( Author( "Lee Revell", "", "Patches" ) );
	authorList.push_back( Author( "Paul Dorman", "", "" ) );
	authorList.push_back( Author( "Pieter Van Isacker (aka aikie)", "", "Dutch manual and translation" ) );
	authorList.push_back( Author( "Samuel Mimram", "", "Packages" ) );
	authorList.push_back( Author( "Sergio Gil Perez de la Manga", "", "Spanish translation" ) );
	authorList.push_back( Author( "Torben Hohn", "", "Bug fix, test" ) );
	authorList.push_back( Author( "Yamasaki Yutaka", "yamasaki@good-day.co.jp", "Japanese translation" ) );
	authorList.push_back( Author( "Willie Sippel", "willie@zeitgeistmedia.net", "GUI graphics, coding" ) );

	QString sAuthors;
	sAuthors += tr( "<b>Main coder and mantainer:</b><br>" );
	sAuthors += "<ul><li><p>Alessandro Cominu (aka Comix)<br>";
	sAuthors += "<i><comix@users.sourceforge.net></i></p></li></ul>";
	
	sAuthors += "<b>Thanks to:</b>";

	sAuthors += "<ul>";
	for ( int i = 0; i < authorList.size(); ++i ) {
		Author a = authorList.at( i );
		sAuthors += "<li><p>";
		sAuthors += "<i>" + a.m_sName + "</i>";
		sAuthors += "</p></li>";
	}
	sAuthors += "</ul>";


	authorsTxt->setText( sAuthors );

	logoLabel->setPixmap( QString ( Skin::getImagePath().append("/about/aboutLogo.png").c_str() ) );
}




AboutDialog::~AboutDialog()
{
}




/**
 * Close the dialog
 */
void AboutDialog::okBtnClicked()
{
	accept();
}

