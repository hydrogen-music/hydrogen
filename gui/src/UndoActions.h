#ifndef UNDOACTIONS_H
#define UNDOACTIONS_H

#include <QtGui>
#include <QDebug>
#include <QUndoCommand>
#include <QPoint>

#include "HydrogenApp.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"
#include "PatternFillDialog.h"

#include "PatternEditor/NotePropertiesRuler.h"
#include "PatternEditor/DrumPatternEditor.h"
#include "PatternEditor/PatternEditorPanel.h"
#include "PatternEditor/NotePropertiesRuler.h"


//=====================================================================================================================================
//test commands
class TestAction : public QUndoCommand
{
public:
    TestAction( const QString &text)
	:  m_text(text) { setText("append text"); }
    virtual void undo()
	{ qDebug() << "Undo " << m_text; }
    virtual void redo()
	{ qDebug() << "Redo " <<  m_text; }
private:
    QString m_text;
};

//~test commands
//=====================================================================================================================================
//song editor commands
class SE_addPatternAction : public QUndoCommand
{
public:
    SE_addPatternAction( int nColumn, int nRow, unsigned nColumnIndex ){
	setText( QString( "Add Pattern ( %1, %2 )" ).arg( nColumn ).arg( nRow ) );
	//setText("add Pattern");
	__nColumn = nColumn;
	__nRow = nRow;
	__nColumnIndex = nColumnIndex;
    }
    virtual void undo()
	{
	    qDebug() << "add Pattern Undo ";
	    HydrogenApp* h2app = HydrogenApp::get_instance();
	    h2app->getSongEditorPanel()->getSongEditor()->deletePattern( __nColumn, __nRow , __nColumnIndex );
	}
    virtual void redo()
	{
	    qDebug() << "add Pattern Redo " ;
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
//	setText("delete Pattern");
	__nColumn = nColumn;
	__nRow = nRow;
	__nColumnIndex = nColumnIndex;
    }
    virtual void undo()
	{
	    qDebug() << "Delete pattern Undo ";
	    HydrogenApp* h2app = HydrogenApp::get_instance();
	    h2app->getSongEditorPanel()->getSongEditor()->addPattern( __nColumn, __nRow );
	}
    virtual void redo()
	{
	    qDebug() << "Delete pattern Redo " ;
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
	setText( QString( "Move Pattern List Item ( %1, %2 )" ).arg( nSourcePattern ).arg( nTargetPattern ) );
//	setText("delete Pattern");
	__nSourcePattern = nSourcePattern;
	__nTargetPattern = nTargetPattern;
    }
    virtual void undo()
	{
	    qDebug() << "Move Pattern List Item Undo ";
	    HydrogenApp* h2app = HydrogenApp::get_instance();
	    h2app->getSongEditorPanel()->getSongEditorPatternList()->movePatternLine( __nTargetPattern, __nSourcePattern );
	}
    virtual void redo()
	{
	    qDebug() << "Move Pattern List Item redo " ;
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
		qDebug() << "Delete complete pattern-sequence  undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->restoreGroupVector( __pFilename );
	}

    virtual void redo()
	{
		qDebug() << "Delete complete pattern-sequence redo " ;
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
		qDebug() << "Delete pattern from list undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->restoreDeletedPatternsFromList( __patternFilename, __sequenceFileName, __patternPosition );
		h2app->getSongEditorPanel()->restoreGroupVector( __sequenceFileName );
		h2app->getSongEditorPanel()->getSongEditor()->updateEditorandSetTrue();
	}

    virtual void redo()
	{
		qDebug() << "Delete pattern from list redo" ;
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
    SE_modifyPatternPropertiesAction(  QString oldPatternName , QString oldPatternCategory, QString newPatternName , QString newPatternCategory, int patternNr ){
	setText( QString( "Modify pattern properties" ) );
	__oldPatternName =  oldPatternName;
	__oldPatternCategory = oldPatternCategory;
	__newPatternName =  newPatternName;
	__newPatternCategory = newPatternCategory;
	__patternNr = patternNr;
    }
    virtual void undo()
	{
		qDebug() << "Modify pattern properties undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->revertPatternPropertiesDialogSettings( __oldPatternName, __oldPatternCategory, __patternNr );
	}

    virtual void redo()
	{
		qDebug() << "Modify pattern properties redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->acceptPatternPropertiesDialogSettings( __newPatternName, __newPatternCategory, __patternNr );
	}
private:
	QString __oldPatternName;
	QString __oldPatternCategory;
	QString __newPatternName;
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
		qDebug() << "copy pattern undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->revertaddEmptyPattern( __patternPosition );
	}

    virtual void redo()
	{
		qDebug() << "copy pattern redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->patternPopup_copyAction( __patternFilename );
	}
private:
	QString __patternFilename;
	int __patternPosition;
};


