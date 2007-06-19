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

#include <QCursor>
#include <QPainter>

#include <hydrogen/Hydrogen.h>
#include <hydrogen/Song.h>
#include <hydrogen/Instrument.h>
#include <hydrogen/note.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/sampler/Sampler.h>
using namespace H2Core;

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "InstrumentEditorPanel.h"
#include "LayerPreview.h"


LayerPreview::LayerPreview( QWidget* pParent )
 : QWidget( pParent )
 , Object( "LayerPreview" )
 , m_pInstrument( NULL )
 , m_nSelectedLayer( 0 )
 , m_bMouseGrab( false )
{
	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );

	setMouseTracking( true );

	int w = 234;
	int h = 20 + m_nLayerHeight * MAX_LAYERS;
	resize( w, h );

	m_speakerPixmap.load( Skin::getImagePath() + "/instrumentEditor/speaker.png" );

	HydrogenApp::getInstance()->addEventListener( this );
}



LayerPreview::~ LayerPreview()
{
	//INFOLOG( "DESTROY" );
}


void LayerPreview::paintEvent(QPaintEvent *ev)
{

	QPainter p( this );
	p.fillRect( ev->rect(), QColor( 58, 62, 72 ) );

	int nLayers = 0;
	for ( int i = 0; i < MAX_LAYERS; i++ ) {
		if ( m_pInstrument ) {
			InstrumentLayer *pLayer = m_pInstrument->getLayer( i );
			if ( pLayer ) {
				nLayers++;
			}
		}
	}

	int nLayer = 0;
	for ( int i = MAX_LAYERS - 1; i >= 0; i-- ) {
		int y = 20 + m_nLayerHeight * i;

		if ( m_pInstrument ) {
			InstrumentLayer *pLayer = m_pInstrument->getLayer( i );

			if ( pLayer ) {
				int x1 = (int)( pLayer->m_fStartVelocity * width() );
				int x2 = (int)( pLayer->m_fEndVelocity * width() );

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
		p.setPen( QColor( 128, 134, 152 ) );
		p.drawRect( 0, y, width() - 1, m_nLayerHeight );
	}

	// selected layer
	p.setPen( QColor( 210, 0, 0 ) );
	int y = 20 + m_nLayerHeight * m_nSelectedLayer;
	p.drawRect( 0, y, width() - 1, m_nLayerHeight );


}



void LayerPreview::selectedInstrumentChangedEvent()
{
	AudioEngine::getInstance()->lock( "LayerPreview::selectedInstrumentChangedEvent" );
	Song *pSong = Hydrogen::getInstance()->getSong();
	if (pSong != NULL) {
		InstrumentList *pInstrList = pSong->getInstrumentList();
		int nInstr = Hydrogen::getInstance()->getSelectedInstrumentNumber();
		if ( nInstr >= (int)pInstrList->getSize() ) {
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
	AudioEngine::getInstance()->unlock();

	// select the last valid layer
	if ( m_pInstrument ) {
		for (int i = MAX_LAYERS - 1; i >= 0; i-- ) {
			if ( m_pInstrument->getLayer( i ) ) {
				m_nSelectedLayer = i;
				break;
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
	UNUSED( ev );
	m_bMouseGrab = false;
	setCursor( QCursor( Qt::ArrowCursor ) );
}



void LayerPreview::mousePressEvent(QMouseEvent *ev)
{
	const unsigned nPosition = 0;
	const float fPan_L = 0.5f;
	const float fPan_R = 0.5f;
	const int nLenght = -1;
	const float fPitch = 0.0f;

	if ( !m_pInstrument ) {
		return;
	}
	if ( ev->y() < 20 ) {
		float fVelocity = (float)ev->x() / (float)width();

		Note *note = new Note( m_pInstrument, nPosition, fVelocity, fPan_L, fPan_R, nLenght, fPitch );
		note->setInstrument( m_pInstrument );
		AudioEngine::getInstance()->getSampler()->note_on(note);

		for ( int i = 0; i < MAX_LAYERS; i++ ) {
			InstrumentLayer *pLayer = m_pInstrument->getLayer( i );
			if ( pLayer ) {
				if ( ( fVelocity > pLayer->m_fStartVelocity ) && ( fVelocity < pLayer->m_fEndVelocity ) ) {
					if ( i != m_nSelectedLayer ) {
						m_nSelectedLayer = i;
						update();
						InstrumentEditorPanel::getInstance()->selectLayer( m_nSelectedLayer );
					}
					break;
				}
			}
		}
	}
	else {
		m_nSelectedLayer = ( ev->y() - 20 ) / m_nLayerHeight;
		InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );

		update();
		InstrumentEditorPanel::getInstance()->selectLayer( m_nSelectedLayer );

		if ( m_pInstrument->getLayer( m_nSelectedLayer ) ) {
			Note *note = new Note( m_pInstrument , nPosition, m_pInstrument->getLayer( m_nSelectedLayer )->m_fEndVelocity - 0.01, fPan_L, fPan_R, nLenght, fPitch );
			AudioEngine::getInstance()->getSampler()->note_on(note);
		}

		if ( pLayer ) {
			int x1 = (int)( pLayer->m_fStartVelocity * width() );
			int x2 = (int)( pLayer->m_fEndVelocity * width() );

			if ( ( ev->x() < x1  + 5 ) && ( ev->x() > x1 - 5 ) ){
				setCursor( QCursor( Qt::SizeHorCursor ) );
				m_bGrabLeft = true;
				m_bMouseGrab = true;
			}
			else if ( ( ev->x() < x2 + 5 ) && ( ev->x() > x2 - 5 ) ){
				setCursor( QCursor( Qt::SizeHorCursor ) );
				m_bGrabLeft = false;
				m_bMouseGrab = true;
			}
			else {
				setCursor( QCursor( Qt::ArrowCursor ) );
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
		InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
		if ( pLayer ) {
			if ( m_bMouseGrab ) {
				if ( m_bGrabLeft ) {
					if ( fVel < pLayer->m_fEndVelocity ) {
						pLayer->m_fStartVelocity = fVel;
					}
				}
				else {
					if ( fVel > pLayer->m_fStartVelocity ) {
						pLayer->m_fEndVelocity = fVel;
					}
				}
				update();
			}
		}
	}
	else {
		m_nSelectedLayer = ( ev->y() - 20 ) / m_nLayerHeight;
		if ( m_nSelectedLayer < MAX_LAYERS ) {
			InstrumentLayer *pLayer = m_pInstrument->getLayer( m_nSelectedLayer );
			if ( pLayer ) {
				int x1 = (int)( pLayer->m_fStartVelocity * width() );
				int x2 = (int)( pLayer->m_fEndVelocity * width() );

				if ( ( x < x1  + 5 ) && ( x > x1 - 5 ) ){
					setCursor( QCursor( Qt::SizeHorCursor ) );
				}
				else if ( ( x < x2 + 5 ) && ( x > x2 - 5 ) ){
					setCursor( QCursor( Qt::SizeHorCursor ) );
				}
				else {
					setCursor( QCursor( Qt::ArrowCursor ) );
				}
			}
			else {
				setCursor( QCursor( Qt::ArrowCursor ) );
			}
		}
	}
}



void LayerPreview::updateAll()
{
	update();
}
