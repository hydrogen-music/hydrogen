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
		 * Caution: In the current implementation not the
		 * whole state of a note in H2Core::NoteProperties
		 * corresponds to its actual one. Only those
		 * associated with the property specified by the mode
		 * variable within the H2Core::NotePropertiesChanges
		 * can be trusted. All others are just taken from
		 * global variables inside
		 * NotePropertiesRuler::wheelEvent or
		 * NotePropertiesRuler::mousePressEvent.
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
		int __nSelectedPatternNumber; ///< Specifies the current
					      ///< (visible) pattern.
		int __nSelectedInstrument; ///< Specifies the instrument
					   ///< selected in the current
					   ///< view.
		bool m_bMouseIsPressed; ///< Indicates whether the
					///< left mouse button is
					///< pressed by the user.
		float __velocity; ///< Velocity the corresponding
				  ///< instrument is hit with or, in
				  ///< other words, loudness of the
				  ///< note AFTER an action of the
				  ///< user was applied. Ranges from
				  ///< #VELOCITY_MIN to #VELOCITY_MAX. 
		float __oldVelocity; ///< Velocity the corresponding
				     ///< instrument is hit with or, in
				     ///< other words, loudness of the
				     ///< note BEFORE an action of the
				     ///< user was applied.. Ranges from
				     ///< #VELOCITY_MIN to #VELOCITY_MAX. 
		float __pan_L; ///< Volume of the left stereo channel
			       ///< AFTER an action of the user was
			       ///< applied. Ranges from #PAN_MIN to
			       ///< #PAN_MAX.
		float __pan_R; ///< Volume of the right stereo channel
			       ///< AFTER an action of the user was
			       ///< applied. Ranges from #PAN_MIN to
			       ///< #PAN_MAX.
		float __oldPan_L; ///< Volume of the left stereo
				  ///< channel BEFORE an action of the
				  ///< user was applied. Ranges from
				  ///< #PAN_MIN to #PAN_MAX.
		float __oldPan_R; ///< < Volume of the right stereo
				  ///< channel BEFORE an action of the
				  ///< user was applied. Ranges from
				  ///< #PAN_MIN to #PAN_MAX.
		float __leadLag; ///< How much the note is leading or
				 ///< lagging the beat AFTER an action
				 ///< of the user was applied. Ranges
				 ///< from #LEAD_LAG_MIN to
				 ///< #LEAD_LAG_MAX.
		float __oldLeadLag; ///< How much the note is leading or
				    ///< lagging the beat BEFORE an
				    ///< action of the user was
				    ///< applied. Ranges from
				    ///< #LEAD_LAG_MIN to #LEAD_LAG_MAX.
		float __probability; ///< Probability of the note being
				     ///< triggered AFTER an action of
				     ///< the user was applied. Ranges
				     ///< from 0 to 1. 
		float __oldProbability; ///< Probability of the note
					///< being triggered BEFORE an
					///< action of the user was
					///< applied. Ranges from 0 to 1.
		int __noteKeyVal; ///< Note key of the corresponding
				  ///< Midi output AFTER an action of
				  ///< the user was applied. Ranges from
				  ///< #KEY_MIN to #KEY_MAX.
		int __oldNoteKeyVal; ///< Note key of the corresponding
				     ///< Midi output BEFORE an action
				     ///< of the user was
				     ///< applied. Ranges from #KEY_MIN 
				     ///< to #KEY_MAX. 
		int __octaveKeyVal; ///< Octave key of the corresponding
				    ///< Midi output AFTER an action of
				    ///< the user was applied. Ranges
				    ///< from #OCTAVE_MIN to
				    ///< #OCTAVE_MAX, has an offset of
				    ///< #OCTAVE_OFFSET, and a default
				    ///< value of #OCTAVE_DEFAULT.
		int __oldOctaveKeyVal; ///< Octave key of the
				       ///< corresponding Midi output
				       ///< AFTER an action of the user
				       ///< was applied. Ranges from
				       ///< #OCTAVE_MIN to #OCTAVE_MAX,
				       ///< has an offset of
				       ///< #OCTAVE_OFFSET, and a
				       ///< default value of
				       ///< #OCTAVE_DEFAULT.
		int __checkXPosition;
	
		int __columnCheckOnXmouseMouve;
		int __undoColumn; ///< x-coordinate of an altered note
				  ///< within the ruler.
		QString __mode; ///< Corresponds to
				///< H2Core::NotePropertiesMode and
				///< specifies the note property
				///< currently displayed in the ruler.

};


#endif
