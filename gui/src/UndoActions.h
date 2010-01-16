#ifndef UNDOACTIONS_H
#define UNDOACTIONS_H

#include <QtGui>
#include <QDebug>
#include <QUndoCommand>

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

#endif // UNDOACTIONS_H
