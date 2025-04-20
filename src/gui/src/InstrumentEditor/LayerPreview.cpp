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

#include "LayerPreview.h"

#include <QtGui>
#include <QtWidgets>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Theme.h>
#include <core/Sampler/Sampler.h>

#include "ComponentView.h"
#include "InstrumentEditorPanel.h"
#include "WaveDisplay.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"
#include "../Skin.h"

using namespace H2Core;

LayerPreview::LayerPreview( ComponentView* pComponentView )
 : QWidget( pComponentView )
 , m_pComponentView( pComponentView )
 , m_bMouseGrab( false )
 , m_bGrabLeft( false )
{
	setAttribute(Qt::WA_OpaquePaintEvent);

	setMouseTracking( true );

	int nWidth = ComponentView::nWidth - ComponentView::nMargin * 2;
	if( InstrumentComponent::getMaxLayers() > 16) {
		// Account for the scroll bar.
		nWidth -= 15;
	}
	
	const int nHeight = LayerPreview::nMargin + LayerPreview::nLayerHeight
		* InstrumentComponent::getMaxLayers();
	resize( nWidth, nHeight );

	m_speakerPixmap.load( Skin::getSvgImagePath() + "/icons/white/speaker.svg" );

	// We get a style similar to the one used for the 2 buttons on top of the
	// instrument editor panel
	setStyleSheet("font-size: 9px; font-weight: bold;");
}

LayerPreview::~LayerPreview() {
}

