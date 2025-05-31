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

#ifndef UNDOACTIONS_H
#define UNDOACTIONS_H

#include <QtGui>
#include <QtWidgets>
#include <QDebug>
#include <QUndoCommand>
#include <QPoint>
#include <vector>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/AutomationPath.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/License.h>

#include "CommonStrings.h"
#include "HydrogenApp.h"
#include "InstrumentEditor/ComponentsEditor.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "InstrumentRack.h"
#include "MainForm.h"
#include "PatternEditor/NotePropertiesRuler.h"
#include "PatternEditor/DrumPatternEditor.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "SongEditor/PatternFillDialog.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SongEditor/SongEditorPatternList.h"
#include "SongEditor/SongEditorPositionRuler.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "Widgets/AutomationPathView.h"
#include "Widgets/EditorDefs.h"


//=====================================================================================================================================
//song editor commands
/** \ingroup docGUI*/
class SE_togglePatternAction : public QUndoCommand
{
public:
	SE_togglePatternAction( int nColumn, int nRow ){
		setText( QObject::tr( "Toggle Pattern ( %1, %2 )" ).arg( nColumn ).arg( nRow ) );
		m_nColumn = nColumn;
		m_nRow = nRow;
	}
	virtual void undo() {
		H2Core::CoreActionController::toggleGridCell( m_nColumn, m_nRow );
	}
	virtual void redo() {
		H2Core::CoreActionController::toggleGridCell( m_nColumn, m_nRow );
	}
private:
	int m_nColumn;
	int m_nRow;
};

/** \ingroup docGUI*/
class SE_movePatternListItemAction : public QUndoCommand
{
public:
	SE_movePatternListItemAction(  int nSourcePattern , int nTargetPattern ){
		setText( QObject::tr( "Move pattern list item ( %1, %2 )" ).arg( nSourcePattern ).arg( nTargetPattern ) );
		__nSourcePattern = nSourcePattern;
		__nTargetPattern = nTargetPattern;
	}
	virtual void undo()
	{
		//qDebug() << "Move Pattern List Item Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->movePatternLine( __nTargetPattern, __nSourcePattern );
	}
	virtual void redo()
	{
		//qDebug() << "Move Pattern List Item redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->movePatternLine( __nSourcePattern , __nTargetPattern );
	}
private:
	int __nSourcePattern;
	int __nTargetPattern;
};


/** \ingroup docGUI*/
class SE_deletePatternSequenceAction : public QUndoCommand
{
public:
	explicit SE_deletePatternSequenceAction( const QString& pFilename ){
		setText( QObject::tr( "Delete complete pattern-sequence" ) );
		__pFilename = pFilename ;
	}
	virtual void undo()
	{
		//qDebug() << "Delete complete pattern-sequence  undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->restoreGroupVector( __pFilename );
	}

	virtual void redo()
	{
		//qDebug() << "Delete complete pattern-sequence redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->clearThePatternSequenceVector( __pFilename );
	}
private:
	QString __pFilename;
};

/** \ingroup docGUI*/
class SE_deletePatternFromListAction : public QUndoCommand
{
public:
	SE_deletePatternFromListAction( const QString& sPatternFilename,
									const QString& sSequenceFilename,
									int nPatternPosition ){
		setText( QObject::tr( "Delete pattern from list" ) );
		m_sPatternFilename =  sPatternFilename;
		m_sSequenceFilename = sSequenceFilename;
		m_nPatternPosition = nPatternPosition;
	}
	virtual void undo() {
		HydrogenApp* h2app = HydrogenApp::get_instance();
		H2Core::CoreActionController::openPattern( m_sPatternFilename,
																		  m_nPatternPosition );
		h2app->getSongEditorPanel()->restoreGroupVector( m_sSequenceFilename );
	}

	virtual void redo() {
		H2Core::CoreActionController::removePattern( m_nPatternPosition );
	}
private:
	QString m_sPatternFilename;
	QString m_sSequenceFilename;
	int m_nPatternPosition;
};

/** \ingroup docGUI*/
class SE_modifyPatternPropertiesAction : public QUndoCommand
{
public:
	SE_modifyPatternPropertiesAction( const int nOldVersion,
									  const QString& oldPatternName,
									  const QString& sOldAuthor,
									  const QString& oldPatternInfo,
									  const H2Core::License& oldLicense,
									  const QString& oldPatternCategory,
									  const int nNewVersion,
									  const QString& newPatternName,
									  const QString sNewAuthor,
									  const QString& newPatternInfo,
									  const H2Core::License& newLicense,
									  const QString& newPatternCategory,
									  int patternNr ){
		setText( QObject::tr( "Modify pattern properties" ) );
		m_nOldVersion =  nOldVersion;
		__oldPatternName =  oldPatternName;
		m_sOldAuthor = sOldAuthor;
		__oldPatternInfo = oldPatternInfo;
		m_oldLicense = oldLicense;
		__oldPatternCategory = oldPatternCategory;
		m_nNewVersion =  nNewVersion;
		__newPatternName = newPatternName;
		m_sNewAuthor = sNewAuthor;
		__newPatternInfo = newPatternInfo;
		m_newLicense = newLicense;
		__newPatternCategory = newPatternCategory;
		__patternNr = patternNr;
	}
	virtual void undo()
	{
		//qDebug() << "Modify pattern properties undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()
			->revertPatternPropertiesDialogSettings(
				m_nOldVersion, __oldPatternName, m_sOldAuthor, __oldPatternInfo,
				m_oldLicense, __oldPatternCategory, __patternNr );
	}

