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
		/**
		 * Changing the resolution of the current view by
		 * multiplying the (private) m_nGridWidth by 2 (for
		 * values larger than 3) or by 1.5 (for all other
		 * values). Afterwards, the updateEditor function will
		 * be called.
		 */
		void zoomIn();
		/**
		 * Changing the resolution of the current view by
		 * dividing the (private) m_nGridWidth by 2 (for
		 * values larger than 3), by 1.5 (for values between
		 * 1.5 and 3), or keeping the width for values smaller
		 * than 1.5. Afterwards, the updateEditor function will
		 * be called.
		 */
		void zoomOut();
		/**
		 * Updating the view of the note property ruler.
		 *
		 * Firstly, it will calculate both the width and the
		 * height of the new view. Afterwards, it deletes the
		 * old (and private) m_pBackground object and creates
		 * a new one. This will then be used draw the ruler
		 * for one of the properties using either the
		 * createVeloctyBackground, createPanBackground,
		 * createLeadLagBackground, or createNoteKeyBackground
		 * (private) function. Finally, the QWidget::update,
		 * which is inherited by the NotePropertiesRuler
		 * object, will be called redraw the current view.
		 */
		void updateEditor();

	private:
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
		/**
		 * Pointer to the PatternEditorPanel provided upon
		 * initializing the NotePropertiesRuler. It will be
		 * used to obtain a pointer to the DrumPatternEditor
		 * using PatternEditorPanel::getDrumPatternEditor and
		 * to update the PianoRollEditor using
		 * PatternEditorPanel::getPianoRollEditor.
		 */
		PatternEditorPanel *m_pPatternEditorPanel;
		/**
		 * Pointer to the currently selected
		 * H2Core::Pattern. It will be used to get the
		 * individual notes, whose properties will be altered
		 * using the NotePropertiesRuler. 
		 */
		H2Core::Pattern *m_pPattern;
		/**
		 * Basic width of the grid in ruler in pixel. It can
		 * be altered by the using the zoomIn and zoomOut
		 * functions.
		 */
		uint m_nGridWidth;
		/**
		 * Width of the editor panel in pixel. It will be
		 * calculated by a constant offset of 20 + width of
		 * the grid in the pattern editor
		 * (H2Core::Preferences::m_nPatternEditorGridHeight)
		 * times biggest number of notes possible (#MAX_NOTES,
		 * which is defined during the compilation of
		 * Hydrogen) time four (basic number of notes a usual
		 * beat).
		 */
		uint m_nEditorWidth;
		/**
		 * Height of the editor panel in pixel. For the
		 * velocity, pan, lead&lag, and probability property
		 * it will be set to 100px. For the note and octave
		 * key 210px will be used instead.
		 */
		uint m_nEditorHeight;

		/**
		 * Pixel-based background of note properties
		 * ruler. It will be deleted and created anew every
		 * time the updateEditor function is called. Its
		 * height is determined by m_nEditorHeight and its
		 * width will be calculated anew using the length of
		 * the current pattern times the width of the grid
		 * (m_nGridWidth) plus a constant offset of 20.
		 */
		QPixmap *m_pBackground;
		/**
		 * Pixel map containing the background of the velocity
		 * OR probability property drawn on top of the
		 * m_pBackground.
		 * /param pixmap Pointer to the background the
		 * properties will be drawn on.
		 */
		void createVelocityBackground(QPixmap *pixmap);
		/**
		 * Pixel map containing the background of the pan
		 * property drawn on top of the m_pBackground.
		 * /param pixmap Pointer to the background the
		 * properties will be drawn on.
		 */
		void createPanBackground(QPixmap *pixmap);
		/**
		 * Pixel map containing the background of the lead&lag
		 * property drawn on top of the m_pBackground.
		 * /param pixmap Pointer to the background the
		 * properties will be drawn on.
		 */
		void createLeadLagBackground(QPixmap *pixmap);
		/**
		 * Pixel map containing the background of the note and
		 * octave key properties drawn on top of the
		 * /param pixmap Pointer to the background the
		 * properties will be drawn on.
		 * m_pBackground.
		 */
		void createNoteKeyBackground(QPixmap *pixmap);
		/**
		 * Not called directly but used to set up some things
		 * for the QPainter.
		 */
		void paintEvent(QPaintEvent *ev);
		/**
		 * Uses the x coordinate of the current mouse position
		 * and the current width of the grid (m_nGridWidth) to
		 * determine the nearest note and to return its x
		 * coordinate, which will be called "column" in the
		 * functions of the NotePropertiesRuler.
		 * \param X coordinate of the mouse obtained either by
		 * QMouseEvent::x or QMouseEvent::y.
		 */
		int getColumn( int mousePositionX );
		/**
		 * Editing of a property of a single note triggered by
		 * the usage of the mouse wheel.
		 *
		 * Using the global variable m_Mode the function will
		 * first determine which property to change the begin
		 * with. It calculates the new value by adding
		 * increment delta and issues a message in the status
		 * bar using
		 * HydrogenApp::setStatusBarMessage. Internally the
		 * global limits of the properties, like #PAN_MIN or
		 * #PROBABILITY_MAX, will be used to assure value in
		 * the printed message is legit. The setting itself is
		 * already using safeguards to ensure proper value.
		 *
		 * \param noteProperties Struct containing all
		 * editable properties of the note pNote at the time
		 * this function is called.
		 * \param pNote Pointer to the note, which property is
		 * going to be changed.
		 * \param delta Increment of the property value. Its
		 * sign will be determined by direction the users
		 * spins the mouse wheel. If the CTRL key is pressed,
		 * this value will be 0.01. Else 0.05 will be used.
		 */
		void editPropertyMouseWheel( H2Core::NoteProperties noteProperties,
					     H2Core::Note *pNote, float delta );
		/**
		 * Editing of a property of a single note triggered by
		 * the usage of the mouse click and a possible mouse
		 * movement.
		 *
		 * Using the global variable m_Mode the function will
		 * first determine which property to change the begin
		 * with. It calculates assigns the new value according
		 * to the y coordinate of the mouse cursor and issues
		 * a message in the status bar using
		 * HydrogenApp::setStatusBarMessage.
		 *
		 * \param noteProperties Struct containing all
		 * editable properties of the note pNote at the time
		 * this function is called.
		 * \param pNote Pointer to the note, which property is
		 * going to be changed.
		 * \param val Floating point version of keyval in the
		 * range between 0.0 and 1.0. It will be used to
		 * calculate/assign the new values of the velocity,
		 * panning, lead&lag, and probability properties.
		 * \param keyval y Coordinate of the cursor position
		 * in pixel. Only the area of the ruler will be used
		 * and thus value is ranging from 0 and
		 * QWidget::height, which NotePropertiesRuler
		 * inherits.
		 * \param ev Mouse event from which the pressing of
		 * the middle button or the CTRL key will be
		 * accessed. Both trigger a resetting of the
		 * corresponding values.
		 */
		void editPropertyMouseClick( H2Core::NoteProperties noteProperties,
					     H2Core::Note *pNote, float val, int keyval,
					     QMouseEvent *ev );
					     
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

		/**
		 * Interface to the EventListener. This function will
		 * call updateEditor every time the pattern is changed
		 * by the user within Hydrogen.
		 */
		virtual void selectedPatternChangedEvent();
		/**
		 * Interface to the EventListener. This function will
		 * call updateEditor every time the instrument is
		 * changed by the user within Hydrogen.
		 */
	        virtual void selectedInstrumentChangedEvent();
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
		int __columnCheckOnXmouseMouve;
		int __undoColumn; ///< x-coordinate of an altered note
				  ///< within the ruler.

};


#endif
