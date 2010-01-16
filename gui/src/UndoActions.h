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
    SE_addPatternAction( int nColumn, int nRow){
	setText("add Pattern");
	__nColumn = nColumn;
	__nRow = nRow;
    }
    virtual void undo()
	{
	    qDebug() << "Undo not implemented yet.. ";
	    //HydrogenApp* h2app = HydrogenApp::get_instance();
	    //h2app->getSongEditorPanel()->getSongEditor()->addPattern( nColumn, nRow );
	}
    virtual void redo()
	{
	    qDebug() << "Redo " ;
	    HydrogenApp* h2app = HydrogenApp::get_instance();
	    h2app->getSongEditorPanel()->getSongEditor()->addPattern( __nColumn, __nRow );
	}
private:
    int __nColumn;
    int __nRow;
};

#endif // UNDOACTIONS_H
