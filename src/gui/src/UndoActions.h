#ifndef UNDOACTIONS_H
#define UNDOACTIONS_H

#include <QtGui>
#include <QDebug>
#include <QUndoCommand>
#include <QPoint>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/pattern.h>

#include "HydrogenApp.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "PatternFillDialog.h"

#include "PatternEditor/NotePropertiesRuler.h"
#include "PatternEditor/DrumPatternEditor.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "PatternEditor/NotePropertiesRuler.h"


//=====================================================================================================================================
//song editor commands
class SE_addPatternAction : public QUndoCommand
{
public:
	SE_addPatternAction( int nColumn, int nRow, unsigned nColumnIndex ){
		setText( QString( "Add Pattern ( %1, %2 )" ).arg( nColumn ).arg( nRow ) );
		__nColumn = nColumn;
		__nRow = nRow;
		__nColumnIndex = nColumnIndex;
	}
	virtual void undo()
	{
		//qDebug() << "add Pattern Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->deletePattern( __nColumn, __nRow , __nColumnIndex );
	}
	virtual void redo()
	{
		//qDebug() << "add Pattern Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->addPattern( __nColumn, __nRow );
	}
private:
	int __nColumn;
	int __nRow;
	unsigned __nColumnIndex;
};


class SE_deletePatternAction : public QUndoCommand
{
public:
	SE_deletePatternAction( int nColumn, int nRow, unsigned nColumnIndex ){
		setText( QString( "Delete Pattern ( %1, %2 )" ).arg( nColumn ).arg( nRow ) );
		__nColumn = nColumn;
		__nRow = nRow;
		__nColumnIndex = nColumnIndex;
	}
	virtual void undo()
	{
		//qDebug() << "Delete pattern Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->addPattern( __nColumn, __nRow );
	}
	virtual void redo()
	{
		//qDebug() << "Delete pattern Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->deletePattern( __nColumn, __nRow, __nColumnIndex );
	}
private:
	int __nColumn;
	int __nRow;
	unsigned __nColumnIndex;
};


