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

#ifndef SOUND_LIBRARY_TREE_H
#define SOUND_LIBRARY_TREE_H

#include "config.h"

#include <QtGui>

#include <hydrogen/Object.h>

class SoundLibraryTree : public QTreeWidget, private Object
{
	Q_OBJECT
	public:
		SoundLibraryTree( QWidget *pParent );

	signals:
		void leftClicked( QPoint pos );
		void rightClicked( QPoint pos );
		void onMouseMove( QMouseEvent* event );


	private slots:


	protected:

		virtual void mousePressEvent(QMouseEvent *event);
		virtual void mouseMoveEvent(QMouseEvent *event);


};



#endif

