/*
 * Hydrogen
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

#ifndef WIDGET_WITH_HIGHLIGHTED_LIST_H
#define WIDGET_WITH_HIGHLIGHTED_LIST_H

/** Widget has a list of items associated with a popup which in turn
	can open dialog windows.
 */
/** \ingroup docGUI*/
class WidgetWithHighlightedList {
public:
	/**
	 * Specifies whether the row corresponding to #m_nRowClicked should
	 * be highlighted and determines the lifecycle of the
	 * highlighting.
	 *
	 * The highlighting starts when the user right-clicks a row and
	 * the associated popup window is shown. This will set
	 * #m_rowSelection to RowSelection::Popup. As soon as the popup
	 * get's hidden (by clicking at an arbitrary position), it will be
	 * reset to RowSelection::None by a callback. If the user clicks
	 * an action of the popup, #m_rowSelection will be upgraded to
	 * RowSelection::Dialog and set the RowSelection::None once the
	 * associated dialog is closed.
	 *
	 * This slightly intricate state handling was introduced to ensure
	 * the associated row is highlighted whenever a popup and the
	 * corresponding dialog is opened. Else, one might very quickly
	 * forget whether "Pattern 6" or "Pattern 7" is the target of the
	 * delete operation.
	 */
	enum class RowSelection {
		/**
		 * No highlighting will be drawn for the row last clicked.
		 */
		None,
		/**
		 * The #m_nRowClicked row was right-clicked and a popup dialog
		 * did open and is still shown.
		 */
		Popup,
		/**
		 * The popup dialog is already closed but the user clicked an
		 * associated action and its dialog is still opened.
		 */
		Dialog
	};

protected:
	WidgetWithHighlightedList() : m_nRowClicked( 0 )
								, m_rowSelection( RowSelection::None ) {};

	/**
	 * Helper variable remembering for row was clicked last.
	 */
	int m_nRowClicked;
	/**
	 * Determines the highlighting of the row associated with #m_nRowClicked.
	 */
	RowSelection m_rowSelection;

};

 #endif
