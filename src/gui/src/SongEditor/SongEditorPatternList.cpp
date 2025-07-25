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

#include "SongEditorPatternList.h"

#include <assert.h>
#include <map>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/License.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

using namespace H2Core;

#include "UndoActions.h"
#include "MainForm.h"
#include "SongEditorPanel.h"
#include "VirtualPatternDialog.h"
#include "../Compatibility/DropEvent.h"
#include "../Compatibility/MouseEvent.h"
#include "../PatternEditor/PatternEditorPanel.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "../PatternPropertiesDialog.h"
#include "../Skin.h"
#include "../Widgets/FileDialog.h"

struct PatternDisplayInfo {
	bool bActive;
	bool bNext;
	QString sPatternName;
};


SongEditorPatternList::SongEditorPatternList( QWidget *parent )
 : QWidget( parent )
 , m_pBackgroundPixmap( nullptr )
 , m_bBackgroundInvalid( true )
 , m_nRowHovered( -1 )
 , m_nRowClicked( 0 )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	const auto pPref = Preferences::get_instance();
	
	m_nWidth = 200;
	m_nGridHeight = pPref->getSongEditorGridHeight();
	setAttribute(Qt::WA_OpaquePaintEvent);

	setAcceptDrops(true);
	setMouseTracking( true );

	m_pPatternBeingEdited = nullptr;

	m_pLineEdit = new QLineEdit( "Inline Pattern Name", this );
	m_pLineEdit->setFrame( false );
	m_pLineEdit->hide();
	m_pLineEdit->setAcceptDrops( false );
	connect( m_pLineEdit, SIGNAL(editingFinished()), this, SLOT(inlineEditingFinished()) );
	connect( m_pLineEdit, SIGNAL(returnPressed()), this, SLOT(inlineEditingEntered()) );

	this->resize( m_nWidth, m_nInitialHeight );

	m_playingPattern_on_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_on.png" );
	m_playingPattern_off_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_off.png" );
	m_playingPattern_empty_Pixmap.load( Skin::getImagePath() + "/songEditor/playingPattern_empty.png" );

	m_pPatternPopup = new QMenu( this );
	m_pPatternPopup->addAction( pCommonStrings->getMenuActionDuplicate(), this,
								SLOT( patternPopup_duplicate() ) );
	m_pPatternPopup->addAction( pCommonStrings->getMenuActionDelete(), this,
								SLOT( patternPopup_delete() ) );
	m_pPatternPopup->addAction( tr("Fill/Clear..."),  this, SLOT( patternPopup_fill() ) );
	m_pPatternPopup->addAction( pCommonStrings->getMenuActionProperties(), this,
								SLOT( patternPopup_properties() ) );
	m_pPatternPopup->addAction( tr("Load Pattern"),  this, SLOT( patternPopup_load() ) );
	m_pPatternPopup->addAction( tr("Save Pattern"),  this, SLOT( patternPopup_save() ) );
	m_pPatternPopup->addAction( tr("Export Pattern"),  this, SLOT( patternPopup_export() ) );
	m_pPatternPopup->addAction( tr("Virtual Pattern"), this, SLOT( patternPopup_virtualPattern() ) );
	m_pPatternPopup->setObjectName( "PatternListPopup" );

	QScrollArea *pScrollArea = dynamic_cast< QScrollArea * >( parentWidget()->parentWidget() );
	assert( pScrollArea );
	m_pDragScroller = new DragScroller( pScrollArea );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( m_nWidth * pixelRatio,
									   height() * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
}



SongEditorPatternList::~SongEditorPatternList()
{
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
	delete m_pDragScroller;
}

