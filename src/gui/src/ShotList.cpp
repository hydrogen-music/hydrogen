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

#include <core/Hydrogen.h>
#include <core/EventQueue.h>

#include "ShotList.h"
#include "HydrogenApp.h"

ShotList::ShotList( const QString& sShotsFilename ) {
	QFile shots( sShotsFilename );
	m_nNextShot = 0;
	
	if ( ! shots.open( QIODevice::ReadOnly ) ) {
		___ERRORLOG( QString( "Cannot open shot list file '%1' " ).arg( shots.fileName() ) );
		return;
	}
	while (! shots.atEnd() ) {
		m_shots << shots.readLine();
	}

	HydrogenApp::get_instance()->addEventListener( this );
}

ShotList::ShotList( const QStringList& shots ) {
	m_shots = shots;

	HydrogenApp::get_instance()->addEventListener( this );
}

ShotList::~ShotList() {
	if ( auto pHydrogenApp = HydrogenApp::get_instance() ) {
		pHydrogenApp->removeEventListener( this );
	}
}

QWidget *ShotList::findWidgetInheriting( QObject *pObject, const QString& sName ) {
	const auto sNameLocal8Bit = sName.toLocal8Bit();
	if ( pObject->inherits( sNameLocal8Bit.data() ) ) {
		return dynamic_cast< QWidget *>( pObject );
	}
	for ( QObject *pC : pObject->children() ) {
		QWidget *pW = findWidgetInheriting( pC, sName );
		if ( pW ) {
			return pW;
		}
	}
	return nullptr;
}

QWidget *ShotList::findWidget( const QString& sName ) {
	for ( QWidget * pTop : QApplication::topLevelWidgets() ) {

		QWidget *pWidget = pTop->findChild< QWidget *>( sName );
		if ( !pWidget && pTop->objectName() == sName ) {
			pWidget = dynamic_cast< QWidget *>( pTop );
		}
		if ( !pWidget ) {
			pWidget = findWidgetInheriting( pTop, sName );
		}
		if ( pWidget ) {
			return pWidget;
		}
	}
	return nullptr;
}

