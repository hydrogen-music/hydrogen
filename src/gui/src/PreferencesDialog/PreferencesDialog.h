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

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <vector>

#include "ui_PreferencesDialog_UI.h"
#include "../Widgets/ColorSelectionButton.h"

#include <core/Object.h>
#include <core/Preferences.h>

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
	void onRejected();
	void onApplicationFontChanged(const QFont& font);
	void onLevel2FontChanged( const QFont& font );
	void onLevel3FontChanged( const QFont& font );
	void onFontSizeChanged( int nIndex );
	void onUILayoutChanged( int nIndex );
	void onColorNumberChanged( int nIndex );
	void onColorSelectionClicked();
	void onColoringMethodChanged( int nIndex );
	void onCustomizePaletteClicked();

private:

	void updateDriverInfo();
	void updateDriverPreferences();
	

	bool m_bNeedDriverRestart;
	QString m_sInitialLanguage;

	/** Caching the corresponding variable in Preferences in case the
		QFontDialog will be cancelled.*/
	QString m_sPreviousApplicationFontFamily;
	QString m_sPreviousLevel2FontFamily;
	QString m_sPreviousLevel3FontFamily;
	H2Core::Preferences::FontSize m_previousFontSize;
	int m_nPreviousVisiblePatternColors;
	std::vector<QColor> m_previousPatternColors;

	QStringList m_fontFamilies;
	std::vector<ColorSelectionButton*> m_colorSelectionButtons;

};

#endif

