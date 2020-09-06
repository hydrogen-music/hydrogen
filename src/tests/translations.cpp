
#include <cppunit/extensions/HelperMacros.h>
#include <QTranslator>

#include "hydrogen/helpers/translations.h"

class UITranslationTest : public CppUnit::TestCase {
	CPPUNIT_TEST_SUITE(UITranslationTest);
	CPPUNIT_TEST(testLanguageSelection);
	CPPUNIT_TEST_SUITE_END();
	
public:
	virtual void setUp()
	{
	}
	void testLanguageSelection()
	{
		// Find translations. i18n_dir() contains only the source *.ts
		// files. Find the *.qm files for the translations relative to
		// the build directory the tests are run from.
		QString testsDir = QCoreApplication::applicationDirPath();
		QString translationsDir = testsDir + "/../../data/i18n";

		// For all available translations, check that we can load with matching language.
		QStringList list = H2Core::Translations::availableTranslations( "hydrogen", translationsDir );

		CPPUNIT_ASSERT( list.size() > 0 );

		for ( QString tr : list ) {
			QLocale fileNameLocale( tr );
			// Find translation for exactly this language
			QStringList languages( tr );
			QString language = H2Core::Translations::findTranslation( languages, "hydrogen", translationsDir );
			CPPUNIT_ASSERT( !language.isNull() );

			// Check translation can be loaded
			QTranslator qtor;
			bool loaded = H2Core::Translations::loadTranslation( QStringList(language), qtor, "hydrogen", translationsDir );
			CPPUNIT_ASSERT( loaded );

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
			// Check language matches that specified by the filename.
			QLocale languageLocale( qtor.language() );
			CPPUNIT_ASSERT( languageLocale == fileNameLocale );
#endif
		}


		// Language selection order test, issue #929

		// Regional English, French, or Brazilian Portugese. We should
		// find an English translation, *not* Brazillian. This covers
		// non-preferred languages matching just because they have the
		// more exact match.
		{
			QStringList languages;
			languages << "en-CA" << "fr-CA" << "pt-BR";
			QString tr = H2Core::Translations::findTranslation( languages, "hydrogen", translationsDir );
			QLocale l(tr);
			CPPUNIT_ASSERT( l.language() == QLocale::English );
		}

		// Similar but with French preferred.
		{
			QStringList languages;
			languages << "fr-CA" << "en-CA" << "pt-BR";
			QString tr = H2Core::Translations::findTranslation( languages, "hydrogen", translationsDir );
			QLocale l(tr);
			CPPUNIT_ASSERT( l.language() == QLocale::French );
		}

		// Make sure exact-match language without region preference
		// doesn't take priority over preferred language.
		{
			QStringList languages;
			languages << "fr-CA" << "de";
			QString tr = H2Core::Translations::findTranslation( languages, "hydrogen", translationsDir );
			QLocale l(tr);
			CPPUNIT_ASSERT( l.language() == QLocale::French );
		}

	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(UITranslationTest);
