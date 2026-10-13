/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <cppunit/extensions/HelperMacros.h>
#include <QTranslator>

#include "core/Helpers/Translations.h"

class UITranslationTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE( UITranslationTest );
	CPPUNIT_TEST( testLanguageSelection );
	CPPUNIT_TEST_SUITE_END();
	
public:
	virtual void setUp()
	{
	}
	void testLanguageSelection()
	{
	___INFOLOG( "" );
	// The .qm catalogs are built next to their .ts sources in the
	// system i18n dir (data/i18n/CMakeLists.txt points the lrelease
	// output there), and the suite redirects its system data folder to
	// the repo's data/ — so the Translations helpers' default directory
	// already finds them. The former applicationDirPath() +
	// "/../../data/i18n" guess depended on catalog copies inside the
	// build dir, which a fresh checkout's build never had.
	QString sTranslationsDir = H2Core::Filesystem::systemInternationalizationDir();

		// For all available translations, check that we can load with matching language.
		QStringList list = H2Core::Translations::availableTranslations( "hydrogen", sTranslationsDir );

		CPPUNIT_ASSERT( list.size() > 0 );

		for ( QString sTr : list ) {
			QLocale fileNameLocale( sTr );
			// Find translation for exactly this language
			QStringList languages( sTr );
			QString sLanguage = H2Core::Translations::findTranslation( languages, "hydrogen", sTranslationsDir );
			CPPUNIT_ASSERT( !sLanguage.isNull() );

			// Check translation can be loaded
			QTranslator qtor;
			bool loaded = H2Core::Translations::loadTranslation( QStringList( sLanguage ), qtor, "hydrogen", sTranslationsDir );
			CPPUNIT_ASSERT( loaded );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 15, 0 )
			// Check language matches that specified by the filename.
			QLocale languageLocale( qtor.language() );
			CPPUNIT_ASSERT( languageLocale == fileNameLocale );
#endif
		}


		// Language selection order test, issue #929

		// Regional English, French, or Brazilian Portuguese. We should
		// find an English translation, *not* Brazilian. This covers
		// non-preferred languages matching just because they have the
		// more exact match.
		{
			QStringList languages;
			languages << "en-CA" << "fr-CA" << "pt-BR";
			QString sTr = H2Core::Translations::findTranslation( languages, "hydrogen", sTranslationsDir );
			QLocale l( sTr );
			CPPUNIT_ASSERT( l.language() == QLocale::English );
		}

		// Similar but with French preferred.
		{
			QStringList languages;
			languages << "fr-CA" << "en-CA" << "pt-BR";
			QString sTr = H2Core::Translations::findTranslation( languages, "hydrogen", sTranslationsDir );
			QLocale l( sTr );
			CPPUNIT_ASSERT( l.language() == QLocale::French );
		}

		// Make sure exact-match language without region preference
		// doesn't take priority over preferred language.
		{
			QStringList languages;
			languages << "fr-CA" << "de";
			QString sTr = H2Core::Translations::findTranslation( languages, "hydrogen", sTranslationsDir );
			QLocale l( sTr );
			CPPUNIT_ASSERT( l.language() == QLocale::French );
		}
		
	___INFOLOG( "passed" );
	}
};

