/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 *
 */

#ifndef PATTERN_FILL_DIALOG_H
#define PATTERN_FILL_DIALOG_H

#include "config.h"


#include "gui/UI/PatternFillDialog_UI.h"
#include "lib/Object.h"

class Pattern;

/**
 * FillRange struct
 */

struct FillRange {

        int fromVal;
        int toVal;
        bool bInsert;
};


/**
 * Pattern Fill Dialog
 */
class PatternFillDialog : public PatternFillDialog_UI, public Object
{
	Q_OBJECT
	private:
		Pattern *pattern;
		FillRange *fillRange;


	public:
		/** Constructor */
		PatternFillDialog(QWidget* parent, FillRange* range);

		/** Destructor */
		~PatternFillDialog();

		void cancelButtonClicked();
		void okButtonClicked();

		void textChanged();

	public slots:

};


#endif


