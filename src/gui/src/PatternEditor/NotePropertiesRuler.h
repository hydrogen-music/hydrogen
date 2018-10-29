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
		/**
		 * Holds a panel to manually alter the properties of
		 * individual notes.
		 *
		 * The properties can basically be changed in two
		 * different ways; either by an incremental increase
		 * or decrease using the mouse wheel or by inserting a
		 * specific value with a mouse click and movement. For
		 * both types pressing the Shift key will allow the
		 * user to apply the changes to all notes in the
		 * pattern. Pressing CTRL in combination with the
		 * mouse wheel leads to an even more fine-grained
		 * control. 
		 */
		NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, H2Core::NotePropertiesMode mode );
		/**
		 * Destructor
		 */
		~NotePropertiesRuler();
		/**
		 * Changing the resolution of the current view by
		 * multiplying the (private) #m_nGridWidth by 2 (for
		 * values larger than 3) or by 1.5 (for all other
		 * values). Afterwards, the updateEditor() function will
		 * be called.
		 */
		void zoomIn();
		/**
		 * Changing the resolution of the current view by
		 * dividing the (private) #m_nGridWidth by 2 (for
		 * values larger than 3), by 1.5 (for values between
		 * 1.5 and 3), or keeping the width for values smaller
		 * than 1.5. Afterwards, the updateEditor() function will
		 * be called.
		 */
		void zoomOut();
		/**
		 * Updating the view of the note property ruler.
		 *
		 * Firstly, it will calculate both the width and the
		 * height of the new view. Afterwards, it deletes the
		 * old (and private) #m_pBackground object and creates
		 * a new one. This will then be used draw the ruler
		 * for one of the properties using either the
		 * createVelocityBackground(), createPanBackground(),
		 * createLeadLagBackground(), or createNoteKeyBackground()
		 * (private) function. Finally, the QWidget::update(),
		 * which is inherited by the NotePropertiesRuler()
		 * object, will be called redraw the current view.
		 */
		void updateEditor();

	private:
		/**
		 * Contains all properties, which can be changed using
		 * the NotePropertiesRuler.
		 *
		 * In addition, the H2Core::Note::__octave can be
		 * changed as well. This case is covered by
		 * H2Core::NOTEKEY and fulfills the same
		 * specifications.
		 */
	        H2Core::NotePropertiesMode m_Mode;
		/**
		 * This struct will contain all relevant properties of
		 * a note for the NotePropertiesRuler and will be
		 * assigned BEFORE the actions triggered by the user
		 * will take place. The particular changes will
		 * therefore not be reflected in it.
		 *
		 * It will be used as a convenient way to streamline
		 * the setting of the different properties and to
		 * construct a H2Core::NotePropertiesChanges
		 * struct. In addition, it will be also used as the
		 * basic element for the #notePropertiesStored list.
		 *
		 * Note that Hydrogen does not store the pattern id of
		 * a note properly during its creation. It will be
		 * added manually using the global variable
		 * #__nSelectedPatternNumber.
		 */
		H2Core::NoteProperties noteProperties;
		/**
		 * Object the new state (AFTER the action took place)
		 * of the note will be assigned to. Unfortunately, the
		 * pattern id is not properly set a note is introduced
		 * into the pattern editor and to assure backwards
		 * compatibility, it shouldn't be assumed to be
		 * present either. Therefore, we have to assign it
		 * manually using the global variable
		 * #__nSelectedPatternNumber instead of just using
		 * H2Core::Note::get_note_properties() function during the
		 * construction of H2Core::NotePropertiesChanges.
		 */
		H2Core::NoteProperties notePropertiesNew;
		/**
		 * This struct will contain all relevant properties of
		 * a note both BEFORE and AFTER the group of actions
		 * triggered by the user is performed. In addition, it
		 * will also contain a H2Core::NotePropertiesMode
		 * enumerator specifying the property, which was
		 * actually changed. It will serve as basic element
		 * for #propertyChangesStack.
		 */
		H2Core::NotePropertiesChanges notePropertiesChanges;
		/**
		 * Whenever a property of a note is altered using the
		 * mousePressEvent() and mouseMoveEvent(), all changes
		 * between the pressing of the mouse button and its
		 * release will be grouped together and considered
		 * belonging to the same intended action of the
		 * user. This way she does not have to tediously
		 * revert all individual changes when moving a
		 * property in the ruler up and down. 
		 *
		 * The changes will be written using the
		 * storeNoteProperties() function and the list is clear
		 * every time the function is entered. Depending on
		 * whether or not the Shift key is pressed by the user
		 * the list will be of length one or the number of
		 * notes for the selected instrument in the current
		 * pattern.
		 *
		 * It will be used in undoMouseMovement() to construct
		 * the undo/redo actions.
		 */
		std::list<H2Core::NoteProperties> notePropertiesStored;
		/**
		 * List, which will contain the changes applied to the
		 * individual notes using the struct
		 * H2Core::NotePropertiesChanges.
		 *
		 * Whenever the note properties are altered using the
		 * mouse wheel in wheelEvent(), the changes will be
		 * constructed from the H2Core::NoteProperties prior
		 * and after the action of the user was performed. In
		 * case of altering the properties using the mouse
		 * button via mousePressEvent() and mouseMoveEvent(), a
		 * grouping of the action will take place. The old
		 * state of the properties will be determined by the
		 * #notePropertiesStored list, which is filled in
		 * storeNoteProperties(), and corresponding new states
		 * are obtained in undoMouseMovement() function.
		 * Depending on whether or not the Shift key is
		 * pressed by the user the list will be of length one
		 * or the number of notes for the selected instrument
		 * in the current pattern.
		 *
		 * It contains all ingredient to construct the
		 * undo/redo actions, which is done by handing it to
		 * the pushUndoAction() function.
		 */
		std::list<H2Core::NotePropertiesChanges> propertyChangesStack;
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
		/**
		 * Global variable storing the current state of the
		 * Shift key.
		 *
		 * It has three different states:
		 *  - > 0 - the shift key is pressed 
		 *  - < 0 - the shift key is not pressed
		 *  - == 0 - the mouse key was just pressed the first time
		 *        and there are no old state available.
		 *
		 * It will be used in the mousePressEvent() function to
		 * determine whether or not to store the current
		 * note properties and/or to trigger a undo/redo
		 * action and it will be reset in the mouseReleaseEvent().
		 */
		int __nShiftKeyState;
		/**
		 * Global variable storing the current state of the
		 * column.
		 *
		 * It has two different states:
		 * - >=  0 - current column.
		 * - == -1 - the mouse key was just pressed the first time
		 *        and there are no old state available.
		 *
		 * It will be used in the mouseMoveEvent() function to
		 * determine whether or not to store the current
		 * note properties and/or to trigger a undo/redo
		 * action and it will be reset in the mouseReleaseEvent().
		 */
		int __nColumnState;
		/**
		 * Pointer to the PatternEditorPanel provided upon
		 * initializing the NotePropertiesRuler. It will be
		 * used to obtain a pointer to the DrumPatternEditor
		 * using PatternEditorPanel::getDrumPatternEditor() and
		 * to update the PianoRollEditor using
		 * PatternEditorPanel::getPianoRollEditor().
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
		 * be altered by the using the zoomIn() and zoomOut()
		 * functions.
		 */
		uint m_nGridWidth;
		/**
		 * Width of the editor panel in pixel. It will be
		 * calculated by a constant offset of 20 + width of
		 * the grid in the pattern editor
		 * (H2Core::Preferences::m_nPatternEditorGridHeight)
		 * times biggest number of notes possible (MAX_NOTES,
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
		 * time the updateEditor() function is called. Its
		 * height is determined by #m_nEditorHeight and its
		 * width will be calculated anew using the length of
		 * the current pattern times the width of the grid
		 * (#m_nGridWidth) plus a constant offset of 20.
		 */
		QPixmap *m_pBackground;
		/**
		 * Pixel map containing the background of the velocity
		 * (H2Core::VELOCITY) OR probability property
		 * (H2Core::PROBABILITY) drawn on top of the
		 * #m_pBackground.
		 * \param pixmap Pointer to the background the
		 * properties will be drawn on.
		 */
		void createVelocityBackground(QPixmap *pixmap);
		/**
		 * Pixel map containing the background of the pan
		 * (H2Core::PAN) property drawn on top of the
		 * #m_pBackground.
		 * \param pixmap Pointer to the background the
		 * properties will be drawn on.
		 */
		void createPanBackground(QPixmap *pixmap);
		/**
		 * Pixel map containing the background of the lead&lag
		 * property (H2Core::LEADLAG) drawn on top of the
		 * #m_pBackground.
		 * \param pixmap Pointer to the background the
		 * properties will be drawn on.
		 */
		void createLeadLagBackground(QPixmap *pixmap);
		/**
		 * Pixel map containing the background of the note and
		 * octave key properties (H2Core::NOTEKEY) drawn on top
		 * of the #m_pBackground.
		 * \param pixmap Pointer to the background the
		 * properties will be drawn on.
		 */
		void createNoteKeyBackground(QPixmap *pixmap);
		/**
		 * Not called directly but used to set up some things
		 * for the QPainter.
		 */
		void paintEvent(QPaintEvent *ev);
		/**
		 * Uses the x coordinate of the current mouse position
		 * and the current width of the grid (#m_nGridWidth) to
		 * determine the nearest note and to return its x
		 * coordinate, which will be called "column" in the
		 * functions of the NotePropertiesRuler.
		 * \param X coordinate of the mouse obtained either by
		 * QMouseEvent::x() or QMouseEvent::y().
		 */
		int getColumn( int mousePositionX );
		/**
		 * Editing of a property of a single note triggered by
		 * the usage of the mouse wheel.
		 *
		 * Using the global variable #m_Mode the function will
		 * first determine which property to change the begin
		 * with. It calculates the new value by adding the
		 * increment delta and issues a message in the status
		 * bar using
		 * HydrogenApp::setStatusBarMessage(). Internally the
		 * global limits of the properties, like #PAN_MIN or
		 * #PROBABILITY_MAX, will be used to assure the values
		 * in the printed messages are legit. The setting of
		 * the properties themselves is already secure using
		 * safeguards to ensure proper value.
		 *
		 * \param noteProperties Struct containing all
		 * editable properties of the note pNote at the time
		 * this function is called.
		 * \param pNote Pointer to the note, which property is
		 * going to be changed.
		 * \param delta Increment of the property value. Its
		 * sign will be determined by direction the users
		 * spins the mouse wheel. If the CTRL key is pressed,
		 * its value will be 0.01. Else 0.05 will be used.
		 */
		void editPropertyMouseWheel( H2Core::NoteProperties noteProperties,
					     H2Core::Note *pNote, float delta );
		/**
		 * Editing of a property of a single note triggered by
		 * the usage of the mouse click and a possible mouse
		 * movement.
		 *
		 * Using the global variable #m_Mode the function will
		 * first determine which property to change the begin
		 * with. It calculates and assigns the new value
		 * according to the y coordinate of the mouse cursor
		 * and issues a message in the status bar using
		 * HydrogenApp::setStatusBarMessage().
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
		 * and thus its value is ranging from 0 and
		 * QWidget::height(), which NotePropertiesRuler
		 * inherits.
		 * \param ev Mouse event from which the pressing of
		 * the middle button or the CTRL key will be
		 * accessed. Both trigger a resetting of the
		 * corresponding values of the panning and lead&lag.
		 */
		void editPropertyMouseClick( H2Core::NoteProperties noteProperties,
					     H2Core::Note *pNote, float val, int keyval,
					     QMouseEvent *ev );
		/**
		 * Editing note properties using the mouse wheel. 
		 *
		 * If the sole mouse wheel without any modifiers is
		 * used, the value of the velocity, probability, pan,
		 * or lead&lag will be altered by 0.05. Pressing the
		 * CTRL key will enable a more fine-grained control of
		 * +/- 0.01. Pressing the SHIFT, either additionally
		 * or exclusively, will apply the changes to all notes
		 * in the pattern.
		 *
		 * All changes associated to one user action will be
		 * used to build a undo/redo action in
		 * pushUndoAction(). This will, unfortunately, result in
		 * a rather tedious incremental undo/redo
		 * procedure. But since the movement of the mouse
		 * wheel lacks nice delimiters, like the press and
		 * release action in case of the mouse movements,
		 * there is (for now) no other way to implement it
		 * (without modifying the QUndoStack).
		 *
		 * \param ev Corresponding mouse event.
		 */
		void wheelEvent(QWheelEvent *ev);
					     
		/**
		 * Editing the note properties using a mouse click and
		 * movement. 
		 *
		 * This function will be the entry point for the
		 * editing of the note properties using the mouse
		 * button. It will call the mouseMoveEvent() to ensure
		 * the user is only altering the properties when the
		 * button is pressed.
		 *
		 * \param ev Corresponding mouse event.
		 */
		void mousePressEvent(QMouseEvent *ev);
		/**
		 * Editing the note properties using a mouse click and
		 * movement. 
		 *
		 * It will only be called inside the mousePressEvent()
		 * function. The user has therefore to press the left
		 * mouse button and move the mouse at the same time to
		 * alter the note properties.
		 *
		 * The y coordinate of the cursor will be used as the
		 * new value assigned to the selected note
		 * property. If in addition the CTRL key was pressed,
		 * the changes do affect all notes of the selected
		 * instrument in the current pattern. But moving the
		 * mouse up and down will produce a lot of actions,
		 * which would render the corresponding undo/redo
		 * actions quite tedious. Instead, all changes
		 * introduced during the pressing and the release of
		 * the mouse button will be grouped into a single
		 * action.
		 *
		 * But apart from the physical release of the mouse
		 * key, two events will be also considered to be the
		 * end of one group and the onset of the next: the
		 * pressing of Shift key and the movement of the
		 * cursor to a different note. The latter ensures all
		 * changes applied to a single note can be undone or
		 * redone in a single action and the user can edit
		 * multiple individual notes without releasing the
		 * mouse button. The #__nColumnState global variable
		 * will be used to keep track of the column the
		 * changes were applied in. The pressing or releasing
		 * of the shift key will alter the #__nShiftKeyState
		 * and allows the user to seamlessly switch between
		 * the single note and multiple note editing and at
		 * the same time to undo/redo all changes in a proper
		 * way. 
		 *
		 * \param ev Corresponding mouse event.
		 */
		void mouseMoveEvent(QMouseEvent *ev);
		/**
		 * Function triggered when releasing the left button of
		 * the mouse.
		 *
		 * It will be used to group the changes applied via
		 * the mouseMoveEvent() into a single undo/redo action
		 * using the undoMouseMovement() function. To clean the
		 * stage for the next pressing of the mouse buttons,
		 * it will also reset the global variables
		 * #__nShiftKeyState to 0 and #__nColumnState to -1.
		 *
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
		 * SE_editNotePropertiesAction(). 
		 *
		 * \param propertyChangesStack List of
		 * H2Core::NotePropertiesChanges applied to at least
		 * one note. If the Shift key was not pressed, the
		 * list will contain only a single object.
		 */
	        void pushUndoAction( std::list<H2Core::NotePropertiesChanges> propertyChangesStack );
		/**
		 * Writes the current state of the note properties
		 * into the global variable notePropertiesStored().
		 *
		 * Firstly, it will clear all entries in the
		 * #notePropertiesStored and then fill it with the note
		 * properties of either a single or all notes for the
		 * selected instrument in the current pattern,
		 * determined by the #__nShiftKeyState variable. Since
		 * Hydrogen does not properly assign pattern index for
		 * newly created notes, it will be inserted manually
		 * using the global variable #__nSelectedPatternNumber.
		 *
		 * \param ev Mouse event using which the position of
		 * the selected note will be obtained.
		 */
		void storeNoteProperties( QMouseEvent* ev );
		/**
		 * Grouping multiple mouseMoveEvent() into a single
		 * undo/redo action. 
		 *
		 * This function will use the #notePropertiesStored
		 * list as the old, and the current properties of the
		 * corresponding notes as the new states and
		 * constructs H2Core::NotePropertiesChanges. These
		 * changes will be pushed on the #propertyChangesStack
		 * list, which is cleared every time this function is
		 * entered. Finally, the list is handed to
		 * pushUndoAction() to create the undo/redo
		 * actions. Since Hydrogen does not properly assign
		 * pattern index for newly created notes, it will be
		 * inserted manually using the global variable
		 * #__nSelectedPatternNumber.
		 */
		void undoMouseMovement();
		/**
		 * Interface to the EventListener. This function will
		 * call updateEditor() every time the pattern is changed
		 * by the user within Hydrogen.
		 */
		virtual void selectedPatternChangedEvent();
		/**
		 * Interface to the EventListener. This function will
		 * call updateEditor() every time the instrument is
		 * changed by the user within Hydrogen.
		 */
	        virtual void selectedInstrumentChangedEvent();

};


#endif