	virtual void redo()
	{
		//qDebug() << "Modify pattern properties redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()
			->acceptPatternPropertiesDialogSettings(
				m_nNewVersion, __newPatternName, m_sNewAuthor, __newPatternInfo,
				m_newLicense, __newPatternCategory, __patternNr );
	}
private:
		int m_nOldVersion;
	QString __oldPatternName;
		QString m_sOldAuthor;
	QString __oldPatternInfo;
		H2Core::License m_oldLicense;
	QString __oldPatternCategory;

		int m_nNewVersion;
	QString __newPatternName;
		QString m_sNewAuthor;
	QString __newPatternInfo;
		H2Core::License m_newLicense;
	QString __newPatternCategory;
	int __patternNr;
};


/** \ingroup docGUI*/
class SE_duplicatePatternAction : public QUndoCommand
{
public:
	SE_duplicatePatternAction( const QString& patternFilename, int patternPosition ){
		setText( QObject::tr( "Duplicate pattern" ) );
		m_sPatternFilename = patternFilename;
		m_nPatternPosition = patternPosition;
	}
	virtual void undo() {
		H2Core::CoreActionController::removePattern( m_nPatternPosition );
	}

	virtual void redo() {
		H2Core::CoreActionController::openPattern( m_sPatternFilename, m_nPatternPosition );
	}
private:
	QString m_sPatternFilename;
	int m_nPatternPosition;
};

/** \ingroup docGUI*/
class SE_insertPatternAction : public QUndoCommand
{
public:
	SE_insertPatternAction( int patternPosition,
							std::shared_ptr<H2Core::Pattern> pPattern )
	{
		setText( QObject::tr( "Add pattern" ) );
		m_nPatternPosition = patternPosition;
		m_pNewPattern =  pPattern;
	}
	~SE_insertPatternAction() {
	}
	virtual void undo() {
		H2Core::CoreActionController::removePattern( m_nPatternPosition );
	}
	virtual void redo() {
		H2Core::CoreActionController::setPattern(
			std::make_shared<H2Core::Pattern>( m_pNewPattern ),
																				 m_nPatternPosition );
	}
private:
	std::shared_ptr<H2Core::Pattern> m_pNewPattern;

	int m_nPatternPosition;
};

/** \ingroup docGUI*/
class SE_loadPatternAction : public QUndoCommand
{
public:
	SE_loadPatternAction( const QString& sPatternName,
						  const QString& sOldPatternName,
						  const QString& sSequenceFilename, int nPatternPosition,
						  bool bDragFromList){
		setText( QObject::tr( "Load/drag pattern" ) );
		m_sPatternName =  sPatternName;
		m_sOldPatternName = sOldPatternName;
		m_sSequenceFilename = sSequenceFilename;
		m_nPatternPosition = nPatternPosition;
		m_bDragFromList = bDragFromList;
	}
	virtual void undo() {
		if( m_bDragFromList ){
			H2Core::CoreActionController::removePattern( m_nPatternPosition );
		} else {
			H2Core::CoreActionController::removePattern( m_nPatternPosition );
			H2Core::CoreActionController::openPattern( m_sOldPatternName, m_nPatternPosition );
		}
		HydrogenApp::get_instance()->getSongEditorPanel()
			->restoreGroupVector( m_sSequenceFilename );
	}

	virtual void redo() {
		if( ! m_bDragFromList ){
			H2Core::CoreActionController::removePattern( m_nPatternPosition );
		}
		H2Core::CoreActionController::openPattern( m_sPatternName, m_nPatternPosition );
	}
private:
	QString m_sPatternName;
	QString m_sOldPatternName;
	QString m_sSequenceFilename;
	int m_nPatternPosition;
	bool m_bDragFromList;
};


/** \ingroup docGUI*/
class SE_fillRangePatternAction : public QUndoCommand
{
public:
	SE_fillRangePatternAction( FillRange* pRange, int nPattern ){
		setText( QObject::tr( "Fill/remove range of pattern" ) );
		__pRange = pRange;
		__from = pRange->fromVal;
		__to = pRange->toVal;
		__bInsert = pRange->bInsert;
		__nPattern = nPattern;
	}
	virtual void undo()
	{
		//qDebug() << "fill/remove range of undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		bool insert;
		if( __bInsert ){
			insert = false;
		}else
		{
			insert = true;
		}
		__pRange->bInsert = insert;
		__pRange->fromVal = __from;
		__pRange->toVal = __to;
		h2app->getSongEditorPanel()->getSongEditorPatternList()->fillRangeWithPattern( __pRange, __nPattern);
	}

