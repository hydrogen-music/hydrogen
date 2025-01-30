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

#include <core/Version.h>
#include "AboutDialogContributorList.h"
#include "AboutDialog.h"
#include "Skin.h"

#include <core/Globals.h>

#include <vector>



AboutDialog::AboutDialog(QWidget* parent)
 : QDialog( parent )
{
	setupUi( this );

	setWindowTitle( tr( "About" ) );

	// Show and enable maximize button. This is key when enlarging the
	// application using a scaling factor and allows the OS to force its size
	// beyond the minimum and make the scrollbars appear.
	setWindowFlags( windowFlags() | Qt::CustomizeWindowHint |
					Qt::WindowMinMaxButtonsHint );

	move( 240, 100 );

	QString about;
	about += QString("<center><b>Hydrogen Drum Machine %1 [%2] </b><br>").arg( H2Core::get_version().c_str() ).arg( __DATE__ );
	about += "<br><b>" + tr( "Website" ) + "</b><br>";
	about += "<a href='http://www.hydrogen-music.org' style='color: #EEE;'>http://www.hydrogen-music.org</a><br>";
	about += "<br><b>" + tr( "Project page") + "</b><br>";
	about += "<a href='https://github.com/hydrogen-music/hydrogen' style='color: #EEE;'>https://github.com/hydrogen-music/hydrogen</a><br>";
	about += "<br><b>" + tr( "Forum" ) + "</b><br>";
	about += "<a href='https://github.com/hydrogen-music/hydrogen/discussions' style='color: #EEE;'>https://github.com/hydrogen-music/hydrogen/discussions</a><br>";
	about += "<br><b>" + tr( "Development mailing list") + "</b><br>";
	about += "<a href='https://lists.sourceforge.net/lists/listinfo/hydrogen-devel' style='color: #EEE;'>https://lists.sourceforge.net/lists/listinfo/hydrogen-devel</a>";

	aboutTxt->setText( about );
	aboutTxt->setOpenExternalLinks( true );

	std::vector<Author> translatorList;
	translatorList.push_back( Author( "Olivier Humbert", "trebmuh@tuxfamily.org", "French translation" ) );
	translatorList.push_back( Author( "Daryl Hanlon", "darylo1@hotmail.com", "Spanish translation" ) );
	translatorList.push_back( Author( "Guocheng Zhu", "aaronbcn@outlook.es", "Chinese (Mainland China) translation" ) );
	QString sAuthors;
	sAuthors += "<b>" + tr( "Main coders and maintainers" ) + ":</b>";
	sAuthors += "<ul><li><p>Philipp Müller (aka theGreatWhiteShark) [2020-now]</p></li>";
	sAuthors += "<li><p>Colin McEwan (aka cme) [2020-now]</p></li></ul></br>";

	sAuthors += "<b>" + tr( "Active translators" ) + ":</b>";
	sAuthors += "<ul>";

	for ( const auto& tt : translatorList ) {
		sAuthors += "<li><p>";
		sAuthors += tt.m_sName + " (<i>" + tt.m_sEmail + "</i>): " + tt.m_sInfo;
		sAuthors += "</p></li>";
	}
	sAuthors += "</ul></br>";

	AboutDialogContributorList contributors;
	auto pContributorList = contributors.getContributorList();

	sAuthors += "<b>" + tr( "Recent contributors" ) + ":</b>";
	sAuthors += "<ul>";

	for ( const auto& tt : *pContributorList ) {
		sAuthors += "<li><p>";
		sAuthors += tt;
		sAuthors += "</p></li>";
	}
	sAuthors += "</ul></br>";

	sAuthors += "<p><a href='https://github.com/hydrogen-music/hydrogen/graphs/contributors' style='color: #EEE;'>" + tr( "A full list of all contributors can be found on" ) +
		" Github</a></p>";
	
	
	sAuthors += "<b>" + tr( "Former main coders and maintainers" ) + ":</b>";
	sAuthors += "<ul><li><p>Alessandro Cominu (aka Comix) [2001-2008]</li>";
	sAuthors += "<li><p>Michael Wolkstein (aka Wolke) [2008-2014]</li>";
	sAuthors += "<li><p>Sebastian Moors (aka Mauser) [2008-2021]</li></ul></br>";

	authorsText->setText( sAuthors );
	authorsText->setOpenExternalLinks( true );

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
