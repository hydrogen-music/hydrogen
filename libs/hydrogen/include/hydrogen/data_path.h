/* Hydrogen
 * Copyright(c) 2002-200/ Alessandro Cominu
 *
 * http://www.hydrogen-music.org
 *
 * DataPath.h
 * Header to define the path to the data files for Hydrogen in such a
 * way that self-contained Mac OS X application bundles can be built.
 * Copyright (c) 2005 Jonathan Dempsey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef DATA_PATH_H
#define DATA_PATH_H

#include <QtCore>

namespace H2Core
{

class DataPath
{
public:
	static QString get_data_path();

private:
	static QString __data_path;
};

};

#endif

