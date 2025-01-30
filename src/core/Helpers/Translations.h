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

#ifndef H2C_TRANSLATIONS_H
#define H2C_TRANSLATIONS_H


#include <core/Object.h>
#include <core/Helpers/Filesystem.h>
#include <QtCore/QString>
#include <QTranslator>
#include <QLocale>
#include <QList>

namespace H2Core
{

///
/// Translations manager
///
/** \ingroup docCore*/
class Translations : public Object<Translations>
{

  // Need methods to:
  // list available translations (return QLocales?)
  // find appropriate translation given list of user prefs
public:
  
  static QStringList availableTranslations( const QString& sFileName,
                                            const QString& sDirectory = H2Core::Filesystem::i18n_dir() ) {
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
  /// Hungarian or Brazilian Portuguese just because they have exact
  /// matching translations, even if they're the last on the system's
  /// preferred UI language list. This seems wrong, as the user's
  /// preference for *language* should be considered more important than
  /// region, particularly here since Hydrogen has no particular region
  /// dependencies.
  ///
  /// So instead, exhaustively search for a match for each of the
  /// user's preferred languages in turn.
  ///

  static QString findTranslation( const QStringList& languages,
								  const QString& sFileName,
								  const QString& sDirectory = H2Core::Filesystem::i18n_dir() )
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

  static bool loadTranslation( const QStringList& languages,
							   QTranslator& tor,
							   const QString& sFileName,
							   const QString& sDirectory = H2Core::Filesystem::i18n_dir() )
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