class SE_movePatternListItemAction : public QUndoCommand
{
public:
	SE_movePatternListItemAction(  int nSourcePattern , int nTargetPattern ){
		setText( QString( "Move pattern list item ( %1, %2 )" ).arg( nSourcePattern ).arg( nTargetPattern ) );
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


class SE_deletePatternSequenceAction : public QUndoCommand
{
public:
	SE_deletePatternSequenceAction(  QString pFilename ){
		setText( QString( "Delete complete pattern-sequence" ) );
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
		h2app->getSongEditorPanel()->getSongEditor()->clearThePatternSequenseVector( __pFilename );
	}
private:
	QString __pFilename;
};

class SE_deletePatternFromListAction : public QUndoCommand
{
public:
	SE_deletePatternFromListAction(  QString patternFilename , QString sequenceFileName, int patternPosition ){
		setText( QString( "Delete pattern from list" ) );
		__patternFilename =  patternFilename;
		__sequenceFileName = sequenceFileName;
		__patternPosition = patternPosition;
	}
	virtual void undo()
	{
		//qDebug() << "Delete pattern from list undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->restoreDeletedPatternsFromList( __patternFilename, __sequenceFileName, __patternPosition );
		h2app->getSongEditorPanel()->restoreGroupVector( __sequenceFileName );
		h2app->getSongEditorPanel()->getSongEditor()->updateEditorandSetTrue();
	}

	virtual void redo()
	{
		//qDebug() << "Delete pattern from list redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->deletePatternFromList( __patternFilename, __sequenceFileName, __patternPosition );
	}
private:
	QString __patternFilename;
	QString __sequenceFileName;
	int __patternPosition;
};

class SE_modifyPatternPropertiesAction : public QUndoCommand
{
public:
	SE_modifyPatternPropertiesAction( QString oldPatternName ,QString oldPatternInfo, QString oldPatternCategory, QString newPatternName , QString newPatternInfo, QString newPatternCategory, int patternNr ){
		setText( QString( "Modify pattern properties" ) );
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


class SE_copyPatternAction : public QUndoCommand
{
public:
	SE_copyPatternAction( QString patternFilename, int patternPosition ){
		setText( QString( "Copy pattern" ) );
		__patternFilename = patternFilename;
		__patternPosition = patternPosition;
	}
	virtual void undo()
	{
		//qDebug() << "copy pattern undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->revertaddEmptyPattern( __patternPosition );
	}

	virtual void redo()
	{
		//qDebug() << "copy pattern redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->patternPopup_copyAction( __patternFilename, __patternPosition );
	}
private:
	QString __patternFilename;
	int __patternPosition;
};

class SE_addEmptyPatternAction : public QUndoCommand
{
public:
	SE_addEmptyPatternAction( QString newPatternName, QString newPatternInfo, QString newPatternCategory, int patternPosition )
	{
		setText( QString( "Add pattern" ) );
		__newPatternName =  newPatternName;
		__newPatternCategory = newPatternCategory;
		__newPatternInfo = newPatternInfo;
		__patternPosition = patternPosition;
	}
	virtual void undo()
	{
		//qDebug() << "Add pattern undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->revertaddEmptyPattern( __patternPosition );
	}
	virtual void redo()
	{
		//qDebug() << "Add pattern redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->addEmptyPattern( __newPatternName , __newPatternInfo,__newPatternCategory, __patternPosition );
	}
private:
	QString __newPatternName;
	QString __newPatternCategory;
	QString __newPatternInfo;

	int __patternPosition;
};

class SE_loadPatternAction : public QUndoCommand
{
public:
	SE_loadPatternAction(  QString patternName, QString oldPatternName, QString sequenceFileName, int patternPosition, bool dragFromList){
		setText( QString( "Load/drag pattern" ) );
		__patternName =  patternName;
		__oldPatternName = oldPatternName;
		__sequenceFileName = sequenceFileName;
		__patternPosition = patternPosition;
		__dragFromList = dragFromList;
	}
	virtual void undo()
	{
		//qDebug() << "Load/drag pattern undo" << __dragFromList;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		if( __dragFromList ){
			h2app->getSongEditorPanel()->getSongEditorPatternList()->deletePatternFromList( __oldPatternName, __sequenceFileName, __patternPosition );
		}else
		{
			h2app->getSongEditorPanel()->getSongEditorPatternList()->restoreDeletedPatternsFromList( __oldPatternName, __sequenceFileName, __patternPosition );
			h2app->getSongEditorPanel()->revertaddEmptyPattern( __patternPosition +1 );
		}
		h2app->getSongEditorPanel()->restoreGroupVector( __sequenceFileName );
		h2app->getSongEditorPanel()->getSongEditor()->updateEditorandSetTrue();
	}

	virtual void redo()
	{
		//qDebug() <<  "Load/drag pattern redo" << __dragFromList << __patternPosition;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		if(!__dragFromList){
			h2app->getSongEditorPanel()->getSongEditorPatternList()->deletePatternFromList( __oldPatternName, __sequenceFileName, __patternPosition );
		}
		h2app->getSongEditorPanel()->getSongEditorPatternList()->loadPatternAction( __patternName, __patternPosition  );
	}
private:
	QString __patternName;
	QString __oldPatternName;
	QString __sequenceFileName;
	int __patternPosition;
	bool __dragFromList;
};


class SE_fillRangePatternAction : public QUndoCommand
{
public:
	SE_fillRangePatternAction( FillRange* pRange, int nPattern ){
		setText( QString( "Fill/remove range of pattern" ) );
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

class SE_movePatternCellAction : public QUndoCommand
{
public:
	SE_movePatternCellAction( std::vector<QPoint> movingCells, std::vector<QPoint> selectedCells, std::vector<QPoint> existingCells, bool bIsCtrlPressed ){
		setText( QString( "Move/copy selected cells" ) );
		__selectedCells = selectedCells;
		__movingCells = movingCells;
		__existingCells = existingCells;
		__bIsCtrlPressed = bIsCtrlPressed;

	}
	virtual void undo()
	{
		//qDebug() <<  "move/copy selected cells undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->movePatternCellAction( __selectedCells, __movingCells, __existingCells, __bIsCtrlPressed, true );
	}

	virtual void redo()
	{
		//qDebug() <<  "move/copy selected cells redo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->movePatternCellAction( __movingCells, __selectedCells, __existingCells, __bIsCtrlPressed, false );
	}
private:
	std::vector<QPoint> __selectedCells;
	std::vector<QPoint> __movingCells;
	std::vector<QPoint> __existingCells;
	bool __bIsCtrlPressed;
};

class SE_editTimeLineAction : public QUndoCommand
{
public:
	SE_editTimeLineAction( int newPosition, float oldBpm, float newBpm ){
		setText( QString( "Edit timeline tempo" ) );
		__newPosition = newPosition;
		__oldBpm = oldBpm;
		__newBpm = newBpm;

	}
	virtual void undo()
	{
		//qDebug() <<  "edit timeline tempo undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		if(__oldBpm >-1 ){
			h2app->getSongEditorPanel()->getSongEditorPositionRuler()->editTimeLineAction( __newPosition, __oldBpm );
		}else
		{
			h2app->getSongEditorPanel()->getSongEditorPositionRuler()->deleteTimeLinePosition( __newPosition );
		}
	}

	virtual void redo()
	{
		//qDebug() <<  "edit timeline tempo redo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPositionRuler()->editTimeLineAction( __newPosition, __newBpm );
	}
private:
	int __newPosition;
	float __oldBpm;
	float __newBpm;
};

//~song editor commands
//=====================================================================================================================================
//time line commands

class SE_deleteTimeLineAction : public QUndoCommand
{
public:
	SE_deleteTimeLineAction( int newPosition, float oldBpm ){
		setText( QString( "Delete timeline tempo" ) );
		__newPosition = newPosition;
		__oldBpm = oldBpm;

	}
	virtual void undo()
	{
		//qDebug() <<  "delete timeline tempo undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPositionRuler()->editTimeLineAction( __newPosition, __oldBpm );

	}

	virtual void redo()
	{
		//qDebug() <<  "delete timeline tempo redo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPositionRuler()->deleteTimeLinePosition( __newPosition );
	}
private:
	int __newPosition;
	float __oldBpm;
	float __newBpm;
};

class SE_editTagAction : public QUndoCommand
{
public:
	SE_editTagAction( QString text, QString oldText, int position ){
		setText( "Edit timeline tag" );
		__text = text;
		__oldText = oldText;
		__position = position;

	}
	virtual void undo()
	{
		//qDebug() <<  "edit timeline tag undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		if( __oldText != "" ){
			h2app->getSongEditorPanel()->getSongEditorPositionRuler()->editTagAction( __oldText, __position , __text );
		}else
		{
			h2app->getSongEditorPanel()->getSongEditorPositionRuler()->deleteTagAction( __text,  __position );
		}

	}

	virtual void redo()
	{
		//qDebug() <<  "edit timeline tag redo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		if( __text == "" ){
			h2app->getSongEditorPanel()->getSongEditorPositionRuler()->deleteTagAction( __oldText,  __position );
		}else
		{
			h2app->getSongEditorPanel()->getSongEditorPositionRuler()->editTagAction( __text, __position, __oldText );
		}
	}
private:
	QString __text;
	QString __oldText;
	int __position;
};

//~time line commands
//=====================================================================================================================================
//pattern editor commands

class SE_addNoteAction : public QUndoCommand
{
public:
	SE_addNoteAction(  int nColumn,
			   int nRow,
			   int selectedPatternNumber,
			   int oldLength,
			   float oldVelocity,
			   float oldPan_L,
			   float oldPan_R,
			   float oldLeadLag,
			   int oldNoteKeyVal,
			   int oldOctaveKeyVal,
			   bool noteExisted,
			   bool listen,
			   bool isMidi,
			   bool isInstrumentMode){

		if( noteExisted ){
			setText( QString( "Delete note ( %1, %2)" ).arg( nColumn ).arg( nRow ) );
		} else {
			setText( QString( "Add note ( %1, %2)" ).arg( nColumn ).arg( nRow ) );
		}
		__nColumn = nColumn;
		__nRow = nRow;
		__selectedPatternNumber = selectedPatternNumber;
		__oldLength = oldLength;
		__oldVelocity = oldVelocity;
		__oldPan_L = oldPan_L;
		__oldPan_R = oldPan_R;
		__oldLeadLag = oldLeadLag;
		__oldNoteKeyVal = oldNoteKeyVal;
		__oldOctaveKeyVal = oldOctaveKeyVal;
		__listen = listen;
		__isMidi = isMidi;
		__isInstrumentMode = isInstrumentMode;
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
												__oldPan_L,
												__oldPan_R,
												__oldLeadLag,
												__oldNoteKeyVal,
												__oldOctaveKeyVal,
												__listen,
												__isMidi,
												__isInstrumentMode,
												false  );
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
												__oldPan_L,
												__oldPan_R,
												__oldLeadLag,
												__oldNoteKeyVal,
												__oldOctaveKeyVal,
												__listen,
												__isMidi,
												__isInstrumentMode,
												false  );
	}