	virtual void redo()
	{
		//qDebug() <<  "fill/remove range of redo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		__pRange->bInsert = __bInsert;
		__pRange->fromVal = __from;
		__pRange->toVal = __to;
		h2app->getSongEditorPanel()->getSongEditorPatternList()->fillRangeWithPattern( __pRange, __nPattern);
	}
private:
	FillRange* __pRange;
	int __from;
	int __to;
	bool __bInsert;
	int __nPattern;
};


/** \ingroup docGUI*/
class SE_modifyPatternCellsAction : public QUndoCommand
{
public:
	SE_modifyPatternCellsAction( const std::vector< QPoint >& addCells,
								 const std::vector< QPoint >& deleteCells,
								 const std::vector< QPoint >& mergeCells,
								 const QString& sText ) {
		setText( sText );
		m_addCells = addCells;
		m_deleteCells = deleteCells;
		m_mergeCells = mergeCells;
	}
	virtual void redo()
	{
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditor()
			->modifyPatternCellsAction( m_addCells, m_deleteCells, m_mergeCells );
	}
	virtual void undo()
	{
		std::vector< QPoint > selectCells;
		HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditor()
			->modifyPatternCellsAction( m_deleteCells, m_addCells, selectCells );
	}
private:
	std::vector< QPoint > m_addCells;
	std::vector< QPoint > m_deleteCells;
	std::vector< QPoint > m_mergeCells;
};

// ~song editor commands
//=====================================================================================================================================
//time line commands

/** \ingroup docGUI*/
class SE_editTimelineAction : public QUndoCommand
{
public:
	SE_editTimelineAction( int nOldColumn, int nNewColumn, float fOldBpm, float fNewBpm, bool bTempoMarkerPresent ){
		setText( QObject::tr( "Edit tempo marker" ) );
		m_nOldColumn = nOldColumn;
		m_nNewColumn = nNewColumn;
		m_fOldBpm = fOldBpm;
		m_fNewBpm = fNewBpm;
		m_bTempoMarkerPresent = bTempoMarkerPresent;
	}
	virtual void undo() {
		if ( m_bTempoMarkerPresent ){
			if ( m_nOldColumn != m_nNewColumn ) {
				H2Core::CoreActionController::deleteTempoMarker( m_nNewColumn );
			}
			H2Core::CoreActionController::addTempoMarker( m_nOldColumn, m_fOldBpm );
		} else {
			H2Core::CoreActionController::deleteTempoMarker( m_nNewColumn );
		}
		HydrogenApp::get_instance()->getSongEditorPanel()->
			getSongEditorPositionRuler()->updateEditor();
	}

	virtual void redo() {
		if ( m_nOldColumn != m_nNewColumn ) {
			H2Core::CoreActionController::deleteTempoMarker( m_nOldColumn );
		}
		H2Core::CoreActionController::addTempoMarker( m_nNewColumn, m_fNewBpm );
		HydrogenApp::get_instance()->getSongEditorPanel()->
			getSongEditorPositionRuler()->updateEditor();
	}
private:
	int m_nOldColumn;
	int m_nNewColumn;
	float m_fOldBpm;
	float m_fNewBpm;
	bool m_bTempoMarkerPresent;
};

/** \ingroup docGUI*/
class SE_deleteTimelineAction : public QUndoCommand
{
public:
	SE_deleteTimelineAction( int nColumn, float fBpm ){
		setText( QObject::tr( "Delete tempo marker" ) );
		m_nColumn = nColumn;
		m_fBpm = fBpm;
	}
	virtual void undo() {
		H2Core::CoreActionController::addTempoMarker( m_nColumn, m_fBpm );
		HydrogenApp::get_instance()->getSongEditorPanel()->
			getSongEditorPositionRuler()->updateEditor();
	}

	virtual void redo() {
		H2Core::CoreActionController::deleteTempoMarker( m_nColumn );
		HydrogenApp::get_instance()->getSongEditorPanel()->
			getSongEditorPositionRuler()->updateEditor();
	}
private:
	int m_nColumn;
	float m_fBpm;
};

/** \ingroup docGUI*/
class SE_editTagAction : public QUndoCommand
{
public:
	SE_editTagAction( const QString& sText, const QString& sOldText, int nPosition ){
		setText( QObject::tr( "Edit timeline tag" ) );
		m_sText = sText;
		m_sOldText = sOldText;
		m_nPosition = nPosition;

	}
	virtual void undo() {
		if ( ! m_sOldText.isEmpty() ){
			H2Core::CoreActionController::addTag( m_nPosition, m_sOldText );
		} else {
			H2Core::CoreActionController::deleteTag( m_nPosition );
		}
	}

	virtual void redo() {
		if ( ! m_sText.isEmpty() ){
			H2Core::CoreActionController::addTag( m_nPosition, m_sText );
		} else {
			H2Core::CoreActionController::deleteTag( m_nPosition );
		}
	}
private:
	QString m_sText;
	QString m_sOldText;
	int m_nPosition;
};

// ~time line commands
//=====================================================================================================================================
//pattern editor commands

