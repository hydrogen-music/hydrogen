/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include "config.h"

#include "ui_PreferencesDialog_UI.h"

#include <hydrogen/Object.h>

///
/// Preferences Dialog
///
class PreferencesDialog : public QDialog, private Ui_PreferencesDialog_UI, public Object
{
	Q_OBJECT
	public:
		PreferencesDialog( QWidget* parent );
		~PreferencesDialog();

	private slots:
		void on_okBtn_clicked();
		void on_cancelBtn_clicked();
		void on_selectApplicationFontBtn_clicked();
		void on_selectMixerFontBtn_clicked();
		void on_restartDriverBtn_clicked();
		void on_driverComboBox_activated( int index );
		void on_bufferSizeSpinBox_valueChanged( int i );
		void on_sampleRateComboBox_editTextChanged( const QString& text );
		void on_midiPortComboBox_activated( int index );
		void on_styleComboBox_activated( int index );
		void on_useLashCheckbox_clicked();

	private:
		bool m_bNeedDriverRestart;

		void updateDriverInfo();
};

#endif