private:
	int __nColumn;
	int __nRow;
	int __selectedPatternNumber;
	int __oldLength;
	float __oldVelocity;
	float __oldPan_L;
	float __oldPan_R;
	float __oldLeadLag;
	int __oldNoteKeyVal;
	int __oldOctaveKeyVal;
	bool __listen;
	bool __isMidi;
	bool __isInstrumentMode;
};


class SE_addNoteRightClickAction : public QUndoCommand
{
public:
	SE_addNoteRightClickAction( int nColumn, int nRow, int selectedPatternNumber ){
		setText( QString( "Add pattern editor NOTE_OFF note ( %1, %2 )" ).arg( nColumn ).arg( nRow ) );
		__nColumn = nColumn;
		__nRow = nRow;
		__selectedPatternNumber = selectedPatternNumber;
	}
	virtual void undo()
	{
		//qDebug() << "Add off note Note Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->addOrDeleteNoteAction( __nColumn, __nRow, __selectedPatternNumber, -1, 0.8f, 0.5f, 0.5f, 0.0, 0, 0, false, false, false, true);
	}
	virtual void redo()
	{
		//qDebug() << "Add off note Note Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->addOrDeleteNoteAction( __nColumn, __nRow, __selectedPatternNumber, -1, 0.8f, 0.5f, 0.5f, 0.0, 0, 0, false, false, false, true);
	}
private:
	int __nColumn;
	int __nRow;
	int __selectedPatternNumber;
};

