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
		/** null element indicating that no action was set*/
		Null = 0,
		Panic,
		Save
	};

	/** Scope the shortcut is applicable to*/
	enum class Category {
		/**
		 * Those shortcuts are fixed and can not be edited by the user
		 * in the #PreferencesDialog.*/
		HardCoded = 0,
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
		Director = 8
	};
	
	/** Some context for an #Action */
	struct ActionInfo {
		Category category;
		QString sDescription;
	};
	
	Shortcuts();
	~Shortcuts();

	void saveTo( XMLNode* pNode );
	static std::shared_ptr<Shortcuts> loadFrom( XMLNode* pNode, bool bSilent = false );

	/**
	 * Creates the default key bindings as fallback when upgrading
	 * from older versions or starting Hydrogen the first time.
	 */
	void createDefaultShortcuts();

	std::vector<Action> getActions( int nKey ) const;
	ActionInfo getActionInfo( Action action ) const;

	const std::map<int, std::vector<Action>> getActionsMap() const;
	
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const;

private:

	void createActionInfoMap();
	void insertShortcut( int nKey, Action action );
	void insertActionInfo( Action action, Category category, const QString& sDescription );

	std::map<Action, ActionInfo> m_actionInfoMap;
	std::map<int, std::vector<Action>> m_actionsMap;
};

inline const std::map<int, std::vector<Shortcuts::Action>> Shortcuts::getActionsMap() const {
	return m_actionsMap;
}
};

#endif
