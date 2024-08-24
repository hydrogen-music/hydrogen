/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Basics/Note.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/AutomationPath.h>
#include <core/CoreActionController.h>
#include <core/Helpers/Filesystem.h>
#include <core/License.h>

#include "CommonStrings.h"
#include "HydrogenApp.h"
#include "MainForm.h"
#include "InstrumentRack.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "SongEditor/PatternFillDialog.h"

#include "PatternEditor/NotePropertiesRuler.h"
#include "PatternEditor/DrumPatternEditor.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "SoundLibrary/SoundLibraryPanel.h"
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
		H2Core::CoreActionController::removePattern( m_nPatternPosition );
	}
	virtual void redo() {
		H2Core::CoreActionController::setPattern( new H2Core::Pattern( m_pNewPattern ),
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
		if( m_bTempoMarkerPresent ){
			H2Core::CoreActionController::addTempoMarker( m_nOldColumn, m_fOldBpm );
		} else {
			H2Core::CoreActionController::deleteTempoMarker( m_nNewColumn );
		}
		HydrogenApp::get_instance()->getSongEditorPanel()->
			getSongEditorPositionRuler()->createBackground();
	}

	virtual void redo() {
		H2Core::CoreActionController::deleteTempoMarker( m_nOldColumn );
		H2Core::CoreActionController::addTempoMarker( m_nNewColumn, m_fNewBpm );
		HydrogenApp::get_instance()->getSongEditorPanel()->
			getSongEditorPositionRuler()->createBackground();
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
			getSongEditorPositionRuler()->createBackground();
	}

	virtual void redo() {
		H2Core::CoreActionController::deleteTempoMarker( m_nColumn );
		HydrogenApp::get_instance()->getSongEditorPanel()->
			getSongEditorPositionRuler()->createBackground();
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
	SE_deselectAndOverwriteNotesAction( const std::vector< H2Core::Note *>& selected,
										const std::vector< H2Core::Note *>& overwritten ) {
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
							 const PatternEditor::Editor& editor ){
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
								 const PatternEditor::Mode& mode,
								 const PatternEditor::Editor& editor,
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
	SE_clearNotesPatternEditorAction( const std::list<  H2Core::Note* >& noteList,
									  int nSelectedInstrument,
									  int selectedPatternNumber ){
		setText( QObject::tr( "Clear notes" ) );

		for ( const auto& pNote : noteList ){
			assert( pNote );
			auto pNewNote = new H2Core::Note(*pNote);
			assert( pNewNote );
			__noteList.push_back( pNewNote );
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
	virtual void redo()
	{
		//qDebug() << "clear note sequence Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionClearNotesRedoAction( __nSelectedInstrument, __selectedPatternNumber );
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
	explicit SE_pasteNotesPatternEditorAction( H2Core::PatternList* pPatternList ) :
		m_pCopiedNotesPatternList( pPatternList ) {
		setText( QObject::tr( "Paste instrument notes" ) );

		m_pAppliedNotesPatternList = new H2Core::PatternList();
	}

	~SE_pasteNotesPatternEditorAction() {
		delete m_pCopiedNotesPatternList;
		delete m_pAppliedNotesPatternList;
	}

	virtual void undo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()->
			getDrumPatternEditor()->functionPasteNotesUndoAction(
				m_pAppliedNotesPatternList );

		// Discard temporary patterns.
		for ( auto& ppPattern : *m_pAppliedNotesPatternList ) {
			m_pAppliedNotesPatternList->del( ppPattern );
		}
		m_pAppliedNotesPatternList->clear();
	}

	virtual void redo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()->
			getDrumPatternEditor()->functionPasteNotesRedoAction(
				m_pCopiedNotesPatternList, m_pAppliedNotesPatternList );
	}

private:
		/** Pattern list containing only the notes copied and waiting for being
		 * pasted. */
		H2Core::PatternList* m_pCopiedNotesPatternList;
		/** Pattern list containing only those notes, which were actually added
		 * to a pattern during the redo part of this class. */
		H2Core::PatternList* m_pAppliedNotesPatternList;
};


/** \ingroup docGUI*/
class SE_fillNotesRightClickAction : public QUndoCommand
{
public:
	SE_fillNotesRightClickAction( const QStringList& notePositions,
								  int nSelectedInstrument,
								  int selectedPatternNumber  ){
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
	SE_dragInstrumentAction( const QString& sDrumkitPath,
							 const QString& sInstrumentName,
							 int nTargetInstrument ){
		setText( QObject::tr( "Drop instrument" ) );
		__sDrumkitPath = sDrumkitPath;
		__sInstrumentName = sInstrumentName;
		__nTargetInstrument = nTargetInstrument;
	}

	~SE_dragInstrumentAction() {}

	virtual void undo()
	{
		//qDebug() << "drop Instrument Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentUndoAction( __nTargetInstrument, false );
	}

	virtual void redo()
	{
		//qDebug() << "drop Instrument Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentRedoAction( __sDrumkitPath, __sInstrumentName, __nTargetInstrument );
	}

private:
	QString __sDrumkitPath;
	QString __sInstrumentName;
	int __nTargetInstrument;
};


/** \ingroup docGUI*/
class SE_deleteInstrumentAction : public QUndoCommand
{
public:
	SE_deleteInstrumentAction( const std::list<  H2Core::Note* >& noteList,
							   const QString& sDrumkitPath,
							   const QString& sInstrumentName,
							   int nSelectedInstrument,
							   bool bReplace )
		: __drumkitPath( sDrumkitPath )
		, __instrumentName( sInstrumentName )
		, __nSelectedInstrument( nSelectedInstrument)
		, m_bReplace( bReplace )
		{
		setText( QString( "%1 [%2]" )
				 .arg( QObject::tr( "Delete instrument " ) )
				 .arg( sInstrumentName ) );

		for ( const auto& ppNote : noteList ){
			auto pNewNote = new H2Core::Note(*ppNote);
			assert( pNewNote );
			__noteList.push_back( pNewNote );
		}
	}

	~SE_deleteInstrumentAction(){
		//qDebug() << "delete left notes ";
		while ( __noteList.size() ) {
			delete __noteList.front();
			__noteList.pop_front();
		}

	}

	virtual void undo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()->
			getDrumPatternEditor()->functionDeleteInstrumentUndoAction(
				__noteList, __nSelectedInstrument, __instrumentName,
				__drumkitPath, m_bReplace );
	}
	virtual void redo() {
		HydrogenApp::get_instance()->getPatternEditorPanel()->
			getDrumPatternEditor()->functionDropInstrumentUndoAction(
				__nSelectedInstrument, m_bReplace );
	}
private:
	std::list< H2Core::Note* > __noteList;
	QString __instrumentName;
	QString __drumkitPath;
	int __nSelectedInstrument;
		/** In case there is just a single instrument left in the drumkit, it
		 * get's replaced with an empty one instead of removed. */
		bool m_bReplace;
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
			/** Replace the current kit with a copy containing an additional
			 * component */
			AddComponent = 2,
			/** Replace the current kit with a copy from which one component was
			 * removed. */
			DeleteComponent = 3,
			/** Editing properties of the current song's kit. */
			EditProperties = 4
		};

		SE_switchDrumkitAction( std::shared_ptr<H2Core::Drumkit> pNewDrumkit,
								std::shared_ptr<H2Core::Drumkit> pOldDrumkit,
								const Type& type,
								const QString& sComponentName = "" ) :
			m_pNewDrumkit( pNewDrumkit ),
			m_pOldDrumkit( pOldDrumkit )
		{
			switch ( type ) {
			case Type::SwitchDrumkit:
				setText( QString( "%1: [%2] -> [%3]" )
						 .arg( QObject::tr( "Switching drumkits" ) )
						 .arg( pOldDrumkit != nullptr ? pOldDrumkit->getName() : "nullptr" )
						 .arg( pNewDrumkit != nullptr ? pNewDrumkit->getName() : "nullptr" ) );
				break;
			case Type::NewDrumkit:
				setText( QObject::tr( "Replace song drumkit with new and empty one" ) );
				break;
			case Type::AddComponent:
				setText( QString( "%1 [%2]" )
						 .arg( QObject::tr( "Adding component" ) )
						 .arg( sComponentName ) );
				break;
			case Type::DeleteComponent:
				setText( QString( "%1 [%2]" )
						 .arg( QObject::tr( "Remove component" ) )
						 .arg( sComponentName ) );
				break;
			case Type::EditProperties:
				setText( HydrogenApp::get_instance()->getCommonStrings()
						 ->getActionEditDrumkitProperties() );
				break;
			}
		}
		virtual void undo() {
			HydrogenApp::get_instance()->getInstrumentRack()->
				getSoundLibraryPanel()->switchDrumkit(
					m_pOldDrumkit, m_pNewDrumkit );
		}
		virtual void redo() {
			HydrogenApp::get_instance()->getInstrumentRack()->
				getSoundLibraryPanel()->switchDrumkit(
					m_pNewDrumkit, m_pOldDrumkit );
		}

	private:
		std::shared_ptr<H2Core::Drumkit> m_pNewDrumkit;
		std::shared_ptr<H2Core::Drumkit> m_pOldDrumkit;
};

class SE_renameComponentAction : public QUndoCommand {
	public:
		SE_renameComponentAction( const QString& sNewName,
								  const QString& sOldName,
								  int nComponentId ) :
			m_sNewName( sNewName ),
			m_sOldName( sOldName ),
			m_nComponentId( nComponentId ) {
			setText( QString( "%1: [%2] -> [%3]" )
					 .arg( QObject::tr( "Rename component" ) )
					 .arg( sOldName ).arg( sNewName ) );
		}
		virtual void undo() {
			InstrumentEditorPanel::get_instance()->getInstrumentEditor()
				->renameComponent( m_nComponentId, m_sOldName );
		}
		virtual void redo() {
			InstrumentEditorPanel::get_instance()->getInstrumentEditor()
				->renameComponent( m_nComponentId, m_sNewName );
		}
	private:
		QString m_sNewName;
		QString m_sOldName;
		int m_nComponentId;
};

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
									   const NotePropertiesRuler::Mode& mode,
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
