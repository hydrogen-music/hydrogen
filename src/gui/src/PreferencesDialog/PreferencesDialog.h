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

#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <vector>
#include <memory>

#include "../Widgets/ColorSelectionButton.h"
#include "../Widgets/LCDCombo.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/IO/JackAudioDriver.h>
#include <core/IO/OssDriver.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/PulseAudioDriver.h>

#include <QtWidgets>
#include <QColorDialog>

///
/// Combo box showing a list of available devices for a given driver.
/// List is calculated lazily when needed.
///
/** \ingroup docGUI docConfiguration*/
class DeviceComboBox : public LCDCombo {

	H2Core::Preferences::AudioDriver m_driver;
	QString m_sHostAPI;

public:
	DeviceComboBox( QWidget *pParent );

	/// Set the driver name to use
	void setDriver( const H2Core::Preferences::AudioDriver& driver ) {
		m_driver = driver; }
	void setHostAPI( QString sHostAPI ) { m_sHostAPI = sHostAPI; }

	virtual void showPopup();
};

///
/// Combo box showing a list of HostAPIs.
///
/** \ingroup docGUI docConfiguration*/
class HostAPIComboBox : public LCDCombo {

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
		void driverComboBoxActivated( int index );
		void portaudioHostAPIComboBoxActivated( int index );
		void latencyTargetSpinBoxValueChanged( int i );
		void bufferSizeSpinBoxValueChanged( int i );
		void sampleRateComboBoxEditTextChanged( const QString& text );
		void midiPortComboBoxActivated( int index );
		void midiOutportComboBoxActivated( int index );		
		void styleComboBoxActivated( int index );
		void on_useLashCheckbox_clicked();
		void onMidiDriverComboBoxIndexChanged( int index );
		void audioDeviceTxtChanged( const QString& );
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
	void exportTheme();
	void importTheme();
	void resetTheme();
	void onIconColorChanged(int);
	void mixerFalloffComboBoxCurrentIndexChanged(int);
	void uiScalingPolicyComboBoxCurrentIndexChanged(int);

private:

	void updateDriverInfo();
	void updateDriverInfoLabel();
	void setDriverInfoOss();
	void setDriverInfoAlsa();
	void setDriverInfoJack();
	void setDriverInfoCoreAudio();
	void setDriverInfoPortAudio();
	void setDriverInfoPulseAudio();
	void updateDriverPreferences();
	void updateAppearanceTab( const std::shared_ptr<H2Core::Theme> pTheme );

	void setColorTreeItemDirty( ColorTreeItem* pItem );
	QColor* getColorById( int nId, std::shared_ptr<H2Core::ColorTheme> uiStyle ) const;
	void setColorById( int nId, const QColor& color, std::shared_ptr<H2Core::ColorTheme> uiStyle );
	void updateColorTree();
	/**
	 * Introduce a temporal smoothing. Otherwise, moving the slider
	 * would draw to heavy on the GUI thread with every change
	 * triggering a recoloring of the whole GUI.
	 */
	void triggerColorSliderTimer();
	std::shared_ptr<H2Core::Theme> m_pCurrentTheme;
	std::shared_ptr<H2Core::Theme> m_pPreviousTheme;
	QColor* m_pCurrentColor;
	int m_nCurrentId;
	QTimer* m_pColorSliderTimer;

	/** Stores which part of the dialog was altered.*/
	H2Core::Preferences::Changes m_changes;

	bool m_bNeedDriverRestart;
	QString m_sInitialLanguage;
	std::vector<ColorSelectionButton*> m_colorSelectionButtons;

	bool m_bMidiTableChanged;

};


#endif

