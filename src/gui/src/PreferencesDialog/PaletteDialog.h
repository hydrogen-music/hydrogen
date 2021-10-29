/*
 * Hydrogen
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef PALETTE_DIALOG_H
#define PALETTE_DIALOG_H

#include <core/Object.h>
#include <QDialog>
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/ColorSelectionButton.h"

class PaletteDialog : public QDialog, private H2Core::Object<PaletteDialog>
{
	H2_OBJECT(PaletteDialog)
	Q_OBJECT

public:
	explicit PaletteDialog( QWidget* pParent );
	~PaletteDialog();

private slots:
	void onRejected();
	void onColorSelectionClicked();

private:
	void addPair( QString sNew, QColor newColor );
	
	std::vector<std::pair<std::shared_ptr<ClickableLabel>, std::shared_ptr<ColorSelectionButton>>> m_colorSelections;
	H2Core::UIStyle m_previousStyle;
	H2Core::UIStyle m_currentStyle;
};
	
#endif
