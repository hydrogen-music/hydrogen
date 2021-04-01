/*
 * Hydrogen
 * Copyright(c) 2021 by the Hydrogen Team
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

#ifndef __SHOTLIST_H
#define __SHOTLIST_H

#include <QtGui>
#include <QtWidgets>

/// Shot List
///
/// Utility for grabbing screenshots of widgets. Not a script, just a shot list.
///
/// Entries are a space-separated list of the form:
///
///  grab <WidgetName|WidgetClass>   -- grab widget named "WidgetName" or of any class inheriting from "WidgetClass"
///  slot <widget> <slot> [<arg>...] -- invoke a slot method on widget
///  fin                             -- quit Hydrogen
///  dump                            -- dump object tree(s)
///  # text                          -- commentary
///
class ShotList : public QObject {

	static QWidget *findWidgetInheriting( QObject *pObject, QString &sName );

	static QWidget *findWidget( QString &sName );

	/// Buffer for construction of Q_ARGs
	class Arg {
		QString m_sArg;
		union {
			int m_n;
			QWidget *m_pWidget;
		};
	public:
		Arg( QString &sArg ) : m_sArg( sArg ) {}

		operator QGenericArgument() {
			if (( m_sArg == "true" )) {
				return Q_ARG( bool, true );
			} else if (( m_sArg == "false" )) {
				return Q_ARG( bool, false );
			}
			bool bIsInt = false;
			m_n = m_sArg.toInt( &bIsInt );
			if ( bIsInt ) {
				return Q_ARG( int, m_n );
			} else if (( m_pWidget = findWidget( m_sArg ) )) {
				return Q_ARG( QWidget *, m_pWidget );
			} else {
				// Last resort, treat as a QString
				return Q_ARG( QString, m_sArg );
			}
		}
	};

private:
	QStringList m_shots;
	int m_nNextShot;

	void shoot( QString s );

public:
	ShotList( QStringList shots ) {
		m_shots = shots;
	}
	ShotList( QString sShotsFilename );

	void shoot();

public slots:
	void nextShot( void );

};


#endif


