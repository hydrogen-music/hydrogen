

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
  
  static QStringList availableTranslations( QString sFileName,
                                            QString sDirectory = H2Core::Filesystem::i18n_dir() ) {
    QStringList translations;
    QDir d( sDirectory );
    QStringList filter = QStringList( sFileName + "_*.qm" );

    for ( QString sEntry : d.entryList( filter ) ) {
      // Extract language or language_REGION string
      sEntry.remove( 0, sFileName.size() + 1 );
      sEntry.remove( sEntry.size() - 3, 3);
      sEntry.replace( '_', '-' );
      translations << sEntry;
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

  static QString findTranslation( QStringList languages, QString sFileName, QString sDirectory = H2Core::Filesystem::i18n_dir() )
  {
    for ( QString sLanguage : languages ) {
      sLanguage.replace( '-', '_' );
      
      for (;;) {
        QString sTransName = sFileName + "_" + sLanguage + ".qm";
        QFileInfo fi( sDirectory, sTransName );
        if ( fi.exists() && fi.isFile() ) {
          sLanguage.replace( '_', '-' );
          return sLanguage;
        }
        int i = sLanguage.lastIndexOf( '_' );
        if ( i == -1 ) {
          break;
        }
        sLanguage.truncate( i );
      }
    }
    return QString();
  }

  static bool loadTranslation( QStringList languages, QTranslator &tor, QString sFileName, QString sDirectory = H2Core::Filesystem::i18n_dir() )
  {
    QString sLanguage = findTranslation( languages, sFileName, sDirectory );
    if ( sLanguage.isNull() ) {
      return false;
    }
    sLanguage.replace( '-', '_' );
    QString sTransName = sFileName + "_" + sLanguage + ".qm";
    return tor.load( sTransName, sDirectory );

  }

};
  
  
}
#endif
