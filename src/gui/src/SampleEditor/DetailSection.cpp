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

#include "DetailSection.h"

#include "DetailWaveDisplay.h"

#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>

using namespace H2Core;

DetailSection::DetailSection( QWidget* pParent ) : QWidget( pParent )
{
	resize( DetailSection::nWidth, DetailSection::nHeight );

	auto pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins( 0, 0, 0, 0 );
    pVBoxLayout->setSpacing( 0 );
	m_pWaveDisplayL = new DetailWaveDisplay( this );
	pVBoxLayout->addWidget( m_pWaveDisplayL );
	m_pWaveDisplayR = new DetailWaveDisplay( this );
	pVBoxLayout->addWidget( m_pWaveDisplayR );
	setLayout( pVBoxLayout );
}

DetailSection::~DetailSection()
{
}

void DetailSection::setDetailSamplePosition(
	int nPosition,
	float fZoomFactor,
	const QString& sType
)
{
	m_pWaveDisplayL->setDetailSamplePosition( nPosition, fZoomFactor, sType );
	m_pWaveDisplayR->setDetailSamplePosition( nPosition, fZoomFactor, sType );
	update();
}

void DetailSection::setSample( std::shared_ptr<Sample> pNewSample )
{
	if ( pNewSample == nullptr ) {
		m_pWaveDisplayL->setLayer( nullptr );
		m_pWaveDisplayR->setLayer( nullptr );
	}
	else {
		auto pLayer = std::make_shared<InstrumentLayer>( pNewSample );
		m_pWaveDisplayL->setLayer( pLayer );
		m_pWaveDisplayR->setLayer( pLayer );
	}
}
