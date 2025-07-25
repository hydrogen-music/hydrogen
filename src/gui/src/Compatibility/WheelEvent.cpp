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

#include <QtGlobal>	// for QT_VERSION

#include "WheelEvent.h"

WheelEvent::WheelEvent( QWheelEvent* pEv ) :
#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	QWheelEvent( pEv->position(), pEv->globalPosition(),
#else
	QWheelEvent( pEv->pos(), pEv->globalPos(),
#endif
				 pEv->pixelDelta(), pEv->angleDelta(), pEv->buttons(),
				 pEv->modifiers(), pEv->phase(), pEv->inverted() ) {
};

WheelEvent::~WheelEvent() {
}

QPointF WheelEvent::globalPosition() const {
#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	return QWheelEvent::globalPosition();
#else
	return QWheelEvent::globalPos();
#endif
}

QPointF WheelEvent::position() const {
#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0 )
	return QWheelEvent::position();
#else
	return QWheelEvent::pos();
#endif
}