/// Single click, select the next pattern
void SongEditorPatternList::mousePressEvent( QMouseEvent *ev )
{
	auto pEv = static_cast<MouseEvent*>( ev );

	__drag_start_position = pEv->position().toPoint();
	
	// -1 to compensate for the 1 pixel offset to align shadows and
	// -grid lines.
	int nRow = (( pEv->position().y() - 1 ) / m_nGridHeight);

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	auto pPatternList = pSong->getPatternList();

	if ( nRow < 0 || nRow >= (int)pPatternList->size() ) {
		ERRORLOG( QString( "Row [%1] out of bound" ).arg( nRow ) );
		return;
	}

	if ( ( ev->button() == Qt::MiddleButton ||
		   ( ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::RightButton ) ||
		   ( ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton ) ||
		   pEv->position().x() < 15 ) &&
		 pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {

		// Mark the pattern to be played once end of currently playing patterns
		// is reached.
		pHydrogen->toggleNextPattern( nRow );
	}
	else {
		CoreActionController::selectPattern( nRow );

		// Notify the user why nothing just happened by highlighting the pattern
		// locked button in the SongEditorPanel.
		if ( pHydrogen->isPatternEditorLocked() &&
			 pHydrogen->getAudioEngine()->getState() ==
			 AudioEngine::State::Playing ) {
			HydrogenApp::get_instance()->getSongEditorPanel()->
				highlightPatternEditorLocked();
		}
		
		if ( ev->button() == Qt::RightButton )  {
			m_nRowClicked = nRow;
			m_pPatternPopup->popup( pEv->globalPosition().toPoint() );
		}
	}

	updateEditor();
}

void SongEditorPatternList::mouseDoubleClickEvent( QMouseEvent *ev )
{
	auto pEv = static_cast<MouseEvent*>( ev );

	const int nRow = pEv->position().y() / m_nGridHeight;
	inlineEditPatternName( nRow );
}

void SongEditorPatternList::inlineEditPatternName( int row )
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();

	if ( row >= (int)pPatternList->size() ) {
		return;
	}
	m_pPatternBeingEdited = pPatternList->get( row );
	m_pLineEdit->setGeometry( 23, row * m_nGridHeight + 1 , m_nWidth - 23, m_nGridHeight  );
	m_pLineEdit->setText( m_pPatternBeingEdited->getName() );
	m_pLineEdit->selectAll();
	m_pLineEdit->show();
	m_pLineEdit->setFocus();
}

void SongEditorPatternList::inlineEditingEntered()
{
	assert( m_pPatternBeingEdited != nullptr );
	
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
	
	/*
	 * Make sure that the entered pattern name is unique.
	 * If it is not, use an unused pattern name.
	 */
	
	QString patternName = pPatternList->findUnusedPatternName( m_pLineEdit->text(), m_pPatternBeingEdited );

	SE_modifyPatternPropertiesAction *action =
		new SE_modifyPatternPropertiesAction( m_pPatternBeingEdited->getVersion(),
											  m_pPatternBeingEdited->getName(),
											  m_pPatternBeingEdited->getAuthor(),
											  m_pPatternBeingEdited->getInfo(),
											  m_pPatternBeingEdited->getLicense(),
											  m_pPatternBeingEdited->getCategory(),
											  m_pPatternBeingEdited->getVersion(),
											  patternName,
											  m_pPatternBeingEdited->getAuthor(),
											  m_pPatternBeingEdited->getInfo(),
											  m_pPatternBeingEdited->getLicense(),
											  m_pPatternBeingEdited->getCategory(),
											  pPatternList->index( m_pPatternBeingEdited ) );
	HydrogenApp::get_instance()->pushUndoCommand( action );
}


void SongEditorPatternList::inlineEditingFinished()
{
	m_pPatternBeingEdited = nullptr;
	m_pLineEdit->hide();
}


void SongEditorPatternList::paintEvent( QPaintEvent *ev )
{
	QPainter painter(this);
	qreal pixelRatio = devicePixelRatio();
	if ( width() != m_pBackgroundPixmap->width() ||
		 height() != m_pBackgroundPixmap->height() ||
		 pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 m_bBackgroundInvalid ) {
		createBackground();
	}
	QRectF srcRect(
			pixelRatio * ev->rect().x(),
			pixelRatio * ev->rect().y(),
			pixelRatio * ev->rect().width(),
			pixelRatio * ev->rect().height()
	);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, srcRect );
}

