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

#ifndef SOLO_BUTTON_H
#define SOLO_BUTTON_H

#include "ColoredButton.h"

/** \ingroup docGUI docWidgets*/
class SoloButton : public ColoredButton, public H2Core::Object<SoloButton> {
	H2_OBJECT( SoloButton )
	Q_OBJECT

   public:
	SoloButton(
		QWidget* pParent,
		const QSize& size = QSize(),
		const QString& sBaseToolTip = "",
		bool bModifyOnChange = false
	);
	~SoloButton();

	SoloButton( const SoloButton& ) = delete;
	SoloButton& operator=( const SoloButton& rhs ) = delete;

   public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes
	) override;
};

#endif