class SE_editNoteLenghtAction : public QUndoCommand
{
public:
	SE_editNoteLenghtAction( int nColumn, int nRealColumn, int row, int length, int oldLength, int selectedPatternNumber ){
		setText( QString( "Change note length" ) );
		__nColumn = nColumn;
		__nRealColumn = nRealColumn;
		__row = row;
		__length = length;
		__oldLength = oldLength;
		__selectedPatternNumber = selectedPatternNumber;
	}
	virtual void undo()
	{
		//qDebug() << "Change note length Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->editNoteLengthAction( __nColumn,  __nRealColumn, __row, __oldLength, __selectedPatternNumber );
	}
	virtual void redo()
	{
		//qDebug() << "Change note length Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->editNoteLengthAction( __nColumn,  __nRealColumn, __row, __length, __selectedPatternNumber );
	}
private:
	int __nColumn;
	int __nRealColumn;
	int __row;
	int __oldLength;
	int __length;
	int __selectedPatternNumber;
};


class SE_clearNotesPatternEditorAction : public QUndoCommand
{
public:
	SE_clearNotesPatternEditorAction(  std::list<  H2Core::Note* > noteList, int nSelectedInstrument, int selectedPatternNumber ){
		setText( QString( "Clear notes" ) );

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
		//qDebug() << "clear note sequense Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionClearNotesUndoAction( __noteList, __nSelectedInstrument, __selectedPatternNumber );
	}
	virtual void redo()
	{
		//qDebug() << "clear note sequense Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionClearNotesRedoAction( __nSelectedInstrument, __selectedPatternNumber );
	}
private:
	std::list< H2Core::Note* > __noteList;
	int __nSelectedInstrument;
	int __selectedPatternNumber;
};

