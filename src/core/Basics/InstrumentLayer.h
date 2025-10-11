/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef H2C_INSTRUMENT_LAYER_H
#define H2C_INSTRUMENT_LAYER_H

#include <memory>
#include <core/Object.h>
#include <core/License.h>

namespace H2Core
{

	class XMLNode;
	class Sample;

	/**
	 * InstrumentLayer is part of an instrument
	 * <br>each layer has it's own :
	 * <br><b>gain</b> which is the ration between the input sample and the output signal,
	 * <br><b>pitch</b> which allows you to play the sample at a faster or lower frequency,
	 * <br><b>start velocity</b> and <b>end velocity</b> which allows you to chose between a layer or another within an instrument
	 * by changing the velocity of the played note. so the only layer of an instrument should start at 0.0 and end at 1.0.
	 */
	/** \ingroup docCore docDataStructure */
	class InstrumentLayer : public H2Core::Object<InstrumentLayer>
	{
		H2_OBJECT(InstrumentLayer)
		public:
		/** constructor
		 * \param pSample the sample to use. It won't be deep-copied but just
		 *   assigned.
		 * */
		InstrumentLayer( std::shared_ptr<Sample> pSample );
		/** copy constructor
		 *
		 * The #H2Core::Sample file contained in @a pOther will be deep-copied
		 * too.
		 *
		 * \param other the instrument layer to copy from
		 */
		InstrumentLayer( std::shared_ptr<InstrumentLayer> pOther );
		/** copy constructor
		 * \param pOther the instrument layer to copy from
		 * \param pSample the sample to use. It won't be deep-copied but just
		 *   assigned.
		 */
		InstrumentLayer( std::shared_ptr<InstrumentLayer> pOther,
						 std::shared_ptr<Sample> pSample );
		/** destructor */
		~InstrumentLayer();

		/** set the gain of the layer */
		void setGain( float gain );
		/** get the gain of the layer */
		float getGain() const;
		/** set the pitch of the layer */
		void setPitch( float pitch );
		/** get the pitch of the layer */
		float getPitch() const;

		/** set the start ivelocity of the layer */
		void setStartVelocity( float start );
		/** get the start velocity of the layer */
		float getStartVelocity() const;
		/** set the end velocity of the layer */
		void setEndVelocity( float end );
		/** get the end velocity of the layer */
		float getEndVelocity() const;

		void				setIsMuted( bool bIsMuted );
		bool				getIsMuted() const;
		void				setIsSoloed( bool bIsSoloed );
		bool				getIsSoloed() const;

		/** set the sample of the layer */
		void setSample( std::shared_ptr<Sample> sample );
		/** get the sample of the layer */
		std::shared_ptr<Sample> getSample() const;

		/**
		 * Calls the #H2Core::Sample::load()
		 * member function of #m_pSample.
		 */
		void loadSample( float fBpm = 120 );
		/*
		 * unload sample and replace it with an empty one
		 */
		void unloadSample();

		/**
		 * save the instrument layer within the given XMLNode
		 *
		 * \param node the XMLNode to feed
		 * \param bSongKit Whether the instrument is part of a
		 *   stand-alone kit or part of a song. In the latter case all samples
		 *   located in the corresponding drumkit folder and just their basenames
		 *   are written. In the former case, each instrument might be
		 *   associated with a different kit and the lookup folder for the
		 *   samples are stored on a per-instrument basis. Whether the path of a
		 *   sample points to a location within a drumkit folder will determine
		 *   whether just the basename or - if not - the absolute path will be
		 *   stored.
		 */
		void saveTo( XMLNode& node, bool bSongKit = false ) const;
		/**
		 * load an instrument layer from an XMLNode
		 *
		 * \param pNode the XMLDode to read from
		 * \param sDrumkitPath the directory holding the drumkit data
		 * @param sSongPath If not empty, absolute path to the .h2song file the
		 *   instrument layer is contained in. It is used to resolve sample
		 *   paths relative to the .h2song file.
		 * \param drumkitLicense License assigned to all #Sample
		 *   contain in the loaded #InstrumentLayer.
		 * \param bSilent if set to true, all log messages except of
		 *   errors and warnings are suppressed.
		 *
		 * \return a new InstrumentLayer instance
		 */
		static std::shared_ptr<InstrumentLayer> loadFrom( const XMLNode& pNode,
														  const QString& sDrumkitPath,
														  const QString& sSongPath = "",
														  const License& drumkitLicense = License(),
														  bool bSilent = false );
		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

	private:
		float m_fGain;               ///< ratio between the input sample and the output signal, 1.0 by default
		float m_fPitch;              ///< the frequency of the sample, 0.0 by default which means output pitch is the same as input pitch
		float m_fStartVelocity;     ///< the start velocity of the sample, 0.0 by default
		float m_fEndVelocity;       ///< the end velocity of the sample, 1.0 by default
		bool				m_bIsMuted;
		bool				m_bIsSoloed;
		std::shared_ptr<Sample> m_pSample;           ///< the underlaying sample

		/** In case we can not load the sample properly, we can use its path -
         * stored in here - to avoid a loss of information.
         *
         * This is a transient property and not stored in a drumkit.xml or
         * .h2song file. */
		QString m_sFallbackSampleFileName;
	};

	// DEFINITIONS

	inline void InstrumentLayer::setGain( float gain )
	{
		m_fGain = gain;
	}

	inline float InstrumentLayer::getGain() const
	{
		return m_fGain;
	}

	inline float InstrumentLayer::getPitch() const
	{
		return m_fPitch;
	}

	inline void InstrumentLayer::setStartVelocity( float start )
	{
		m_fStartVelocity = start;
	}

	inline float InstrumentLayer::getStartVelocity() const
	{
		return m_fStartVelocity;
	}

	inline void InstrumentLayer::setEndVelocity( float end )
	{
		m_fEndVelocity = end;
	}

	inline float InstrumentLayer::getEndVelocity() const
	{
		return m_fEndVelocity;
	}

inline void InstrumentLayer::setIsMuted( bool bIsMuted ) {
	m_bIsMuted = bIsMuted;
}
inline bool InstrumentLayer::getIsMuted() const {
	return m_bIsMuted;
}
inline void InstrumentLayer::setIsSoloed( bool bIsSoloed ) {
	m_bIsSoloed = bIsSoloed;
}
inline bool InstrumentLayer::getIsSoloed() const {
	return m_bIsSoloed;
}

	inline std::shared_ptr<Sample> InstrumentLayer::getSample() const
	{
		return m_pSample;
	}

};

#endif // H2C_INSTRUMENT_LAYER_H

/* vim: set softtabstop=4 noexpandtab: */
