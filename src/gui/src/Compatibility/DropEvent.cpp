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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <core/config.h>

#include "DropEvent.h"

DropEvent::DropEvent( QDropEvent* pEv ) :
#ifdef H2CORE_HAVE_QT6
	QDropEvent( pEv->position(), pEv->possibleActions(), pEv->mimeData(),
				pEv->buttons(), pEv->modifiers() ) {
#else
	QDropEvent( pEv->pos(), pEv->possibleActions(), pEv->mimeData(),
				pEv->mouseButtons(), pEv->keyboardModifiers() ) {
#endif
};

DropEvent::~DropEvent() {
}

QPointF DropEvent::position() const {
#ifdef H2CORE_HAVE_QT6
	return QDropEvent::position();
#else
	return QDropEvent::pos();
#endif
}
