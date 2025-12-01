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
#include "WaveDisplay.h"
#include "../Rack.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../HydrogenApp.h"
#include "../../Skin.h"

using namespace H2Core;

LayerPreview::LayerPreview( ComponentView* pComponentView )
 : QWidget( pComponentView )
 , m_pComponentView( pComponentView )
 , m_bMouseGrab( false )
 , m_bGrabLeft( false )
{
	setAttribute(Qt::WA_OpaquePaintEvent);

	setMouseTracking( true );

	const int nHeight = LayerPreview::nHeader + LayerPreview::nLayerHeight
		* InstrumentComponent::getMaxLayers();
	setFixedHeight( nHeight );

	m_speakerPixmap.load( Skin::getSvgImagePath() + "/icons/white/speaker.svg" );

	// We get a style similar to the one used for the 2 buttons on top of the
	// instrument editor panel
	setStyleSheet("font-size: 9px; font-weight: bold;");
}

LayerPreview::~LayerPreview() {
}

void LayerPreview::paintEvent( QPaintEvent* ev )
{
	QPainter p( this );

	auto createGradient = [=]( const QColor& color ) {
		QLinearGradient gradient(
			QPointF( 0, 0 ), QPointF( 0, LayerPreview::nLayerHeight / 2 )
		);
		gradient.setColorAt( 0, color.darker( WaveDisplay::nGradientScaling ) );
		gradient.setColorAt(
			1, color.lighter( WaveDisplay::nGradientScaling )
		);
		gradient.setSpread( QGradient::ReflectSpread );

		return gradient;
	};

	auto pPref = H2Core::Preferences::get_instance();
	const auto pColorTheme = pPref->getColorTheme();
	const auto pFontTheme = pPref->getFontTheme();

	const auto gradientDefault = createGradient( pColorTheme->m_accentColor );
	const auto gradientMute = createGradient( pColorTheme->m_muteColor );
	const auto gradientSolo = createGradient( pColorTheme->m_soloColor );

	p.setFont( QFont(
		pFontTheme->m_sLevel2FontFamily, getPointSize( pFontTheme->m_fontSize )
	) );

	p.fillRect( ev->rect(), pColorTheme->m_windowColor );

	const auto pComponent = m_pComponentView->getComponent();
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
		highlightColor = pColorTheme->m_highlightColor;
	}
	else {
		highlightColor = pColorTheme->m_lightColor;
	}
	const QColor missingLayerColor = pColorTheme->m_buttonRedColor;

	auto drawLayer = [&]( const QString& sLabel, const LayerInfo& info,
						  QLinearGradient* pGradient, const QColor* pColor ) {
		// Background
		p.fillRect(
			0, info.nStartY, width(), LayerPreview::nLayerHeight,
			pColorTheme->m_windowColor
		);

		if ( pGradient != nullptr ) {
			p.fillRect(
				info.nStartX, info.nStartY, info.nEndX - info.nStartX,
				LayerPreview::nLayerHeight, segmentGradient
			);
		}

		if ( pColor != nullptr ) {
			p.fillRect(
				info.nStartX, info.nStartY, info.nEndX - info.nStartX,
				LayerPreview::nLayerHeight, pColorTheme->m_buttonRedColor
			);
		}

		// Label
		QColor layerTextColor = pColorTheme->m_windowTextColor;
		layerTextColor.setAlpha( 155 );
		p.setPen( layerTextColor );
		p.drawText(
			10, info.nStartY + 1, width() - 10, LayerPreview::nLayerHeight,
			Qt::AlignLeft | Qt::AlignVCenter,
			QString( "%1: %2" ).arg( info.nId ).arg( sLabel )
		);

		// Border
		p.setPen( layerTextColor.darker( 145 ) );
		p.drawRect(
			LayerPreview::nBorder, info.nStartY,
			width() - 3 * LayerPreview::nBorder, LayerPreview::nLayerHeight
		);
	};

	// This object will cache the extents of each layer to properly
	// render the header later on. We sort the layers by highest end
	// point in the order to paint layers of higher velocity on top of
	// those with lower one. We do this, because our default velocity
	// 0.8 and the regular user is more likely to experience the higher
	// ones.
	std::set<LayerInfo> layerInfos;
	int nCount = 1;
	if ( pComponent != nullptr && pComponent->hasSamples() ) {
		int nCurrentY = LayerPreview::nHeader;
		for ( const auto& ppLayer : *pComponent ) {
			if ( ppLayer == nullptr ) {
				continue;
			}

			const int x1 = (int) ( ppLayer->getStartVelocity() * width() );
			const int x2 = (int) ( ppLayer->getEndVelocity() * width() );

			LayerInfo info{
				x1, x2, nCurrentY, nCount, nCount == nSelectedLayer
			};
			layerInfos.insert( info );

			QString sLabel = "< - >";

			auto pSample = ppLayer->getSample();
			if ( pSample != nullptr ) {
				sLabel = pSample->getFileName();
				if ( pSample->getIsModified() ) {
					sLabel.append( "*" );
				}
				if ( ppLayer->getIsMuted() || pComponent->getIsMuted() ) {
					segmentGradient = gradientMute;
				}
				else if ( ppLayer->getIsSoloed() ) {
					segmentGradient = gradientSolo;
				}
				else {
					segmentGradient = gradientDefault;
				}
				drawLayer( sLabel, info, &segmentGradient, nullptr );
			}
			else {
				sLabel = ppLayer->getFallbackSampleFileName();
				drawLayer( sLabel, info, nullptr, &missingLayerColor );
			}

			nCurrentY += LayerPreview::nLayerHeight;
			++nCount;
		}
	}
	else {
		LayerInfo info{ 0, width(), LayerPreview::nHeader, 0, false };
		drawLayer( "< - >", info, nullptr, nullptr );
	}

	// We render the header after our first swipe over all layers in order to
	// get the overlaps right.
	int nCurrentEnd = width() - 2 * LayerPreview::nBorder;
	int ii = 0;
	for ( auto iinfo = layerInfos.rbegin(); iinfo != layerInfos.rend();
		  ++iinfo ) {
		if ( iinfo->nStartX > nCurrentEnd ) {
			// The borders of this layer were already drawn. Probably it was
			// covered by another one.
			++ii;
			continue;
		}

		const int nVisibleEnd = std::min( iinfo->nEndX, nCurrentEnd );
		const int nVisibleStart =
			std::max( iinfo->nStartX, LayerPreview::nBorder );

		// Labels for layers to the left will have a
		// lighter color as those to the right.
		nColorScaling =
			static_cast<int>( std::round(
				static_cast<float>( ii ) / static_cast<float>( nLayers ) * 2 *
				static_cast<float>( nColorScalingWidth )
			) ) -
			nColorScalingWidth + 100;
		layerLabelColor = pColorTheme->m_windowColor.lighter( nColorScaling );

		p.fillRect(
			iinfo->nStartX, 0, nVisibleEnd - nVisibleStart,
			LayerPreview::nHeader - LayerPreview::nBorder, layerLabelColor
		);

		// Check whether there are other layers overlapping into the current
		// region.
		for ( const auto& iinfoOther : layerInfos ) {
			if ( iinfoOther.nEndX <= nVisibleStart ||
				 iinfoOther.nEndX >= nVisibleEnd ) {
				// No overlap.
				continue;
			}
			QPen pen;
			pen.setStyle( Qt::DashLine );
			pen.setColor( pColorTheme->m_windowTextColor.darker( 145 ) );
			p.setPen( pen );
			p.drawLine(
				iinfoOther.nEndX, LayerPreview::nBorder, iinfoOther.nEndX,
				LayerPreview::nHeader - LayerPreview::nBorder
			);
			if ( iinfoOther.nStartX > nVisibleStart ) {
				p.drawLine(
					iinfoOther.nStartX, LayerPreview::nBorder, iinfoOther.nStartX,
					LayerPreview::nHeader - LayerPreview::nBorder
				);
			}
		}

		// Borders
		if ( iinfo->bSelected ) {
			p.setPen( highlightColor );
		}
		else {
			p.setPen( pColorTheme->m_windowTextColor.darker( 145 ) );
		}
		p.drawLine(
			nVisibleStart, LayerPreview::nBorder, nVisibleStart,
			LayerPreview::nHeader - LayerPreview::nBorder
		);
		p.drawLine(
			nVisibleStart, LayerPreview::nBorder, nVisibleEnd,
			LayerPreview::nBorder
		);
		p.drawLine(
			nVisibleStart, LayerPreview::nHeader - LayerPreview::nBorder,
			nVisibleEnd, LayerPreview::nHeader
		);
		if ( iinfo->nEndX < nCurrentEnd ||
			 ( iinfo->nEndX >= width() - LayerPreview::nBorder ) ||
			 ( iinfo->nEndX == nCurrentEnd && iinfo->bSelected ) ) {
			p.drawLine(
				nVisibleEnd, LayerPreview::nBorder, nVisibleEnd,
				LayerPreview::nHeader - LayerPreview::nBorder
			);
		}

		nCurrentEnd = nVisibleStart;
		++ii;
	}

	// Ensure the selected layer is properly highlighted in the header.
	for ( const auto& iinfo : layerInfos ) {
		if ( !iinfo.bSelected ) {
			continue;
		}

		const int nVisibleEnd =
			std::min( iinfo.nEndX, width() - 2 * LayerPreview::nBorder );
		const int nVisibleStart =
			std::max( iinfo.nStartX, LayerPreview::nBorder );

		p.setPen( highlightColor );
		p.drawLine(
			nVisibleStart, LayerPreview::nBorder, nVisibleStart,
			LayerPreview::nHeader - LayerPreview::nBorder
		);
		p.drawLine(
			nVisibleStart, LayerPreview::nBorder, nVisibleEnd,
			LayerPreview::nBorder
		);
		p.drawLine(
			nVisibleStart, LayerPreview::nHeader - LayerPreview::nBorder,
			nVisibleEnd, LayerPreview::nHeader
		);
		p.drawLine(
			nVisibleEnd, LayerPreview::nBorder, nVisibleEnd,
			LayerPreview::nHeader - LayerPreview::nBorder
		);
		break;
	}

	// If there are no layers, draw an empty header
	if ( layerInfos.size() == 0 ) {
		p.fillRect(
			LayerPreview::nBorder, LayerPreview::nBorder, width() - 2 * LayerPreview::nBorder,
			LayerPreview::nHeader - LayerPreview::nBorder, layerLabelColor
		);
		p.setPen( pColorTheme->m_windowTextColor.darker( 145 ) );
		p.drawRect(
			LayerPreview::nBorder, LayerPreview::nBorder, width() - 3 * LayerPreview::nBorder,
			LayerPreview::nHeader - 2 * LayerPreview::nBorder
		);
	}

	// The number indicating the layer in the header should always win and be
	// visible. That's why render them in another swipe.
	p.setFont( QFont( pFontTheme->m_sLevel2FontFamily, getPointSizeButton() ) );
	for ( const auto& iinfo : layerInfos ) {
		if ( iinfo.bSelected ) {
			p.setPen( highlightColor );
		}
		else {
			p.setPen( pColorTheme->m_windowTextColor );
		}
		p.drawText(
			iinfo.nStartX, 0, iinfo.nEndX - iinfo.nStartX,
			LayerPreview::nHeader, Qt::AlignCenter,
			QString( "%1" ).arg( iinfo.nId + 1 )
		);
	}

	// border
	QPen penBorder( Qt::black );
	penBorder.setWidth( LayerPreview::nBorder );
	p.setPen( penBorder );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, 0, 0, height() );
	p.drawLine( 0, height(), width(), height() );
	p.drawLine( width() - 1, 0, width(), height() );

	// selected layer
	p.setPen( highlightColor );

	p.drawRect(
		LayerPreview::nBorder,
		LayerPreview::nHeader + LayerPreview::nLayerHeight * nSelectedLayer,
		width() - 3 * LayerPreview::nBorder, LayerPreview::nLayerHeight
	);
}

