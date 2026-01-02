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
#include "../Rack.h"
#include "../../Compatibility/DropEvent.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../HydrogenApp.h"
#include "../../Skin.h"
#include "../../UndoActions.h"
#include "../../Widgets/WaveDisplay.h"
#include "core/Midi/Midi.h"

using namespace H2Core;

LayerPreview::LayerPreview( ComponentView* pComponentView )
	: QWidget( pComponentView ),
	  m_pComponentView( pComponentView ),
	  m_drag( Drag::None ),
	  m_dragStartPoint( QPointF() ),
      m_nLastDragLayer( 0 )
{
	setAcceptDrops( true );
	setAttribute( Qt::WA_OpaquePaintEvent );

	setMouseTracking( true );

	const auto pComponent = pComponentView->getComponent();
	if ( pComponent != nullptr ) {
		const int nHeight =
			LayerPreview::nHeader +
			LayerPreview::nLayerHeight * pComponent->getLayers().size();
		setFixedHeight( nHeight );
	}

	m_speakerPixmap.load(
		Skin::getSvgImagePath() + "/icons/white/speaker.svg"
	);

	// We get a style similar to the one used for the 2 buttons on top of the
	// instrument editor panel
	setStyleSheet("font-size: 9px; font-weight: bold;");

	updatePreview();
}

LayerPreview::~LayerPreview() {
}

void LayerPreview::updatePreview()
{
	const auto pComponent = m_pComponentView->getComponent();

	const int nLayers =
		pComponent != nullptr ? pComponent->getLayers().size() : 0;

	// We show at least one empty layer
	setFixedHeight(
		LayerPreview::nHeader + LayerPreview::nBorder +
		std::max( nLayers, 1 ) *
			( LayerPreview::nLayerHeight )
	);

	update();
}

int LayerPreview::yToLayer( int nY )
{
	int nLayer;
	if ( nY < LayerPreview::nHeader ) {
		nLayer = 0;
	}
	else {
		nLayer = static_cast<int>( std::floor(
			static_cast<float>( nY - LayerPreview::nHeader ) /
			static_cast<float>( LayerPreview::nLayerHeight )
		) );
	}

	return nLayer;
}

void LayerPreview::dragEnterEvent( QDragEnterEvent* event )
{
	if ( event->mimeData()->hasFormat( "text/uri-list" ) ||
		 event->mimeData()->hasFormat( "text/plain" ) ) {
	 	event->acceptProposedAction();
	}

}

void LayerPreview::dragMoveEvent( QDragMoveEvent* event )
{
	event->accept();

	auto pEv = static_cast<DropEvent*>( static_cast<QDropEvent*>( event ) );
	const int nCurrentLayer = LayerPreview::yToLayer( pEv->position().y() );
	if ( nCurrentLayer != m_nLastDragLayer ) {
		m_nLastDragLayer = nCurrentLayer;
		update();
	}
}

