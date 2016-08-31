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

#include <hydrogen/version.h>
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
	about += QString("<center><b>Hydrogen Drum Machine %1 [%2] </b><br>").arg( H2Core::get_version().c_str() ).arg( __DATE__ );
	about += tr( "<b>Website</b><br>" );
	about += "http://www.hydrogen-music.org<br><br>";
	about += tr( "<b>Project page</b><br>");
	about += "http://sourceforge.net/projects/hydrogen<br><br>";
	about += tr( "<b>Mailing lists</b><br>");
	about += "http://lists.sourceforge.net/lists/listinfo/hydrogen-users<br>";
	about += "https://lists.sourceforge.net/lists/listinfo/hydrogen-announce"<br>";
	about += "http://lists.sourceforge.net/lists/listinfo/hydrogen-devel<br>";
	aboutTxt->setText( about );



	std::vector<Author> authorList;
	authorList.push_back( Author( "Antonio Piraino (aka Journeyman)", "http://www.storiepvtride.it", "Italian manual" ) );
	authorList.push_back( Author( "Artemiy Pavlov (aka Artemio)", "www.artemiolabs.com", "drum kits, demo patterns, web site" ) );
	authorList.push_back( Author( "Alexandre Prokoudine", "", "Russian translation" ) );
	authorList.push_back( Author( "Aur&#233;lien Leblond", "", "coding, bug fixes" ) );
	authorList.push_back( Author( "Ben Powers", "", "docs" ) );
	authorList.push_back( Author( "Benjamin Flaming", "", "JACK patches, bug fix" ) );
	authorList.push_back( Author( "Carlo Impagliazzo (aka Truijllo)", "", "testing, ideas.." ) );
	authorList.push_back( Author( "Chris Mennie", "http://chrismennie.ca/", "MIDI coding" ) );
	authorList.push_back( Author( "Chris Wareham", "", "NetBSD patch" ) );
	authorList.push_back( Author( "Christian Vorhof", "", "interface design concept" ) );
	authorList.push_back( Author( "Daniil Kolpakov", "", "" ) );
	authorList.push_back( Author( "Daniel Tonda Castillo", "", "Spanish manual" ) );
	authorList.push_back( Author( "Daryl Hanlon","darylohara@gmail.com","Spanish translation" ) );
	authorList.push_back( Author( "Dave Allan", "", "manual review" ) );
	authorList.push_back( Author( "Dave Fancella", "", "" ) );
	authorList.push_back( Author( "Dave Phillips", "", "bug reports, ideas" ) );
	authorList.push_back( Author( "Derrick Karpo", "", "patches, testing" ) );
	authorList.push_back( Author( "Dmitry Ivanov", "", "" ) );
	authorList.push_back( Author( "Ede Wolf", "", "FAQ, testing" ) );
	authorList.push_back( Author( "Elizeu Santos-Neto", "", "Portuguese(Brazil) translation" ) );
	authorList.push_back( Author( "Emiliano Grilli (aka Emillo)", "www.emillo.net", "drum kits, demo patterns" ) );
	authorList.push_back( Author( "Esben Stien", "", "" ) );
	authorList.push_back( Author( "Francesco Cabras", "", "patches, testing" ) );
	authorList.push_back( Author( "Gabriel M. Beddingfield", "gabriel@teuton.org", "patches, ideas" ) );
	authorList.push_back( Author( "Gene", "", "patches, testing" ) );
	authorList.push_back( Author( "Greg Bonik","gregory@bonik.org","pulseaudio coding" ) );
	authorList.push_back( Author( "Jakob Lund", "jlund05@imada.sdu.dk", "coding" ) );
	authorList.push_back( Author( "Jason Schaefer", "schaefer.jason@gmail.com", "patches, lead/lag feature" ) );
	authorList.push_back( Author( "James Stone", "", "Bugfixes" ) );
	authorList.push_back( Author( "Jay Alexander Fleming", "", "Serbian translation" ) );
	authorList.push_back( Author( "Jesse Chappel", "", "JACK patches" ) );
	authorList.push_back( Author( "J&#233;r&#233;my Zurcher", "", "coding") ); 
	authorList.push_back( Author( "Jonas Melzer", "", "German manual" ) );
	authorList.push_back( Author( "Jonathan Dempsey", "jonathandempsey@fastmail.fm", "Mac OSX port" ) );
	authorList.push_back( Author( "Kevin Dahan (aka Unet)", "", "French translation" ) );
	authorList.push_back( Author( "Lee Revell", "", "patches" ) );
	authorList.push_back( Author( "Matt Walker", "", "" ) );
	authorList.push_back( Author( "Michael Wolkstein", "m.wolkstein@gmx.de", "coding" ) );
	authorList.push_back( Author( "Miguel Anxo Bouzada","mbouzada@gmail.com","Galician translation" ) );
	authorList.push_back( Author( "Nikos Papadopoylos", "", "Greek translation" ) );
	authorList.push_back( Author( "Noel Darlow", "", "manual review" ) );
	authorList.push_back( Author( "Olivier Humbert", "", "French translation" ) );
	authorList.push_back( Author( "Paul Dorman", "", "" ) );
	authorList.push_back( Author( "Pawel Piatek (aka Xj)","xj@wp.pl","coding, bugfixing" ) );
	authorList.push_back( Author( "Pieter Van Isacker (aka aikie)", "", "Dutch manual and translation" ) );
	authorList.push_back( Author( "Samuel Mimram", "", "packages" ) );
	authorList.push_back( Author( "Sebastian Moors (aka mauser)", "mauser@smoors.de", "coding" ) );
	authorList.push_back( Author( "Sergio Gil Perez de la Manga", "", "Spanish translation" ) );
	authorList.push_back( Author( "Simon Donike", "", "German translation" ) );
	authorList.push_back( Author( "Steve Boyer", "", "Windows cross compilation scripts" ) );
	authorList.push_back( Author( "Thijs Van Severen", "http://audio-and-linux.blogspot.be/", "manual, website, coding" ) ); 
	authorList.push_back( Author( "Torben Hohn", "", "bugfixing, test" ) );
	authorList.push_back( Author( "Yamasaki Yutaka", "yamasaki@good-day.co.jp", "Japanese translation" ) );
	authorList.push_back( Author( "Willie Sippel", "willie@zeitgeistmedia.net", "GUI graphics, coding" ) );
	

	QString sAuthors;
	sAuthors += tr( "<b>Main coder and maintainer:</b><br>" );
	sAuthors += "<ul><li><p>Alessandro Cominu (aka Comix) [2001-2008]</li>";
	sAuthors += "<li><p>Michael Wolkstein (aka Wolke) [2008-2014]</li>";
	sAuthors += "<li><p>Sebastian Moors (aka Mauser) [2008-now]</li></ul>";

	sAuthors += tr( "<b>Thanks to:</b>" );

	sAuthors += "<ul>";

	for ( uint i = 0; i < authorList.size(); ++i ) {
		Author a = authorList.at( i );
		sAuthors += "<li><p>";
		sAuthors += "<i>" + a.m_sName + " - " + a.m_sInfo + "</i>";
		sAuthors += "</p></li>";
	}
	sAuthors += "</ul>";


	authorsText->setText( sAuthors );

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
