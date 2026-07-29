/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#ifndef EDITOR_PATH_EXERCISER_H
#define EDITOR_PATH_EXERCISER_H

#include <QObject>
#include <QTimer>

#include <core/Preferences/Shortcuts.h>
#include "MainForm.h"

/**
 * \ingroup docGUI
 *
 * EditorPathExerciser (ADR 0033)
 *
 * Drives every safe Shortcuts::Action through MainForm::executeShortcut in
 * editor (mirror) mode. The goal is to surface any code path that reaches an
 * ASSERT_NO_EDITOR_MODE site — which calls assert(false) and SIGABRTs the
 * process. Because the assert kills the process, the test is out-of-process:
 * the harness spawns the GUI with --exercise-editor-paths, and if the GUI
 * crashes (non-zero exit / signal), the test fails.
 *
 * Actions that open modal file dialogs, quit the application, or launch
 * external programs are skipped — they would block the headless event loop.
 *
 * The exerciser dispatches one action per QTimer tick so the event loop
 * processes between actions (some actions post events or rely on the loop).
 * After the last action, it quits the application cleanly.
 */
class EditorPathExerciser : public QObject {
	Q_OBJECT

public:
	explicit EditorPathExerciser( MainForm* pMainForm, QObject* pParent = nullptr );
	~EditorPathExerciser() override = default;

	void start();

private slots:
	void exerciseNextAction();

private:
	/** Build the ordered list of actions to exercise, skipping unsafe ones. */
	void buildActionList();

	/** Provide safe default arguments for the given action. */
	ShortcutArgs defaultArgsFor( H2Core::Shortcuts::Action action ) const;

	/** Human-readable name for logging. */
	static QString actionName( H2Core::Shortcuts::Action action );

	MainForm* m_pMainForm;
	QTimer m_timer;
	std::vector<H2Core::Shortcuts::Action> m_actions;
	std::size_t m_nIndex = 0;
};

#endif // EDITOR_PATH_EXERCISER_H