void ShotList::shoot( const QString& s ) {
	___INFOLOG( QString( "Taking shot: %1" ).arg( s.trimmed() ) );
	QStringList words = s.trimmed().split( QRegExp( "\\s+" ) );
	if ( s.size() == 0 ) {
		return;
	}
	QString sCmd = words[ 0 ];

	if ( sCmd.startsWith( "#" ) || sCmd == "" ) {
		// Empty line or "#" to start a comment
	} else if ( sCmd.compare( "fin", Qt::CaseInsensitive) == 0 ) {
		// Finish the shot list and quit Hydrogen

		// Since the shot lists do also toggle some buttons that mark
		// the overall song modified, we need to discard the flag in
		// order to avoid a popup dialog.
		H2Core::Hydrogen::get_instance()->setIsModified( false );
		
		QTimer::singleShot( 1, QApplication::instance(), &QApplication::closeAllWindows );
	} else if ( sCmd.compare( "dump", Qt::CaseInsensitive) == 0 ) {
		// Dump object tree for debugging
		for ( QWidget *pTop : QApplication::topLevelWidgets() ) {
			pTop->dumpObjectTree();
		}
	} else if ( sCmd.compare( "grab", Qt::CaseInsensitive ) == 0 ) {

		if ( words.size() < 2 ) {
			___ERRORLOG( QString( "Syntax: grab <widget> [as <filename>] [size w d] [offset x y ]." ) );
		} else {
			words.pop_front();
			QString sWidgetName = words[0];
			words.pop_front();
			QRect rect( 0, 0, -1, -1 );
			QString sFileName = QString( "%1.png" ).arg( sWidgetName );
			while ( !words.empty() ) {
				if ( words[0] == "as" ) {
					words.pop_front();
					if ( words.size() < 1 ) {
						___ERRORLOG( QString( "Syntax: grab ... as <filename>" ) );
					} else {
						sFileName = words[0];
						words.pop_front();
					}
				} else if ( words[0] == "size" ) {
					words.pop_front();
					if ( words.size() < 2 ) {
						___ERRORLOG( QString( "Syntax: grab ... size <width> <height>" ) );
					} else {
						rect.setWidth( words[0].toInt() );
						rect.setHeight( words[1].toInt() );
						words.pop_front();
						words.pop_front();
					}
				} else if ( words[0] == "offset" ) {
					words.pop_front();
					if ( words.size() < 2 ) {
						___ERRORLOG( QString( "Syntax: grab ... offset <width> <height>" ) );
					} else {
						rect.setX( words[0].toInt() );
						rect.setY( words[1].toInt() );
						words.pop_front();
						words.pop_front();
					}
				} else {
					___ERRORLOG( QString( "Syntax: grab <widget> [as <filename>] [size w d] [offset x y ]."
										  " Unexpected '%1'" ).arg( words[0] ) );
					break;
				}
			}

			QWidget *pWidget = findWidget( sWidgetName );
			if ( pWidget ) {
				QPixmap p = pWidget->grab();
				QRect oldRect = rect;
				// Scale 'rect' up to match device pixels of pixmap
				rect = QRect( rect.topLeft() * p.devicePixelRatio(),
							  rect.size() * p.devicePixelRatio() );
				if ( rect.width() <= 0 ) {
					rect.setWidth( p.rect().width() );
				}
				if ( rect.height() <= 0 ) {
					rect.setHeight( p.rect().height() );
				}
				QRect grabRect = rect.intersected( p.rect() );
				p.copy( grabRect ).save( sFileName );
				___INFOLOG( QString( "Saved grabbed widget %1" ).arg( sFileName ) );
			} else {
				___ERRORLOG( QString( "Couldn't find widget named '%1' to grab" ).arg( sWidgetName ) );
			}
		}

	} else if ( sCmd.compare( "slot", Qt::CaseInsensitive ) == 0 ) {

		if ( words.size() >= 3 ) {
			QString sWidgetName = words[ 1 ];
			QString sMethodName = words[ 2 ];
			QWidget *pWidget = findWidget( sWidgetName );

			if ( pWidget ) {
				___INFOLOG( QString( "Invoking '%1' on '%2'" ).arg( sMethodName, sWidgetName ) );
				bool bSuccess = false;
				const auto sMethodNameLocal8Bit = sMethodName.toLocal8Bit();
				switch ( words.size() ) {
				case 3:
					bSuccess = QMetaObject::invokeMethod(
						pWidget, sMethodNameLocal8Bit.data(), Qt::DirectConnection );
					break;
				case 4:
					bSuccess = QMetaObject::invokeMethod(
						pWidget, sMethodNameLocal8Bit.data(), Qt::DirectConnection,
						Arg( words[3] ) );
					break;
				case 5:
					bSuccess = QMetaObject::invokeMethod(
						pWidget, sMethodNameLocal8Bit.data(), Qt::DirectConnection,
						Arg( words[3] ), Arg( words[4] ) );
					break;
				case 6:
					bSuccess = QMetaObject::invokeMethod(
						pWidget, sMethodNameLocal8Bit.data(), Qt::DirectConnection,
						Arg( words[3] ), Arg( words[4] ), Arg( words[5] ) );
					break;
				default:
					___ERRORLOG( "Unsupported number of arguments in %0" );
				}

				if ( !bSuccess ) {
					___ERRORLOG( QString( "Couldn't invoke '%1' on '%2'" )
								 .arg( sMethodName, sWidgetName ) );
				} else {
					___INFOLOG( "OK" );
				}
			} else {
				___ERRORLOG( QString( "Couldn't find widget named '%1' to invoke '%2' on" )
							 .arg( sWidgetName, sMethodName ) );
			}
		} else {
			___ERRORLOG( QString( "Syntax: slot <widget> <method> [args]" ) );
		}
	} else {
		___ERRORLOG( QString("Unknown command '%1'").arg( sCmd ) );
	}
}

void ShotList::shoot() {
	m_nNextShot = 0;
	if ( m_shots.size() != 0 ) {
		nextShot();
	}
}

void ShotList::nextShotEvent() {
	QMetaObject::invokeMethod( this, "nextShot", Qt::QueuedConnection );
}

void ShotList::nextShot( void ) {
	if ( ( m_nNextShot + 1) < m_shots.size() ) {
		H2Core::EventQueue::get_instance()->push_event( H2Core::EVENT_NEXT_SHOT, 0 );
	}
	shoot( m_shots[ m_nNextShot++ ] );
}
