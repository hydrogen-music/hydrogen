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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef TAG_EDIT_H
#define TAG_EDIT_H

#include <QtGui>

#include <core/Object.h>

/** This widget allows the user to display and edit the tags of an artifact. It
 * is designed to be integrated into a bigger property dialog.
 *
 * Since all supported property dialogs - song, pattern, and drumkit - are
 * modals, we do not have to care about font or color changes in here.
 *
 * \ingroup docGUI docWidgets*/
class TagEdit : public QWidget, public H2Core::Object<TagEdit> {
	H2_OBJECT( TagEdit )
	Q_OBJECT

	static constexpr int nButtonWidth = 28;
	static constexpr int nIconWidth = 22;
	static constexpr int nMargin = 0;
	static constexpr int nMinimumRows = 4;

   public:
	TagEdit( QWidget* pParent );
	~TagEdit();

	QStringList getTags() const;
	void setTags( const QStringList& tags );

   private:
	/** Rows will always appended at the second last position (above the row
	 * bearing the "+" button). */
	void addRow( const QString& sText );
	void removeRow( int nIndex );

	void updateStyleSheet();

	QTableWidget* m_pTable;
};

#endif