void SongEditorPatternList::createBackground()
{
	const auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	m_bBackgroundInvalid = false;

	QFont boldTextFont( pPref->getTheme().m_font.m_sLevel2FontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	boldTextFont.setBold( true );

	//Do not redraw anything if Export is active.
	//https://github.com/hydrogen-music/hydrogen/issues/857	
	if ( pHydrogen->getIsExportSessionActive() ) {
		return;
	}
	
	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return;
	}

	const auto pPatternList = pSong->getPatternList();
	int nPatterns = pPatternList->size();
	int nSelectedPattern = pHydrogen->getSelectedPatternNumber();

	int newHeight = m_nGridHeight * nPatterns + 1;

	if ( m_nWidth != m_pBackgroundPixmap->width() ||
		 newHeight != m_pBackgroundPixmap->height() ||
		 m_pBackgroundPixmap->devicePixelRatio() != devicePixelRatio() ) {
		if (newHeight == 0) {
			newHeight = 1;	// the pixmap should not be empty
		}
		delete m_pBackgroundPixmap;
		qreal pixelRatio = devicePixelRatio();
		m_pBackgroundPixmap = new QPixmap( m_nWidth  * pixelRatio , newHeight * pixelRatio );	// initialize the pixmap
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
		this->resize( m_nWidth, newHeight );
	}

	QColor backgroundColor = pPref->getTheme().m_color.m_songEditor_backgroundColor.darker( 120 );
	QColor backgroundColorSelected = pPref->getTheme().m_color.m_songEditor_selectedRowColor.darker( 114 );
	QColor backgroundColorAlternate =
		pPref->getTheme().m_color.m_songEditor_alternateRowColor.darker( 132 );
	QColor backgroundColorVirtual =
		pPref->getTheme().m_color.m_songEditor_virtualRowColor;

	QPainter p( m_pBackgroundPixmap );


	// Offset the pattern list by one pixel to align the dark shadows
	// at the bottom of each row with the grid lines in the song editor.
	p.fillRect( QRect( 0, 0, width(), 1 ), pPref->getTheme().m_color.m_windowColor );
	
	p.setFont( boldTextFont );
	for ( int ii = 0; ii < nPatterns; ii++ ) {
		int y = m_nGridHeight * ii + 1;
		
		if ( ii == nSelectedPattern ) {
			Skin::drawListBackground( &p, QRect( 0, y, width(), m_nGridHeight ),
									  backgroundColorSelected, false );
		} else {
			const auto pPattern = pPatternList->get( ii );
			if ( pPattern != nullptr && pPattern->isVirtual() ) {
				Skin::drawListBackground( &p, QRect( 0, y, width(), m_nGridHeight ),
										  backgroundColorVirtual,
										  ii == m_nRowHovered );
			}
			else if ( ( ii % 2 ) == 0 ) {
				Skin::drawListBackground( &p, QRect( 0, y, width(), m_nGridHeight ),
										  backgroundColor,
										  ii == m_nRowHovered );
			}
			else {
				Skin::drawListBackground( &p, QRect( 0, y, width(), m_nGridHeight ),
										  backgroundColorAlternate,
										  ii == m_nRowHovered );
			}
		}
	}

	std::unique_ptr<PatternDisplayInfo[]> PatternArray{new PatternDisplayInfo[nPatterns]};

	auto pAudioEngine = pHydrogen->getAudioEngine();
	pAudioEngine->lock( RIGHT_HERE );
	auto pPlayingPatterns = pAudioEngine->getPlayingPatterns();

	//assemble the data..
	for ( int i = 0; i < nPatterns; i++ ) {
		auto pPattern = pSong->getPatternList()->get(i);
		if ( pPattern == nullptr ) {
			continue;
		}

		if ( pPlayingPatterns->index( pPattern ) != -1 ) {
			PatternArray[i].bActive = true;
		} else {
			PatternArray[i].bActive = false;
		}

		if ( pAudioEngine->getNextPatterns()->index( pPattern ) != -1 ) {
			PatternArray[i].bNext = true;
		} else {
			PatternArray[i].bNext = false;
		}

		PatternArray[i].sPatternName = pPattern->getName();
	}
	pAudioEngine->unlock();

	/// paint the foreground (pattern name etc.)
	for ( int i = 0; i < nPatterns; i++ ) {
		if ( i == nSelectedPattern ) {
			p.setPen( pPref->getTheme().m_color.m_songEditor_selectedRowTextColor );
		}
		else {
			p.setPen( pPref->getTheme().m_color.m_songEditor_textColor );
		}

		int text_y = i * m_nGridHeight;

		p.drawText( 25, text_y - 1, m_nWidth - 25, m_nGridHeight + 2,
					Qt::AlignVCenter, PatternArray[i].sPatternName);

		Skin::Stacked mode = Skin::Stacked::None;
		if ( PatternArray[i].bNext && PatternArray[i].bActive) {
			mode = Skin::Stacked::OffNext;
		}
		else if ( PatternArray[i].bNext ) {
			mode = Skin::Stacked::OnNext;
		}
		else if (PatternArray[i].bActive) {
			mode = Skin::Stacked::On;
		}
		else if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
			mode = Skin::Stacked::Off;
		}
		
		if ( mode != Skin::Stacked::None ) {
			Skin::drawStackedIndicator( &p, 5, text_y + 4, mode );
		}

	}
}