/** \ingroup docGUI*/
class SE_addOrRemoveNoteAction : public QUndoCommand
{
public:
	SE_addOrRemoveNoteAction( int nColumn,
							  int nInstrumentId,
							  const QString& sType,
							  int nPatternNumber,
							  int nOldLength,
							  float fOldVelocity,
							  float fOldPan,
							  float fOldLeadLag,
							  int nOldKey,
							  int nOldOctave,
							  float fOldProbability,
							  bool bIsDelete,
							  bool bIsNoteOff,
							  bool bIsMappedToDrumkit,
							  Editor::Action addNoteAction =
							  Editor::Action::None
 ){

		if ( bIsDelete ){
			setText( QString( "%1 [column: %2, id: %3, type: %4, pattern: %5]" )
					 .arg( QObject::tr( "Delete note" ) ).arg( nColumn ).
					 arg( nInstrumentId ).arg( sType ).arg( nPatternNumber ) );
		} else {
			setText( QString( "%1 [column: %2, id: %3, type: %4, pattern: %5]" )
					 .arg( QObject::tr( "Add note" ) ).arg( nColumn ).
					 arg( nInstrumentId ).arg( sType ).arg( nPatternNumber ) );
		}
		m_nColumn = nColumn;
		m_nInstrumentId = nInstrumentId;
		m_sType = sType;
		m_nPatternNumber = nPatternNumber;
		m_nOldLength = nOldLength;
		m_fOldVelocity = fOldVelocity;
		m_fOldPan = fOldPan;
		m_fOldLeadLag = fOldLeadLag;
		m_nOldKey = nOldKey;
		m_nOldOctave = nOldOctave;
		m_fOldProbability = fOldProbability;
		m_bIsDelete = bIsDelete;
		m_bIsNoteOff = bIsNoteOff;
		m_bIsMappedToDrumkit = bIsMappedToDrumkit;
		m_addNoteAction = addNoteAction;
	}
	virtual void undo() {
		PatternEditor::addOrRemoveNoteAction( m_nColumn,
											  m_nInstrumentId,
											  m_sType,
											  m_nPatternNumber,
											  m_nOldLength,
											  m_fOldVelocity,
											  m_fOldPan,
											  m_fOldLeadLag,
											  m_nOldKey,
											  m_nOldOctave,
											  m_fOldProbability,
											  ! m_bIsDelete,
											  m_bIsNoteOff,
											  m_bIsMappedToDrumkit,
											  m_addNoteAction );
	}
	virtual void redo() {
		PatternEditor::addOrRemoveNoteAction( m_nColumn,
											  m_nInstrumentId,
											  m_sType,
											  m_nPatternNumber,
											  m_nOldLength,
											  m_fOldVelocity,
											  m_fOldPan,
											  m_fOldLeadLag,
											  m_nOldKey,
											  m_nOldOctave,
											  m_fOldProbability,
											  m_bIsDelete,
											  m_bIsNoteOff,
											  m_bIsMappedToDrumkit,
											  m_addNoteAction );
		// Only on the first redo the corresponding action is triggered.
		m_addNoteAction = Editor::Action::None;
	}
private:
	int m_nColumn;
	int m_nInstrumentId;
	QString m_sType;
	int m_nPatternNumber;
	int m_nOldLength;
	float m_fOldVelocity;
	float m_fOldPan;
	float m_fOldLeadLag;
	int m_nOldKey;
	int m_nOldOctave;
	float m_fOldProbability;
	bool m_bIsDelete;
	bool m_bIsNoteOff;
	bool m_bIsMappedToDrumkit;
	Editor::Action m_addNoteAction;
};

// Deselect some notes and overwrite them
/** \ingroup docGUI*/
class SE_patternSizeChangedAction : public QUndoCommand
{
public:
	SE_patternSizeChangedAction( int nNewLength, int nOldLength,
								 double fNewDenominator, double fOldDenominator,
								 int nSelectedPatternNumber ) {
		setText( QObject::tr( "Altering the length of the current pattern" ) );
		m_nNewLength = nNewLength;
		m_nOldLength = nOldLength;
		m_fNewDenominator = fNewDenominator;
		m_fOldDenominator = fOldDenominator;
		m_nSelectedPatternNumber = nSelectedPatternNumber;
	}

	virtual void undo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()
			->patternSizeChangedAction( m_nOldLength, m_fOldDenominator,
										m_nSelectedPatternNumber );
	}

	virtual void redo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()
			->patternSizeChangedAction( m_nNewLength, m_fNewDenominator,
										m_nSelectedPatternNumber );
	}

private:
	int m_nNewLength;
	int m_nOldLength;
	double m_fNewDenominator;
	double m_fOldDenominator;
	int m_nSelectedPatternNumber;
};

// Deselect some notes and overwrite them
/** \ingroup docGUI*/
class SE_deselectAndOverwriteNotesAction : public QUndoCommand
{
public:
	SE_deselectAndOverwriteNotesAction(
		const std::vector< std::shared_ptr<H2Core::Note> >& selected,
		const std::vector< std::shared_ptr<H2Core::Note> >& overwritten )
		{
		setText( QObject::tr( "Overwrite %1 notes" ).arg( overwritten.size() ) );
		for ( auto ppNote : selected ) {
			if ( ppNote != nullptr ) {
				m_selected.push_back( std::make_shared<H2Core::Note>( ppNote ) );
			}
		}
		for ( auto ppNote : overwritten ) {
			if ( ppNote != nullptr ) {
				m_overwritten.push_back( std::make_shared<H2Core::Note>( ppNote ) );
			}
		}
	}

