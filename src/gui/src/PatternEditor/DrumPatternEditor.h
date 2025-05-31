/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */


#ifndef DRUM_PATTERN_EDITOR_H
#define DRUM_PATTERN_EDITOR_H

#include "../Selection.h"
#include "PatternEditor.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Helpers/Filesystem.h>

#include <QtGui>
#include <QtWidgets>

///
/// Drum pattern editor
///
/** \ingroup docGUI*/
class DrumPatternEditor : public PatternEditor,
						  public H2Core::Object<DrumPatternEditor>
{
    H2_OBJECT(DrumPatternEditor)
	Q_OBJECT

	public:
		DrumPatternEditor( QWidget* parent );
		~DrumPatternEditor();

		void moveCursorDown( QKeyEvent* ev, Editor::Step step );
		void moveCursorUp( QKeyEvent* ev, Editor::Step step );

		// Selected notes are indexed by their address to ensure that a
		// note is definitely uniquely identified. This carries the risk
		// that state pointers to deleted notes may find their way into
		// the selection.
		std::vector<SelectionIndex> elementsIntersecting( const QRect& r ) override;

	public slots:
		void updateEditor( bool bPatternOnly = false ) override;
		void selectAll() override;

	private:
		void createBackground() override;

		void paintEvent(QPaintEvent *ev) override;

		QString renameCompo( const QString& OriginalName );
};


#endif
