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
#ifndef ACTION_H
#define ACTION_H
#include <hydrogen/Object.h>

enum mmcEventType {
	MMC_STOP,
	MMC_PLAY,
	MMC_DEFERRED_PLAY,
	MMC_FAST_FWD,
	MMC_REWIND,
	MMC_RECORD_STROBE,
	MMC_RECORD_EXIT,
	MMC_PAUSE
};

class actionManager : public Object
{
	private:
		static actionManager *instance;
		QString mmcEventList[254];

	public:
		
		
		
		static actionManager* getInstance();

		bool handleAction(mmcEventType);
		void registerMMCEvent( mmcEventType , QString );

		actionManager();
		~actionManager();
};
#endif