void SongEditorPatternList::patternPopup_virtualPattern()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song" );
		return;
	}
	auto pPatternList = pSong->getPatternList();
	if ( pPatternList == nullptr ) {
		ERRORLOG( "no pattern list");
		return;
	}

	VirtualPatternDialog* pDialog = new VirtualPatternDialog( this );
	QListWidget* pPatternListWidget = pDialog->patternList;
	pPatternListWidget->setSortingEnabled(1);
	
	auto pPatternClicked = pPatternList->get( m_nRowClicked );

	std::map<QString, std::shared_ptr<Pattern>> patternNameMap;

	for ( const auto& pPattern : *pPatternList ) {
		QString sPatternName = pPattern->getName();

		if ( sPatternName == pPatternClicked->getName() ) {
			// Current pattern. A virtual pattern must not contain itself.
			continue;
		}

		patternNameMap[ sPatternName ] = pPattern;

		QListWidgetItem* pNewItem =
			new QListWidgetItem( sPatternName, pPatternListWidget);
		pPatternListWidget->insertItem( 0, pNewItem );

		if ( pPatternClicked->getVirtualPatterns()->find( pPattern ) !=
			 pPatternClicked->getVirtualPatterns()->end() ) {
			// pattern is already contained in virtual pattern.
			pNewItem->setSelected( true );
		}
	}

	if ( pDialog->exec() == QDialog::Accepted ) {
		pPatternClicked->virtualPatternsClear();
		for ( int ii = 0; ii < pPatternListWidget->count(); ++ii ) {
			QListWidgetItem* pListItem = pPatternListWidget->item( ii );
			if ( pListItem != nullptr && pListItem->isSelected() ) {
				if ( patternNameMap.find( pListItem->text() ) !=
					 patternNameMap.end() ) {
					pPatternClicked->virtualPatternsAdd(
						patternNameMap[ pListItem->text() ] );
				}
				else {
					ERRORLOG( QString( "Selected pattern [%1] could not be retrieved" )
							  .arg( pListItem->text() ) );
				}
			}
		}
		
		pHydrogen->updateVirtualPatterns();
	}
	
	delete pDialog;
}



