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


#ifndef PATCH_BAY_H
#define PATCH_BAY_H


#include <map>
#include <memory>
#include <utility>

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/PatternList.h>

#include "LCDCombo.h"
#include "LCDDisplay.h"

class PatchBay : public QDialog, public H2Core::Object<PatchBay>
{
	H2_OBJECT(PatchBay)
	Q_OBJECT

public:

	PatchBay( QWidget* pParent,
			  H2Core::PatternList* pPatternList,
			  std::shared_ptr<H2Core::Drumkit> pDrumkit );
	~PatchBay();

private slots:
		void applyButtonClicked();

private:
		void setup();

		QVBoxLayout* m_pMainLayout;

		std::vector<LCDDisplay*> m_patternTypesBoxes;
		std::vector<LCDCombo*> m_drumkitInstrumentBoxes;

		H2Core::PatternList* m_pPatternList;
		std::shared_ptr<H2Core::Drumkit> m_pDrumkit;

		/** Used to correlate the choices in #m_drumkitInstrumentBoxes with the
		 * instruments of #m_pDrumkit.
		 *
		 * The index of the vector correspond to the index in the combo box, the
		 * int to instrument's ID, and the QString to the label shown in the
		 * combo box. */
		std::vector<std::pair<int, QString>> m_instrumentLabels;
};

#endif