void LayerPreview::mouseReleaseEvent( QMouseEvent* ev )
{
	m_bMouseGrab = false;

	if ( m_pComponentView->getComponent() == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );

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
			
			if ( ( pEv->position().x() < x1  + 5 ) &&
				 ( pEv->position().x() > x1 - 5 ) ){
				setCursor( QCursor( Qt::SizeHorCursor ) );
				showLayerStartVelocity(pLayer, ev);
			}
			else if ( ( pEv->position().x() < x2 + 5 ) &&
					  ( pEv->position().x() > x2 - 5 ) ) {
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

	auto pEv = static_cast<MouseEvent*>( ev );

	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();

	const float fVelocity = (float)pEv->position().x() / (float)width();

	if ( pEv->position().y() < 20 ) {
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
		const int nClickedLayer =
			( pEv->position().y() - 20 ) / LayerPreview::nLayerHeight;
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

				if ( ( pEv->position().x() < x1  + 5 ) &&
					 ( pEv->position().x() > x1 - 5 ) ){
					setCursor( QCursor( Qt::SizeHorCursor ) );
					m_bGrabLeft = true;
					m_bMouseGrab = true;
					showLayerStartVelocity(pLayer, ev);
				}
				else if ( ( pEv->position().x() < x2 + 5 ) &&
						  ( pEv->position().x() > x2 - 5 ) ){
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

	auto pEv = static_cast<MouseEvent*>( ev );
	const int x = pEv->position().x();
	const int y = pEv->position().y();

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
		int nHoveredLayer = ( pEv->position().y() - 20 ) /
			LayerPreview::nLayerHeight;
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

void LayerPreview::showLayerStartVelocity( const std::shared_ptr<InstrumentLayer> pLayer,
										   QMouseEvent* pEvent )
{
	const float fVelo = pLayer->getStartVelocity();

	auto pEv = static_cast<MouseEvent*>( pEvent );

	QToolTip::showText( pEv->globalPosition().toPoint(),
			tr( "Dec. = %1\nMIDI = %2" )
				.arg( QString::number( fVelo, 'f', 2) )
				.arg( getMidiVelocityFromRaw( fVelo ) +1 ),
			this);
}

void LayerPreview::showLayerEndVelocity( const std::shared_ptr<InstrumentLayer> pLayer,
										 QMouseEvent* pEvent )
{
	const float fVelo = pLayer->getEndVelocity();

	auto pEv = static_cast<MouseEvent*>( pEvent );

	QToolTip::showText( pEv->globalPosition().toPoint(),
			tr( "Dec. = %1\nMIDI = %2" )
				.arg( QString::number( fVelo, 'f', 2) )
				.arg( getMidiVelocityFromRaw( fVelo ) +1 ),
			this);
}

int LayerPreview::getPointSizeButton() const
{
	auto pPref = H2Core::Preferences::get_instance();
	
	int nPointSize;
	
	switch( pPref->getFontTheme()->m_fontSize ) {
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
