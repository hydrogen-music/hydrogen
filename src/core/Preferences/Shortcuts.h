/*
 * Hydrogen
 * Copyright(c) 2023-2023 The hydrogen development team
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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef SHORTCUTS_H
#define SHORTCUTS_H

#include <QString>
#include <QKeySequence>
#include <map>
#include <vector>
#include <memory>

#include <core/Object.h>

namespace H2Core {

class XMLNode;

class Shortcuts : public H2Core::Object<Shortcuts> {
	H2_OBJECT(Shortcuts)

public:
	enum class Action {
		Panic					 = 100,
		Save					 = 101,
		SaveAs					 = 102,
		Undo					 = 103,
		Redo					 = 104,
		TogglePlayback			 = 105,
		TogglePlaybackAtCursor	 = 106,
		BeatCounter				 = 107,
		TapTempo				 = 108,
		BPMIncrease				 = 109,
		BPMDecrease				 = 110,
		JumpToStart				 = 111,
		JumpBarForward			 = 112,
		JumpBarBackward			 = 113,
		PlaylistNextSong		 = 114,
		PlaylistPrevSong		 = 115,

		// Virtual MIDI keyboard
		VK_C2					 = 400,
		VK_C_sharp2				 = 401,
		VK_D2					 = 402,
		VK_D_sharp2				 = 403,
		VK_E2					 = 404,
		VK_F2					 = 405,
		VK_F_sharp2				 = 406,
		VK_G2					 = 407,
		VK_G_sharp2				 = 408,
		VK_A2					 = 409,
		VK_A_sharp2				 = 410,
		VK_B2					 = 411,
		VK_C3					 = 412,
		VK_C_sharp3				 = 413,
		VK_D3					 = 414,
		VK_D_sharp3				 = 415,
		VK_E3					 = 416,
		VK_F3					 = 417,
		VK_F_sharp3				 = 418,
		VK_G3					 = 419,
		VK_G_sharp3				 = 420,
		VK_A3					 = 421,
		VK_A_sharp3				 = 422,
		VK_B3					 = 423,
		
		/** null element indicating that no action was set*/
		Null					 = 1000
	};

	/** Scope the shortcut is applicable to*/
	enum class Category {
		/**
		 * Those shortcuts are fixed and can not be edited by the user
		 * in the #PreferencesDialog.*/
		HardCoded = -1,
		None = 0,
		/** Enabled in all windows of Hydrogen */
		Global = 1,
		MainWindow = 2,
		/** Enabled in both pattern and song editor*/
		Editors = 3,
		/** Shortcuts associated with the virtual keyboard */
		VirtualKeyboard = 4,
		Mixer = 5,
		PlaylistEditor = 6,
		SampleEditor = 7,
		Director = 8,
		/** Not intended to be assigned to a shortcut but used for
			filtering instead.*/
		All = 100
	};
	static QString categoryToQString( Category category );
	
	/** Some context for an #Action */
	struct ActionInfo {
		Category category;
		QString sDescription;
	};
	
	Shortcuts();
	Shortcuts( const std::shared_ptr<Shortcuts> pOther );
	~Shortcuts();

	void saveTo( XMLNode* pNode );
	static std::shared_ptr<Shortcuts> loadFrom( XMLNode* pNode, bool bSilent = false );

	/**
	 * Creates the default key bindings as fallback when upgrading
	 * from older versions or starting Hydrogen the first time.
	 *
	 * If no QApplication is present, Qt will segfault when attempting
	 * to access standard keys as done within this functions. It has
	 * to be used with care.
	 */
	void createDefaultShortcuts();

	std::vector<Action> getActions( QKeySequence keySequence ) const;
	/**
	 * Removes a single shortcut from #m_actionsMap.
	 *
	 * A Key can be assigned to multilpe actions but a specific action
	 * just to a single key. 
	 */
	void deleteShortcut( Action action );
	void insertShortcut( QKeySequence keySequence, Action action );
	ActionInfo getActionInfo( Action action ) const;

	QKeySequence getKeySequence( Action action ) const;

	const std::map<Action, ActionInfo> getActionInfoMap() const;

	bool requiresDefaults() const;
	
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const;

private:

	void createActionInfoMap();
	void insertActionInfo( Action action, Category category, const QString& sDescription );

	std::map<Action, ActionInfo> m_actionInfoMap;
	std::map<QKeySequence, std::vector<Action>> m_actionsMap;

	/**
	 * 
	 */
	bool m_bRequiresDefaults;
};

inline const std::map<Shortcuts::Action, Shortcuts::ActionInfo> Shortcuts::getActionInfoMap() const {
	return m_actionInfoMap;
}
inline bool Shortcuts::requiresDefaults() const {
	return m_bRequiresDefaults;
}
};

#endif
