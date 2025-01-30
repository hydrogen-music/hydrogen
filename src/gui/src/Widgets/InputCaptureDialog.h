/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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

#ifndef INPUT_CAPTURE_DIALOG_H
#define INPUT_CAPTURE_DIALOG_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

class InputCaptureDialog : public QDialog,
						   public H2Core::Object<InputCaptureDialog>
{
	H2_OBJECT(InputCaptureDialog)
	Q_OBJECT

public:
	enum class Type {
		IntMidi,
		Int,
		Float,
		String,
	};

	InputCaptureDialog( QWidget* pParent, const QString& sTitle,
						const QString& sLabel, const Type& type,
						float fMin = 0, float fMax = 0 );
	~InputCaptureDialog();

	QString text() const;

protected:
	void keyPressEvent( QKeyEvent* ev ) override;

	QLabel* m_pLabel;
	QLabel* m_pLabelTitle;
	QLabel* m_pLabelBounds;
	QLineEdit* m_pLineEdit;

	/** Used to identify a particular input within an action*/
	QString m_sLabel;
	/** Used to identify the overall action which may encompass
	 * multiple input captures */
	QString m_sTitle;
	Type m_type;
	float m_fMax;
	float m_fMin;
};

#endif
