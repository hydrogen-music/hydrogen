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

#ifndef __SHOTLIST_H
#define __SHOTLIST_H

#include <QtGui>
#include <QtWidgets>
#include "EventListener.h"

/// Shot List
///
/// Utility for grabbing screenshots of widgets. Not a script, just a shot list.
///
/// Commands are word-oriented
///
///  - grab \<WidgetName|WidgetClass\>       -- grab widget named "WidgetName" or of any class inheriting from "WidgetClass"
///     + [ size \<w\> \<h\> ]               -- size of area to grab. 0 or (-)ve mean widget's entire width or height
///     + [ ofset \<x\> \<y\> ]              -- offset from the widget's origin for beginning of grab
///     + [ as \<filename\> ]                -- filename to save as, including file type extension
///  - slot \<widget\> \<slot\> [\<arg\>...] -- invoke a slot method on widget.
///  - dump                                  -- dump object tree(s)
///  - # \<text\>                            -- commentary (note that the space is needed!)
///  - fin                                   -- quit Hydrogen
///
/// By naming widgets appropriately and exposing their functionality as slots, it should be possible for the
/// application to allow a lot of flexibility in how screenshots are set up in shot lists.
///
/** \ingroup docGUI*/
class ShotList : public QObject, public EventListener {
	Q_OBJECT

	/// Find a widget which inherits the named class
	static QWidget *findWidgetInheriting( QObject *pObject, const QString& sName );

	/// Find a widget by name
	static QWidget *findWidget( const QString& sName );

	/// Buffer for construction of Q_ARGs.
	class Arg {
		QString m_sArg;
		union {
			int m_n;
			QWidget *m_pWidget;
			bool m_b;
		};
	public:
		Arg( const QString& sArg ) : m_sArg( sArg ) {}

		operator QGenericArgument() {
			if (( m_sArg == "true" )) {
				m_b = true;
				return Q_ARG( bool, m_b );
			} else if (( m_sArg == "false" )) {
				m_b = false;
				return Q_ARG( bool, m_b );
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

	void shoot( const QString& s );

public:
	ShotList( const QStringList& shots );
	ShotList( const QString& sShotsFilename );
	~ShotList();

	void shoot();
	void nextShotEvent() override;

public slots:
	void nextShot( void );

};


#endif


