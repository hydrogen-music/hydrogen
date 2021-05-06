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


#include "ui_PreferencesDialog_UI.h"

#include <core/Object.h>

///
/// Preferences Dialog
///
class PreferencesDialog : public QDialog, private Ui_PreferencesDialog_UI, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT
	public:
		explicit PreferencesDialog( QWidget* parent );
		~PreferencesDialog();
		static QString m_sColorRed;
							  
	private slots:
		void on_okBtn_clicked();
		void on_cancelBtn_clicked();
		void on_selectApplicationFontBtn_clicked();
		void on_selectMixerFontBtn_clicked();
		void on_restartDriverBtn_clicked();
		void on_driverComboBox_activated( int index );
		void on_bufferSizeSpinBox_valueChanged( int i );
		void on_resampleComboBox_currentIndexChanged ( int index );
		void on_sampleRateComboBox_editTextChanged( const QString& text );
		void on_midiPortComboBox_activated( int index );
		void on_midiOutportComboBox_activated( int index );		
		void on_styleComboBox_activated( int index );
		void on_useLashCheckbox_clicked();
		void onMidiDriverComboBoxIndexChanged( int index );
		void toggleTrackOutsCheckBox(bool toggled);
		void toggleOscCheckBox(bool toggled);
		void coloringMethodCombo_currentIndexChanged (int index);


private:

	void updateDriverInfo();
	void updateDriverPreferences();

	/** Triggered every time a different option for the application
		font was chosen in the corresponding QFontDialog*/
	void onCurrentApplicationFontChanged(const QFont& font);
	/** Triggered only after the final choice of the application
		font in the corresponding QFontDialog*/
	void onApplicationFontSelected(const QFont& font);
	/** Triggered when the corresponding QFontDialog is cancelled.*/
	void onApplicationFontRejected();

	bool m_bNeedDriverRestart;
	QString m_sInitialLanguage;

	/** Caching the corresponding variable in Preferences in case the
		QFontDialog will be cancelled.*/
	QString m_sPreviousApplicationFontFamily;
	/** Caching the corresponding variable in Preferences in case the
		QFontDialog will be cancelled.*/
	int m_nPreviousApplicationFontPointSize;

};

#endif

