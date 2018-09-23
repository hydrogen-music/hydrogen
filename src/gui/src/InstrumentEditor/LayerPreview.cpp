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

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/sampler/Sampler.h>
using namespace H2Core;

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "InstrumentEditorPanel.h"
#include "LayerPreview.h"

const char* LayerPreview::__class_name = "LayerPreview";

LayerPreview::LayerPreview( QWidget* pParent )
 : QWidget( pParent )
 , Object( __class_name )
 , m_pInstrument( NULL )
 , m_nSelectedComponent( 0 )
 , m_nSelectedLayer( 0 )
 , m_bMouseGrab( false )
{
	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );

	setMouseTracking( true );

//
	int w = 276;
	if( InstrumentComponent::getMaxLayers() > 16)
		w = 261;
	int h = 20 + m_nLayerHeight * InstrumentComponent::getMaxLayers();
	resize( w, h );

	m_speakerPixmap.load( Skin::getImagePath() + "/instrumentEditor/speaker.png" );

	HydrogenApp::get_instance()->addEventListener( this );

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
	p.fillRect( ev->rect(), QColor( 58, 62, 72 ) );

	int nLayers = 0;
	for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
		if ( m_pInstrument ) {
			InstrumentComponent* p_compo = m_pInstrument->get_component(m_nSelectedComponent);
			if(p_compo) {
				InstrumentLayer *pLayer = p_compo->get_layer( i );
				if ( pLayer ) {
					nLayers++;
				}
			}
		}
	}

	int nLayer = 0;
	for ( int i = InstrumentComponent::getMaxLayers() - 1; i >= 0; i-- ) {
		int y = 20 + m_nLayerHeight * i;
		QString label = "< - >";
		
		if ( m_pInstrument ) {
			InstrumentComponent* p_compo = m_pInstrument->get_component(m_nSelectedComponent);
			if(p_compo) {
				InstrumentLayer *pLayer = p_compo->get_layer( i );
				
				if ( pLayer ) {
					Sample* pSample = pLayer->get_sample();
					if( pSample != NULL) {
						label = pSample->get_filename();
					}
					
					int x1 = (int)( pLayer->get_start_velocity() * width() );
					int x2 = (int)( pLayer->get_end_velocity() * width() );
					
					int red = (int)( 128.0 / nLayers * nLayer );
					int green = (int)( 134.0 / nLayers * nLayer );
					int blue = (int)( 152.0 / nLayers * nLayer );
					QColor layerColor( red, green, blue );
					
					p.fillRect( x1, 0, x2 - x1, 19, layerColor );
					p.setPen( QColor( 230, 230, 230 ) );
					p.drawText( x1, 0, x2 - x1, 20, Qt::AlignCenter, QString("%1").arg( i + 1 ) );
					
					if ( m_nSelectedLayer == i ) {
						p.setPen( QColor( 210, 0, 0 ) );
					}
					p.drawRect( x1, 1, x2 - x1 - 1, 18 );	// bordino in alto
					
					// layer view
					p.fillRect( 0, y, width(), m_nLayerHeight, QColor( 25, 44, 65 ) );
					p.fillRect( x1, y, x2 - x1, m_nLayerHeight, QColor( 90, 160, 233 ) );
					
					nLayer++;
				}
				else {
					// layer view
					p.fillRect( 0, y, width(), m_nLayerHeight, QColor( 59, 73, 96 ) );
				}
			}
			else {
				// layer view
				p.fillRect( 0, y, width(), m_nLayerHeight, QColor( 59, 73, 96 ) );
			}
		}
		else {
			// layer view
			p.fillRect( 0, y, width(), m_nLayerHeight, QColor( 59, 73, 96 ) );
		}
		p.setPen( QColor( 128, 134, 152 ) );
		p.drawRect( 0, y, width() - 1, m_nLayerHeight );
		p.drawText( 10, y, width() - 10, 20, Qt::AlignLeft, QString( "%1: %2" ).arg( i + 1 ).arg( label ) );
	}
	
	// selected layer
	p.setPen( QColor( 210, 0, 0 ) );
	int y = 20 + m_nLayerHeight * m_nSelectedLayer;
	p.drawRect( 0, y, width() - 1, m_nLayerHeight );
	
	
}



