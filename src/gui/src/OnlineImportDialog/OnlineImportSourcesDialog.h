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

#ifndef ONLINE_IMPORT_SOURCES_DIALOG_H
#define ONLINE_IMPORT_SOURCES_DIALOG_H

#include <QDialog>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QPushButton;
class QTableWidget;
QT_END_NAMESPACE

/**
 * Dialog for managing online import source URLs.
 *
 * Lists all configured sources with a status LED (grey/green/red),
 * allows editing, adding, and removing sources. Layout mirrors TagEdit:
 * LED in left column, URL edit in middle (stretch), action buttons in right.
 * The "+" add button is always in the bottom row, right column.
 *
 * \ingroup docGUI
 */
class OnlineImportSourcesDialog : public QDialog {
	Q_OBJECT

	static constexpr int nLedSize = 14;
	static constexpr int nIconSize = 22;
	static constexpr int nButtonWidth = 28;
	static constexpr int nMinimumWidth = 1350;

public:
	explicit OnlineImportSourcesDialog( QWidget* pParent );
	~OnlineImportSourcesDialog() override;

	/** Returns the list of source URLs entered by the user. */
	QStringList getSources() const;

private:
	void buildLayout();
	void populateTable();
	void addRow( const QString& sUrl );
	void checkAllSourceStatus();
	void updateStyleSheet();

	QTableWidget* m_pTable;
	QPushButton* m_pCheckButton;
	QPushButton* m_pOkButton;
	QPushButton* m_pCancelButton;

	/** Local copy of the sources registered in Preferences. This will serve as
	 * the single source of truth while the dialog is opened. */
	QStringList m_sources;
};

inline QStringList OnlineImportSourcesDialog::getSources() const {
	return m_sources;
}

#endif // ONLINE_IMPORT_SOURCES_DIALOG_H