	~SE_deselectAndOverwriteNotesAction() {
	}

	virtual void undo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()
			->undoDeselectAndOverwriteNotes( m_selected, m_overwritten );
	}

	virtual void redo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()
			->deselectAndOverwriteNotes( m_selected, m_overwritten );
	}

private:
	std::vector< std::shared_ptr<H2Core::Note> > m_selected;
	std::vector< std::shared_ptr<H2Core::Note> > m_overwritten;
};

/** \ingroup docGUI*/
class SE_editNotePropertiesAction : public QUndoCommand
{
public:
	SE_editNotePropertiesAction( const PatternEditor::Property& property,
								 int nPatternNumber,
								 int nColumn,
								 int nInstrumentId,
								 int nOldInstrumentId,
								 const QString& sType,
								 const QString& sOldType,
								 float fVelocity,
								 float fOldVelocity,
								 float fPan,
								 float fOldPan,
								 float fLeadLag,
								 float fOldLeadLag,
								 float fProbability,
								 float fOldProbability,
								 int nLength,
								 int nOldLength,
								 int nKey,
								 int nOldKey,
								 int nOctave,
								 int nOldOctave ) :
		m_property( property ),
		m_nPatternNumber( nPatternNumber ),
		m_nColumn( nColumn ),
		m_nInstrumentId( nInstrumentId ),
		m_nOldInstrumentId( nOldInstrumentId ),
		m_sType( sType ),
		m_sOldType( sOldType ),
		m_fVelocity( fVelocity ),
		m_fOldVelocity( fOldVelocity ),
		m_fPan( fPan ),
		m_fOldPan( fOldPan ),
		m_fLeadLag( fLeadLag ),
		m_fOldLeadLag( fOldLeadLag ),
		m_fProbability( fProbability ),
		m_fOldProbability( fOldProbability ),
		m_nLength( nLength ),
		m_nOldLength( nOldLength ),
		m_nKey( nKey ),
		m_nOldKey( nOldKey ),
		m_nOctaveKey( nOctave ),
		m_nOldOctaveKey( nOldOctave ) {

		setText( QObject::tr( "Edit note property %1" )
				 .arg( PatternEditor::propertyToQString( property ) ) );
	}
	virtual void undo() {
		PatternEditor::editNotePropertiesAction( m_property,
												 m_nPatternNumber,
												 m_nColumn,
												 m_nInstrumentId,
												 m_nOldInstrumentId,
												 m_sType,
												 m_sOldType,
												 m_fOldVelocity,
												 m_fOldPan,
												 m_fOldLeadLag,
												 m_fOldProbability,
												 m_nOldLength,
												 m_nOldKey,
												 m_nKey,
												 m_nOldOctaveKey,
												 m_nOctaveKey );
	}
	virtual void redo() {
		PatternEditor::editNotePropertiesAction( m_property,
												 m_nPatternNumber,
												 m_nColumn,
												 m_nOldInstrumentId,
												 m_nInstrumentId,
												 m_sOldType,
												 m_sType,
												 m_fVelocity,
												 m_fPan,
												 m_fLeadLag,
												 m_fProbability,
												 m_nLength,
												 m_nKey,
												 m_nOldKey,
												 m_nOctaveKey,
												 m_nOldOctaveKey );
	}

private:
		PatternEditor::Property m_property;
		int m_nPatternNumber;
		int m_nColumn;
		/** Row selected in #DrumPatternEditor the moment the action was
		 * created. */
		int m_nInstrumentId;
		int m_nOldInstrumentId;
		QString m_sType;
		QString m_sOldType;
		float m_fVelocity;
		float m_fOldVelocity;
		float m_fPan;
		float m_fOldPan;
		float m_fLeadLag;
		float m_fOldLeadLag;
		float m_fProbability;
		float m_fOldProbability;
		int m_nLength;
		int m_nOldLength;
		int m_nKey;
		int m_nOldKey;
		int m_nOctaveKey;
		int m_nOldOctaveKey;
};

/** \ingroup docGUI*/
class SE_moveInstrumentAction : public QUndoCommand
{
public:
	SE_moveInstrumentAction(  int nSourceIndex, int nTargetIndex  ){
		const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		setText( QString( "%1 [%2] -> [%3]" )
				 .arg( pCommonStrings->getActionMoveInstrument() )
				 .arg( nSourceIndex ) .arg( nTargetIndex ) );
		m_nSourceIndex = nSourceIndex;
		m_nTargetIndex = nTargetIndex;
	}
	virtual void undo() {
		H2Core::CoreActionController::moveInstrument(
			m_nTargetIndex, m_nSourceIndex );
	}
	virtual void redo() {
		H2Core::CoreActionController::moveInstrument(
			m_nSourceIndex, m_nTargetIndex );
	}
private:
	int m_nSourceIndex;
	int m_nTargetIndex;
};