class SE_addEmptyPatternAction : public QUndoCommand
{
public:
    SE_addEmptyPatternAction(  QString newPatternName , QString newPatternCategory , int patternPosition){
	setText( QString( "Add pattern" ) );
	__newPatternName =  newPatternName;
	__newPatternCategory = newPatternCategory;
	__patternPosition = patternPosition;
    }
    virtual void undo()
	{
		qDebug() << "Add pattern undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->revertaddEmptyPattern( __patternPosition );
	}

    virtual void redo()
	{
		qDebug() << "Add pattern redo" ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->addEmptyPattern( __newPatternName ,__newPatternCategory, __patternPosition );
	}
private:
	QString __newPatternName;
	QString __newPatternCategory;
	int __patternPosition;
};

class SE_loadPatternAction : public QUndoCommand
{
public:
    SE_loadPatternAction(  QString patternName, QString oldPatternName, QString sequenceFileName, int patternPosition){
	setText( QString( "Load/drag pattern" ) );
	__patternName =  patternName;
	__oldPatternName = oldPatternName;
	__sequenceFileName = sequenceFileName;
	__patternPosition = patternPosition;
    }
    virtual void undo()
	{
		qDebug() << "Load/drag pattern undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->restoreDeletedPatternsFromList( __oldPatternName, __sequenceFileName, __patternPosition );
		h2app->getSongEditorPanel()->revertaddEmptyPattern( __patternPosition +2 );
		h2app->getSongEditorPanel()->restoreGroupVector( __sequenceFileName );
		h2app->getSongEditorPanel()->getSongEditor()->updateEditorandSetTrue();
	}

    virtual void redo()
	{
		qDebug() <<  "Load/drag pattern redo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPatternList()->deletePatternFromList( __oldPatternName, __sequenceFileName, __patternPosition );
		h2app->getSongEditorPanel()->getSongEditorPatternList()->loadPatternAction( __patternName, __patternPosition  );
	}
private:
	QString __patternName;
	QString __oldPatternName;
	QString __sequenceFileName;
	int __patternPosition;
};


class SE_fillRangePatternAction : public QUndoCommand
{
public:
    SE_fillRangePatternAction( FillRange* pRange, int nPattern ){
	setText( QString( "fill/remove range of pattern" ) );
	__pRange = pRange;
	__from = pRange->fromVal;
	__to = pRange->toVal;
	__bInsert = pRange->bInsert;
	__nPattern = nPattern;
    }
    virtual void undo()
	{
		qDebug() << "fill/remove range of undo";
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
		qDebug() <<  "fill/remove range of redo";
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
    SE_movePatternCellAction( std::vector<QPoint> movingCells, std::vector<QPoint> selectedCells, bool bIsCtrlPressed ){
	setText( QString( "move/copy selected cells" ) );
	__selectedCells = selectedCells;
	__movingCells = movingCells;
	__bIsCtrlPressed = bIsCtrlPressed;
	
    }
    virtual void undo()
	{
		qDebug() <<  "move/copy selected cells undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->movePatternCellAction( __selectedCells, __movingCells, __bIsCtrlPressed, true );
	}

    virtual void redo()
	{
		qDebug() <<  "move/copy selected cells redo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditor()->movePatternCellAction( __movingCells, __selectedCells, __bIsCtrlPressed, false );
	}
private:
	std::vector<QPoint> __selectedCells;
	std::vector<QPoint> __movingCells;
	bool __bIsCtrlPressed;
};

class SE_editTimeLineAction : public QUndoCommand
{
public:
    SE_editTimeLineAction( int newPosition, float oldBpm, float newBpm ){
	setText( QString( "edit timeline tempo" ) );
	__newPosition = newPosition;
	__oldBpm = oldBpm;
	__newBpm = newBpm;
	
    }
    virtual void undo()
	{
		qDebug() <<  "edit timeline tempo undo";
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
		qDebug() <<  "edit timeline tempo redo";
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
	setText( QString( "delete timeline tempo" ) );
	__newPosition = newPosition;
	__oldBpm = oldBpm;
	
    }
    virtual void undo()
	{
		qDebug() <<  "delete timeline tempo undo";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getSongEditorPanel()->getSongEditorPositionRuler()->editTimeLineAction( __newPosition, __oldBpm );
		
	}

