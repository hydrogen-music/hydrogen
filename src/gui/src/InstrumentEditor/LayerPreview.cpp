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

#include <QtGui>
#include <QtWidgets>

#include <core/Hydrogen.h>
#include <core/Basics/Song.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Sampler/Sampler.h>
#include <core/Preferences/Theme.h>

using namespace H2Core;

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "InstrumentEditorPanel.h"
#include "LayerPreview.h"

LayerPreview::LayerPreview( QWidget* pParent )
 : QWidget( pParent )
 , m_pInstrument( nullptr )
 , m_nSelectedComponent( 0 )
 , m_nSelectedLayer( 0 )
 , m_bMouseGrab( false )
 , m_bGrabLeft( false )
{
	setAttribute(Qt::WA_OpaquePaintEvent);

	setMouseTracking( true );

	int width = 276;
	if( InstrumentComponent::getMaxLayers() > 16) {
		width = 261;
	}
	
	int height = 20 + m_nLayerHeight * InstrumentComponent::getMaxLayers();
	resize( width, height );

	m_speakerPixmap.load( Skin::getSvgImagePath() + "/icons/white/speaker.svg" );

	HydrogenApp::get_instance()->addEventListener( this );

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &LayerPreview::onPreferencesChanged );

	/**
	 * We get a style similar to the one used for the 2 buttons on top of the instrument editor panel
	 */
	this->setStyleSheet("font-size: 9px; font-weight: bold;");
}

LayerPreview::~ LayerPreview()
{
	//INFOLOG( "DESTROY" );
}

void LayerPreview::set_selected_component( int SelectedComponent )
{
	m_nSelectedComponent = SelectedComponent;
}

void LayerPreview::paintEvent(QPaintEvent *ev)
{
	QPainter p( this );
	
	auto pPref = H2Core::Preferences::get_instance();

	QFont fontText( pPref->getTheme().m_font.m_sLevel2FontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	QFont fontButton( pPref->getTheme().m_font.m_sLevel2FontFamily, getPointSizeButton() );
	
	p.fillRect( ev->rect(), pPref->getTheme().m_color.m_windowColor );

	int nLayers = 0;
	for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
		if ( m_pInstrument != nullptr ) {
			auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );
			if ( pComponent != nullptr ) {
				auto pLayer = pComponent->getLayer( i );
				if ( pLayer != nullptr ) {
					nLayers++;
				}
			}
		}
	}
	
	// How much the color of the labels for the individual layers
	// are allowed to diverge from the general window color.
	int nColorScalingWidth = 90;
	int nColorScaling = 100;

	QColor layerLabelColor, layerSegmentColor, highlightColor;
	if ( InstrumentEditorPanel::get_instance()->getInstrumentEditor()->getIsActive() ) {
		highlightColor = pPref->getTheme().m_color.m_highlightColor;
	} else {
		highlightColor = pPref->getTheme().m_color.m_lightColor;
	}

	int nLayer = 0;
	for ( int i = InstrumentComponent::getMaxLayers() - 1; i >= 0; i-- ) {
		int y = 20 + m_nLayerHeight * i;
		QString label = "< - >";
		
		if ( m_pInstrument != nullptr ) {
			auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );
			if ( pComponent != nullptr ) {
				auto pLayer = pComponent->getLayer( i );
				
				if ( pLayer != nullptr && nLayers > 0 ) {
					auto pSample = pLayer->get_sample();
					if ( pSample != nullptr ) {
						label = pSample->get_filename();
						layerSegmentColor =
							pPref->getTheme().m_color.m_accentColor.lighter( 130 );
					} else {
						layerSegmentColor =
							pPref->getTheme().m_color.m_buttonRedColor;
					}
						
					
					int x1 = (int)( pLayer->get_start_velocity() * width() );
					int x2 = (int)( pLayer->get_end_velocity() * width() );

					// Labels for layers to the left will have a
					// lighter color as those to the right.
					nColorScaling =
						static_cast<int>(std::round( static_cast<float>(nLayer) /
													 static_cast<float>(nLayers) * 2 *
													 static_cast<float>(nColorScalingWidth) ) ) -
						nColorScalingWidth + 100;
					layerLabelColor =
						pPref->getTheme().m_color.m_windowColor.lighter( nColorScaling );
					
					p.fillRect( x1, 0, x2 - x1, 19, layerLabelColor );
					p.setPen( pPref->getTheme().m_color.m_windowTextColor );
					p.setFont( fontButton );
					p.drawText( x1, 0, x2 - x1, 20, Qt::AlignCenter, QString("%1").arg( i + 1 ) );
					
					if ( m_nSelectedLayer == i ) {
						p.setPen( highlightColor );
					} else {
						p.setPen( pPref->getTheme().m_color.m_windowTextColor.darker( 145 ) );
					}
					p.drawRect( x1, 1, x2 - x1 - 1, 18 );	// bordino in alto
					
					// layer view
					p.fillRect( 0, y, width(), m_nLayerHeight,
								pPref->getTheme().m_color.m_windowColor );
					p.fillRect( x1, y, x2 - x1, m_nLayerHeight, layerSegmentColor );
					
					nLayer++;
				}
				else {
					// layer view
					p.fillRect( 0, y, width(), m_nLayerHeight,
								pPref->getTheme().m_color.m_windowColor );
				}
			}
			else {
				// layer view
				p.fillRect( 0, y, width(), m_nLayerHeight,
							pPref->getTheme().m_color.m_windowColor );
			}
		}
		else {
			// layer view
			p.fillRect( 0, y, width(), m_nLayerHeight,
							pPref->getTheme().m_color.m_windowColor );
		}
		QColor layerTextColor = pPref->getTheme().m_color.m_windowTextColor;
		layerTextColor.setAlpha( 155 );
		p.setPen( layerTextColor );
		p.setFont( fontText );
		p.drawText( 10, y, width() - 10, 20, Qt::AlignLeft, QString( "%1: %2" ).arg( i + 1 ).arg( label ) );
		p.setPen( layerTextColor.darker( 145 ) );
		p.drawRect( 0, y, width() - 1, m_nLayerHeight );
	}

	// selected layer
	p.setPen( highlightColor );
	int y = 20 + m_nLayerHeight * m_nSelectedLayer;
	p.drawRect( 0, y, width() - 1, m_nLayerHeight );
}