void LayerPreview::paintEvent(QPaintEvent *ev)
{
	QPainter p( this );

	auto createGradient = [=]( const QColor& color ) {
		QLinearGradient gradient(
			QPointF( 0, 0 ), QPointF( 0, LayerPreview::nLayerHeight / 2 ) );
		gradient.setColorAt(
			0, color.darker( WaveDisplay::nGradientScaling ) );
		gradient.setColorAt(
			1, color.lighter( WaveDisplay::nGradientScaling ) );
		gradient.setSpread( QGradient::ReflectSpread );

		return gradient;
	};

	auto pPref = H2Core::Preferences::get_instance();
	const auto gradientDefault = createGradient(
		pPref->getTheme().m_color.m_accentColor );
	const auto gradientMute = createGradient(
		pPref->getTheme().m_color.m_muteColor );
	const auto gradientSolo = createGradient(
		pPref->getTheme().m_color.m_soloColor );

	QFont fontText( pPref->getTheme().m_font.m_sLevel2FontFamily,
					getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	QFont fontButton( pPref->getTheme().m_font.m_sLevel2FontFamily,
					  getPointSizeButton() );
	
	p.fillRect( ev->rect(), pPref->getTheme().m_color.m_windowColor );

	auto pComponent = m_pComponentView->getComponent();
	const int nSelectedLayer = m_pComponentView->getSelectedLayer();

	int nLayers = 0;
	if ( pComponent != nullptr ) {
		nLayers = pComponent->getLayers().size();
	}
	
	// How much the color of the labels for the individual layers
	// are allowed to diverge from the general window color.
	int nColorScalingWidth = 90;
	int nColorScaling = 100;

	QColor layerLabelColor, highlightColor;
	QLinearGradient segmentGradient;
	if ( pComponent != nullptr ) {
		highlightColor = pPref->getTheme().m_color.m_highlightColor;
	} else {
		highlightColor = pPref->getTheme().m_color.m_lightColor;
	}

	int nLayer = 0;
	for ( int i = InstrumentComponent::getMaxLayers() - 1; i >= 0; i-- ) {
		const int y = LayerPreview::nMargin + LayerPreview::nLayerHeight * i;
		QString label = "< - >";
		
		if ( pComponent != nullptr ) {
			auto pLayer = pComponent->getLayer( i );
				
			if ( pLayer != nullptr && nLayers > 0 ) {
				auto pSample = pLayer->getSample();
				if ( pSample != nullptr ) {
					label = pSample->getFilename();
				}
				else {
					label = tr( "missing sample" );
				}

				const int x1 = (int)( pLayer->getStartVelocity() * width() );
				const int x2 = (int)( pLayer->getEndVelocity() * width() );

				// Labels for layers to the left will have a
				// lighter color as those to the right.
				nColorScaling = static_cast<int>(
					std::round( static_cast<float>(nLayer) /
								static_cast<float>(nLayers) * 2 *
								static_cast<float>(nColorScalingWidth) ) ) -
					nColorScalingWidth + 100;
				layerLabelColor =
					pPref->getTheme().m_color.m_windowColor.lighter( nColorScaling );

				// Header
				p.fillRect( x1, 0, x2 - x1, 19, layerLabelColor );
				p.setPen( pPref->getTheme().m_color.m_windowTextColor );
				p.setFont( fontButton );
				p.drawText( x1, 0, x2 - x1, 20, Qt::AlignCenter, QString("%1").arg( i + 1 ) );

				// Border
				if ( nSelectedLayer == i ) {
					p.setPen( highlightColor );
				}
				else {
					p.setPen( pPref->getTheme().m_color.m_windowTextColor.darker( 145 ) );
				}
				p.drawRect( x1, 1, x2 - x1 - 1, 18 );	// bordino in alto
					
				// layer view
				p.fillRect( 0, y, width(), LayerPreview::nLayerHeight,
							pPref->getTheme().m_color.m_windowColor );
				if ( pSample != nullptr ) {
					if ( pLayer->getIsMuted() ) {
						segmentGradient = gradientMute;
					}
					else if ( pLayer->getIsSoloed() ) {
						segmentGradient = gradientSolo;
					}
					else {
						segmentGradient = gradientDefault;
					}
					p.fillRect( x1, y, x2 - x1, LayerPreview::nLayerHeight,
								segmentGradient );
				}
				else {
					p.fillRect( x1, y, x2 - x1, LayerPreview::nLayerHeight,
								pPref->getTheme().m_color.m_buttonRedColor );
				}

				nLayer++;
			}
			else {
				// layer view
				p.fillRect( 0, y, width(), LayerPreview::nLayerHeight,
							pPref->getTheme().m_color.m_windowColor );
			}
		}
		else {
			// layer view
			p.fillRect( 0, y, width(), LayerPreview::nLayerHeight,
						pPref->getTheme().m_color.m_windowColor );
		}

		QColor layerTextColor = pPref->getTheme().m_color.m_windowTextColor;
		layerTextColor.setAlpha( 155 );
		p.setPen( layerTextColor );
		p.setFont( fontText );
		p.drawText( 10, y, width() - 10, 20, Qt::AlignLeft, QString( "%1: %2" ).arg( i + 1 ).arg( label ) );
		p.setPen( layerTextColor.darker( 145 ) );
		p.drawRect( 0, y, width() - 1, LayerPreview::nLayerHeight );
	}

	// border
	p.setPen( Qt::black );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, 0, 0, height() );
	p.drawLine( 0, height(), width(), height() );
	p.drawLine( width(), 0, width(), height() );

	// selected layer
	p.setPen( highlightColor );
	const int y = LayerPreview::nMargin +
		LayerPreview::nLayerHeight * nSelectedLayer;
	p.drawRect( 0, y, width() - 1, LayerPreview::nLayerHeight );
}

void LayerPreview::mouseReleaseEvent(QMouseEvent *ev)
{
	m_bMouseGrab = false;

	if ( m_pComponentView->getComponent() == nullptr ) {
		return;
	}

	/*
	 * We want the tooltip to still show if mouse pointer
	 * is over an active layer's boundary
	 */
	auto pCompo = m_pComponentView->getComponent();
	if ( pCompo != nullptr ) {
		auto pLayer = pCompo->getLayer(
			m_pComponentView->getSelectedLayer() );

		if ( pLayer ) {
			int x1 = (int)( pLayer->getStartVelocity() * width() );
			int x2 = (int)( pLayer->getEndVelocity() * width() );
			
			if ( ( ev->x() < x1  + 5 ) && ( ev->x() > x1 - 5 ) ){
				setCursor( QCursor( Qt::SizeHorCursor ) );
				showLayerStartVelocity(pLayer, ev);
			}
			else if ( ( ev->x() < x2 + 5 ) && ( ev->x() > x2 - 5 ) ) {
				setCursor( QCursor( Qt::SizeHorCursor ) );
				showLayerEndVelocity(pLayer, ev);
			}
		}
	}
}

