/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef PATTERN_FILL_DIALOG_H
#define PATTERN_FILL_DIALOG_H


#include <QtGui>
#include <QtWidgets>

#include "ui_PatternFillDialog_UI.h"

#include <core/Object.h>

namespace H2Core
{
	class Pattern;
}

struct FillRange {

	int fromVal;
	int toVal;
	bool bInsert;
};


///
/// Pattern Fill Dialog
///
class PatternFillDialog : public QDialog, public Ui_PatternFillDialog_UI, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT
	public:
		PatternFillDialog( QWidget* parent, FillRange* range );
		~PatternFillDialog();

	private slots:
		void on_cancelBtn_clicked();
		void on_okBtn_clicked();
		void on_fromText_textChanged(const QString & text);
		void on_toText_textChanged(const QString & text);

	private:
		FillRange* __fill_range;

		/// Does some name check
		void __text_changed();


};


#endif


