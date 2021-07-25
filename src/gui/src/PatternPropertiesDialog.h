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

#ifndef PATTERN_PROPERTIES_DIALOG_H
#define PATTERN_PROPERTIES_DIALOG_H


#include <QtGui>
#include <QtWidgets>

#include "ui_PatternPropertiesDialog_UI.h"

namespace H2Core
{
	class Pattern;
}

///
///Pattern Properties Dialog
///
class PatternPropertiesDialog : public QDialog, public Ui_PatternPropertiesDialog_UI
{
	Q_OBJECT
	public:
		PatternPropertiesDialog( QWidget* parent, H2Core::Pattern* pattern, int nselectedPattern, bool save );

		~PatternPropertiesDialog();

		/// Does some name check
		void defaultNameCheck( QString , bool);

	private slots:
		void on_cancelBtn_clicked();
		void on_okBtn_clicked();

	private:
		H2Core::Pattern *pattern;
		int __nselectedPattern;
		bool __savepattern;
};


#endif