void LayerPreview::mousePressEvent(QMouseEvent *ev)
{
	const int nPosition = 0;
	const int nSelectedLayer = m_pComponentView->getSelectedLayer();
	const auto pComponent = m_pComponentView->getComponent();
	if ( pComponent == nullptr ) {
		return;
	}

	auto pInstrumentEditorPanel =
		HydrogenApp::get_instance()->getInstrumentRack()->getInstrumentEditorPanel();
	const auto pInstrument = pInstrumentEditorPanel->getInstrument();

	const float fVelocity = (float)ev->x() / (float)width();

	if ( ev->y() < 20 ) {
		if ( pComponent->hasSamples() && pInstrument != nullptr ) {
			auto pNote = std::make_shared<Note>(
				pInstrument, nPosition, fVelocity );

			// We register the current component to be rendered. This will cause
			// all other components _not_ to be rendered. Because we do not
			// provide a selected layer, the Sampler will select one for us
			// based on the current sample selection algorithm.
			pNote->setSelectedLayerInfo( nullptr, pComponent );

			Hydrogen::get_instance()->getAudioEngine()->getSampler()->noteOn(pNote);
		}

		int nNewLayer = -1;
		for ( int ii = 0; ii < InstrumentComponent::getMaxLayers(); ii++ ) {
			auto pLayer = pComponent->getLayer( ii );
			if ( pLayer != nullptr ) {
				if ( fVelocity > pLayer->getStartVelocity() &&
					 fVelocity < pLayer->getEndVelocity() ) {
					if ( ii != nSelectedLayer ) {
						nNewLayer = ii;
					}
					break;
				}
			}
		}

		if ( nNewLayer != -1 ) {
			m_pComponentView->setSelectedLayer( nNewLayer );
			m_pComponentView->updateView();
		}
	}
	else {
		const int nClickedLayer = ( ev->y() - 20 ) / LayerPreview::nLayerHeight;
		if ( nClickedLayer < InstrumentComponent::getMaxLayers() &&
			 nClickedLayer >= 0 ) {
			m_pComponentView->setSelectedLayer( nClickedLayer );
			m_pComponentView->updateView();

			auto pLayer = pComponent->getLayer( nClickedLayer );
			if ( pLayer != nullptr && pInstrument != nullptr ) {
				// We register the current component to be rendered using a
				// specific layer. This will cause all other components _not_ to
				// be rendered.
				auto pSelectedLayerInfo = std::make_shared<SelectedLayerInfo>();
				pSelectedLayerInfo->pLayer = pLayer;

				const auto pNote = std::make_shared<Note>(
					pInstrument, nPosition, fVelocity );
				pNote->setSelectedLayerInfo( pSelectedLayerInfo, pComponent );

				Hydrogen::get_instance()->getAudioEngine()->getSampler()->
					noteOn( pNote );

				int x1 = (int)( pLayer->getStartVelocity() * width() );
				int x2 = (int)( pLayer->getEndVelocity() * width() );

				if ( ( ev->x() < x1  + 5 ) && ( ev->x() > x1 - 5 ) ){
					setCursor( QCursor( Qt::SizeHorCursor ) );
					m_bGrabLeft = true;
					m_bMouseGrab = true;
					showLayerStartVelocity(pLayer, ev);
				}
				else if ( ( ev->x() < x2 + 5 ) && ( ev->x() > x2 - 5 ) ){
					setCursor( QCursor( Qt::SizeHorCursor ) );
					m_bGrabLeft = false;
					m_bMouseGrab = true;
					showLayerEndVelocity(pLayer, ev);
				}
				else {
					setCursor( QCursor( Qt::ArrowCursor ) );
				}
			}
		}
	}
}