// ~pattern editor commands

class SE_switchDrumkitAction : public QUndoCommand {
	public:
		/** Switching entire drumkits is a rather clean way to accomplish a
		 * number of different task. To still display the proper text in the
		 * undo history, this enum is used to indicate the intention calling
		 * this undo action. */
		enum class Type {
			/** Actual switching of two fully-fledge kits */
			SwitchDrumkit = 0,
			/** Replace the current kit with an empty one */
			NewDrumkit = 1,
			/** Editing properties of the current song's kit. */
			EditProperties = 2
		};

		SE_switchDrumkitAction( std::shared_ptr<H2Core::Drumkit> pNewDrumkit,
								std::shared_ptr<H2Core::Drumkit> pOldDrumkit,
								const Type& type,
								const QString& sComponentName = "" ) :
			m_pNewDrumkit( pNewDrumkit ),
			m_pOldDrumkit( pOldDrumkit )
		{
			const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
				switch ( type ) {
			case Type::SwitchDrumkit:
				setText( QString( "%1: [%2] -> [%3]" )
						 .arg( pCommonStrings->getActionSwitchDrumkit() )
						 .arg( pOldDrumkit != nullptr ? pOldDrumkit->getName() : "nullptr" )
						 .arg( pNewDrumkit != nullptr ? pNewDrumkit->getName() : "nullptr" ) );
				break;
			case Type::NewDrumkit:
				setText( pCommonStrings->getActionNewDrumkit() );
				break;
			case Type::EditProperties:
				setText( pCommonStrings->getActionEditDrumkitProperties() );
				break;
			}
		}
		virtual void undo() {
			SoundLibraryPanel::switchDrumkit( m_pOldDrumkit, m_pNewDrumkit );
		}
		virtual void redo() {
			SoundLibraryPanel::switchDrumkit( m_pNewDrumkit, m_pOldDrumkit );
		}

	private:
		std::shared_ptr<H2Core::Drumkit> m_pNewDrumkit;
		std::shared_ptr<H2Core::Drumkit> m_pOldDrumkit;
};

/** \ingroup docGUI*/
class SE_addInstrumentAction : public QUndoCommand {
	public:
		enum class Type {
			/** Create and add a new instrument */
			AddEmptyInstrument = 0,
			/** Add an instrument from another drumkit. */
			DropInstrument = 1
		};
		SE_addInstrumentAction( std::shared_ptr<H2Core::Instrument> pInstrument,
								int nIndex, Type type )
		: m_pInstrument( pInstrument )
		, m_nIndex( nIndex ) {
			const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
				switch ( type ) {
			case Type::AddEmptyInstrument:
				setText( pCommonStrings->getActionAddInstrument() );
				break;
			case Type::DropInstrument:
				setText( QString( "%1 [%2]" )
						 .arg( pCommonStrings->getActionDropInstrument() )
						 .arg( pInstrument != nullptr ? pInstrument->getName() :
							   "nullptr" ) );
				break;
			default:
				___ERRORLOG( QString( "Unknown type [%1]" )
							 .arg( static_cast<int>(type) ) );
			}
		}

		virtual void undo() {
			H2Core::CoreActionController::removeInstrument( m_pInstrument );
		}
		virtual void redo() {
			H2Core::CoreActionController::addInstrument( m_pInstrument, m_nIndex );
		}
	private:
		std::shared_ptr<H2Core::Instrument> m_pInstrument;
		/** `-1` indicates that the instrument will be appended. */
		int m_nIndex;
};

/** \ingroup docGUI*/
class SE_deleteInstrumentAction : public QUndoCommand
{
public:
	SE_deleteInstrumentAction( std::shared_ptr<H2Core::Instrument> pInstrument,
							   int nIndex )
		: m_pInstrument( pInstrument )
		, m_nIndex( nIndex ) {
		const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		setText( QString( "%1 [%2]" )
				 .arg( pCommonStrings->getActionDeleteInstrument() )
				 .arg( pInstrument->getName() ) );
	}
	~SE_deleteInstrumentAction(){}

	virtual void undo() {
		H2Core::CoreActionController::addInstrument( m_pInstrument, m_nIndex );
	}
	virtual void redo() {
		H2Core::CoreActionController::removeInstrument( m_pInstrument );
	}
	private:
		std::shared_ptr<H2Core::Instrument> m_pInstrument;
		int m_nIndex;
};

/** Instruments are self-contained units within a #H2Core::Drumkit. Each
 * #H2Core::Note carries a shared pointer to the #H2Core::Instrument used to
 * render it. Due to the latter we have to be careful when changing members an
 * instrument, like adding/moving/removing #H2Core::InstrumentComponent or
 * #H2Core::InstrumentLayer. Instead, we just replace the instrument as a whole
 * in the drumkit. This way the one stored in the #H2Core::Note within the queue
 * of #H2Core::AudioEngine and #H2Core::Sampler is still valid. */
