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

#include "config.h"
#include "version.h"
#include "AboutDialog.h"
#include "Skin.h"

#include <hydrogen/globals.h>

#include <vector>



AboutDialog::AboutDialog(QWidget* parent)
 : QDialog( parent )
{
	setupUi( this );

	setWindowTitle( tr( "About" ) );
	setWindowIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );
	move( 240, 100 );

	QString about;
	about += QString("<center><b>Hydrogen Drum Machine %1 [%2] </b><br>").arg( get_version().c_str() ).arg( __DATE__ );
	about += tr( "<b>Website</b><br>" );
	about += "http://www.hydrogen-music.org<br><br>";
	about += tr( "<b>Project page</b><br>");
	about += "http://sourceforge.net/projects/hydrogen<br><br>";
	about += tr( "<b>Mailing lists</b><br>");
	about += "http://lists.sourceforge.net/lists/listinfo/hydrogen-announce<br>";
	about += "http://lists.sourceforge.net/lists/listinfo/hydrogen-devel<br>";
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
	authorList.push_back( Author( "Gabriel M. Beddingfield", "gabriel@teuton.org", "Patches, ideas" ) );
	authorList.push_back( Author( "Gene", "", "Patches, testing" ) );
	authorList.push_back( Author( "Jakob Lund", "jlund05@imada.sdu.dk", " coding" ) );
	authorList.push_back( Author( "Jason Schaefer", "schaefer.jason@gmail.com", " Patches, lead/lag feature" ) );
        authorList.push_back( Author( "J&#233;r&#233;my Zurcher", "", "coding") );
	authorList.push_back( Author( "Jesse Chappel", "", "Jack patches" ) );
	authorList.push_back( Author( "Jonas Melzer", "", "German manual" ) );
	authorList.push_back( Author( "Jonathan Dempsey", "jonathandempsey@fastmail.fm", "Mac OSX port" ) );
	authorList.push_back( Author( "Journeyman", "jman-@masternet.it", " manual") );
	authorList.push_back( Author( "Kevin Dahan (aka Unet)", "", "French translation" ) );
	authorList.push_back( Author( "Lee Revell", "", "Patches" ) );
	authorList.push_back( Author( "Matt Walker", "", "" ) );
	authorList.push_back( Author( "Michael Wolkstein", "m.wolkstein@gmx.de", "coding" ) );
	authorList.push_back( Author( "Nikos Papadopoylos", "", "Greek translation" ) );
	authorList.push_back( Author( "Paul Dorman", "", "" ) );
	authorList.push_back( Author( "Pieter Van Isacker (aka aikie)", "", "Dutch manual and translation" ) );
	authorList.push_back( Author( "Samuel Mimram", "", "Packages" ) );
	authorList.push_back( Author( "Sebastian Moors (aka mauser)", "mauser@smoors.de", "coding" ) );
	authorList.push_back( Author( "Sergio Gil Perez de la Manga", "", "Spanish translation" ) );
	authorList.push_back( Author( "Simon Donike", "", "German translation" ) );
        authorList.push_back( Author( "Thijs Van Severen", "", "manual, website, coding" ) );
	authorList.push_back( Author( "Torben Hohn", "", "Bug fix, test" ) );
	authorList.push_back( Author( "Yamasaki Yutaka", "yamasaki@good-day.co.jp", "Japanese translation" ) );
	authorList.push_back( Author( "Willie Sippel", "willie@zeitgeistmedia.net", "GUI graphics, coding" ) );

	QString sAuthors;
	sAuthors += tr( "<b>Main coder and mantainer:</b><br>" );
	sAuthors += "<ul><li><p>Alessandro Cominu (aka Comix)<br>";
	sAuthors += "<i><comix@users.sourceforge.net></i></p></li></ul>";

	sAuthors += QString( "<b>" ) + trUtf8( "Translator:%1Alessandro Cominu" ).arg( "</b><br><ul><li><p>" ) + QString( "</p></li></ul>" );

	sAuthors += "<b>Thanks to:</b>";

	sAuthors += "<ul>";

	for ( uint i = 0; i < authorList.size(); ++i ) {
		Author a = authorList.at( i );
		sAuthors += "<li><p>";
		sAuthors += "<i>" + a.m_sName + "</i>";
		sAuthors += "</p></li>";
	}
	sAuthors += "</ul>";


	authorsTxt->append( sAuthors );

	logoLabel->setPixmap( QPixmap( Skin::getImagePath() +"/about/aboutLogo.png" ) );
}




AboutDialog::~AboutDialog()
{
}




/**
 * Close the dialog
 */
void AboutDialog::on_okBtn_clicked()
{
	accept();
}