void LayerPreview::mouseMoveEvent( QMouseEvent *ev )
{
	auto pComponent = m_pComponentView->getComponent();
	if ( pComponent == nullptr ) {
		return;
	}
	const int nSelectedLayer = m_pComponentView->getSelectedLayer();

	const int x = ev->pos().x();
	const int y = ev->pos().y();

	if ( y < 20 ) {
		setCursor( QCursor( m_speakerPixmap ) );
		return;
	}

	float fVel = (float)x / (float)width();
	if (fVel < 0 ) {
		fVel = 0;
	}
	else  if (fVel > 1) {
		fVel = 1;
	}
	if ( m_bMouseGrab ) {
		auto pLayer = pComponent->getLayer( nSelectedLayer );
		if ( pLayer != nullptr ) {
			bool bChanged = false;
			if ( m_bGrabLeft ) {
				if ( fVel < pLayer->getEndVelocity()) {
					pLayer->setStartVelocity( fVel );
					bChanged = true;
					showLayerStartVelocity( pLayer, ev );
				}
			}
			else {
				if ( fVel > pLayer->getStartVelocity()) {
					pLayer->setEndVelocity( fVel );
					bChanged = true;
					showLayerEndVelocity( pLayer, ev );
				}
			}

			if ( bChanged ) {
				update();
				Hydrogen::get_instance()->setIsModified( true );
			}
		}
	}
	else {
		int nHoveredLayer = ( ev->y() - 20 ) / LayerPreview::nLayerHeight;
		if ( nHoveredLayer < InstrumentComponent::getMaxLayers() &&
			 nHoveredLayer >= 0 ) {

			auto pHoveredLayer = pComponent->getLayer( nHoveredLayer );
			if ( pHoveredLayer != nullptr ) {
				int x1 = (int)( pHoveredLayer->getStartVelocity() * width() );
				int x2 = (int)( pHoveredLayer->getEndVelocity() * width() );
					
				if ( ( x < x1  + 5 ) && ( x > x1 - 5 ) ){
					setCursor( QCursor( Qt::SizeHorCursor ) );
					showLayerStartVelocity(pHoveredLayer, ev);
				}
				else if ( ( x < x2 + 5 ) && ( x > x2 - 5 ) ){
					setCursor( QCursor( Qt::SizeHorCursor ) );
					showLayerEndVelocity(pHoveredLayer, ev);
				}
				else {
					setCursor( QCursor( Qt::ArrowCursor ) );
					QToolTip::hideText();
				}
			}
			else {
				setCursor( QCursor( Qt::ArrowCursor ) );
				QToolTip::hideText();
			}
		}
		else {
			setCursor( QCursor( Qt::ArrowCursor ) );
			QToolTip::hideText();
		}
	}
}

int LayerPreview::getMidiVelocityFromRaw( const float raw )
{
	return static_cast<int> (raw * 127);
}

void LayerPreview::showLayerStartVelocity( const std::shared_ptr<InstrumentLayer> pLayer, const QMouseEvent* pEvent )
{
	const float fVelo = pLayer->getStartVelocity();

	QToolTip::showText( pEvent->globalPos(),
			tr( "Dec. = %1\nMIDI = %2" )
				.arg( QString::number( fVelo, 'f', 2) )
				.arg( getMidiVelocityFromRaw( fVelo ) +1 ),
			this);
}

void LayerPreview::showLayerEndVelocity( const std::shared_ptr<InstrumentLayer> pLayer, const QMouseEvent* pEvent )
{
	const float fVelo = pLayer->getEndVelocity();

	QToolTip::showText( pEvent->globalPos(),
			tr( "Dec. = %1\nMIDI = %2" )
				.arg( QString::number( fVelo, 'f', 2) )
				.arg( getMidiVelocityFromRaw( fVelo ) +1 ),
			this);
}

int LayerPreview::getPointSizeButton() const
{
	auto pPref = H2Core::Preferences::get_instance();
	
	int nPointSize;
	
	switch( pPref->getTheme().m_font.m_fontSize ) {
	case H2Core::FontTheme::FontSize::Small:
		nPointSize = 6;
		break;
	case H2Core::FontTheme::FontSize::Medium:
		nPointSize = 8;
		break;
	case H2Core::FontTheme::FontSize::Large:
		nPointSize = 12;
		break;
	}

	return nPointSize;
}
