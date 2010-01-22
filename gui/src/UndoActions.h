#ifndef UNDOACTIONS_H
#define UNDOACTIONS_H

#include <QtGui>
#include <QDebug>
#include <QUndoCommand>

#include "HydrogenApp.h"
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"

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
#endif // UNDOACTIONS_H
