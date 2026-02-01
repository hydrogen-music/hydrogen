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

#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/JackDriver.h>
#include <core/IO/OssDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/PulseAudioDriver.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Shortcuts.h>

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
	void setHostAPI( const QString& sHostAPI ) { m_sHostAPI = sHostAPI; }

	virtual void showPopup();
};

///
/// Combo box showing a list of HostAPIs.
///
/** \ingroup docGUI docConfiguration*/
class HostAPIComboBox : public LCDCombo {

public:
	HostAPIComboBox( QWidget *pParent );
	void setValue( const QString& sHostAPI );
	virtual void showPopup();
};

/** Node in the Color tree of the appearance tab.
 *
 * \ingroup docGUI docConfiguration
 */
class IndexedTreeItem : public QTreeWidgetItem {

public:
	IndexedTreeItem( int nId, QTreeWidgetItem* pParent, const QString& sLabel );
	IndexedTreeItem( int nId, QTreeWidgetItem* pParent, const QStringList& labels );
	IndexedTreeItem( int nId, QTreeWidget* pParent, const QString& sLabel );
	IndexedTreeItem( int nId, QTreeWidget* pParent, const QStringList& labels );
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
		void on_restartAudioDriverBtn_clicked();
		void on_restartMidiDriverButton_clicked();
		void driverComboBoxActivated( int index );
		void portaudioHostAPIComboBoxActivated( int index );
		void styleComboBoxActivated( int index );
		void toggleTrackOutsCheckBox(bool toggled);
		void toggleOscCheckBox(bool toggled);
	void onRejected();
	void onApplicationFontComboBoxActivated( int );
	void onLevel2FontComboBoxActivated( int );
	void onLevel3FontComboBoxActivated( int );
	void onFontSizeChanged( int nIndex );
	void onUILayoutChanged( int nIndex );
	void onColorNumberChanged( int nIndex );
	void onColorSelectionClicked();
	void onColoringMethodChanged( int nIndex );
		void onIndicateNotePlaybackChanged( int );
		void onIndicateEffectiveNoteLengthChanged( int );
	// void onCustomizePaletteClicked();
	void colorTreeSelectionChanged();
	void colorButtonChanged();
	void rsliderChanged(int);
	void gsliderChanged(int);
	void bsliderChanged(int);
	void hsliderChanged(int);
	void ssliderChanged(int);
	void vsliderChanged(int);
	void exportTheme();
	void importTheme();
	void resetTheme();
	void onIconColorChanged(int);
	void mixerFalloffComboBoxCurrentIndexChanged(int);
	void uiScalingPolicyComboBoxCurrentIndexChanged(int);
	void defineShortcut();
	/** Removes the shortcut associated with a specific action.*/
	void clearShortcut();
	/** Adds an additional instance of an action in the shortcut
	 * table.
	 *
	 * Per default each will be just present once. This way multiple
	 * shortcuts can be assigned to the same action.
	 */
	void duplicateActions();

private:

		void applyCurrentColor();
	void updateColors();
	void updateAudioDriverInfo();
	void updateAudioDriverInfoLabel();
	void updateMidiDriverInfo();
	void setAudioDriverInfoOss();
	void setAudioDriverInfoAlsa();
	void setAudioDriverInfoJack();
	void setAudioDriverInfoCoreAudio();
	void setAudioDriverInfoPortAudio();
	void setAudioDriverInfoPulseAudio();
	void writeAudioDriverPreferences();
	void writeMidiDriverPreferences();
	void updateAppearanceTab( std::shared_ptr<H2Core::Theme> pTheme );

	void initializeShortcutsTab();
	void updateShortcutsTab();
	/**
	 * Local copy of the shortcut map. It will be written back to
	 * #Preferences in case #m_bShortcutsChanged and OK button is hit.
	 */
	std::shared_ptr<H2Core::Shortcuts> m_pShortcuts;
	bool m_bShortcutsChanged;
	std::vector<H2Core::Shortcuts::Category> m_shortcutCategories;
	H2Core::Shortcuts::Category m_selectedCategory;
	/**
	 * Used to maintain the selection of items done by the user when
	 * updating/recreating the shortcut table.
	 */
	std::vector<std::pair<H2Core::Shortcuts::Action,QKeySequence>> m_lastShortcutsSelected;

	void setIndexedTreeItemDirty( IndexedTreeItem* pItem );
	std::unique_ptr<QColor> getColorById(
		int nId, std::shared_ptr<H2Core::ColorTheme> pColorTheme ) const;
	void setColorById( int nId, const QColor& color,
					  std::shared_ptr<H2Core::ColorTheme> pColorTheme );
	void updateColorTree();
	/**
	 * Introduce a temporal smoothing. Otherwise, moving the slider
	 * would draw to heavy on the GUI thread with every change
	 * triggering a recoloring of the whole GUI.
	 */
	void triggerColorSliderTimer();
	std::shared_ptr<H2Core::Theme> m_pCurrentTheme;
	std::shared_ptr<H2Core::Theme> m_pPreviousTheme;
	std::unique_ptr<QColor> m_pCurrentColor;
	int m_nCurrentId;
	QTimer* m_pColorSliderTimer;

	/** Stores which part of the dialog was altered.*/
	H2Core::Preferences::Changes m_changes;

	bool m_bAudioDriverRestartRequired;
	bool m_bMidiDriverRestartRequired;
	QString m_sInitialLanguage;
	std::vector<ColorSelectionButton*> m_colorSelectionButtons;
};


#endif

