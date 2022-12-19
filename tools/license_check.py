# Hydrogen
# Copyright(c) 2008-2022 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
#
# http://www.hydrogen-music.org
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see https://www.gnu.org/licenses

import os;
import sys;

# Recursive function scan the provided path and all subfolders and
# calls readFile() on all text files found.
def scanFolder( rootPath ):
    with os.scandir( rootPath ) as it:
        for eentry in it:
            if eentry.is_dir():
                scanFolder( eentry.path )
            elif eentry.is_file():
                readFile( eentry.path )
                

# Determines whether a provided file features a license notice or not
# and throws an exception in case it doesn't find one.
def readFile( filePath ):

    # Check whether we should check for a notice.
    for eexception in blacklist:
        if ( eexception in filePath ):
            return
    if ( '_UI.ui' in filePath ):
        return
    if ( '/tests/data/' in filePath ):
        return

    fileDescriptor = os.open( filePath, os.O_RDONLY )

    # Only the first n bytes will be used when checking for a
    # license notice
    nBytes = 1000
    fileContent = os.read( fileDescriptor, nBytes )

    checkForLicense( str( fileContent ), filePath )

    os.close( fileDescriptor )

# Check for lines corresponding to the GPLv2+ license notice.
def checkForLicense( fileContent, filePath ):
    if ( not ( "This program is free software; you can redistribute it and/or modify" in fileContent and
               "it under the terms of the GNU General Public License as published by" in fileContent and
               "the Free Software Foundation; either version 2 of the License, or" in fileContent and
               "(at your option) any later version." in fileContent and
               "This program is distributed in the hope that it will be useful," in fileContent and
               "but WITHOUT ANY WARRANTY, without even the implied warranty of" in fileContent and
               "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the" in fileContent and
               "GNU General Public License for more details." in fileContent and
               "You should have received a copy of the GNU General Public License" in fileContent and
               "along with this program. If not, see https://www.gnu.org/licenses" ) ):
        print( "no license notice found in:")
        print( fileContent )
        raise Exception( 'No license notice found in file' + filePath )

# Predefined list of files which are okay to not have a license
# notice. The main intention of this script is to detect _new_ and
# contributed source files without a notice.
blacklist = [ '/src/core/CMakeLists.txt', # none
              '/src/player/CMakeLists.txt', # none
              'src/gui/CMakeLists.txt', # none
              'src/cli/CMakeLists.txt', # none
              'src/tests/CMakeLists.txt', # none
              'src/core/IO/JackMidiDriver.cpp', # FreeBSD
              'src/core/IO/JackMidiDriver.h', # FreeBSD
              'src/core/Nsm.h', # ISC
              'src/core/config.dox', # none
              'src/core/config.h.in', # none
              'src/www/metaInfo.inc' # none
             ]

try:
    rootFolder = os.getcwd() + '/src'
    print( 'Scanning "' + rootFolder + '" for license notices' )
    scanFolder( rootFolder )
        
    print( 'Scanning done' )
    
    sys.exit( 0 )
    
except Exception as error:
    print( error )
    sys.exit( 1 )

