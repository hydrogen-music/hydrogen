/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <memory>

#include <QtWidgets>

#include "Widgets/WidgetWithLicenseProperty.h"

#include <core/Object.h>

class Button;
class LCDCombo;
class LCDDisplay;
class LCDSpinBox;
class LCDTextEdit;
class TagEdit;

namespace H2Core {
class Pattern;
}

/** \ingroup docGUI*/
class PatternPropertiesDialog : public QDialog,
								protected WidgetWithLicenseProperty,
								public H2Core::Object<PatternPropertiesDialog>

{
	H2_OBJECT( PatternPropertiesDialog )
	Q_OBJECT
   public:
	enum Action {
		/** With no additional actions, the window title suggests a change of
		 * properties and content changed will the written to the provided
		 * pattern. */
		None = 0x00,
		/** Instead of writing the changes to the supplied #m_pPattern directly,
		 * the dialog uses an undo action to exchange the pattern. This is
		 * suitable for patterns within the pattern list of the current song. */
		ModifyViaUndo = 0x01,
		/** Ensures the pattern has an unique name and alters the window title.
		 */
		Duplicate = 0x02,
		/** Provides the user write access the path of the underlying resource
		 * as well and alters the window title. */
		SaveAs = 0x04
	};

	PatternPropertiesDialog(
		QWidget* pParent,
		std::shared_ptr<H2Core::Pattern> pPattern,
		int nSelectedPattern,
		Action action
	);

	~PatternPropertiesDialog();

   private slots:
	void on_cancelBtn_clicked();
	void on_okBtn_clicked();
	void licenseComboBoxChanged( int );

   private:
	std::shared_ptr<H2Core::Pattern> m_pPattern;
	int m_nSelectedPattern;
	Action m_action;

	LCDDisplay* m_pPathEdit;
	LCDDisplay* m_pPatternNameTxt;
	LCDSpinBox* m_pVersionSpinBox;
	LCDDisplay* m_pAuthorTxt;
	LCDCombo* m_pLicenseComboBox;
	LCDDisplay* m_pLicenseStringTxt;
	LCDTextEdit* m_pPatternDescTxt;
	TagEdit* m_pTagEdit;
	Button* m_pOkBtn;
	Button* m_pCancelBtn;
	Button* m_pPathBrowseButton;
};

#endif
