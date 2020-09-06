

#ifndef H2C_TRANSLATIONS_H
#define H2C_TRANSLATIONS_H


#include <hydrogen/object.h>
#include <hydrogen/helpers/filesystem.h>
#include <QtCore/QString>
#include <QTranslator>
#include <QLocale>
#include <QList>

namespace H2Core
{

///
/// Translations manager
///
class Translations : public Object
{

  // Need methods to:
  // list available translations (return QLocales?)
  // find appropriate translation given list of user prefs
public:
  
  static QStringList availableTranslations( QString fileName,
                                            QString directory = H2Core::Filesystem::i18n_dir() ) {
    QStringList translations;
    QDir d( directory );
    QStringList filter = QStringList( fileName + "_*.qm" );

    for ( QString entry : d.entryList( filter ) ) {
      // Extract language or language_REGION string
      entry.remove( 0, fileName.size() + 1 );
      entry.remove( entry.size() - 3, 3);
      entry.replace( '_', '-' );
      translations << entry;
    }
    return translations;
  }
  
  ///
  /// The standard QTranslation::load will prefer an exact match of a
  /// languae-REGION pair, regardless of its position in the preferred
  /// UI languages list. This can lead, for instance, to Qt selecting
  /// Hungarian or Brazilian Portugese just because they have exact
  /// matching translations, even if they're the last on the system's
  /// preferred UI language list. This seems wrong, as the user's
  /// preference for *language* shoud be considered more important than
  /// region, particularly here since Hydrogen has no particular region
  /// dependencies.
  ///
  /// So instead, exhaustively search for a match for each of the
  /// user's preferred languages in turn.
  ///

  static QString findTranslation( QStringList languages, QString fileName, QString directory = H2Core::Filesystem::i18n_dir() )
  {
    for ( QString language : languages ) {
      language.replace( '-', '_' );
      
      for (;;) {
        QString transName = fileName + "_" + language + ".qm";
        QFileInfo fi( directory, transName );
        if ( fi.exists() && fi.isFile() ) {
          return language;
        }
        int i = language.lastIndexOf( '_' );
        if ( i == -1 ) {
          break;
        }
        language.truncate( i );
      }
    }
    return QString();
  }

  static bool loadTranslation( QStringList languages, QTranslator &tor, QString fileName, QString directory = H2Core::Filesystem::i18n_dir() )
  {
    QString language = findTranslation( languages, fileName, directory );
    if ( language.isNull() ) {
      return false;
    }
    QString transName = fileName + "_" + language + ".qm";
    return tor.load( transName, directory );

  }

};
  
  
}
#endif