class SE_replaceInstrumentAction : public QUndoCommand {
	public:
		enum class Type {
			/** Replace the instrument with a copy containing an additional
			 * component */
			AddComponent = 0,
			/** Replace the instrument with a copy from which one component was
			 * removed. */
			DeleteComponent = 1,
			/** There must be at least one instrument in a drumkit. Instead of
			 * the deleting the last one, it will be replaced by an empty
			 * one. */
			DeleteLastInstrument = 2,
			/** This could definitely be done more efficiently. But compared to
			 * altering other instrument parameters, its name will most probably
			 * only change very rarely. */
			RenameInstrument = 3,
			/** At least one layer of one component was added. */
			AddLayer,
			/** At least one layer of one component was deleted. */
			DeleteLayer,
			/** At least one layer of one component was editing via the
			 * SampleEditor. */
			EditLayer
		};

		SE_replaceInstrumentAction( std::shared_ptr<H2Core::Instrument> pNew,
									std::shared_ptr<H2Core::Instrument> pOld,
									SE_replaceInstrumentAction::Type type,
									const QString& sName,
									const QString& sOldName = "" ) :
			m_pNew( pNew ),
			m_pOld( pOld )
		{
			const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
				switch ( type ) {
			case Type::AddComponent:
				setText( QString( "%1 [%2]" )
						 .arg( pCommonStrings->getActionAddComponent() )
						 .arg( sName ) );
				break;
			case Type::DeleteComponent:
				setText( QString( "%1 [%2]" )
						 .arg( pCommonStrings->getActionDeleteComponent() )
						 .arg( sName ) );
				break;
			case Type::DeleteLastInstrument:
				setText( QString( "%1 [%2]" )
						 .arg( pCommonStrings->getActionDeleteInstrument() )
						 .arg( sName ) );
				break;
			case Type::RenameInstrument:
				setText( QString( "%1 [%2] -> [%3]" )
						 .arg( pCommonStrings->getActionRenameInstrument() )
						 .arg( sOldName ).arg( sName ) );
				break;
			case Type::AddLayer:
				setText( QString( "%1 [%2]: [%3]" )
						 .arg( pCommonStrings->getActionAddInstrumentLayer() )
						 .arg( pNew != nullptr ? pNew->getName() : "nullptr" )
						 .arg( sName ) );
			case Type::DeleteLayer:
				setText( QString( "%1 [%2]: [%3]" )
						 .arg( pCommonStrings->getActionDeleteInstrumentLayer() )
						 .arg( pNew != nullptr ? pNew->getName() : "nullptr" )
						 .arg( sName ) );
			case Type::EditLayer:
				setText( QString( "%1 [%2]: [%3]" )
						 .arg( pCommonStrings->getActionEditInstrumentLayer() )
						 .arg( pNew != nullptr ? pNew->getName() : "nullptr" )
						 .arg( sName ) );
				break;
			default:
				___ERRORLOG( QString( "Unknown type [%1]" )
							 .arg( static_cast<int>(type) ) );
			}
		}
		virtual void undo() {
			H2Core::CoreActionController::replaceInstrument( m_pOld, m_pNew );
		}
		virtual void redo() {
			H2Core::CoreActionController::replaceInstrument( m_pNew, m_pOld );
		}

	private:
		std::shared_ptr<H2Core::Instrument> m_pNew;
		std::shared_ptr<H2Core::Instrument> m_pOld;
};

class SE_renameComponentAction : public QUndoCommand {
	public:
		SE_renameComponentAction( const QString& sNewName,
								  const QString& sOldName,
								  int nComponentId ) :
			m_sNewName( sNewName ),
			m_sOldName( sOldName ),
			m_nComponentId( nComponentId ) {
			const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
				setText( QString( "%1: [%2] -> [%3]" )
					 .arg( pCommonStrings->getActionRenameComponent() )
					 .arg( sOldName ).arg( sNewName ) );
		}
		virtual void undo() {
			HydrogenApp::get_instance()->getInstrumentRack()->
				getInstrumentEditorPanel()->getComponentsEditor()->
				renameComponent( m_nComponentId, m_sOldName );
		}
		virtual void redo() {
			HydrogenApp::get_instance()->getInstrumentRack()->
				getInstrumentEditorPanel()->getComponentsEditor()->
				renameComponent( m_nComponentId, m_sNewName );
		}
	private:
		QString m_sNewName;
		QString m_sOldName;
		int m_nComponentId;
};

/** \ingroup docGUI*/
class SE_setInstrumentTypeAction : public QUndoCommand
{
public:
	SE_setInstrumentTypeAction( int nInstrumentId,
								const H2Core::DrumkitMap::Type& sNewType,
								const H2Core::DrumkitMap::Type& sOldType,
								const QString& sInstrumentName )
		: m_nInstrumentId( nInstrumentId )
		, m_sNewType( sNewType )
		, m_sOldType( sOldType ) {
		const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
		setText( QString( "%1 [%2]: [%3] -> [%4]" )
				 .arg( pCommonStrings->getActionSetInstrumentType() )
				 .arg( sInstrumentName ).arg( sOldType ).arg( sNewType ) );
	}
	~SE_setInstrumentTypeAction(){}