void LayerPreview::drumkitLoadedEvent() {
	selectedInstrumentChangedEvent();
}

void LayerPreview::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		selectedInstrumentChangedEvent();
	}
}

void LayerPreview::selectedInstrumentChangedEvent()
{
	m_pInstrument = Hydrogen::get_instance()->getSelectedInstrument();
	
	bool bSelectedLayerChanged = false;
	
	// select the last valid layer
	if ( m_pInstrument != nullptr ) {
		for ( int i = InstrumentComponent::getMaxLayers() - 1; i >= 0; i-- ) {
			auto pComponent = m_pInstrument->get_component( m_nSelectedComponent );
			if ( pComponent != nullptr && pComponent->getLayer( i ) ) {
				m_nSelectedLayer = i;
				bSelectedLayerChanged = true;
				break;
			}
			else {
				m_nSelectedLayer = 0;
				bSelectedLayerChanged = true;
			}
		}
	}
	else {
		m_nSelectedLayer = 0;
		bSelectedLayerChanged = true;
	}

	if ( bSelectedLayerChanged ) {
		InstrumentEditorPanel::get_instance()->selectLayer( m_nSelectedLayer );
	}
		
	update();
}

void LayerPreview::mouseReleaseEvent(QMouseEvent *ev)
{
	m_bMouseGrab = false;
	setCursor( QCursor( Qt::ArrowCursor ) );

	if ( m_pInstrument == nullptr ) {
		return;
	}

	/*
	 * We want the tooltip to still show if mouse pointer
	 * is over an active layer's boundary
	 */
	auto pCompo = m_pInstrument->get_component( m_nSelectedComponent );
	if ( pCompo ) {
		auto pLayer = pCompo->getLayer( m_nSelectedLayer );

		if ( pLayer ) {
			int x1 = (int)( pLayer->get_start_velocity() * width() );
			int x2 = (int)( pLayer->get_end_velocity() * width() );
			
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

	if ( m_pInstrument == nullptr ) {
		return;
	}
	if ( ev->y() < 20 ) {
		const float fVelocity = (float)ev->x() / (float)width();

		if ( m_pInstrument->hasSamples() ) {
			auto pNote = std::make_shared<Note>( m_pInstrument, nPosition,
												 fVelocity );
			pNote->setSpecificCompoIdx( m_nSelectedComponent );
			Hydrogen::get_instance()->getAudioEngine()->getSampler()->noteOn(pNote);
		}
		
		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if(pCompo){
				auto pLayer = pCompo->getLayer( i );
				if ( pLayer ) {
					if ( ( fVelocity > pLayer->get_start_velocity()) && ( fVelocity < pLayer->get_end_velocity() ) ) {
						if ( i != m_nSelectedLayer ) {
							m_nSelectedLayer = i;
							update();
							InstrumentEditorPanel::get_instance()->selectLayer( m_nSelectedLayer );
						}
						break;
					}
				}
			}
		}
	}
	else {
		m_nSelectedLayer = ( ev->y() - 20 ) / m_nLayerHeight;

		update();
		InstrumentEditorPanel::get_instance()->selectLayer( m_nSelectedLayer );
		
		auto pCompo = m_pInstrument->get_component(m_nSelectedComponent);
		if( pCompo != nullptr ) {
			auto pLayer = pCompo->getLayer( m_nSelectedLayer );
			if ( pLayer != nullptr ) {
				const float fVelocity = pLayer->get_end_velocity() - 0.01;
				auto pNote = std::make_shared<Note>(
					m_pInstrument, nPosition, fVelocity );
				pNote->setSpecificCompoIdx( m_nSelectedComponent );
				Hydrogen::get_instance()->getAudioEngine()->getSampler()->
					noteOn( pNote );
				
				int x1 = (int)( pLayer->get_start_velocity() * width() );
				int x2 = (int)( pLayer->get_end_velocity() * width() );
				
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
	if ( m_pInstrument == nullptr ) {
		return;
	}

	int x = ev->pos().x();
	int y = ev->pos().y();

	float fVel = (float)x / (float)width();
	if (fVel < 0 ) {
		fVel = 0;
	}
	else  if (fVel > 1) {
		fVel = 1;
	}

	if ( y < 20 ) {
		setCursor( QCursor( m_speakerPixmap ) );
		return;
	}
	if ( m_bMouseGrab ) {
		auto pLayer = m_pInstrument->get_component( m_nSelectedComponent )->getLayer( m_nSelectedLayer );
		if ( pLayer ) {
			if ( m_bMouseGrab ) {
				bool bChanged = false;
				if ( m_bGrabLeft ) {
					if ( fVel < pLayer->get_end_velocity()) {
						pLayer->set_start_velocity( fVel );
						bChanged = true;
						showLayerStartVelocity( pLayer, ev );
					}
				}
				else {
					if ( fVel > pLayer->get_start_velocity()) {
						pLayer->set_end_velocity( fVel );
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
	}
	else {
		m_nSelectedLayer = ( ev->y() - 20 ) / m_nLayerHeight;
		if ( m_nSelectedLayer < InstrumentComponent::getMaxLayers() ) {
			auto pComponent = m_pInstrument->get_component(m_nSelectedComponent);
			if( pComponent ){
				auto pLayer = pComponent->getLayer( m_nSelectedLayer );
				if ( pLayer ) {
					int x1 = (int)( pLayer->get_start_velocity() * width() );
					int x2 = (int)( pLayer->get_end_velocity() * width() );
					
					if ( ( x < x1  + 5 ) && ( x > x1 - 5 ) ){
						setCursor( QCursor( Qt::SizeHorCursor ) );
						showLayerStartVelocity(pLayer, ev);
					}
					else if ( ( x < x2 + 5 ) && ( x > x2 - 5 ) ){
						setCursor( QCursor( Qt::SizeHorCursor ) );
						showLayerEndVelocity(pLayer, ev);
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
}

void LayerPreview::updateAll()
{
	update();
}

int LayerPreview::getMidiVelocityFromRaw( const float raw )
{
	return static_cast<int> (raw * 127);
}

void LayerPreview::showLayerStartVelocity( const std::shared_ptr<InstrumentLayer> pLayer, const QMouseEvent* pEvent )
{
	const float fVelo = pLayer->get_start_velocity();

	QToolTip::showText( pEvent->globalPos(),
			tr( "Dec. = %1\nMIDI = %2" )
				.arg( QString::number( fVelo, 'f', 2) )
				.arg( getMidiVelocityFromRaw( fVelo ) +1 ),
			this);
}

void LayerPreview::showLayerEndVelocity( const std::shared_ptr<InstrumentLayer> pLayer, const QMouseEvent* pEvent )
{
	const float fVelo = pLayer->get_end_velocity();

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

void LayerPreview::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Font |
					 H2Core::Preferences::Changes::Colors ) ) {
		update();
	}
}
