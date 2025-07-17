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


#ifndef BPM_SPIN_BOX_H
#define BPM_SPIN_BOX_H

#include <core/Object.h>

#include "../Widgets/LCDSpinBox.h"

#include <QtGui>
#include <QPushButton>


/** \ingroup docGUI docWidgets*/
class BpmSpinBox : public LCDSpinBox
				 , public H2Core::Object<BpmSpinBox>
{
    H2_OBJECT(BpmSpinBox)
	Q_OBJECT

	public:
		BpmSpinBox( QWidget *pParent, const QSize& size );
		~BpmSpinBox();

	private:
		/** Ensure the two digits are shown after point. This yields the initial
		 * tempo "120.00" displayed on first-boot, which is consistent with what
		 * most modern DAWs do. Hopefully, this emphasizes a little more that 1)
		 * the widget handles tempo and 2) that one can set non-integer
		 * values. */
		QString textFromValue( double fValue ) const override;
};

#endif