void SongEditorPatternList::patternPopup_load()
{
	auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pPattern = pSong->getPatternList()->get( m_nRowClicked );
	if ( pPattern == nullptr ) {
		return;
	}

	QString sPath = pPref->getLastOpenPatternDirectory();
	if ( ! Filesystem::dir_readable( sPath, false ) ){
		sPath = Filesystem::patterns_dir();
	}

	FileDialog fd(this);
	fd.setAcceptMode( QFileDialog::AcceptOpen );
	fd.setFileMode( QFileDialog::ExistingFile );
	fd.setNameFilter( Filesystem::patterns_filter_name );
	fd.setDirectory( sPath );
	fd.setWindowTitle( QString( tr( "Open Pattern to Replace " )
								.append( pPattern->getName() ) ) );

	if (fd.exec() != QDialog::Accepted) {
		return;
	}
	const QString sPatternPath = fd.selectedFiles().first();

	const QString sPrevPatternPath = Filesystem::tmp_file_path(
		"patternLoad.h2pattern" );
	if ( ! pPattern->save( sPrevPatternPath ) ) {
		QMessageBox::warning( this, "Hydrogen",
							  tr("Could not save pattern to temporary directory.") );
		return;
	}
	const QString sSequencePath = Filesystem::tmp_file_path(
		"patternLoad-SEQ.xml" );
	if ( ! pSong->saveTempPatternList( sSequencePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export sequence.") );
		return;
	}
	pPref->setLastOpenPatternDirectory( fd.directory().absolutePath() );

	SE_loadPatternAction *action =
		new SE_loadPatternAction( sPatternPath, sPrevPatternPath, sSequencePath,
								  m_nRowClicked, false );
	HydrogenApp::get_instance()->pushUndoCommand( action );
}

void SongEditorPatternList::patternPopup_export()
{
	HydrogenApp::get_instance()->getMainForm()->
		action_file_export_pattern_as( m_nRowClicked );
	return;
}

void SongEditorPatternList::patternPopup_save()
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pCommonStrings = pHydrogenApp->getCommonStrings();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	const auto pPattern = pSong->getPatternList()->get( m_nRowClicked );
	if ( pPattern == nullptr ) {
		return;
	}

	// TODO: fix me. This should access the path the pattern was stored at
	// previously.
	const QString sPath = QString( "%1/%2%3" )
		.arg( Filesystem::patterns_dir() ).arg( pPattern->getName() )
		.arg( Filesystem::patterns_ext );

	if ( ! pPattern->save( sPath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export pattern.") );
	}
	else {
		pHydrogenApp->showStatusBarMessage( tr( "Pattern saved." ) );
		pHydrogen->getSoundLibraryDatabase()->updatePatterns();
	}
}

void SongEditorPatternList::patternPopup_edit()
{
	HydrogenApp::get_instance()->getPatternEditorPanel()->show();
	HydrogenApp::get_instance()->getPatternEditorPanel()->setFocus();
}



void SongEditorPatternList::patternPopup_properties()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pPattern = pSong->getPatternList()->get( m_nRowClicked );

	PatternPropertiesDialog *dialog =
		new PatternPropertiesDialog( this, pPattern, m_nRowClicked, false);
	dialog->exec();
	delete dialog;
	dialog = nullptr;
}


void SongEditorPatternList::acceptPatternPropertiesDialogSettings(
	const int nNewVersion,
	const QString& newPatternName,
	const QString& sNewAuthor,
	const QString& newPatternInfo,
	const License& newLicense,
	const QString& newPatternCategory,
	int patternNr )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto patternList = pSong->getPatternList();
	auto pattern = patternList->get( patternNr );
	pattern->setVersion( nNewVersion );
	pattern->setName( newPatternName );
	pattern->setAuthor( sNewAuthor );
	pattern->setInfo( newPatternInfo );
	pattern->setLicense( newLicense );
	pattern->setCategory( newPatternCategory );
	pHydrogen->setIsModified( true );

	EventQueue::get_instance()->pushEvent( Event::Type::PatternModified, -1 );
}


