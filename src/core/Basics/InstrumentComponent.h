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

#ifndef H2C_INSTRUMENTCOMPONENT_H
#define H2C_INSTRUMENTCOMPONENT_H

#include <cassert>
#include <vector>
#include <memory>

#include <QString>

#include <core/Object.h>
#include <core/License.h>

namespace H2Core
{

class InstrumentLayer;
class XMLNode;

/** \ingroup docCore docDataStructure */
class InstrumentComponent : public H2Core::Object<InstrumentComponent>
{
		H2_OBJECT(InstrumentComponent)
	public:

		enum class Selection {
			Velocity,
			RoundRobin,
			Random,
		};
		static QString SelectionToQString( const Selection& selection );

		InstrumentComponent( const QString& sName = "", float fGain = 1.0 );
		InstrumentComponent( std::shared_ptr<InstrumentComponent> other );
		~InstrumentComponent();

		void				saveTo( XMLNode& node, bool bSongKit = false ) const;
		static std::shared_ptr<InstrumentComponent> loadFrom( const XMLNode& pNode,
															   const QString& sDrumkitPath,
															   const QString& sSongPath = "",
															   const License& drumkitLicense = License(),
															   bool bSilent = false );

		void				setName( const QString& sName );
		const QString&		getName() const;

		std::shared_ptr<InstrumentLayer>	operator[]( int ix ) const;
		std::shared_ptr<InstrumentLayer>	getLayer( int idx ) const;
	/**
	 * Get all initialized layers.
	 *
	 * In it's current design #__layers is always of #MAX_LAYERS
	 * length and all layer not used are set to nullptr. This
	 * convenience function is used to query only those
	 * #InstrumentLayer which were properly initialized.
	 */
	const std::vector<std::shared_ptr<InstrumentLayer>> getLayers() const;
		void				setLayer( std::shared_ptr<InstrumentLayer> layer, int idx );

		void				setGain( float gain );
		float				getGain() const;
		
		void				setIsMuted( bool bIsMuted );
		bool				getIsMuted() const;
		void				setIsSoloed( bool bIsSoloed );
		bool				getIsSoloed() const;

		void setSelection( const Selection& selection );
		Selection getSelection() const;

		/** Whether the component contains at least one non-missing
		 * sample */
		bool hasSamples() const;

		bool isAnyLayerSoloed() const;

		/**  @return #m_nMaxLayers.*/
		static int			getMaxLayers();
		/** @param layers Sets #m_nMaxLayers.*/
		static void			setMaxLayers( int layers );
	
		/** Iteration */
	std::vector<std::shared_ptr<InstrumentLayer>>::iterator begin();
	std::vector<std::shared_ptr<InstrumentLayer>>::iterator end();

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
		QString 			m_sName;
		float				m_fGain;

		bool				m_bIsMuted;
		bool				m_bIsSoloed;
		/** how Hydrogen will chose the sample to use */
		Selection		m_selection;

		/** Maximum number of layers to be used in the
		 *  Instrument editor.
		 *
		 * It is set by setMaxLayers(), queried by
		 * getMaxLayers(), and inferred from
		 * Preferences::m_nMaxLayers. Default value assigned in
		 * Preferences::Preferences(): 16. */
		static int			m_nMaxLayers;
		std::vector<std::shared_ptr<InstrumentLayer>>	m_layers;
};

// DEFINITIONS
inline void InstrumentComponent::setName( const QString& sName ) {
	m_sName = sName;
}
inline const QString& InstrumentComponent::getName() const {
	return m_sName;
}

inline void InstrumentComponent::setGain( float gain )
{
	m_fGain = gain;
}

inline float InstrumentComponent::getGain() const
{
	return m_fGain;
}

inline void InstrumentComponent::setIsMuted( bool bIsMuted ) {
	m_bIsMuted = bIsMuted;
}
inline bool InstrumentComponent::getIsMuted() const {
	return m_bIsMuted;
}
inline void InstrumentComponent::setIsSoloed( bool bIsSoloed ) {
	m_bIsSoloed = bIsSoloed;
}
inline bool InstrumentComponent::getIsSoloed() const {
	return m_bIsSoloed;
}

inline void InstrumentComponent::setSelection( const Selection& selection ) {
	m_selection = selection;
}

inline InstrumentComponent::Selection InstrumentComponent::getSelection() const {
	return m_selection;
}

inline std::shared_ptr<InstrumentLayer> InstrumentComponent::operator[]( int nIdx ) const
{
	if ( nIdx < 0 || nIdx >= m_layers.size() ) {
		return nullptr;
	}
	return m_layers[ nIdx ];
}

inline std::shared_ptr<InstrumentLayer> InstrumentComponent::getLayer( int nIdx ) const
{
	if ( nIdx < 0 || nIdx >= m_layers.size() ) {
		return nullptr;
	}
	return m_layers[ nIdx ];
}
};


#endif
