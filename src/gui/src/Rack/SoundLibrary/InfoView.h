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

#ifndef INFO_VIEW_H
#define INFO_VIEW_H

#include <map>
#include <memory>

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

#include "../../Widgets/WidgetWithScalableFont.h"

namespace H2Core {
class SoundLibraryInfo;
}

/** \ingroup docGUI*/
class InfoView : public QWidget,
				 protected WidgetWithScalableFont<8, 10, 12>,
				 private H2Core::Object<InfoView> {
	H2_OBJECT( InfoView )
	Q_OBJECT
   public:
	InfoView( QWidget* parent );
	~InfoView();

	void updateContent( std::shared_ptr<H2Core::SoundLibraryInfo> pInfo );
	void updateVisibility();

   private:
	void mousePressEvent( QMouseEvent* event ) override;

	QLabel* m_pDetailName;
	QLabel* m_pDetailAuthor;
	QLabel* m_pDetailInfo;
	QLabel* m_pDetailLicense;
	QLabel* m_pDetailCategory;
	QLabel* m_pDetailPath;

	QMenu* m_pMenu;
};

#endif
