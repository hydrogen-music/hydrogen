/*
 * Hydrogen
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

#ifndef INLINE_EDIT_H
#define INLINE_EDIT_H

#include <QLineEdit>

#include <core/config.h>
#include <core/Object.h>

/** QLineEdit variant used to rename pattern and instrument names in the
 * sidebars of the editors.
 *
 * It mostly behaves like a regular QLineEdit but the input can be discarded by
   hitting Esc.
 *
 *  \ingroup docGUI docWidgets*/
class InlineEdit : public QLineEdit, public H2Core::Object<InlineEdit>
{
    H2_OBJECT(InlineEdit)
	Q_OBJECT

public:
	InlineEdit( QWidget* pParent );
	~InlineEdit();

	void startEditing( QRect rect, const QString& sText  );

signals:
	void editAccepted();
	void editRejected();

private:
	void keyPressEvent( QKeyEvent* pEvent ) override;
};

#endif
