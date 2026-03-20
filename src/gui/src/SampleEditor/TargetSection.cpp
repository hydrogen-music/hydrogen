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

#include "TargetSection.h"

#include <core/Basics/InstrumentLayer.h>

#include "SampleEditor.h"
#include "SampleEnvelope.h"
#include "Widgets/WaveDisplay.h"

using namespace H2Core;

TargetSection::TargetSection( SampleEditor* pParent )
	: QWidget( pParent ), m_bEnvelopeLocked( false )
{
	setFixedSize( TargetSection::nWidth, TargetSection::nHeight );

	auto pMainLayout = new QStackedLayout();
	pMainLayout->setStackingMode( QStackedLayout::StackAll );
	setLayout( pMainLayout );

	m_pSampleEnvelope = new SampleEnvelope( pParent );
	pMainLayout->addWidget( m_pSampleEnvelope );

	auto pWaveWidget = new QWidget( this );
	auto pWaveLayout = new QVBoxLayout();
	pWaveLayout->setContentsMargins( 0, 0, 0, 0 );
	pWaveLayout->setSpacing( 0 );
	pWaveWidget->setLayout( pWaveLayout );

	m_pWaveDisplayL = new WaveDisplay( this, WaveDisplay::Channel::Left );
	m_pWaveDisplayL->setFallbackLabel( "" );
	m_pWaveDisplayL->setLabel( WaveDisplay::Label::Fallback );
	pWaveLayout->addWidget( m_pWaveDisplayL );
	m_pWaveDisplayR = new WaveDisplay( this, WaveDisplay::Channel::Right );
	m_pWaveDisplayR->setFallbackLabel( "" );
	m_pWaveDisplayR->setLabel( WaveDisplay::Label::Fallback );
	pWaveLayout->addWidget( m_pWaveDisplayR );
	pMainLayout->addWidget( pWaveWidget );
}

TargetSection::~TargetSection()
{
}

void TargetSection::setEnvelopeLocked( bool bLocked )
{
	m_bEnvelopeLocked = bLocked;
	if ( m_pSampleEnvelope != nullptr ) {
		m_pSampleEnvelope->setLocked( bLocked );
	}
}

void TargetSection::setLayer( std::shared_ptr<InstrumentLayer> pLayer )
{
	m_pWaveDisplayL->setLayer( pLayer );
	m_pWaveDisplayR->setLayer( pLayer );
	update();
}

void TargetSection::update()
{
	m_pSampleEnvelope->update();
	m_pWaveDisplayL->update();
	m_pWaveDisplayR->update();
}
