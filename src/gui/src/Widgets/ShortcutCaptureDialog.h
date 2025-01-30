/*
 * Hydrogen
 * Copyright(c) 2023-2025 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
 *
 * Copyright (C) 1999-2011 by Werner Schweer and others
 * Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
 *
 * Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
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

#ifndef SHORTCUT_CAPTURE_DIALOG_H
#define SHORTCUT_CAPTURE_DIALOG_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

class ShortcutCaptureDialog : public QDialog,
							  public H2Core::Object<ShortcutCaptureDialog>
{
	H2_OBJECT(ShortcutCaptureDialog)
	Q_OBJECT

public:
	ShortcutCaptureDialog( QWidget* pParent );
	~ShortcutCaptureDialog();

private slots:
	/** return the shortcut to parent widget */
	void apply();

private:
	void keyPressEvent( QKeyEvent* ev ) override;
	
	QLabel* m_pLabel;
	int  m_nKey;
};

#endif
