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
 * $Id: PreferencesDialog.h,v 1.8 2005/05/01 19:50:59 comix Exp $
 *
 */


#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include "config.h"

#include "UI/PreferencesDialog_UI.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include "qspinbox.h"
#include "qlineedit.h"
#include "qcombobox.h"
#include "qlabel.h"
#include "qpixmap.h"
#include "qcheckbox.h"
#include "qfontdialog.h"

#include "lib/Object.h"

/// Preferences Dialog
class PreferencesDialog : public PreferencesDialog_UI, public Object
{
	Q_OBJECT
	public:
		PreferencesDialog(QWidget* parent);
		~PreferencesDialog();

	private:
		bool m_bNeedDriverRestart;

		void cancelBtnClicked();
		void okBtnClicked();
		void driverChanged();
		void bufferSizeChanged();
		void sampleRateChanged();

		void updateDriverInfo();
		void useMetronomeCheckboxChanged();
		void selectApplicationFont();
		void selectMixerFont();
		void restartDriverBtnClicked();
		void midiPortChannelChanged();
		void guiStyleChanged();
};

#endif

