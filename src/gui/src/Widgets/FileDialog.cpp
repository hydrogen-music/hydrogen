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

#include "FileDialog.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"

#include <core/config.h>
#include <core/Helpers/Filesystem.h>

FileDialog::FileDialog( QWidget *pParent )
 : QFileDialog( pParent ) {

#if not defined(WIN32) and not defined(__APPLE__) // Linux
	// Occassionally users reported freezing of Hydrogen when interactive with
	// native file dialogs on Linux, e.g. #2165. Qt's bug tracker is full of
	// such tickets and it seems to be a problem with the interaction of the Qt
	// library with the underlying windows manager. Since I
	// (@theGreatWhiteShark) was never able to reproduce this issue and we have
	// no control of which Qt version will be combined using with window
	// manager, we will circumvent this issue by avoid all native file dialogs.
	setOption( QFileDialog::DontUseNativeDialog, true );
#endif
}

FileDialog::~FileDialog() {
}

void FileDialog::accept() {
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	QFileInfo fileInfo( selectedFiles().first() );
	if ( acceptMode() == QFileDialog::AcceptSave &&
		 ! H2Core::Filesystem::dir_writable(
			 fileInfo.absoluteDir().absolutePath(), false ) ) {
			QMessageBox::warning( this, "Hydrogen",
								  pCommonStrings->getFileDialogMissingWritePermissions(),
								  QMessageBox::Ok );
			return;
	}

	QFileDialog::accept();
}
