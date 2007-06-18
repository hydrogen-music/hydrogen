
/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include <hydrogen/Globals.h>
#include <hydrogen/Object.h>


namespace H2Core {

class ADSR;
class Sample;

/// A layer...
class InstrumentLayer : public Object
{
	public:
		float m_fStartVelocity;		///< Start velocity
		float m_fEndVelocity;		///< End velocity
		float m_fPitch;
		float m_fGain;
		Sample *m_pSample;

		InstrumentLayer( Sample *pSample );
		~InstrumentLayer();

};



/// Instrument class
class Instrument : public Object
{
	public:
		ADSR* m_pADSR;

		std::string m_sId;			///< ID of the instrument
		std::string m_sName;		///< Instrument name
		bool m_bFilterActive;	///< Is filter active?
		float m_fCutoff;			///< Filter cutoff (0..1)
		float m_fResonance;		///< Filter resonant frequency (0..1)
		float m_fRandomPitchFactor;
		bool m_bActive;		///< is the instrument active?
		float m_fVolume;		///< Volume of the instrument
		bool m_bIsMuted;
		bool m_bIsSoloed;
		float m_fPeak_L;		///< current peak value (left)
		float m_fPeak_R;		///< current peak value (right)
		float m_fPan_L;			///< Pan of the instrument (left)
		float m_fPan_R;			///< Pan of the instrument (right)
		std::string m_sDrumkitName;	///< Drumkit name
		float m_fGain;
		int m_nMuteGroup;		///< Mute group
		float m_fFXLevel[MAX_FX];	///< Ladspa FX level

		Instrument(
				const std::string& sId = "",
				const std::string& sName = "",
				float fVolume = 1.0,
				bool bMuted = false,
				float fPan_L = 1.0,
				float fPan_R = 1.0,
				const std::string& sDrumkitName = ""
		);
		~Instrument();


		InstrumentLayer* getLayer( int nLayer );
		void setLayer( InstrumentLayer* pLayer, unsigned nLayer );

		static Instrument* loadInstrument( const std::string& sDrumkitName, const std::string& sInstrumentName );

	private:
		InstrumentLayer* m_layers[MAX_LAYERS];
};


// Instrument List
class InstrumentList : public Object {
	public:
		InstrumentList();
		~InstrumentList();

		void add(Instrument* pInstrument);
		Instrument* get(unsigned int pos);
		int getPos(Instrument* inst);
		unsigned int getSize();

		void del( int pos );

		void replace( Instrument* pNewInstr, unsigned nPos );

	private:
		std::vector<Instrument*> m_list;
		std::map<Instrument*, unsigned> m_posmap;
};

};

#endif


