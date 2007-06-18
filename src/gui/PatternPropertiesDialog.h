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
 * $Id: PatternPropertiesDialog.h,v 1.7 2005/05/01 19:50:58 comix Exp $
 *
 */

#ifndef PATTERN_PROPERTIES_DIALOG_H
#define PATTERN_PROPERTIES_DIALOG_H

#include "config.h"
#include "qlineedit.h"
#include "qpixmap.h"
#include "qpushbutton.h"

#include "gui/UI/PatternPropertiesDialog_UI.h"
#include "lib/Song.h"

/**
 * Pattern Properties Dialog
 */
class PatternPropertiesDialog : public PatternPropertiesDialog_UI {
	Q_OBJECT
	private:
		Pattern *pattern;
		void textChanged();

	public:
		/** Constructor */
		PatternPropertiesDialog(QWidget* parent, Pattern *pattern);

		/** Destructor */
		~PatternPropertiesDialog();

		void cancelButtonClicked();
		void okButtonClicked();


	public slots:

};


#endif