void SongEditorPatternList::revertPatternPropertiesDialogSettings(
	const int nOldVersion,
	const QString& oldPatternName,
	const QString& sOldAuthor,
	const QString& oldPatternInfo,
	const License& oldLicense,
	const QString& oldPatternCategory,
	int patternNr )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto patternList = pSong->getPatternList();
	auto pattern = patternList->get( patternNr );
	pattern->setVersion( nOldVersion );
	pattern->setName( oldPatternName );
	pattern->setAuthor( sOldAuthor );
	pattern->setInfo( oldPatternInfo );
	pattern->setLicense( oldLicense );
	pattern->setCategory( oldPatternCategory );
	pHydrogen->setIsModified( true );
	EventQueue::get_instance()->pushEvent( Event::Type::PatternModified, -1 );
}


void SongEditorPatternList::patternPopup_delete()
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pPattern = pSong->getPatternList()->get( m_nRowClicked );
	if ( pPattern == nullptr ) {
		return;
	}

	const QString sPatternPath = Filesystem::tmp_file_path(
		"patternDelete.h2pattern" );

	if ( ! pPattern->save( sPatternPath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
		return;
	}

	const QString sSequencePath = Filesystem::tmp_file_path(
		"patternDelete-SEQ.xml" );
	if ( ! pSong->saveTempPatternList( sSequencePath ) ) {
		QMessageBox::warning( this, "Hydrogen", tr("Could not export sequence.") );
		return;
	}

	SE_deletePatternFromListAction *action =
		new SE_deletePatternFromListAction( sPatternPath, sSequencePath,
											m_nRowClicked );
	HydrogenApp::get_instance()->pushUndoCommand( action );
}

void SongEditorPatternList::patternPopup_duplicate()
{
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	const auto pPatternList = pSong->getPatternList();
	const auto pPattern = pPatternList->get( m_nRowClicked );
	if ( pPattern == nullptr ) {
		return;
	}

	auto pNewPattern = std::make_shared<Pattern>( pPattern );

	// In case the original pattern does not feature license and/or author, fall
	// back to the ones set in the song. This way user can just set those
	// parameters once in the SongPropertiesDialog and reuse those values in
	// here.
	if ( pNewPattern->getAuthor().isEmpty() &&
		 pSong->getAuthor() != "hydrogen" &&
		 pSong->getAuthor() != "Unknown Author" ) {
		pNewPattern->setAuthor( pSong->getAuthor() );
	}
	if ( pNewPattern->getLicense().isEmpty() &&
		 ! pSong->getLicense().isEmpty() ) {
		pNewPattern->setLicense( pSong->getLicense() );
	}

	PatternPropertiesDialog *dialog = new PatternPropertiesDialog(
		this, pNewPattern, m_nRowClicked, true );

	if ( dialog->exec() == QDialog::Accepted ) {
		const QString sPath = Filesystem::tmp_file_path(
			"patternDuplicate.h2pattern" );
		if ( ! pNewPattern->save( sPath ) ) {
			QMessageBox::warning( this, "Hydrogen", tr("Could not save pattern to temporary directory.") );
		}
		else {
			SE_duplicatePatternAction *action =
				new SE_duplicatePatternAction( sPath, m_nRowClicked + 1 );
			HydrogenApp::get_instance()->pushUndoCommand( action );
		}
	}

	delete dialog;
}

void SongEditorPatternList::patternPopup_fill()
{
	FillRange range;
	PatternFillDialog *dialog = new PatternFillDialog( this, &range );

	// use a PatternFillDialog to get the range and mode data
	if ( dialog->exec() == QDialog::Accepted ) {

		SE_fillRangePatternAction *action =
			new SE_fillRangePatternAction( &range, m_nRowClicked );
		HydrogenApp::get_instance()->pushUndoCommand( action );
	}

	delete dialog;
}


