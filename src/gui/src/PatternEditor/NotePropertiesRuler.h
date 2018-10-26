/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef NOTE_PROPERTIES_RULER_H
#define NOTE_PROPERTIES_RULER_H

#include "../EventListener.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <hydrogen/object.h>
#include <hydrogen/basics/note.h>

namespace H2Core
{
	class Pattern;
	class NoteKey;
}

class PatternEditorPanel;

class NotePropertiesRuler : public QWidget, public H2Core::Object, public EventListener
{
    H2_OBJECT
	Q_OBJECT
	public:
	        NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, H2Core::NotePropertiesMode mode );
		~NotePropertiesRuler();

		void zoomIn();
		void zoomOut();

		//public slots:
		void updateEditor();

	private:
		static const int m_nKeys = 24;
		static const int m_nBasePitch = 12;
	
		/**
		 * Contains all properties, which can be changed using
		 * the NotePropertiesRuler.
		 *
		 * In addition, the H2Core::Note::__octave can be
		 * changed as well. This case is covered by NOTEKEY
		 * and fulfills the same specifications.
		 */
	        H2Core::NotePropertiesMode m_Mode;
		/**
		 * This struct will contain all relevant properties of
		 * a note for the NotePropertiesRuler and will be
		 * assigned BEFORE the actions triggered by the user
		 * will take place. The particular changes will
		 * therefore not be reflected in it yet.
		 *
		 * It will be used as a convenient way to streamline
		 * the setting of the different properties and to
		 * construct a H2Core::NotePropertiesChanges struct.
		 *
		 * Note that Hydrogen does not store the pattern id of
		 * a note properly during its creation. It will be
		 * added manually using the global variable
		 * __nSelectedPatternNumber.
		 */
		H2Core::NoteProperties noteProperties;
		/**
		 * This struct contains the former state of the
		 * H2Core::NoteProperties, which will be used to
		 * construct the undo/redo action in
		 * pushUndoAction. Why could we not just use the state
		 * prior to action invoked by the user? Moving the
		 * cursor does trigger a lot of actions and it would
		 * tedious of the user to revert them all one by
		 * one. Instead, we will keep track of the state prior
		 * to all movements of the mouse in this global struct
		 * and update it only when the mouse cursor moves to a
		 * different note.
		 *
		 * Note that Hydrogen does not store the pattern id of
		 * a note properly during its creation. It will be
		 * added manually using the global variable
		 * __nSelectedPatternNumber.
		 */
		H2Core::NoteProperties notePropertiesOld;
		/**
		 * Auxiliary object the new state (AFTER the action
		 * took place) of the note will be assigned
		 * to. Unfortunately, the pattern id is not properly
		 * set a note is introduced into the pattern editor
		 * and to assure backwards compatibility, it shouldn't
		 * be assumed to be present either. Therefore, we have
		 * to assign it manually instead of just using
		 * Note::get_note_properties function during the
		 * construction of NotePropertiesChanges.
		 *
		 * Note that Hydrogen does not store the pattern id of
		 * a note properly during its creation. It will be
		 * added manually using the global variable
		 * __nSelectedPatternNumber.
		 */
		H2Core::NoteProperties notePropertiesNew;
		/**
		 * This struct will contain all relevant properties of
		 * a note both BEFORE and AFTER the action triggered
		 * by the user is performed. In addition, it will also
		 * contain a H2Core::NotePropertiesMode enumerator
		 * specifying the property, which was actually changed.
		 */
		H2Core::NotePropertiesChanges notePropertiesChanges;

		PatternEditorPanel *m_pPatternEditorPanel;
		H2Core::Pattern *m_pPattern;
		float m_nGridWidth;
		uint m_nEditorWidth;
		uint m_nEditorHeight;

		QPixmap *m_pBackground;
	
		void createVelocityBackground(QPixmap *pixmap);
		void createPanBackground(QPixmap *pixmap);
		void createLeadLagBackground(QPixmap *pixmap);
		void createNoteKeyBackground(QPixmap *pixmap);
		void paintEvent(QPaintEvent *ev);
		/**
		 * Function triggered when pressing the left button of
		 * the mouse.
		 * \param ev Corresponding mouse event.
		 */
		void mousePressEvent(QMouseEvent *ev);
		/**
		 * Function triggered when moving the mouse. This will
		 * only be called inside the
		 * NotePropertiesRuler::mousePressEvent. The user has
		 * therefore to press the left mouse button and move
		 * the mouse at the same time to alter the properties
		 * of a note.
		 * \param ev Corresponding mouse event.
		 */
		void mouseMoveEvent(QMouseEvent *ev);
		/**
		 * Function triggered when moving the mouse wheel.
		 * \param ev Corresponding mouse event.
		 */
		void wheelEvent(QWheelEvent *ev);
		/**
		 * Function triggered when releasing the left button of
		 * the mouse.
		 * \param ev Corresponding mouse event.
		 */
		void mouseReleaseEvent(QMouseEvent *ev);
		/**
		 * Takes a list of explicit changes of the properties
		 * of at least one note and pushes them onto the
		 * QUndoStack. This way Hydrogen is able to revert all
		 * changes introduced by the user.
		 *
		 * The creation of the action, which is able to revert
		 * the changes, is done using
		 * SE_editNotePropertiesAction. 
		 *
		 * \param propertyChangesStack List of
		 * H2Core::NotePropertiesChanges applied to at least
		 * one note. If the SHIFT key was not pressed, the
		 * list will contain only a single object.
		 */
	        void pushUndoAction( std::list<H2Core::NotePropertiesChanges> propertyChangesStack );
		/**
		 * Called within NotePropertiesRuler::mousePressEvent
		 * to access the current state of the note before
		 * triggering its change using
		 * NotePropertiesRuler::mouseMoveEvent. 
		 * \param x Used to access the note the user is
		 * pointing at.
		 * \param y Used to access the current value of the
		 * property.
		 */
		void pressAction( int x, int y);

		// Implements EventListener interface
		virtual void selectedPatternChangedEvent();
	        virtual void selectedInstrumentChangedEvent();
		//~ Implements EventListener interface
		/**
		 * Global variable holding the id of the currently
		 * selected pattern. Unfortunately, Hydrogen does not
		 * store the pattern id into a note during its
		 * creation. We have to keep track of it using this
		 * variable and assign it manually to the
		 * H2Core::NoteProperties in order to make the
		 * undo/redo work.
		 */
		int __nSelectedPatternNumber;
		bool m_bMouseIsPressed; ///< Indicates whether the
					///< left mouse button is
					///< pressed by the user.
		int __checkXPosition;
	
		int __columnCheckOnXmouseMouve;
		int __undoColumn; ///< x-coordinate of an altered note
				  ///< within the ruler.

};


#endif