void LayerPreview::selectedInstrumentChangedEvent()
{
	AudioEngine::get_instance()->lock( RIGHT_HERE );
	Song *pSong = Hydrogen::get_instance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->get_instrument_list();
		int nInstr = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		if ( nInstr >= (int)pInstrList->size() ) {
			nInstr = -1;
		}

		if (nInstr == -1) {
			m_pInstrument = NULL;
		}
		else {
			m_pInstrument = pInstrList->get( nInstr );
		}
	}
	else {
		m_pInstrument = NULL;
	}
	AudioEngine::get_instance()->unlock();
	
	/*
	if ( m_pInstrument ) {
		InstrumentComponent* p_tmpCompo = m_pInstrument->get_component( m_nSelectedComponent );
		if(!p_tmpCompo) {
			for(int i = 0 ; i < InstrumentComponent::getMaxLayers() ; i++) {
				p_tmpCompo = m_pInstrument->get_component( i );
				if(p_tmpCompo) {
					m_nSelectedComponent = i;
					break;
				}
			}
		}
	}
	*/
	
	// select the last valid layer
	if ( m_pInstrument ) {
		for (int i = InstrumentComponent::getMaxLayers() - 1; i >= 0; i-- ) {
			InstrumentComponent* p_compo = m_pInstrument->get_component(m_nSelectedComponent);
			if ( p_compo ) {
				if ( p_compo->get_layer( i ) ) {
					m_nSelectedLayer = i;
					break;
				}
			}
			else {
				m_nSelectedLayer = 0;
			}
		}
	}
	else {
		m_nSelectedLayer = 0;
	}

	update();
}



void LayerPreview::mouseReleaseEvent(QMouseEvent *ev)
{
	m_bMouseGrab = false;
	setCursor( QCursor( Qt::ArrowCursor ) );

	/*
	 * We want the tooltip to still show if mouse pointer
	 * is over an active layer's boundary
	 */
	InstrumentComponent *pCompo = m_pInstrument->get_component(m_nSelectedComponent);
	if ( pCompo ) {
		InstrumentLayer *pLayer = pCompo->get_layer( m_nSelectedLayer );
		
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
	const unsigned nPosition = 0;
	const float fPan_L = 0.5f;
	const float fPan_R = 0.5f;
	const int nLength = -1;
	const float fPitch = 0.0f;

	if ( !m_pInstrument ) {
		return;
	}
	if ( ev->y() < 20 ) {
		float fVelocity = (float)ev->x() / (float)width();

		Note *note = new Note( m_pInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch );
		note->set_specific_compo_id( m_nSelectedComponent );
		AudioEngine::get_instance()->get_sampler()->note_on(note);
		
		for ( int i = 0; i < InstrumentComponent::getMaxLayers(); i++ ) {
			InstrumentComponent *pCompo = m_pInstrument->get_component(m_nSelectedComponent);
			if(pCompo){
				InstrumentLayer *pLayer = pCompo->get_layer( i );
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
		
		InstrumentComponent *pCompo = m_pInstrument->get_component(m_nSelectedComponent);
		if(pCompo) {
			InstrumentLayer *pLayer = pCompo->get_layer( m_nSelectedLayer );
			if ( pLayer ) {
				Note *note = new Note( m_pInstrument , nPosition, m_pInstrument->get_component(m_nSelectedComponent)->get_layer( m_nSelectedLayer )->get_end_velocity() - 0.01, fPan_L, fPan_R, nLength, fPitch );
				note->set_specific_compo_id( m_nSelectedComponent );
				AudioEngine::get_instance()->get_sampler()->note_on(note);
				
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
	if ( !m_pInstrument ) {
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
		InstrumentLayer *pLayer = m_pInstrument->get_component(m_nSelectedComponent)->get_layer( m_nSelectedLayer );
		if ( pLayer ) {
			if ( m_bMouseGrab ) {
				if ( m_bGrabLeft ) {
					if ( fVel < pLayer->get_end_velocity()) {
						pLayer->set_start_velocity(fVel);
						showLayerStartVelocity(pLayer, ev);
					}
				}
				else {
					if ( fVel > pLayer->get_start_velocity()) {
						pLayer->set_end_velocity( fVel );
						showLayerEndVelocity(pLayer, ev);
					}
				}
				update();
			}
		}
	}
	else {
		m_nSelectedLayer = ( ev->y() - 20 ) / m_nLayerHeight;
		if ( m_nSelectedLayer < InstrumentComponent::getMaxLayers() ) {
			InstrumentComponent* p_compo = m_pInstrument->get_component(m_nSelectedComponent);
			if(p_compo){
				InstrumentLayer *pLayer = p_compo->get_layer( m_nSelectedLayer );
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

void LayerPreview::showLayerStartVelocity( const InstrumentLayer* pLayer, const QMouseEvent* pEvent )
{
	const float fVelo = pLayer->get_start_velocity();

	QToolTip::showText( pEvent->globalPos(),
			trUtf8( "Dec. = %1\nMIDI = %2" )
				.arg( QString::number( fVelo, 'f', 2) )
				.arg( getMidiVelocityFromRaw( fVelo ) +1 ),
			this);
}

void LayerPreview::showLayerEndVelocity( const InstrumentLayer* pLayer, const QMouseEvent* pEvent )
{
	const float fVelo = pLayer->get_end_velocity();

	QToolTip::showText( pEvent->globalPos(),
			trUtf8( "Dec. = %1\nMIDI = %2" )
				.arg( QString::number( fVelo, 'f', 2) )
				.arg( getMidiVelocityFromRaw( fVelo ) +1 ),
			this);
}
