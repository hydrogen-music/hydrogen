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

#include "../Widgets/ColorSelectionButton.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Object.h>
#include <QtWidgets>
#include <QColorDialog>

///
/// Combo box showing a list of available devices for a given driver.
/// List is calculated lazily when needed.
///
/** \ingroup docGUI docConfiguration*/
class DeviceComboBox : public QComboBox {

	bool m_bHasDevices;
	QString m_sDriver;
	QString m_sHostAPI;

public:
	DeviceComboBox( QWidget *pParent );

	/// Set the driver name to use
	void setDriver( QString sDriver ) { m_sDriver = sDriver; }
	void setHostAPI( QString sHostAPI ) { m_sHostAPI = sHostAPI; }

	virtual void showPopup();
};

///
/// Combo box showing a list of HostAPIs.
///
/** \ingroup docGUI docConfiguration*/
class HostAPIComboBox : public QComboBox {

public:
	HostAPIComboBox( QWidget *pParent );
	void setValue( QString sHostAPI );
	virtual void showPopup();
};

/** Node in the Color tree of the appearance tab.
 *
 * \ingroup docGUI docConfiguration
 */
class ColorTreeItem : public QTreeWidgetItem {

public:
	ColorTreeItem( int nId, QTreeWidgetItem* pParent, QString sLabel );
	ColorTreeItem( int nId, QTreeWidget* pParent, QString sLabel );
	int getId() const;
	
private:
	int m_nId;
};

#include "ui_PreferencesDialog_UI.h"
///
/// Preferences Dialog
///
/** \ingroup docGUI docConfiguration*/
class PreferencesDialog :  public QDialog, private Ui_PreferencesDialog_UI,  public H2Core::Object<PreferencesDialog>
{
	H2_OBJECT(PreferencesDialog)
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
		void on_portaudioHostAPIComboBox_activated( int index );
		void on_bufferSizeSpinBox_valueChanged( int i );
		void on_resampleComboBox_currentIndexChanged ( int index );
		void on_sampleRateComboBox_editTextChanged( const QString& text );
		void on_midiPortComboBox_activated( int index );
		void on_midiOutportComboBox_activated( int index );		
		void on_styleComboBox_activated( int index );
		void on_useLashCheckbox_clicked();
		void onMidiDriverComboBoxIndexChanged( int index );
		void on_m_pAudioDeviceTxt_currentTextChanged( QString );
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
	// void onCustomizePaletteClicked();
	void colorTreeSelectionChanged();
	void colorButtonChanged();
	void rsliderChanged(int);
	void gsliderChanged(int);
	void bsliderChanged(int);
	void hsliderChanged(int);
	void ssliderChanged(int);
	void vsliderChanged(int);
	void updateColors();
	void resetColors();

private:

	void updateDriverInfo();
	void updateDriverPreferences();

	void setColorTreeItemDirty( ColorTreeItem* pItem );
	QColor* getColorById( int nId, H2Core::ColorTheme* uiStyle ) const;
	void setColorById( int nId, const QColor& color, H2Core::ColorTheme* uiStyle );
	void updateColorTree();
	/**
	 * Introduce a temporal smoothing. Otherwise, moving the slider
	 * would draw to heavy on the GUI thread with every change
	 * triggering a recoloring of the whole GUI.
	 */
	void triggerColorSliderTimer();
	H2Core::ColorTheme m_currentColors;
	H2Core::ColorTheme m_previousColors;
	QColor* m_pCurrentColor;
	int m_nCurrentId;
	QTimer* m_pColorSliderTimer;

	bool m_bNeedDriverRestart;
	QString m_sInitialLanguage;

	/** Caching the corresponding variable in Preferences in case the
		QFontDialog will be cancelled.*/
	QString m_sPreviousApplicationFontFamily;
	QString m_sPreviousLevel2FontFamily;
	QString m_sPreviousLevel3FontFamily;
	H2Core::FontTheme::FontSize m_previousFontSize;
	int m_nPreviousVisiblePatternColors;
	std::vector<QColor> m_previousPatternColors;

	QStringList m_fontFamilies;
	std::vector<ColorSelectionButton*> m_colorSelectionButtons;

};


#endif

