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

#include "SampleEditor.h"
#include "../HydrogenApp.h"
#include "InstrumentEditor/InstrumentEditor.h"
#include "../widgets/Button.h"

#include <hydrogen/data_path.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/sample.h>
#include <hydrogen/audio_engine.h>

#include <QModelIndex>
#include <QTreeWidget>
#include <QMessageBox>

using namespace H2Core;
using namespace std;

SampleEditor::SampleEditor ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ( "SampleEditor" )
{
	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "SampleEditor" ) );
	setFixedSize ( width(), height() );
	installEventFilter( this );
	m_pSampleEditorStatus = false; //set true if sample changes are save
}


SampleEditor::~SampleEditor()
{
	INFOLOG ( "DESTROY" );
}


bool SampleEditor::getCloseQuestion()
{
	bool close = false;
	int err = QMessageBox::information( this, "Hydrogen", tr( "Close dialog! maybe there is some unsaved work on sample.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( err == 0 ) close = true;
	return close;

}


void SampleEditor::setSampleName( QString name )
{
	QString newfilename = name.section( '/', -1 );
//	newfilename.replace( "." + newfilename.section( '.', -1 ), "");

	QString windowname = "SampleEditor " + newfilename;
	m_samplename = name;
	setWindowTitle ( windowname );
}