	virtual void undo() {
		H2Core::CoreActionController::setInstrumentType(
			m_nInstrumentId, m_sOldType );
	}
	virtual void redo() {
		H2Core::CoreActionController::setInstrumentType(
			m_nInstrumentId, m_sNewType );
	}
	private:
		int m_nInstrumentId;
		H2Core::DrumkitMap::Type m_sNewType;
		H2Core::DrumkitMap::Type m_sOldType;
};

/** \ingroup docGUI*/
class SE_automationPathAddPointAction : public QUndoCommand
{
public:
	SE_automationPathAddPointAction( H2Core::AutomationPath *path, float x, float y)
	{
		setText( QObject::tr( "Add point" ) );
		__path = path;
		__x = x;
		__y = y;
	}

	virtual void undo()
	{
		__path->remove_point( __x );

		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getAutomationPathView()->update();
	}

	virtual void redo()
	{
		__path->add_point( __x, __y );

		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getAutomationPathView()->update();
	}
private:
	H2Core::AutomationPath* __path;
	float __x;
	float __y;
};


/** \ingroup docGUI*/
class SE_automationPathRemovePointAction : public QUndoCommand
{
public:
	SE_automationPathRemovePointAction( H2Core::AutomationPath *path, float x, float y)
	{
		setText( QObject::tr( "Remove point" ) );
		__path = path;
		__x = x;
		__y = y;
	}

	virtual void redo()
	{
		__path->remove_point( __x );

		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getAutomationPathView()->update();
	}

	virtual void undo()
	{
		__path->add_point( __x, __y );

		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getAutomationPathView()->update();
	}
private:
	H2Core::AutomationPath* __path;
	float __x;
	float __y;
};


/** \ingroup docGUI*/
class SE_automationPathMovePointAction : public QUndoCommand
{
public:
	SE_automationPathMovePointAction( H2Core::AutomationPath *path, float ox, float oy, float tx, float ty)
	{
		setText( QObject::tr( "Move point" ) );
		__path = path;
		__ox = ox;
		__oy = oy;
		__tx = tx;
		__ty = ty;
	}

	virtual void redo()
	{
		__path->remove_point( __ox );
		__path->add_point( __tx, __ty );

		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getAutomationPathView()->update();
	}

	virtual void undo()
	{
		__path->remove_point( __tx );
		__path->add_point( __ox, __oy );

		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getAutomationPathView()->update();
	}
private:
	H2Core::AutomationPath* __path;
	float __ox;
	float __oy;
	float __tx;
	float __ty;
};

////////////////////////////////////////////////////////////////////////////////
// Playlist related actions.
class SE_addEntryToPlaylistAction : public QUndoCommand {
public:
	SE_addEntryToPlaylistAction( std::shared_ptr<H2Core::PlaylistEntry> pEntry,
								 int nIndex = -1 )
		: m_pEntry( pEntry )
		, m_nIndex( nIndex ){
		setText( QObject::tr( "Add song to playlist" ) );
	}

	virtual void redo() {
		H2Core::CoreActionController::addToPlaylist( m_pEntry, m_nIndex );
	}

	virtual void undo() {
		H2Core::CoreActionController::removeFromPlaylist( m_pEntry, m_nIndex );
	}
private:
	std::shared_ptr<H2Core::PlaylistEntry> m_pEntry;
	int m_nIndex;
};

class SE_removeEntryFromPlaylistAction : public QUndoCommand {
public:
	SE_removeEntryFromPlaylistAction( std::shared_ptr<H2Core::PlaylistEntry> pEntry,
									  int nIndex = -1 )
		: m_pEntry( pEntry )
		, m_nIndex( nIndex ) {
		setText( QObject::tr( "Remove song to playlist" ) );
	}

	virtual void redo() {
		H2Core::CoreActionController::removeFromPlaylist( m_pEntry, m_nIndex );
	}

	virtual void undo() {
		H2Core::CoreActionController::addToPlaylist( m_pEntry, m_nIndex );
	}
private:
	std::shared_ptr<H2Core::PlaylistEntry> m_pEntry;
	int m_nIndex;
};

class SE_replacePlaylistAction : public QUndoCommand {
public:
	SE_replacePlaylistAction( std::shared_ptr<H2Core::Playlist> pPlaylist )
		: m_pNewPlaylist( pPlaylist ) {
		setText( QObject::tr( "Replace playlist" ) );

		m_pOldPlaylist = H2Core::Hydrogen::get_instance()->getPlaylist();
	}

	virtual void redo() {
		H2Core::CoreActionController::setPlaylist( m_pNewPlaylist );
		HydrogenApp::get_instance()->getMainForm()->
			setPreviousAutoSavePlaylistFile( "" );
	}

	virtual void undo() {
		H2Core::CoreActionController::setPlaylist( m_pOldPlaylist );
		HydrogenApp::get_instance()->getMainForm()->
			setPreviousAutoSavePlaylistFile( "" );
	}
private:
	std::shared_ptr<H2Core::Playlist> m_pNewPlaylist;
	std::shared_ptr<H2Core::Playlist> m_pOldPlaylist;
};



#endif // UNDOACTIONS_H
