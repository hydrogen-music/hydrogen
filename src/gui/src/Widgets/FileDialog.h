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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <core/Object.h>

/** Custom file dialog checking whether the user has write access to
 * the selected folder before allowing to save a file.
 */
/** \ingroup docGUI docWidgets*/
class FileDialog : public QFileDialog, public H2Core::Object<FileDialog>
{
    H2_OBJECT(FileDialog)
	
public:

	FileDialog( QWidget* pParent );
	~FileDialog();

public slots:
	void accept() override;
};
#endif