void SongEditorPatternList::fillRangeWithPattern( FillRange* pRange, int nPattern )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );

	auto pPatternList = pSong->getPatternList();
	auto pPattern = pPatternList->get( nPattern );
	auto pColumns = pSong->getPatternGroupVector();	// E' la lista di "colonne" di pattern
	std::shared_ptr<PatternList> pColumn = nullptr;

	int nColumn, nColumnIndex;
	bool bHasPattern = false;
	int fromVal = pRange->fromVal - 1;
	int toVal   = pRange->toVal;

	// Add patternlists to PatternGroupVector as necessary
	int nDelta = toVal - pColumns->size() + 1;

	for ( int i = 0; i < nDelta; i++ ) {
		pColumn = std::make_shared<PatternList>();
		pColumns->push_back( pColumn );
	}

	// Fill or Clear each cell in range
	for ( nColumn = fromVal; nColumn < toVal; nColumn++ ) {

		// expand Pattern
		pColumn = ( *pColumns )[ nColumn ];
		
		assert( pColumn );

		bHasPattern = false;

		// check whether the pattern (and column) already exists
		for ( nColumnIndex = 0; pColumn && nColumnIndex < (int)pColumn->size(); nColumnIndex++) {

			if ( pColumn->get( nColumnIndex ) == pPattern ) {
				bHasPattern = true;
				break;
			}
		}

		if ( pRange->bInsert && !bHasPattern ) {       //fill
			pColumn->add( pPattern);
		}
		else if ( !pRange->bInsert && bHasPattern ) {  // clear
			pColumn->del( pPattern);
		}
	}

		// remove all the empty patternlists at the end of the song
		for ( int i = pColumns->size() - 1; i != 0 ; i-- ) {
			auto pList = (*pColumns)[ i ];
			int nSize = pList->size();
			if ( nSize == 0 ) {
				pColumns->erase( pColumns->begin() + i );
			}
			else {
				break;
			}
		}
	pHydrogen->getAudioEngine()->unlock();


	// Update
	pHydrogen->setIsModified( true );
	HydrogenApp::get_instance()->getSongEditorPanel()->updateEditors();
}

void SongEditorPatternList::updateEditor() {
	m_bBackgroundInvalid = true;
	update();
}

///drag & drop
void SongEditorPatternList::dragEnterEvent(QDragEnterEvent *event)
{
	if ( event->mimeData()->hasFormat("text/plain") ) {
			event->acceptProposedAction();
	}
}


void SongEditorPatternList::dropEvent(QDropEvent *event)
{
	auto pEv = static_cast<DropEvent*>( event );

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	QString sText = event->mimeData()->text();
	const QMimeData* mimeData = event->mimeData();
	
	int nTargetPattern = 0;
	if(m_nGridHeight > 0)
	{
		nTargetPattern = pEv->position().y() / m_nGridHeight;
	}
	
	if( sText.startsWith("Songs:") || sText.startsWith("move instrument:") || sText.startsWith("importInstrument:")){
		event->acceptProposedAction();
		return;
	}
	
	if ( sText.startsWith("move pattern:") ) {
		QStringList tokens = sText.split( ":" );
		bool bOK = true;

		int nSourcePattern = tokens[1].toInt(&bOK);
		if ( ! bOK ) {
			return;
		}

		if ( nSourcePattern == nTargetPattern ) {
			event->acceptProposedAction();
			return;
		}

		auto pAction = new SE_movePatternListItemAction(
			nSourcePattern , nTargetPattern ) ;
		HydrogenApp::get_instance()->pushUndoCommand( pAction );

		event->acceptProposedAction();
	} 
	else if( sText.startsWith("file://") && mimeData->hasUrls() )
	{
		//Dragging a file from an external file manager
		auto pPatternList = pSong->getPatternList();
		QList<QUrl> urlList = mimeData->urls();

		int successfullyAddedPattern = 0;
		
		for (int i = 0; i < urlList.size(); i++)
		{
			QString patternFilePath = urlList.at(i).toLocalFile();
			if( patternFilePath.endsWith(".h2pattern") )
			{
				auto pPattern = Pattern::load( patternFilePath );
				if ( pPattern)
				{
					auto pNewPattern = pPattern;
			
					if(!pPatternList->checkName( pNewPattern->getName() ) ){
						pNewPattern->setName( pPatternList->findUnusedPatternName( pNewPattern->getName() ) );
					}
					
					auto pAction = new SE_insertPatternAction(
						nTargetPattern + successfullyAddedPattern, pNewPattern );
					HydrogenApp::get_instance()->pushUndoCommand( pAction );

					successfullyAddedPattern++;
				}
				else
				{
					ERRORLOG( QString("Error loading pattern %1")
							  .arg(patternFilePath) );
				}
			}
		}
	} 
	else 
	{
		QStringList tokens = sText.split( "::" );
		QString sPatternName = tokens.at( 1 );

		//create a unique sequencefilename
		auto pPattern = pSong->getPatternList()->get( nTargetPattern );

		QString oldPatternName = pPattern->getName();

		QString sequenceFilename = Filesystem::tmp_file_path( "SEQ.xml" );
		const bool bDrag = QString( tokens.at(0) ).contains( "drag pattern" );

		SE_loadPatternAction *pAction = new SE_loadPatternAction(
			sPatternName, oldPatternName, sequenceFilename, nTargetPattern,
			bDrag );

		HydrogenApp::get_instance()->pushUndoCommand( pAction );
	}
}



