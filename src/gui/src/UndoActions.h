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

#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/AutomationPath.h>
#include <core/Helpers/Filesystem.h>

#include "HydrogenApp.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SongEditor/PatternFillDialog.h"

#include "PatternEditor/NotePropertiesRuler.h"
#include "PatternEditor/DrumPatternEditor.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "PatternEditor/NotePropertiesRuler.h"
#include "Widgets/AutomationPathView.h"


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
		H2Core::Hydrogen::get_instance()->getCoreActionController()->toggleGridCell( m_nColumn, m_nRow );
	}
	virtual void redo() {
		H2Core::Hydrogen::get_instance()->getCoreActionController()->toggleGridCell( m_nColumn, m_nRow );
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
	explicit SE_deletePatternSequenceAction(  QString pFilename ){
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
	SE_deletePatternFromListAction( QString sPatternFilename,
									QString sSequenceFilename,
									int nPatternPosition ){
		setText( QObject::tr( "Delete pattern from list" ) );
		m_sPatternFilename =  sPatternFilename;
		m_sSequenceFilename = sSequenceFilename;
		m_nPatternPosition = nPatternPosition;
	}
	virtual void undo() {
		HydrogenApp* h2app = HydrogenApp::get_instance();
		H2Core::Hydrogen::get_instance()->getCoreActionController()->openPattern( m_sPatternFilename,
																		  m_nPatternPosition );
		h2app->getSongEditorPanel()->restoreGroupVector( m_sSequenceFilename );
	}

	virtual void redo() {
		H2Core::Hydrogen::get_instance()->getCoreActionController()->removePattern( m_nPatternPosition );
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
	SE_modifyPatternPropertiesAction( QString oldPatternName ,QString oldPatternInfo, QString oldPatternCategory, QString newPatternName , QString newPatternInfo, QString newPatternCategory, int patternNr ){
		setText( QObject::tr( "Modify pattern properties" ) );
		__oldPatternName =  oldPatternName;
		__oldPatternCategory = oldPatternCategory;
		__oldPatternInfo = oldPatternInfo;
		__newPatternName = newPatternName;
		__newPatternInfo = newPatternInfo;
		__newPatternCategory = newPatternCategory;
		__patternNr = patternNr;
	}
	virtual void undo()
	{
		//qDebug() << "Modify pattern properties undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->revertPatternPropertiesDialogSettings( __oldPatternName, __oldPatternInfo, __oldPatternCategory, __patternNr );
	}

	virtual void redo()
	{
		//qDebug() << "Modify pattern properties redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->acceptPatternPropertiesDialogSettings( __newPatternName, __newPatternInfo, __newPatternCategory, __patternNr );
	}
private:
	QString __oldPatternName;
	QString __oldPatternInfo;
	QString __oldPatternCategory;

	QString __newPatternName;
	QString __newPatternInfo;
	QString __newPatternCategory;
	int __patternNr;
};


/** \ingroup docGUI*/
class SE_duplicatePatternAction : public QUndoCommand
{
public:
	SE_duplicatePatternAction( QString patternFilename, int patternPosition ){
		setText( QObject::tr( "Duplicate pattern" ) );
		m_sPatternFilename = patternFilename;
		m_nPatternPosition = patternPosition;
	}
	virtual void undo() {
		H2Core::Hydrogen::get_instance()->getCoreActionController()->removePattern( m_nPatternPosition );
	}

	virtual void redo() {
		H2Core::Hydrogen::get_instance()->getCoreActionController()->openPattern( m_sPatternFilename, m_nPatternPosition );
	}
private:
	QString m_sPatternFilename;
	int m_nPatternPosition;
};

/** \ingroup docGUI*/
class SE_insertPatternAction : public QUndoCommand
{
public:
	SE_insertPatternAction( int patternPosition, H2Core::Pattern* pPattern )
	{
		setText( QObject::tr( "Add pattern" ) );
		m_nPatternPosition = patternPosition;
		m_pNewPattern =  pPattern;
	}
	~SE_insertPatternAction()
	{
		delete m_pNewPattern;
	}
	virtual void undo() {
		H2Core::Hydrogen::get_instance()->getCoreActionController()->removePattern( m_nPatternPosition );
	}
	virtual void redo() {
		H2Core::Hydrogen::get_instance()->getCoreActionController()->setPattern( new H2Core::Pattern( m_pNewPattern ),
																				 m_nPatternPosition );
	}
private:
	H2Core::Pattern* m_pNewPattern;

	int m_nPatternPosition;
};

/** \ingroup docGUI*/
class SE_loadPatternAction : public QUndoCommand
{
public:
	SE_loadPatternAction( QString sPatternName, QString sOldPatternName,
						  QString sSequenceFilename, int nPatternPosition,
						  bool bDragFromList){
		setText( QObject::tr( "Load/drag pattern" ) );
		m_sPatternName =  sPatternName;
		m_sOldPatternName = sOldPatternName;
		m_sSequenceFilename = sSequenceFilename;
		m_nPatternPosition = nPatternPosition;
		m_bDragFromList = bDragFromList;
	}
	virtual void undo() {
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		HydrogenApp* h2app = HydrogenApp::get_instance();
		if( m_bDragFromList ){
			pCoreActionController->removePattern( m_nPatternPosition );
		} else {
			pCoreActionController->removePattern( m_nPatternPosition );
			pCoreActionController->openPattern( m_sOldPatternName, m_nPatternPosition );
		}
		h2app->getSongEditorPanel()->restoreGroupVector( m_sSequenceFilename );
	}

	virtual void redo() {
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		if( ! m_bDragFromList ){
			pCoreActionController->removePattern( m_nPatternPosition );
		}
		pCoreActionController->openPattern( m_sPatternName, m_nPatternPosition );
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
	SE_modifyPatternCellsAction( std::vector< QPoint > & addCells, std::vector< QPoint > & deleteCells,
								 std::vector< QPoint > & mergeCells, QString sText ) {
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
		auto pSongEditorPositionRuler = HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler();
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		if( m_bTempoMarkerPresent ){
			pCoreActionController->addTempoMarker( m_nOldColumn, m_fOldBpm );
		} else {
			pCoreActionController->deleteTempoMarker( m_nNewColumn );
		}
		pSongEditorPositionRuler->createBackground();
	}

	virtual void redo() {
		auto pSongEditorPositionRuler = HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler();
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		pCoreActionController->deleteTempoMarker( m_nOldColumn );
		pCoreActionController->addTempoMarker( m_nNewColumn, m_fNewBpm );
		pSongEditorPositionRuler->createBackground();
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
		auto pSongEditorPositionRuler = HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler();
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		pCoreActionController->addTempoMarker( m_nColumn, m_fBpm );
		pSongEditorPositionRuler->createBackground();
	}

	virtual void redo() {
		auto pSongEditorPositionRuler = HydrogenApp::get_instance()->getSongEditorPanel()->getSongEditorPositionRuler();
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		pCoreActionController->deleteTempoMarker( m_nColumn );
		pSongEditorPositionRuler->createBackground();
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
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		if ( ! m_sOldText.isEmpty() ){
			pCoreActionController->addTag( m_nPosition, m_sOldText );
		} else {
			pCoreActionController->deleteTag( m_nPosition );
		}
	}

	virtual void redo() {
		auto pCoreActionController = H2Core::Hydrogen::get_instance()->getCoreActionController();
		if ( ! m_sText.isEmpty() ){
			pCoreActionController->addTag( m_nPosition, m_sText );
		} else {
			pCoreActionController->deleteTag( m_nPosition );
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
class SE_addOrDeleteNoteAction : public QUndoCommand
{
public:
	SE_addOrDeleteNoteAction(  int nColumn,
							   int nRow,
							   int selectedPatternNumber,
							   int oldLength,
							   float oldVelocity,
							   float fOldPan,
							   float oldLeadLag,
							   int oldNoteKeyVal,
							   int oldOctaveKeyVal,
							   float probability,
							   bool isDelete,
							   bool listen,
							   bool isMidi,
							   bool isInstrumentMode,
							   bool isNoteOff ){

		if( isDelete ){
			setText( QObject::tr( "Delete note ( %1, %2)" ).arg( nColumn ).arg( nRow ) );
		} else {
			setText( QObject::tr( "Add note ( %1, %2)" ).arg( nColumn ).arg( nRow ) );
		}
		__nColumn = nColumn;
		__nRow = nRow;
		__selectedPatternNumber = selectedPatternNumber;
		__oldLength = oldLength;
		__oldVelocity = oldVelocity;
		m_fOldPan = fOldPan;
		__oldLeadLag = oldLeadLag;
		__oldNoteKeyVal = oldNoteKeyVal;
		__oldOctaveKeyVal = oldOctaveKeyVal;
		__probability = probability;
		__listen = listen;
		__isMidi = isMidi;
		__isInstrumentMode = isInstrumentMode;
		__isDelete = isDelete;
		__isNoteOff = isNoteOff;
	}
	virtual void undo()
	{
		//qDebug() << "Add note Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		__isMidi = false; // undo is never a midi event.
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->addOrDeleteNoteAction(  __nColumn,
												__nRow,
												__selectedPatternNumber,
												__oldLength,
												__oldVelocity,
												m_fOldPan,
												__oldLeadLag,
												__oldNoteKeyVal,
												__oldOctaveKeyVal,
												__probability,
												__listen,
												__isMidi,
												__isInstrumentMode,
												__isNoteOff,
												!__isDelete );
	}
	virtual void redo()
	{
		//qDebug() << "Add Note Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->addOrDeleteNoteAction(  __nColumn,
												__nRow,
												__selectedPatternNumber,
												__oldLength,
												__oldVelocity,
												m_fOldPan,
												__oldLeadLag,
												__oldNoteKeyVal,
												__oldOctaveKeyVal,
												__probability,
												__listen,
												__isMidi,
												__isInstrumentMode,
												__isNoteOff,
												__isDelete );
	}
private:
	int __nColumn;
	int __nRow;
	int __selectedPatternNumber;
	int __oldLength;
	float __oldVelocity;
	float m_fOldPan;
	float __oldLeadLag;
	int __oldNoteKeyVal;
	int __oldOctaveKeyVal;
	float __probability;
	bool __listen;
	bool __isMidi;
	bool __isInstrumentMode;
	bool __isDelete;
	bool __isNoteOff;
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
	SE_deselectAndOverwriteNotesAction( std::vector< H2Core::Note *> &selected, std::vector< H2Core::Note *> &overwritten ) {
		setText( QObject::tr( "Overwrite %1 notes" ).arg( overwritten.size() ) );
		for ( auto pNote : selected ) {
			m_selected.push_back( new H2Core::Note ( pNote ) );
		}
		for ( auto pNote : overwritten ) {
			m_overwritten.push_back( new H2Core::Note ( pNote ) );
		}
	}

	~SE_deselectAndOverwriteNotesAction() {
		for ( auto pNote : m_selected ) {
			delete pNote;
		}
		for ( auto pNote : m_overwritten ) {
			delete pNote;
		}
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
	std::vector< H2Core::Note *> m_selected;
	std::vector< H2Core::Note *> m_overwritten;
};

/** \ingroup docGUI*/
class SE_moveNoteAction : public QUndoCommand
{
public:
	SE_moveNoteAction( int nOldPosition, int nOldInstrument, int nPattern, int nNewPosition, int nNewInstrument,
					   H2Core::Note *pNote )
	{
		m_nOldPosition = nOldPosition;
		m_nOldInstrument = nOldInstrument;
		m_nPattern = nPattern;
		m_nNewPosition = nNewPosition;
		m_nNewInstrument = nNewInstrument;
		m_pNote = new H2Core::Note( pNote );
	}

	~SE_moveNoteAction()
	{
		delete m_pNote;
	}

	virtual void undo()
	{
		HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()
			->moveNoteAction( m_nNewPosition, m_nNewInstrument, m_nPattern,
							  m_nOldPosition, m_nOldInstrument, m_pNote );
	}

	virtual void redo()
	{
		HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()
			->moveNoteAction( m_nOldPosition, m_nOldInstrument, m_nPattern,
							  m_nNewPosition, m_nNewInstrument, m_pNote );
	}

private:
	int m_nOldPosition;
	int m_nOldInstrument;
	int m_nPattern;
	int m_nNewPosition;
	int m_nNewInstrument;
	H2Core::Note *m_pNote;
};

/** \ingroup docGUI*/
class SE_editNoteLengthAction : public QUndoCommand
{
public:
	SE_editNoteLengthAction( int nColumn, int nRealColumn, int nRow, int nLength,
							 int nOldLength, int nSelectedPatternNumber,
							 int nSelectedInstrumentNumber,
							 PatternEditor::Editor editor ){
		setText( QObject::tr( "Change note length" ) );
		m_nColumn = nColumn;
		m_nRealColumn = nRealColumn;
		m_nRow = nRow;
		m_nLength = nLength;
		m_nOldLength = nOldLength;
		m_nSelectedPatternNumber = nSelectedPatternNumber;
		m_nSelectedInstrumentNumber = nSelectedInstrumentNumber;
		m_editor = editor;
	}
	virtual void undo()
	{
		// For now it does not matter which derived class of the
		// PatternEditor will execute the call to
		// editNoteLengthAction(). 
		HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()
			->editNoteLengthAction( m_nColumn, m_nRealColumn, m_nRow,
									m_nOldLength, m_nSelectedPatternNumber,
									m_nSelectedInstrumentNumber, m_editor );
	}
	virtual void redo()
	{
		// For now it does not matter which derived class of the
		// PatternEditor will execute the call to
		// editNoteLengthAction(). 
		HydrogenApp::get_instance()->getPatternEditorPanel()->getDrumPatternEditor()
			->editNoteLengthAction( m_nColumn, m_nRealColumn, m_nRow,
									m_nLength, m_nSelectedPatternNumber,
									m_nSelectedInstrumentNumber, m_editor );
	}
private:
	int m_nColumn;
	int m_nRealColumn;
	int m_nRow;
	int m_nLength;
	int m_nOldLength;
	int m_nSelectedPatternNumber;
	int m_nSelectedInstrumentNumber;
	PatternEditor::Editor m_editor;
};


/** \ingroup docGUI*/
class SE_editNotePropertiesAction : public QUndoCommand
{
public:
	SE_editNotePropertiesAction( int nColumn,
								 int nRealColumn,
								 int nRow,
								 int nSelectedPatternNumber,
								 int nSelectedInstrumentNumber,
								 PatternEditor::Mode mode,
								 PatternEditor::Editor editor,
								 float fVelocity,
								 float fOldVelocity,
								 float fPan,
								 float fOldPan,
								 float fLeadLag,
								 float fOldLeadLag,
								 float fProbability,
								 float fOldProbability ){
		setText( QObject::tr( "Change note properties piano roll" )
				 .append( QString( ": [%1" )
						  .arg( PatternEditor::modeToQString( mode ) ) ) );
		m_nColumn = nColumn;
		m_nRealColumn = nRealColumn;
		m_nRow = nRow;
		m_nSelectedPatternNumber = nSelectedPatternNumber;
		m_nSelectedInstrumentNumber = nSelectedInstrumentNumber;
		m_mode = mode;
		m_editor = editor;
		m_fVelocity = fVelocity;
		m_fOldVelocity = fOldVelocity;
		m_fPan = fPan;
		m_fOldPan = fOldPan;
		m_fLeadLag = fLeadLag;
		m_fOldLeadLag = fOldLeadLag;
		m_fProbability = fProbability;
		m_fOldProbability = fOldProbability;
	}
	virtual void undo()
	{
		// For now it does not matter which derived class of the
		// PatternEditor will execute the call to
		// editNoteLengthAction(). 
		HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditor()->
			editNotePropertiesAction( m_nColumn,
									  m_nRealColumn,
									  m_nRow,
									  m_nSelectedPatternNumber,
									  m_nSelectedInstrumentNumber,
									  m_mode,
									  m_editor,
									  m_fOldVelocity,
									  m_fOldPan,
									  m_fOldLeadLag,
									  m_fOldProbability );
	}
	virtual void redo()
	{
		// For now it does not matter which derived class of the
		// PatternEditor will execute the call to
		// editNoteLengthAction(). 
		HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditor()->
			editNotePropertiesAction( m_nColumn,
									  m_nRealColumn,
									  m_nRow,
									  m_nSelectedPatternNumber,
									  m_nSelectedInstrumentNumber,
									  m_mode,
									  m_editor,
									  m_fVelocity,
									  m_fPan,
									  m_fLeadLag,
									  m_fProbability );
	}

private:
	int m_nColumn;
	int m_nRealColumn;
	int m_nRow;
	int m_nSelectedPatternNumber;
	int m_nSelectedInstrumentNumber;
	PatternEditor::Mode m_mode;
	PatternEditor::Editor m_editor;
	float m_fVelocity;
	float m_fOldVelocity;
	float m_fPan;
	float m_fOldPan;
	float m_fLeadLag;
	float m_fOldLeadLag;
	float m_fProbability;
	float m_fOldProbability;
};

/** \ingroup docGUI*/
class SE_clearNotesPatternEditorAction : public QUndoCommand
{
public:
	SE_clearNotesPatternEditorAction(  std::list<  H2Core::Note* > noteList, int nSelectedInstrument, int selectedPatternNumber ){
		setText( QObject::tr( "Clear notes" ) );

		std::list < H2Core::Note *>::iterator pos;
		for ( pos = noteList.begin(); pos != noteList.end(); ++pos){
			H2Core::Note *pNote;
			pNote = new H2Core::Note(*pos);
			assert( pNote );
			__noteList.push_back( pNote );
		}

		__nSelectedInstrument = nSelectedInstrument;
		__selectedPatternNumber = selectedPatternNumber;
	}

	~SE_clearNotesPatternEditorAction(){
		//qDebug() << "delete left notes ";
		while ( __noteList.size() ) {
			delete __noteList.front();
			__noteList.pop_front();
		}

	}

	virtual void undo()
	{
		//qDebug() << "clear note sequence Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionClearNotesUndoAction( __noteList, __nSelectedInstrument, __selectedPatternNumber );
	}
	virtual void redo() {
		H2Core::Hydrogen::get_instance()->getCoreActionController()
			->clearInstrumentInPattern( __nSelectedInstrument,
										__selectedPatternNumber );
	}
private:
	std::list< H2Core::Note* > __noteList;
	int __nSelectedInstrument;
	int __selectedPatternNumber;
};

/** \ingroup docGUI*/
class SE_pasteNotesPatternEditorAction : public QUndoCommand
{
public:
	explicit SE_pasteNotesPatternEditorAction(const std::list<H2Core::Pattern*> & patternList)
	{
		//qDebug() << "paste note sequence Create ";
		setText( QObject::tr( "Paste instrument notes" ) );

		std::list < H2Core::Pattern *>::const_iterator pos;
		for ( pos = patternList.begin(); pos != patternList.end(); ++pos)
		{
			H2Core::Pattern *pPattern = *pos;
			assert( pPattern );
			__patternList.push_back(pPattern);
		}
	}

	~SE_pasteNotesPatternEditorAction()
	{
		//qDebug() << "paste note sequence Destroy ";
		while ( __patternList.size() > 0)
		{
			delete __patternList.front();
			__patternList.pop_front();
		}
		while ( __appliedList.size() > 0)
		{
			delete __appliedList.front();
			__appliedList.pop_front();
		}
	}

	virtual void undo()
	{
		//qDebug() << "paste note sequence Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionPasteNotesUndoAction( __appliedList );
	}

	virtual void redo()
	{
		//qDebug() << "paste note sequence Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionPasteNotesRedoAction( __patternList, __appliedList );
	}

private:
	std::list< H2Core::Pattern* > __patternList;
	std::list< H2Core::Pattern* > __appliedList;
};


/** \ingroup docGUI*/
class SE_fillNotesRightClickAction : public QUndoCommand
{
public:
	SE_fillNotesRightClickAction( QStringList notePositions, int nSelectedInstrument, int selectedPatternNumber  ){
		setText( QObject::tr( "Fill notes" ) );
		__notePositions = notePositions;
		__nSelectedInstrument= nSelectedInstrument;
		__selectedPatternNumber = selectedPatternNumber;
	}
	virtual void undo()
	{
		//qDebug() << "fill notes Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionFillNotesUndoAction( __notePositions, __nSelectedInstrument, __selectedPatternNumber );
	}
	virtual void redo()
	{
		//qDebug() << "fill notes Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionFillNotesRedoAction( __notePositions, __nSelectedInstrument, __selectedPatternNumber );
	}
private:
	QStringList __notePositions;
	int __nSelectedInstrument;
	int __selectedPatternNumber;
};


/** \ingroup docGUI*/
class SE_randomVelocityRightClickAction : public QUndoCommand
{
public:
	SE_randomVelocityRightClickAction( QStringList noteVeloValue, QStringList oldNoteVeloValue, int nSelectedInstrument, int selectedPatternNumber  ){
		setText( QObject::tr( "Random velocity" ) );
		__noteVeloValue = noteVeloValue;
		__oldNoteVeloValue = oldNoteVeloValue;
		__nSelectedInstrument= nSelectedInstrument;
		__selectedPatternNumber = selectedPatternNumber;
	}
	virtual void undo()
	{
		//qDebug() << "Random velocity Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionRandomVelocityAction( __oldNoteVeloValue, __nSelectedInstrument, __selectedPatternNumber );
	}
	virtual void redo()
	{
		//qDebug() << "Random velocity Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionRandomVelocityAction( __noteVeloValue, __nSelectedInstrument, __selectedPatternNumber );
	}
private:
	QStringList __noteVeloValue;
	QStringList __oldNoteVeloValue;
	int __nSelectedInstrument;
	int __selectedPatternNumber;
};



/** \ingroup docGUI*/
class SE_moveInstrumentAction : public QUndoCommand
{
public:
	SE_moveInstrumentAction(  int nSourceInstrument, int nTargetInstrument  ){
		setText( QObject::tr( "Move instrument" ) );
		__nSourceInstrument = nSourceInstrument;
		__nTargetInstrument = nTargetInstrument;
	}
	virtual void undo()
	{
		//qDebug() << "move Instrument Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionMoveInstrumentAction( __nTargetInstrument, __nSourceInstrument );
	}
	virtual void redo()
	{
		//qDebug() << "move Instrument Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionMoveInstrumentAction( __nSourceInstrument, __nTargetInstrument );
	}
private:
	int __nSourceInstrument;
	int __nTargetInstrument;
};

/** \ingroup docGUI*/
class SE_dragInstrumentAction : public QUndoCommand
{
public:
	SE_dragInstrumentAction( QString sDrumkitPath, QString sInstrumentName, int nTargetInstrument ){
		setText( QObject::tr( "Drop instrument" ) );
		__sDrumkitPath = sDrumkitPath;
		__sInstrumentName = sInstrumentName;
		__nTargetInstrument = nTargetInstrument;
		__addedComponents = new std::vector<int>();
	}

	~SE_dragInstrumentAction()
	{
		delete __addedComponents;
	}

	virtual void undo()
	{
		//qDebug() << "drop Instrument Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentUndoAction( __nTargetInstrument, __addedComponents );
	}

	virtual void redo()
	{
		//qDebug() << "drop Instrument Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentRedoAction( __sDrumkitPath, __sInstrumentName, __nTargetInstrument, __addedComponents );
	}

private:
	QString __sDrumkitPath;
	QString __sInstrumentName;
	int __nTargetInstrument;
	std::vector<int>* __addedComponents;
};


/** \ingroup docGUI*/
class SE_deleteInstrumentAction : public QUndoCommand
{
public:
	SE_deleteInstrumentAction(  std::list<  H2Core::Note* > noteList, QString sDrumkitPath, QString sInstrumentName, int nSelectedInstrument ){
		setText( QObject::tr( "Delete instrument " ) );

		std::list < H2Core::Note *>::iterator pos;
		for ( pos = noteList.begin(); pos != noteList.end(); ++pos){
			H2Core::Note *pNote;
			pNote = new H2Core::Note(*pos);
			assert( pNote );
			__noteList.push_back( pNote );
		}
		__drumkitPath = sDrumkitPath;
		__instrumentName = sInstrumentName;
		__nSelectedInstrument = nSelectedInstrument;
	}

	~SE_deleteInstrumentAction(){
		//qDebug() << "delete left notes ";
		while ( __noteList.size() ) {
			delete __noteList.front();
			__noteList.pop_front();
		}

	}

	virtual void undo()
	{
		//qDebug() << "delete Instrument Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDeleteInstrumentUndoAction( __noteList, __nSelectedInstrument, __instrumentName, __drumkitPath );
	}
	virtual void redo()
	{
		//qDebug() << "delete Instrument Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		//delete an instrument from list
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentUndoAction( __nSelectedInstrument, new std::vector<int>() );
	}
private:
	std::list< H2Core::Note* > __noteList;
	QString __instrumentName;
	QString __drumkitPath;
	int __nSelectedInstrument;
};



/** \ingroup docGUI*/
class SE_mainMenuAddInstrumentAction : public QUndoCommand
{
public:
	SE_mainMenuAddInstrumentAction(){
		setText( QObject::tr( "Drop instrument" ) );
	}
	virtual void undo()
	{
		//qDebug() << "drop Instrument Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionAddEmptyInstrumentUndo();
	}
	virtual void redo()
	{
		//qDebug() << "drop Instrument Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionAddEmptyInstrumentRedo();
	}
private:
};

// ~pattern editor commands
//=====================================================================================================================================
//piano roll editor commands


/** \ingroup docGUI*/
class SE_addOrDeleteNotePianoRollAction : public QUndoCommand
{
public:
	SE_addOrDeleteNotePianoRollAction( int nColumn,
									   int pressedLine,
									   int selectedPatternNumber,
									   int nSelectedInstrumentnumber,
									   int oldLength,
									   float oldVelocity,
									   float fOldPan,
									   float oldLeadLag,
									   int oldNoteKeyVal,
									   int oldOctaveKeyVal,
									   float fProbability,
									   bool isDelete ) {
		setText( QObject::tr( "Add piano roll note ( %1, %2 )" ).arg( nColumn ).arg( pressedLine ) );
		__nColumn = nColumn;
		__pressedLine = pressedLine;
		__selectedPatternNumber = selectedPatternNumber;
		__nSelectedInstrumentnumber = nSelectedInstrumentnumber;
		__oldLength = oldLength;
		__oldVelocity = oldVelocity;
		m_fOldPan = fOldPan;
		__oldLeadLag = oldLeadLag;
		__oldNoteKeyVal = oldNoteKeyVal;
		__oldOctaveKeyVal = oldOctaveKeyVal;
		__probability = fProbability;
		__isDelete = isDelete;

	}
	virtual void undo()
	{
		//qDebug() << "Add Piano Roll note Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addOrDeleteNoteAction( __nColumn,
																					 __pressedLine,
																					 __selectedPatternNumber,
																					 __nSelectedInstrumentnumber,
																					 __oldLength,
																					 __oldVelocity,
																					 m_fOldPan,
																					 __oldLeadLag,
																					 __oldNoteKeyVal,
																					 __oldOctaveKeyVal,
																					 __probability,
																					 false,
																					 !__isDelete );
	}
	virtual void redo()
	{
		//qDebug() << "Add Piano Roll Note Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addOrDeleteNoteAction( __nColumn,
																					 __pressedLine,
																					 __selectedPatternNumber,
																					 __nSelectedInstrumentnumber,
																					 __oldLength,
																					 __oldVelocity,
																					 m_fOldPan,
																					 __oldLeadLag,
																					 __oldNoteKeyVal,
																					 __oldOctaveKeyVal,
																					 __probability,
																					 false,
																					 __isDelete );
	}
private:
	int __nColumn;
	int __pressedLine;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
	int __oldLength;
	float __oldVelocity;
	float m_fOldPan;
	float __oldLeadLag;
	int __oldNoteKeyVal;
	int __oldOctaveKeyVal;
	float __probability;
	bool __isDelete;
};

/** \ingroup docGUI*/
class SE_addPianoRollNoteOffAction : public QUndoCommand
{
public:
	SE_addPianoRollNoteOffAction( int nColumn, int pressedLine, int selectedPatternNumber, int nSelectedInstrumentnumber ){
		setText( QObject::tr( "Add  piano roll NOTE_OFF note ( %1, %2 )" ).arg( nColumn ).arg( pressedLine ) );
		__nColumn = nColumn;
		__pressedLine = pressedLine;
		__selectedPatternNumber = selectedPatternNumber;
		__nSelectedInstrumentnumber = nSelectedInstrumentnumber;
	}
	virtual void undo()
	{
		//qDebug() << "Add off note Note Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addOrDeleteNoteAction( __nColumn, __pressedLine, __selectedPatternNumber,  __nSelectedInstrumentnumber, -1, 0.8f, 0.f, 0.0, 0, 0 , 1.0f, true, true );
	}
	virtual void redo()
	{
		//qDebug() << "Add off note Note Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addOrDeleteNoteAction( __nColumn, __pressedLine, __selectedPatternNumber,  __nSelectedInstrumentnumber, -1, 0.8f, 0.f, 0.0, 0, 0, 1.0f, true, false );

	}
private:
	int __nColumn;
	int __pressedLine;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
};

/** \ingroup docGUI*/
class SE_moveNotePianoRollAction : public QUndoCommand
{
	public:
	SE_moveNotePianoRollAction( int nOldPosition, H2Core::Note::Octave oldOctave, H2Core::Note::Key oldKey, int nPattern,
								int nNewPosition, H2Core::Note::Octave newOctave, H2Core::Note::Key newKey,
								H2Core::Note *pNote )
	{
		m_nOldPosition = nOldPosition;
		m_oldOctave = oldOctave;
		m_oldKey = oldKey;
		m_nPattern = nPattern;
		m_nNewPosition = nNewPosition;
		m_newOctave = newOctave;
		m_newKey = newKey;
		m_pNote = new H2Core::Note( pNote );
	}

	private:
	int m_nOldPosition;
	H2Core::Note::Octave m_oldOctave;
	H2Core::Note::Key m_oldKey;
	int m_nPattern;
	int m_nNewPosition;
	H2Core::Note::Octave m_newOctave;
	H2Core::Note::Key m_newKey;
	H2Core::Note *m_pNote;

	~SE_moveNotePianoRollAction()
	{
		delete m_pNote;
	}

	virtual void undo()
	{
		HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditor()
		->moveNoteAction( m_nNewPosition, m_newOctave, m_newKey, m_nPattern,
						  m_nOldPosition, m_oldOctave, m_oldKey, m_pNote );
	}

	virtual void redo()
	{
		HydrogenApp::get_instance()->getPatternEditorPanel()->getPianoRollEditor()
		->moveNoteAction( m_nOldPosition, m_oldOctave, m_oldKey, m_nPattern,
						  m_nNewPosition, m_newOctave, m_newKey, m_pNote );
	}

};

// ~piano roll editor commands
//=====================================================================================================================================
//Note Properties Ruler commands

/** \ingroup docGUI*/
class SE_editNotePropertiesVolumeAction : public QUndoCommand
{
public:

	SE_editNotePropertiesVolumeAction( int undoColumn,
									   NotePropertiesRuler::Mode mode,
					   int nSelectedPatternNumber,
					   int nSelectedInstrument,
					   float velocity,
					   float oldVelocity,
					   float pan,
					   float oldPan,
					   float leadLag,
					   float oldLeadLag,
					   float probability,
					   float oldProbability,
					   int noteKeyVal,
					   int oldNoteKeyVal,
					   int octaveKeyVal,
					   int oldOctaveKeyVal)
	{
		setText( QObject::tr( "Edit note property %1" )
				 .arg( NotePropertiesRuler::modeToQString( mode ) ) );
		__undoColumn = undoColumn;
		__mode = mode;
		__nSelectedPatternNumber = nSelectedPatternNumber;
		__nSelectedInstrument = nSelectedInstrument;
		__velocity = velocity;
		__oldVelocity = oldVelocity;
		m_fPan = pan;
		m_fOldPan = oldPan;
		__leadLag = leadLag;
		__oldLeadLag = oldLeadLag;
		__probability = probability;
		__oldProbability = oldProbability;
		__noteKeyVal = noteKeyVal;
		__oldNoteKeyVal = oldNoteKeyVal;
		__octaveKeyVal = octaveKeyVal;
		__oldOctaveKeyVal = oldOctaveKeyVal;
	}
	
	virtual void undo()
	{
		//qDebug() << "edit note property Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();

		h2app->getPatternEditorPanel()->getDrumPatternEditor()->undoRedoAction( __undoColumn,
											__mode,
											__nSelectedPatternNumber,
											__nSelectedInstrument,
											__oldVelocity,
											m_fOldPan,
											__oldLeadLag,
											__oldProbability,
											__oldNoteKeyVal,
											__oldOctaveKeyVal );
	}
	virtual void redo()
	{
		//qDebug() << "edit note property Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->undoRedoAction( __undoColumn,
											__mode,
											__nSelectedPatternNumber,
											__nSelectedInstrument,
											__velocity,
											m_fPan,
											__leadLag,
											__probability,
											__noteKeyVal,
											__octaveKeyVal );
	}
private:


	int __undoColumn;
	NotePropertiesRuler::Mode __mode;
	int __nSelectedPatternNumber;
	int __nSelectedInstrument;
	float __velocity;
	float __oldVelocity;
	float m_fPan;
	float m_fOldPan;
	float __leadLag;
	float __oldLeadLag;
	float __probability;
	float __oldProbability;
	int __noteKeyVal;
	int __oldNoteKeyVal;
	int __octaveKeyVal;
	int __oldOctaveKeyVal;
};

// ~Note Properties Ruler commands
//=====================================================================================================================================



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

#endif // UNDOACTIONS_H
