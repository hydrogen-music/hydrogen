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
		// Command-based shortcuts with 0 arguments
		FirstWith0Args			 	=   0,
		Panic					 	=   1,
		Play					 	=   2,
		Pause					 	=   3,
		Stop					 	=   4,
		PlayPauseToggle			 	=   5,
		PlayStopToggle			 	=   6,
		PlayPauseToggleAtCursor	 	=   7,

		RecordReady				 	=   8,
		RecordStrobe			 	=   9,
		RecordStrobeToggle		 	=  10,
		RecordExit				 	=  11,

		MasterMute				 	=  12,
		MasterUnmute			 	=  13,
		MasterMuteToggle		 	=  14,
		MasterVolumeIncrease	 	=  15,
		MasterVolumeDecrease	 	=  16,

		JumpToStart				 	=  17,
		JumpBarForward			 	=  18,
		JumpBarBackward			 	=  19,

		BPMIncreaseCoarse		 	=  20,
		BPMDecreaseCoarse		 	=  21,
		BPMIncreaseFine			 	=  22,
		BPMDecreaseFine			 	=  23,

		BeatCounter				 	=  24,
		TapTempo				 	=  25,

		PlaylistNextSong		 	=  26,
		PlaylistPrevSong		 	=  27,

		TimelineToggle			 	=  28,
		MetronomeToggle			 	=  29,
		JackTransportToggle		 	=  30,
		JackTimebaseToggle		 	=  31,
		SongModeToggle			 	=  32,
		LoopModeToggle			 	=  33,

		LoadNextDrumkit			 	=  34,
		LoadPrevDrumkit			 	=  35,

		CountIn						=  36,
		CountInPauseToggle			=  37,
		CountInStopToggle			=  38,

		LastWith0Args			 	=  99,

		// Command-based shortcuts with 1 arguments
		FirstWith1Args			 	= 100,
		BPM						 	= 101,
		MasterVolume			 	= 102,
		JumpToBar				 	= 103,

		SelectNextPattern		 	= 104,
		SelectOnlyNextPattern	 	= 105,
		SelectAndPlayPattern	 	= 106,

		PlaylistSong			 	= 107,

		TimelineDeleteMarker	 	= 108,
		TimelineDeleteTag		 	= 109,

		SelectInstrument		 	= 110,
		StripVolumeIncrease		 	= 112,
		StripVolumeDecrease		 	= 113,
		StripMuteToggle			 	= 114,
		StripSoloToggle			 	= 115,

		LastWith1Args			 	= 199,

		// Command-based shortcuts with 2 arguments
		FirstWith2Args			 	= 200,
		StripVolume				 	= 201,
		StripPan				 	= 202,
		StripFilterCutoff		 	= 203,

		TimelineAddMarker		 	= 204,
		TimelineAddTag			 	= 205,

		ToggleGridCell			 	= 206,

		LastWith2Args			 	= 299,

		// Command-based shortcuts with 3 arguments
		FirstWithManyArgs		 	= 300,
		LayerPitch				 	= 301,
		LayerGain				 	= 302,
		
		StripEffectLevel		 	= 303,

		LastWithManyArgs		 	= 399,

		// Virtual MIDI keyboard
		VK_36_C2				 	= 400,
		VK_37_C_sharp2			 	= 401,
		VK_38_D2				 	= 402,
		VK_39_D_sharp2			 	= 403,
		VK_40_E2				 	= 404,
		VK_41_F2				 	= 405,
		VK_42_F_sharp2			 	= 406,
		VK_43_G2				 	= 407,
		VK_44_G_sharp2			 	= 408,
		VK_45_A2				 	= 409,
		VK_46_A_sharp2			 	= 410,
		VK_47_B2				 	= 411,
		VK_48_C3				 	= 412,
		VK_49_C_sharp3			 	= 413,
		VK_50_D3				 	= 414,
		VK_51_D_sharp3			 	= 415,
		VK_52_E3				 	= 416,
		VK_53_F3				 	= 417,
		VK_54_F_sharp3			 	= 418,
		VK_55_G3				 	= 419,
		VK_56_G_sharp3			 	= 420,
		VK_57_A3				 	= 421,
		VK_58_A_sharp3			 	= 422,
		VK_59_B3				 	= 423,

		// MainForm actions
		NewSong					 	= 500,
		OpenSong				 	= 501,
		EditSongProperties		 	= 502,
		OpenDemoSong			 	= 503,
		SaveSong				 	= 504,
		SaveAsSong				 	= 505,
		OpenPattern				 	= 506,
		ExportPattern			 	= 507,
		ExportSong				 	= 508,
		ExportMIDI				 	= 509,
		ExportLilyPond			 	= 510,
		Quit					 	= 511,

		Undo					 	= 512,
		Redo					 	= 513,
		ShowUndoHistory			 	= 514,

		NewDrumkit				 	= 515,
		OpenDrumkit				 	= 516,
		EditDrumkitProperties	 	= 517,
		SaveDrumkitToSoundLibrary	= 518,
		SaveDrumkitToSession		= 519,
		ExportDrumkit			 	= 520,
		ImportDrumkit			 	= 521,
		ImportOnlineDrumkit		 	= 522,

		AddInstrument			 	= 523,
		ClearAllInstruments		 	= 524,
		AddComponent			 	= 525,

		ShowPlaylist			 	= 526,
		ShowDirector			 	= 527,
		ShowMixer				 	= 528,
		ShowRack		 	= 529,
		ShowAutomation			 	= 530,
		ShowTimeline			 	= 531,
		ShowPlaybackTrack		 	= 532,
		ShowFullscreen			 	= 533,

		InputInstrument			 	= 534,
		InputDrumkit			 	= 535,
		ShowPreferencesDialog	 	= 536,

		ShowAudioEngineInfo		 	= 537,
		ShowFilesystemInfo		 	= 538,
		LogLevelNone			 	= 539,
		LogLevelError			 	= 540,
		LogLevelWarning			 	= 541,
		LogLevelInfo			 	= 542,
		LogLevelDebug			 	= 543,
		OpenLogFile				 	= 544,
		DebugPrintObjects		 	= 545,

		OpenManual				 	= 546,
		ShowAbout				 	= 547,
		ShowReportBug			 	= 548,
		ShowDonate				 	= 549,

		// Playlist editor
		PlaylistAddSong			 	= 600,
		PlaylistAddCurrentSong	 	= 601,
		PlaylistRemoveSong		 	= 602,
		NewPlaylist				 	= 603,
		OpenPlaylist			 	= 604,
		SavePlaylist			 	= 605,
		SaveAsPlaylist			 	= 606,

		PlaylistAddScript	 	 	= 607,
		PlaylistEditScript		 	= 608,
		PlaylistRemoveScript	 	= 609,
		PlaylistCreateScript	 	= 610,

		/** null element indicating that no action was set*/
		Null					 	= 1000
	};

	/** Scope the shortcut is applicable to*/
	enum class Category {
		None = -1,
		CommandNoArgs = 0,
		/** Enabled in all windows of Hydrogen */
		Command1Args = 1,
		Command2Args = 2,
		/** Core commands */
		CommandManyArgs = 3,
		/** Shortcuts associated with the virtual keyboard */
		VirtualKeyboard = 4,
		MainMenu = 5,
		PlaylistEditor = 6,
		/** Not intended to be assigned to a shortcut but used for
			filtering instead.*/
		All = 100
	};
	static QString categoryToQString( const Category& category );
	
	/** Some context for an #Action */
	struct ActionInfo {
		Category category;
		QString sDescription;
	};
	
	Shortcuts();
	Shortcuts( const std::shared_ptr<Shortcuts> pOther );
	~Shortcuts();

	void saveTo( XMLNode& node ) const;
	static std::shared_ptr<Shortcuts> loadFrom( const XMLNode& node, bool bSilent = false );

	/**
	 * Creates the default key bindings as fallback when upgrading
	 * from older versions or starting Hydrogen the first time.
	 *
	 * If no QApplication is present, Qt will segfault when attempting
	 * to access standard keys as done within this functions. It has
	 * to be used with care.
	 */
	void createDefaultShortcuts();

	std::vector<Action> getActions( const QKeySequence& keySequence ) const;
	/**
	 * Removes mapping between @a keySequence and @a action in
	 * #m_actionsMap.
	 */
	void deleteShortcut( const QKeySequence& keySequence, const Action& action );
	void insertShortcut( const QKeySequence& keySequence, const Action& action );
	ActionInfo getActionInfo( const Action& action ) const;

	/**
	 * Returns the first (or primary) key sequence mapped to an @a
	 * action.
	 *
	 * It is possible to map several shortcuts to a single action but,
	 * however, it is not possible to display more than one as hint in
	 * the generated GUI menus. Only the first one is used for this
	 * purpose.
	 */
	QKeySequence getKeySequence( const Action& action ) const;

	/**
	 * Returns all key sequences mapped to an @a action.
	 */
	std::vector<QKeySequence> getKeySequences( const Action& action ) const;

	const std::map<Action, ActionInfo>& getActionInfoMap() const;

	bool requiresDefaults() const;
	
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:

	void createActionInfoMap();
	void insertActionInfo( const Action& action,
						   const Category& category,
						   const QString& sDescription );

	std::map<Action, ActionInfo> m_actionInfoMap;
	std::map<QKeySequence, std::vector<Action>> m_actionsMap;

	/**
	 * 
	 */
	bool m_bRequiresDefaults;
};

inline const std::map<Shortcuts::Action, Shortcuts::ActionInfo>& Shortcuts::getActionInfoMap() const {
	return m_actionInfoMap;
}
inline bool Shortcuts::requiresDefaults() const {
	return m_bRequiresDefaults;
}
};

#endif