class SE_pasteNotesPatternEditorAction : public QUndoCommand
{
public:
	SE_pasteNotesPatternEditorAction(const std::list<H2Core::Pattern*> & patternList)
	{
		//qDebug() << "paste note sequence Create ";
		setText( QString( "Paste instrument notes" ) );

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


class SE_fillNotesRightClickAction : public QUndoCommand
{
public:
	SE_fillNotesRightClickAction( QStringList notePositions, int nSelectedInstrument, int selectedPatternNumber  ){
		setText( QString( "Fill notes" ) );
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


class SE_randomVelocityRightClickAction : public QUndoCommand
{
public:
	SE_randomVelocityRightClickAction( QStringList noteVeloValue, QStringList oldNoteVeloValue, int nSelectedInstrument, int selectedPatternNumber  ){
		setText( QString( "Random velocity" ) );
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



class SE_moveInstrumentAction : public QUndoCommand
{
public:
	SE_moveInstrumentAction(  int nSourceInstrument, int nTargetInstrument  ){
		setText( QString( "Move instrument" ) );
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

class SE_dragInstrumentAction : public QUndoCommand
{
public:
	SE_dragInstrumentAction(  QString sDrumkitName, QString sInstrumentName, int nTargetInstrument, bool Merge  ){
		setText( QString( "Drop instrument" ) );
		__sDrumkitName = sDrumkitName;
		__sInstrumentName = sInstrumentName;
		__nTargetInstrument = nTargetInstrument;
		__bMerge = Merge;
	}
	virtual void undo()
	{
		//qDebug() << "drop Instrument Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentUndoAction( __nTargetInstrument );
	}
	virtual void redo()
	{
		//qDebug() << "drop Instrument Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentRedoAction( __sDrumkitName, __sInstrumentName, __nTargetInstrument, __bMerge );
	}
private:
	QString __sDrumkitName;
	QString __sInstrumentName;
	int __nTargetInstrument;
	bool __bMerge;
};


class SE_deleteInstrumentAction : public QUndoCommand
{
public:
	SE_deleteInstrumentAction(  std::list<  H2Core::Note* > noteList, QString drumkitName, QString instrumentName, int nSelectedInstrument ){
		setText( QString( "Delete instrument " ) );

		std::list < H2Core::Note *>::iterator pos;
		for ( pos = noteList.begin(); pos != noteList.end(); ++pos){
			H2Core::Note *pNote;
			pNote = new H2Core::Note(*pos);
			assert( pNote );
			__noteList.push_back( pNote );
		}
		__drumkitName = drumkitName;
		__instrumentName = instrumentName;
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
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDeleteInstrumentUndoAction( __noteList, __nSelectedInstrument, __instrumentName, __drumkitName );
	}
	virtual void redo()
	{
		//qDebug() << "delete Instrument Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		//delete an instrument from list
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->functionDropInstrumentUndoAction( __nSelectedInstrument );
	}
private:
	std::list< H2Core::Note* > __noteList;
	QString __instrumentName;
	QString __drumkitName;
	int __nSelectedInstrument;
};



class SE_mainMenuAddInstrumentAction : public QUndoCommand
{
public:
	SE_mainMenuAddInstrumentAction(){
		setText( QString( "Drop instrument" ) );
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

//~pattern editor commands
//=====================================================================================================================================
//piano roll editor commands


class SE_addNotePianoRollAction : public QUndoCommand
{
public:
	SE_addNotePianoRollAction( int nColumn,
				   int pressedLine,
				   int selectedPatternNumber,
				   int nSelectedInstrumentnumber,
				   int oldLength,
				   float oldVelocity,
				   float oldPan_L,
				   float oldPan_R,
				   float oldLeadLag,
				   int oldNoteKeyVal,
				   int oldOctaveKeyVal ){
		setText( QString( "Add piano roll note ( %1, %2 )" ).arg( nColumn ).arg( pressedLine ) );
		__nColumn = nColumn;
		__pressedLine = pressedLine;
		__selectedPatternNumber = selectedPatternNumber;
		__nSelectedInstrumentnumber = nSelectedInstrumentnumber;
		__oldLength = oldLength;
		__oldVelocity = oldVelocity;
		__oldPan_L = oldPan_L;
		__oldPan_R = oldPan_R;
		__oldLeadLag = oldLeadLag;
		__oldNoteKeyVal = oldNoteKeyVal;
		__oldOctaveKeyVal = oldOctaveKeyVal;

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
											     __oldPan_L,
											     __oldPan_R,
											     __oldLeadLag,
											     __oldNoteKeyVal,
											     __oldOctaveKeyVal,
											     false );
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
											     __oldPan_L,
											     __oldPan_R,
											     __oldLeadLag,
											     __oldNoteKeyVal,
											     __oldOctaveKeyVal,
											     false );
	}
private:
	int __nColumn;
	int __pressedLine;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
	int __oldLength;
	float __oldVelocity;
	float __oldPan_L;
	float __oldPan_R;
	float __oldLeadLag;
	int __oldNoteKeyVal;
	int __oldOctaveKeyVal;
};

class SE_addPianoRollNoteOffAction : public QUndoCommand
{
public:
	SE_addPianoRollNoteOffAction( int nColumn, int pressedLine, int selectedPatternNumber, int nSelectedInstrumentnumber ){
		setText( QString( "Add  piano roll NOTE_OFF note ( %1, %2 )" ).arg( nColumn ).arg( pressedLine ) );
		__nColumn = nColumn;
		__pressedLine = pressedLine;
		__selectedPatternNumber = selectedPatternNumber;
		__nSelectedInstrumentnumber = nSelectedInstrumentnumber;
	}
	virtual void undo()
	{
		//qDebug() << "Add off note Note Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addOrDeleteNoteAction( __nColumn, __pressedLine, __selectedPatternNumber,  __nSelectedInstrumentnumber, -1, 0.8f, 0.5f, 0.5f, 0.0, 0, 0 , true);
	}
	virtual void redo()
	{
		//qDebug() << "Add off note Note Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addOrDeleteNoteAction( __nColumn, __pressedLine, __selectedPatternNumber,  __nSelectedInstrumentnumber, -1, 0.8f, 0.5f, 0.5f, 0.0, 0, 0, true);

	}
private:
	int __nColumn;
	int __pressedLine;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
};


class SE_editPianoRollNoteLengthAction : public QUndoCommand
{
public:
	SE_editPianoRollNoteLengthAction( int nColumn, int nRealColumn, int length, int oldLength, int selectedPatternNumber, int nSelectedInstrumentnumber, int pressedLine){
		setText( QString( "Change piano roll note length " ) );
		__nColumn = nColumn;
		__nRealColumn = nRealColumn;
		__length = length;
		__oldLength = oldLength;
		__selectedPatternNumber = selectedPatternNumber;
		__nSelectedInstrumentnumber = nSelectedInstrumentnumber;
		__pressedLine = pressedLine;
	}
	virtual void undo()
	{
		//qDebug() << "Change note length Piano Roll Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->editNoteLengthAction( __nColumn, __nRealColumn, __oldLength, __selectedPatternNumber, __nSelectedInstrumentnumber,  __pressedLine);
	}
	virtual void redo()
	{
		//qDebug() << "Change note length Piano RollRedo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->editNoteLengthAction(    __nColumn,
											       __nRealColumn,
											       __length,
											       __selectedPatternNumber,
											       __nSelectedInstrumentnumber,
											       __pressedLine );
	}
private:
	int __nColumn;
	int __nRealColumn;
	int __oldLength;
	int __length;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
	int __pressedLine;
};

class SE_editNotePropertiesPianoRollAction : public QUndoCommand
{
public:
	SE_editNotePropertiesPianoRollAction(   int nColumn,
						int nRealColumn,
						int selectedPatternNumber,
						int selectedInstrumentnumber,
						float velocity,
						float oldVelocity,
						float pan_L,
						float oldPan_L,
						float pan_R,
						float oldPan_R,
						float leadLag,
						float oldLeadLag,
						int pressedLine ){
		setText( QString( "Change note properties piano roll" ) );
		__nColumn = nColumn;
		__nRealColumn = nRealColumn;
		__selectedPatternNumber = selectedPatternNumber;
		__nSelectedInstrumentnumber = selectedInstrumentnumber;
		__velocity = velocity;
		__oldVelocity = oldVelocity;
		__pan_L = pan_L;
		__oldPan_L = oldPan_L;
		__pan_R = pan_R;
		__oldPan_R = oldPan_R;
		__leadLag = leadLag;
		__oldLeadLag = oldLeadLag;
		__pressedLine = pressedLine;
	}
	virtual void undo()
	{
		//qDebug() << "Change Note properties Piano Roll Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->editNotePropertiesAction( __nColumn,
												__nRealColumn,
												__selectedPatternNumber,
												__nSelectedInstrumentnumber,
												__oldVelocity,
												__oldPan_L,
												__oldPan_R,
												__oldLeadLag,
												__pressedLine );
	}
	virtual void redo()
	{
		//qDebug() << "Change Note properties Piano RollRedo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->editNotePropertiesAction( __nColumn,
												__nRealColumn,
												__selectedPatternNumber,
												__nSelectedInstrumentnumber,
												__velocity,
												__pan_L,
												__pan_R,
												__leadLag,
												__pressedLine );
	}

private:
	int __nColumn;
	int __nRealColumn;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
	float __velocity;
	float __oldVelocity;
	float __pan_L;
	float __oldPan_L;
	float __pan_R;
	float __oldPan_R;
	float __leadLag;
	float __oldLeadLag;
	int __pressedLine;
};

//~piano roll editor commands
//=====================================================================================================================================
//Note Properties Ruler commands

class SE_editNotePropertiesVolumeAction : public QUndoCommand
{
public:

	SE_editNotePropertiesVolumeAction( int undoColumn,
					   QString mode,
					   int nSelectedPatternNumber,
					   int nSelectedInstrument,
					   float velocity,
					   float oldVelocity,
					   float pan_L,
					   float oldPan_L,
					   float pan_R,
					   float oldPan_R,
					   float leadLag,
					   float oldLeadLag,
					   int noteKeyVal,
					   int oldNoteKeyVal,
					   int octaveKeyVal,
					   int oldOctaveKeyVal)
	{


		setText( QString( "Edit note property" ) );
		__undoColumn = undoColumn;
		__mode = mode;
		__nSelectedPatternNumber = nSelectedPatternNumber;
		__nSelectedInstrument = nSelectedInstrument;
		__velocity = velocity;
		__oldVelocity = oldVelocity;
		__pan_L = pan_L;
		__oldPan_L = oldPan_L;
		__pan_R = pan_R;
		__oldPan_R = oldPan_R;
		__leadLag = leadLag;
		__oldLeadLag = oldLeadLag;
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
											__oldPan_L,
											__oldPan_R,
											__oldLeadLag,
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
											__pan_L,
											__pan_R,
											__leadLag,
											__noteKeyVal,
											__octaveKeyVal );
	}
private:


	int __undoColumn;
	QString __mode;
	int __nSelectedPatternNumber;
	int __nSelectedInstrument;
	float __velocity;
	float __oldVelocity;
	float __pan_L;
	float __oldPan_L;
	float __pan_R;
	float __oldPan_R;
	float __leadLag;
	float __oldLeadLag;
	int __noteKeyVal;
	int __oldNoteKeyVal;
	int __octaveKeyVal;
	int __oldOctaveKeyVal;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
};

//~Note Properties Ruler commands
//=====================================================================================================================================


#endif // UNDOACTIONS_H