void LayerPreview::dropEvent( QDropEvent* event )
{
    auto pEv = static_cast<DropEvent*>( event );

	const QMimeData* mimeData = pEv->mimeData();
	QString sText = pEv->mimeData()->text();

	const int nDropLayer = LayerPreview::yToLayer( pEv->position().y() );

    m_drag = Drag::None;
    m_nLastDragLayer = -1;

	if ( mimeData->hasUrls() ) {
		QList<QUrl> urlList = mimeData->urls();

		QStringList filePaths;
		for ( const auto& uurl : urlList ) {
			const auto sPath = uurl.toLocalFile();
			if ( !sPath.isEmpty() ) {
				filePaths << sPath;
			}
		}

		if ( filePaths.size() > 0 ) {
			// Ensure we insert the layers at the right spot by moving the
			// selected layer to the drop point.
			m_pComponentView->setSelectedLayer( nDropLayer );

			m_pComponentView->setLayers( filePaths, false, false );
		}
	}
	else if ( sText.startsWith( "ComponentViewLayer" ) ) {
		pEv->acceptProposedAction();

		const int nStartLayer = LayerPreview::yToLayer( m_dragStartPoint.y() );

		const auto pInstrument =
			Hydrogen::get_instance()->getSelectedInstrument();
		const auto pComponent = m_pComponentView->getComponent();
		if ( pInstrument == nullptr || pComponent == nullptr ) {
			return;
		}
		auto pHydrogenApp = HydrogenApp::get_instance();
		const auto pCommonStrings = pHydrogenApp->getCommonStrings();

		auto pNewInstrument = std::make_shared<Instrument>( pInstrument );
		auto pNewComponent =
			pNewInstrument->getComponent( pInstrument->index( pComponent ) );
		if ( pNewComponent == nullptr ) {
			ERRORLOG( "Hiccup while looking up component" );
			return;
		}

		pNewInstrument->moveLayer(
			pNewComponent, nStartLayer, nDropLayer, Event::Trigger::Suppress
		);

		pHydrogenApp->pushUndoCommand( new SE_replaceInstrumentAction(
			pNewInstrument, pInstrument,
			SE_replaceInstrumentAction::Type::MoveLayer, pComponent->getName()
		) );
		m_pComponentView->setSelectedLayer( nDropLayer );
		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2]" )
				.arg( pCommonStrings->getActionMoveInstrumentLayer() )
				.arg( pComponent->getName() )
		);
	}
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

    // Base color used for the header segment corresponding to an individual
    // layer. It will be used for the middle section. The further left, the
    // lighter the other segments will be (derived from this color) and the
    // further right, the darker.
    const QColor headerBaseColor = pColorTheme->m_windowColor;
	QColor highlightColor;
	QLinearGradient segmentGradient;
	if ( pComponent != nullptr ) {
		highlightColor = pColorTheme->m_highlightColor;
	}
	else {
		highlightColor = pColorTheme->m_lightColor;
	}
	const QColor missingLayerColor = pColorTheme->m_buttonRedColor;

	auto pickHeaderColor = [&]( int nLayer ) {
		if ( nLayers <= 1 ) {
			return headerBaseColor;
		}
		// How much the color of the labels for the individual layers are
		// allowed to diverge from the base color.
		const int nColorScalingWidth = 90;
        // 100 is the base value for Qt indicating that nothing should change.
		const int nColorScaling = 100 - nColorScalingWidth / 2 +
								  static_cast<int>( std::round(
									  static_cast<float>( nLayers - nLayer ) /
									  static_cast<float>( nLayers - 1 ) *
									  static_cast<float>( nColorScalingWidth )
								  ) );

		return headerBaseColor.lighter( nColorScaling );
	};

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
	m_layerInfos.clear();
	int nCount = 1;
	if ( pComponent != nullptr && pComponent->hasSamples() ) {
		int nCurrentY = LayerPreview::nHeader;
		for ( const auto& ppLayer : *pComponent ) {
			if ( ppLayer == nullptr ) {
				continue;
			}

			const int x1 = (int) ( ppLayer->getStartVelocity() * width() );
			const int x2 = (int) ( ppLayer->getEndVelocity() * width() );

			const bool bSelected =
				nSelectedLayer != -1 ? nCount == ( nSelectedLayer + 1 ) : false;
			LayerInfo info{ x1, x2, nCurrentY, nCount, bSelected };
			m_layerInfos.insert( info );

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

	if ( m_layerInfos.size() == 0 ) {
	  // If there are no layers, draw an empty header
		p.fillRect(
			LayerPreview::nBorder, LayerPreview::nBorder, width() - 2 * LayerPreview::nBorder,
			LayerPreview::nHeader - LayerPreview::nBorder, headerBaseColor
		);
		p.setPen( pColorTheme->m_windowTextColor.darker( 145 ) );
		p.drawRect(
			LayerPreview::nBorder, LayerPreview::nBorder, width() - 3 * LayerPreview::nBorder,
			LayerPreview::nHeader - 2 * LayerPreview::nBorder
		);
	}
	else {
	  // We initialize the header with a strong and eye catching color in order
	  // to help the user detect holes between layers.
		p.fillRect(
			LayerPreview::nBorder, LayerPreview::nBorder, width() - 2 * LayerPreview::nBorder,
			LayerPreview::nHeader - LayerPreview::nBorder, missingLayerColor
		);

	}

	// We render the header after our first swipe over all layers in order to
	// get the overlaps right.
	int nCurrentEnd = width() - 2 * LayerPreview::nBorder;
	int ii = nLayers;
	for ( auto iinfo = m_layerInfos.rbegin(); iinfo != m_layerInfos.rend();
		  ++iinfo ) {
		if ( iinfo->nStartX > nCurrentEnd ) {
			// The borders of this layer were already drawn. Probably it was
			// covered by another one.
			--ii;
			continue;
		}

		const int nVisibleEnd = std::min( iinfo->nEndX, nCurrentEnd );
		const int nVisibleStart =
			std::max( iinfo->nStartX, LayerPreview::nBorder );

		const auto layerLabelColor = pickHeaderColor( ii );

		p.fillRect(
			std::max( iinfo->nStartX, LayerPreview::nBorder ), 0,
			nVisibleEnd - nVisibleStart,
			LayerPreview::nHeader - LayerPreview::nBorder, layerLabelColor
		);

		// Check whether there are other layers overlapping into the current
		// region.
		for ( const auto& iinfoOther : m_layerInfos ) {
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
		--ii;
	}

	// The number indicating the layer in the header should always win and be
	// visible. That's why render them in another swipe.
	p.setFont( QFont( pFontTheme->m_sLevel2FontFamily, getPointSizeButton() ) );
	p.setPen( pColorTheme->m_windowTextColor );
	for ( const auto& iinfo : m_layerInfos ) {
		if ( iinfo.bSelected ) {
			continue;
        }
		p.drawText(
			iinfo.nStartX, 0, iinfo.nEndX - iinfo.nStartX,
			LayerPreview::nHeader, Qt::AlignCenter,
			QString( "%1" ).arg( iinfo.nId )
		);
	}

	// Ensure the selected layer is properly highlighted in the header.
    ii = nLayers;
	for ( auto iinfo = m_layerInfos.rbegin(); iinfo != m_layerInfos.rend();
		  ++iinfo ) {
		if ( !iinfo->bSelected ) {
            --ii;
			continue;
		}

		const int nVisibleEnd =
			std::min( iinfo->nEndX, width() - 2 * LayerPreview::nBorder );
		const int nVisibleStart =
			std::max( iinfo->nStartX, LayerPreview::nBorder );

		p.setPen( highlightColor );
        p.fillRect(
			nVisibleStart, LayerPreview::nBorder, nVisibleEnd - nVisibleStart,
			LayerPreview::nHeader - LayerPreview::nBorder, pickHeaderColor( ii )
		);
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

		p.drawText(
			iinfo->nStartX, 0, iinfo->nEndX - iinfo->nStartX,
			LayerPreview::nHeader, Qt::AlignCenter,
			QString( "%1" ).arg( iinfo->nId )
		);

		break;
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
	if ( nSelectedLayer != -1 ) {
		p.setPen( highlightColor );
		p.drawRect(
			LayerPreview::nBorder,
			LayerPreview::nHeader + LayerPreview::nLayerHeight * nSelectedLayer,
			width() - 3 * LayerPreview::nBorder, LayerPreview::nLayerHeight
		);
	}

	// highlight dragged layer
	if ( m_drag == Drag::Position && m_nLastDragLayer != -1 ) {
		p.setPen( highlightColor );
		const int nY = LayerPreview::nHeader +
					   LayerPreview::nLayerHeight * m_nLastDragLayer;
		p.drawLine(
			LayerPreview::nBorder, nY, width() - LayerPreview::nBorder, nY
		);
	}
}

void LayerPreview::mouseDoubleClickEvent( QMouseEvent* ev )
{
	const auto pComponent = m_pComponentView->getComponent();
	if ( pComponent == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
	const auto nMaxLayers = pComponent->getLayers().size();

	const int nY = pEv->position().y();
	if ( nY <= LayerPreview::nHeader || pComponent->getLayers().size() == 0 ) {
		return;
	}

	const int nClickedLayer = static_cast<int>( std::floor(
		static_cast<float>( nY - LayerPreview::nHeader ) /
		static_cast<float>( LayerPreview::nLayerHeight )
	) );
	m_pComponentView->replaceLayer( nClickedLayer, "" );
}

void LayerPreview::mouseReleaseEvent( QMouseEvent* ev )
{
    m_drag = Drag::None;
}

void LayerPreview::mousePressEvent( QMouseEvent* ev )
{
	const int nPosition = 0;
	const int nSelectedLayer = m_pComponentView->getSelectedLayer();
	const auto pComponent = m_pComponentView->getComponent();
	if ( pComponent == nullptr ) {
		return;
	}

	const auto pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	if ( pInstrument == nullptr ) {
		// What is displayed in the component editor _is_ the selected
		// instrument. In case it is nullptr, we are working on inconsistent
		// data.
		ERRORLOG( "Invalid selected instrument" );
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
    m_drag = Drag::None;
    m_dragStartPoint = pEv->position();
    m_dragStartTimeStamp = pEv->timestamp();
    m_nLastDragLayer = LayerPreview::yToLayer( pEv->position().y() );

	const auto nMaxLayers = pComponent->getLayers().size();

	const int nX = pEv->position().x();
	const float fVelocity = std::clamp(
		static_cast<float>( nX ) / static_cast<float>( width() ), VELOCITY_MIN,
		VELOCITY_MAX
	);

	if ( pEv->position().y() < LayerPreview::nHeader ) {
		if ( pComponent->hasSamples() ) {
			auto pNote =
				std::make_shared<Note>( pInstrument, nPosition, fVelocity );

			// We register the current component to be rendered. This will cause
			// all other components _not_ to be rendered. Because we do not
			// provide a selected layer, the Sampler will select one for us
			// based on the current sample selection algorithm.
			pNote->setSelectedLayerInfo( nullptr, pComponent );

			Hydrogen::get_instance()->getAudioEngine()->getSampler()->noteOn(
				pNote
			);
		}

		// Update the layer selection based on the click position. Since there
		// can very well be overlaps, we have to ensure to pick the top most one
		// (which starts at the highest velocity).
		for ( auto iinfo = m_layerInfos.rbegin(); iinfo != m_layerInfos.rend();
			  ++iinfo ) {
			if ( nX > iinfo->nStartX && nX < iinfo->nEndX ) {
				if ( iinfo->nId != nSelectedLayer + 1 ) {
					m_pComponentView->setSelectedLayer( iinfo->nId - 1 );
					m_pComponentView->updateView();
				}
				break;
			}
		}
	}
	else if ( nMaxLayers > 0 ) {
		const int nClickedLayer = static_cast<int>( std::floor(
			static_cast<float>( pEv->position().y() - LayerPreview::nHeader ) /
			static_cast<float>( LayerPreview::nLayerHeight )
		) );
		auto pLayer = pComponent->getLayer( nClickedLayer );
		if ( pLayer != nullptr ) {
            m_drag = Drag::Initialized;

			m_pComponentView->setSelectedLayer( nClickedLayer );
			m_pComponentView->updateView();

			// We register the current component to be rendered using a
			// specific layer. This will cause all other components _not_ to
			// be rendered.
			auto pSelectedLayerInfo = std::make_shared<SelectedLayerInfo>();
			pSelectedLayerInfo->pLayer = pLayer;

			const auto pNote =
				std::make_shared<Note>( pInstrument, nPosition, fVelocity );
			pNote->setSelectedLayerInfo( pSelectedLayerInfo, pComponent );

			Hydrogen::get_instance()->getAudioEngine()->getSampler()->noteOn(
				pNote
			);
		}
	}
	else {
		// There is just an empty layer. But we allow to select it.
		// Otherwise UX would feel a little awkward.
		m_pComponentView->setSelectedLayer( 0 );
		m_pComponentView->updateView();
	}
}

void LayerPreview::mouseMoveEvent( QMouseEvent* ev )
{
	auto pComponent = m_pComponentView->getComponent();
	if ( pComponent == nullptr ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( ev );
	const int nX = pEv->position().x();
	const int nY = pEv->position().y();
    const int nSelectedLayer = m_pComponentView->getSelectedLayer();

	switch ( m_drag ) {
		case Drag::None: {
			if ( nY < LayerPreview::nHeader ) {
				setCursor( QCursor( m_speakerPixmap ) );
				return;
			}

			// We are hovering over an arbitrary layer.
			const int nHoveredLayer = static_cast<int>( std::floor(
				static_cast<float>(
					pEv->position().y() - LayerPreview::nHeader
				) /
				static_cast<float>( LayerPreview::nLayerHeight )
			) );
			auto pHoveredLayer = pComponent->getLayer( nHoveredLayer );
			if ( pHoveredLayer != nullptr ) {
				const int nStartX = static_cast<int>( std::round(
					pHoveredLayer->getStartVelocity() *
					static_cast<float>( width() )
				) );
				const int nEndX = static_cast<int>( std::round(
					pHoveredLayer->getEndVelocity() *
					static_cast<float>( width() )
				) );

				if ( ( nX < nStartX + LayerPreview::nBorderGrabMargin ) &&
					 ( nX > nStartX - LayerPreview::nBorderGrabMargin ) ) {
					setCursor( QCursor( Qt::SizeHorCursor ) );
					showLayerStartVelocity( pHoveredLayer, ev );
				}
				else if ( ( nX < nEndX + LayerPreview::nBorderGrabMargin ) &&
						  ( nX > nEndX - LayerPreview::nBorderGrabMargin ) ) {
					setCursor( QCursor( Qt::SizeHorCursor ) );
					showLayerEndVelocity( pHoveredLayer, ev );
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
			break;
		}
		case Drag::Initialized: {
			// We have not decided yet whether we do a horizontal or vertical
			// drag.
			if ( ( pEv->position() - m_dragStartPoint ).manhattanLength() <=
					 QApplication::startDragDistance() &&
				 ( pEv->timestamp() - m_dragStartTimeStamp ) <=
					 QApplication::startDragTime() ) {
                // Not there yet.
                break;
			}

            // On a perfectly diangular movement we will go with horizontal drag
            // since this is the more probable action.
			if ( std::abs( pEv->position().x() - m_dragStartPoint.x() ) >=
				 std::abs( pEv->position().y() - m_dragStartPoint.y() ) ) {
				// Horizontal drag

				auto pLayer = pComponent->getLayer( nSelectedLayer );
				if ( pLayer == nullptr ) {
					setCursor( QCursor( Qt::ArrowCursor ) );
					QToolTip::hideText();
					return;
				}

				const int nStartX = static_cast<int>( std::round(
					pLayer->getStartVelocity() * static_cast<float>( width() )
				) );
				const int nEndX = static_cast<int>( std::round(
					pLayer->getEndVelocity() * static_cast<float>( width() )
				) );
				const int nXDragStart = m_dragStartPoint.x();
				if ( ( nXDragStart < nStartX + LayerPreview::nBorderGrabMargin
					 ) &&
					 ( nXDragStart > nStartX - LayerPreview::nBorderGrabMargin
					 ) ) {
					m_drag = Drag::VelocityStart;
				}
				else if ( ( nXDragStart <
							nEndX + LayerPreview::nBorderGrabMargin ) &&
						  ( nXDragStart >
							nEndX - LayerPreview::nBorderGrabMargin ) ) {
					m_drag = Drag::VelocityEnd;
				}
				else {
					// The user missed the border. Invalid drag action.
					m_drag = Drag::None;
				}
			}
			else {
				// Vertical drag.
                m_drag = Drag::Position;
				auto pDrag = new QDrag( this );
				auto pMimeData = new QMimeData;
				pMimeData->setText( "ComponentViewLayer" );
				pDrag->setMimeData( pMimeData );
				if ( pEv->modifiers() & Qt::ShiftModifier ) {
					pDrag->exec( Qt::CopyAction );
				}
				else {
					pDrag->exec( Qt::MoveAction );
				}

                // Once exec() returns, the drag is done.
                m_drag = Drag::None;
			}
			break;
		}
		case Drag::VelocityStart:
		case Drag::VelocityEnd: {
			// We are dragging a border of the currently selected layer.
			auto pLayer = pComponent->getLayer( nSelectedLayer );
			if ( pLayer == nullptr ) {
				setCursor( QCursor( Qt::ArrowCursor ) );
				QToolTip::hideText();
				return;
			}

			const float fVelocity = std::clamp(
				static_cast<float>( nX ) / static_cast<float>( width() ),
				VELOCITY_MIN, VELOCITY_MAX
			);

			bool bChanged = false;
			if ( m_drag == Drag::VelocityStart ) {
				if ( fVelocity < pLayer->getEndVelocity() ) {
					pLayer->setStartVelocity( fVelocity );
					bChanged = true;
					showLayerStartVelocity( pLayer, ev );
				}
			}
			else {
				if ( fVelocity > pLayer->getStartVelocity() ) {
					pLayer->setEndVelocity( fVelocity );
					bChanged = true;
					showLayerEndVelocity( pLayer, ev );
				}
			}

			if ( bChanged ) {
				update();
				Hydrogen::get_instance()->setIsModified( true );
			}
			break;
		}
		case Drag::Position:
			// Vertical drag
			break;
		default:
			ERRORLOG( QString( "Unknown drag type [%1]" )
						  .arg( static_cast<int>( m_drag ) ) );
	}
}

int LayerPreview::getMidiVelocityFromRaw( const float raw )
{
	return static_cast<int>( Midi::parameterFromIntClamp(
		static_cast<int>( raw * static_cast<float>( Midi::ParameterMaximum ) )
	) );
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

QString LayerPreview::DragToQString( const Drag& drag )
{
	switch ( drag ) {
		case Drag::None:
			return "None";
		case Drag::Initialized:
			return "Initialized";
		case Drag::Position:
			return "Position";
		case Drag::VelocityEnd:
			return "VelocityEnd";
		case Drag::VelocityStart:
			return "VelocityStart";
		default:
			return QString( "Unknown Drag [%1]" )
				.arg( static_cast<int>( drag ) );
	}
}
