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

#endif // UNDOACTIONS_H