void SongEditorPatternList::movePatternLine( int nSourcePattern , int nTargetPattern )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pPatternList = pSong->getPatternList();

	// move patterns...
	auto pSourcePattern = pPatternList->get( nSourcePattern );
	if ( nSourcePattern < nTargetPattern) {
		for (int nPatr = nSourcePattern; nPatr < nTargetPattern; nPatr++) {
			auto pPattern = pPatternList->get(nPatr + 1);
			pPatternList->replace( nPatr, pPattern );
		}
		pPatternList->replace( nTargetPattern, pSourcePattern );
	}
	else {
		for (int nPatr = nSourcePattern; nPatr > nTargetPattern; nPatr--) {
			auto pPattern = pPatternList->get(nPatr - 1);
			pPatternList->replace( nPatr, pPattern );
		}
		pPatternList->replace( nTargetPattern, pSourcePattern );
	}

	if ( pHydrogen->isPatternEditorLocked() ) {
		pHydrogen->updateSelectedPattern();
	} else  {
		pHydrogen->setSelectedPatternNumber(
			nTargetPattern, true, Event::Trigger::Default );
	}
	HydrogenApp::get_instance()->getSongEditorPanel()->updateEditors();
	pHydrogen->setIsModified( true );
}

void SongEditorPatternList::leaveEvent( QEvent* ev ) {
	UNUSED( ev );
	m_nRowHovered = -1;
	updateEditor();
}

void SongEditorPatternList::mouseMoveEvent(QMouseEvent *event)
{
	auto pEv = static_cast<MouseEvent*>( event );
	//
	// Update the highlighting of the hovered row.
	if ( pEv->position().y() / m_nGridHeight != m_nRowHovered ) {
		m_nRowHovered = pEv->position().y() / m_nGridHeight;
		updateEditor();
	}
	
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}
	if ( (pEv->position().y() / m_nGridHeight) == (__drag_start_position.y() / m_nGridHeight) ) {
		return;
	}
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	auto pPatternList = pSong->getPatternList();
	int row = (__drag_start_position.y() / m_nGridHeight);
	if ( row >= (int)pPatternList->size() ) {
		return;
	}
	auto pPattern = pPatternList->get( row );
	QString sName = "<unknown>";
	if ( pPattern ) {
		sName = pPattern->getName();
	}
	QString sText = QString("move pattern:%1:%2").arg( row ).arg( sName );

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData);
	//drag->setPixmap(iconPixmap);

	m_pDragScroller->startDrag();
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
	m_pDragScroller->endDrag();

	QWidget::mouseMoveEvent(event);
}