    virtual void redo()
	{
		qDebug() <<  "delete timeline tempo redo";
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
	setText( "edit timeline tag" );
	__text = text;
	__oldText = oldText;
	__position = position;
	
    }
    virtual void undo()
	{
		qDebug() <<  "edit timeline tag undo";
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
		qDebug() <<  "edit timeline tag redo";
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
			   int oldOctaveKeyVal ){
	setText( QString( "Add Note ( %1, %2)" ).arg( nColumn ).arg( nRow ) );
	//setText("add Pattern");
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
	}
	virtual void undo()
	{
		qDebug() << "Add note Undo ";
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
												__oldOctaveKeyVal );
	} 
	virtual void redo()
	{
		qDebug() << "Add Note Redo " ;
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
												__oldOctaveKeyVal );
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
};


class SE_addNoteRightClickAction : public QUndoCommand
{
public:
	SE_addNoteRightClickAction( int nColumn, int nRow, int selectedPatternNumber ){
	setText( QString( "Add off note Note ( %1, %2 )" ).arg( nColumn ).arg( nRow ) );
	//setText("add Pattern");
	__nColumn = nColumn;
	__nRow = nRow;
	__selectedPatternNumber = selectedPatternNumber;
	}
	virtual void undo()
	{
		qDebug() << "Add off note Note Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->addOrDeleteNoteAction( __nColumn, __nRow, __selectedPatternNumber, -1, 0.8f, 0.5f, 0.5f, 0.0, 0, 0 );
	}
	virtual void redo()
	{
		qDebug() << "Add off note Note Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->addNoteRightClickAction( __nColumn, __nRow, __selectedPatternNumber );
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
		qDebug() << "Change note length Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->editNoteLenghtAction( __nColumn,  __nRealColumn, __row, __oldLength, __selectedPatternNumber );
	}
	virtual void redo()
	{
		qDebug() << "Change note length Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getDrumPatternEditor()->editNoteLenghtAction( __nColumn,  __nRealColumn, __row, __length, __selectedPatternNumber );
	}
private:
	int __nColumn;
	int __nRealColumn;
	int __row;
	int __oldLength;
	int __length;
	int __selectedPatternNumber;
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
	setText( QString( "Add Piano Roll Note ( %1, %2 )" ).arg( nColumn ).arg( pressedLine ) );
	//setText("add Pattern");
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
		qDebug() << "Add Piano Roll note Undo ";
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
											     __oldOctaveKeyVal );
	}
	virtual void redo()
	{
		qDebug() << "Add Piano Roll Note Redo " ;
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
											     __oldOctaveKeyVal );
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

class SE_addNoteRightClickPianoRollAction : public QUndoCommand
{
public:
	SE_addNoteRightClickPianoRollAction( int nColumn, int pressedLine, int selectedPatternNumber, int nSelectedInstrumentnumber ){
	setText( QString( "Add off note Note ( %1, %2 )" ).arg( nColumn ).arg( pressedLine ) );
	//setText("add Pattern");
	__nColumn = nColumn;
	__pressedLine = pressedLine;
	__selectedPatternNumber = selectedPatternNumber;
	__nSelectedInstrumentnumber = nSelectedInstrumentnumber;
	}
	virtual void undo()
	{
		qDebug() << "Add off note Note Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addOrDeleteNoteAction( __nColumn, __pressedLine, __selectedPatternNumber,  __nSelectedInstrumentnumber, -1, 0.8f, 0.5f, 0.5f, 0.0, 0, 0 );
	}
	virtual void redo()
	{
		qDebug() << "Add off note Note Redo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->addNoteRightClickAction( __nColumn, __pressedLine, __selectedPatternNumber,  __nSelectedInstrumentnumber);
	}
private:
	int __nColumn;
	int __pressedLine;
	int __selectedPatternNumber;
	int __nSelectedInstrumentnumber;
};


class SE_editNoteLenghtPianoRollAction : public QUndoCommand
{
public:
	SE_editNoteLenghtPianoRollAction( int nColumn, int nRealColumn, int length, int oldLength, int selectedPatternNumber, int nSelectedInstrumentnumber, int pressedLine){
	setText( QString( "Change note length Piano Roll" ) );
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
		qDebug() << "Change note length Piano Roll Undo ";
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->editNoteLenghtAction( __nColumn, __nRealColumn, __oldLength, __selectedPatternNumber, __nSelectedInstrumentnumber,  __pressedLine);
	}
	virtual void redo()
	{
		qDebug() << "Change note length Piano RollRedo " ;
		HydrogenApp* h2app = HydrogenApp::get_instance();
		h2app->getPatternEditorPanel()->getPianoRollEditor()->editNoteLenghtAction(    __nColumn,
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
	setText( QString( "Change Note properties Piano Roll" ) );
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
		qDebug() << "Change Note properties Piano Roll Undo ";
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
		qDebug() << "Change Note properties Piano RollRedo " ;
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


	setText( QString( "edit note property" ) );
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
		qDebug() << "edit note property Undo ";
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
		qDebug() << "edit note property Redo " ;
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
