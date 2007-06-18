#ifndef CUI_H
#define CUI_H

#include "lib/Song.h"

class CUI {
	public:
		CUI(string sSongFilename);
		~CUI();

		void handleKey( char ch );

	private:
		enum UI_mode {
			MENU_MODE,
			PATTERN_EDITOR_MODE,
			HELP_MODE
		};

		UI_mode m_mode;

		int m_nCursor_x;
		int m_nCursor_y;

		void printInfo();
		void showHelp();
		void moveCursor();
		void showPatternEditor();
};

#endif

