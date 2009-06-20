/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <QtGui>

#include "../HydrogenApp.h"
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanel.h"
#include "SongEditor.h"
#include <hydrogen/hydrogen.h>

namespace H2Core
{

SongEditorPanelBpmWidget::SongEditorPanelBpmWidget( QWidget* pParent, int beat )
	: QDialog( pParent )
	, Object( "SoundLibraryPropertiesDialog" )
	, m_stimelineposition ( beat )
{
	setupUi( this );
	INFOLOG( "INIT" );
	setWindowTitle( trUtf8( "TimeLine Edit" ) );	
	setFixedSize( width(), height() );
	lineEditBEAT->setText(QString("%1").arg(m_stimelineposition + 1));
}




SongEditorPanelBpmWidget::~SongEditorPanelBpmWidget()
{
	INFOLOG( "DESTROY" );

}

}