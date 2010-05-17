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

#ifndef H2_DRUMKIT_H
#define H2_DRUMKIT_H

#include <hydrogen/Object.h>
#include <hydrogen/xml_helper.h>
#include <hydrogen/instrument.h>

namespace H2Core
{

/**
\ingroup H2Core
\brief	Drumkit info
*/
class Drumkit : public Object {
    H2_OBJECT
    public:
        /** \brief drumkit constructor, does nothing */
        Drumkit();
        /** \brief drumkit destructor, delete__ instruments */
	    ~Drumkit();
        
        /** \brief load drumkit information from a path
         * \param dk_path like one returned by Filesystem::drumkit_path
         * \return a Drumkit on success, 0 otherwise
         */
        static Drumkit* load( const QString& dk_path );
        /// Save a drumkit using given parameters and current song drumkit
        static bool save( const QString& name, const QString& author, const QString& info, const QString& license );
        /// Installs a drumkit
        static bool install( const QString& filename );
        /// Remove a drumkit from the disk
        static bool removeDrumkit( const QString& name );

        /** \brief set __instruments, delete existing one */
        void setInstrumentList( InstrumentList* l ) { if(__instruments) { delete __instruments; } __instruments = l; }
        InstrumentList* getInstrumentList()         { return __instruments; }   ///< returns __instrumetns

        void setPath( const QString& path )         { __path = path; }          ///< sets __path
        const QString& getPath()                    { return __path; }          ///< returns __path

        void setName( const QString& name )         { __name = name; }          ///< sets __name
        const QString& getName()                    { return __name; }          ///< returns __name
        
        void setAuthor( const QString& author )     { __author = author; }      ///< sets __author
        const QString& getAuthor()                  { return __author; }        ///< returns __author
        
        void setInfo( const QString& info )         { __info = info; }          ///< sets __info
        const QString& getInfo()                    { return __info; }          ///< returns __info
        
        void setLicense( const QString& license )   { __license = license; }    ///< sets __license
        const QString& getLicense()                 { return __license; }       ///< returns __license

        void dump();
    
    private:
        QString __path;                 ///< absolute drumkit path
        QString __name;                 ///< drumkit name
        QString __author;               ///< drumkit author
        QString __info;                 ///< drumkit free text
        QString __license;              ///< drumkit license description
        InstrumentList *__instruments;  ///< the list of instruments
        /// xml related save operations
        bool save( );
        static Drumkit* load_from( XMLNode* node );
};

};

#endif // H2_DRUMKIT_H

/* vim: set softtabstop=4 expandtab: */
