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

#include "MouseEvent.h"

MouseEvent::MouseEvent( QMouseEvent* pEv ) :
#ifdef H2CORE_HAVE_QT6
	QMouseEvent( pEv->type(), pEv->position(), pEv->globalPosition(),
				 pEv->button(), pEv->buttons(), pEv->modifiers() ) {
#else
	QMouseEvent( pEv->type(), pEv->pos(), pEv->globalPos(), pEv->button(),
				 pEv->buttons(), pEv->modifiers() ) {
#endif
};

MouseEvent::~MouseEvent() {
}

QPointF MouseEvent::globalPosition() const {
#ifdef H2CORE_HAVE_QT6
	return QMouseEvent::globalPosition();
#else
	return QMouseEvent::globalPos();
#endif
}

QPointF MouseEvent::position() const {
#ifdef H2CORE_HAVE_QT6
	return QMouseEvent::position();
#else
	return QMouseEvent::pos();
#endif
}
