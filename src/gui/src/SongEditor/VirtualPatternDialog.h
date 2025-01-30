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

#ifndef VIRTUAL_PATTERN_DIALOG_H
#define VIRTUAL_PATTERN_DIALOG_H


#include <QtGui>
#include <QtWidgets>

#include "ui_VirtualPatternDialog_UI.h"

#include <core/Object.h>

namespace H2Core
{
	class Pattern;
	class PatternList;
}


///
/// Virtual Pattern Dialog
///
/** \ingroup docGUI*/
class VirtualPatternDialog :  public QDialog, public Ui_VirtualPatternDialog_UI,  public H2Core::Object<VirtualPatternDialog>
{
    H2_OBJECT(VirtualPatternDialog)
	Q_OBJECT
	public:
		explicit VirtualPatternDialog( QWidget* parent );
		~VirtualPatternDialog();

	private slots:
		void on_cancelBtn_clicked();
		void on_okBtn_clicked();

	private:



};


#endif


