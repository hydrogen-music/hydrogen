/*
 * Hydrogen
 * Copyright(c) 2008-2023 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef MAPPINGTABLE_H_
#define MAPPINGTABLE_H_

#include <core/Object.h>
#include <QTableWidget>
#include <QKeyEvent>

/** Custom QTableWidget class for the mapping table in the
 * #DrumkitPropertiesDialog in order to navigation between the individual
 * type combo boxes using the tab key. */
class MappingTable : public QTableWidget, public H2Core::Object<MappingTable> {
	H2_OBJECT( MappingTable )
	Q_OBJECT

public:
	MappingTable( QWidget* pParent ) : QTableWidget( pParent ){};
	~MappingTable(){};

private:
	virtual void keyPressEvent( QKeyEvent* pEvent ) override {
		if ( pEvent->key() == Qt::Key_Tab || pEvent->key() == Qt::Key_Backtab ) {
			  QKeyEvent newEvent( pEvent->type(),
								  pEvent->key() == Qt::Key_Tab ? Qt::Key_Down : Qt::Key_Up,
								  pEvent->modifiers(),
								  pEvent->nativeScanCode(),
								  pEvent->nativeVirtualKey(),
								  pEvent->nativeModifiers(),
								  pEvent->text(),
								  pEvent->isAutoRepeat(),
								  pEvent->count() );
             pEvent->accept();
            QTableWidget::keyPressEvent( &newEvent );
            return;
        }
        QTableWidget::keyPressEvent( pEvent);
	};
};

#endif // MAPPINGTABLE_H_
