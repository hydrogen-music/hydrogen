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

#include "LayerWaveDisplay.h"

#include "ComponentView.h"
#include "../../Compatibility/DropEvent.h"

using namespace H2Core;

LayerWaveDisplay::LayerWaveDisplay( ComponentView* pComponentView )
	: WaveDisplay( pComponentView ),
	  m_pComponentView( pComponentView )
{
	setAcceptDrops( true );
}

LayerWaveDisplay::~LayerWaveDisplay()
{
}

void LayerWaveDisplay::dragEnterEvent( QDragEnterEvent* event )
{
	if ( event->mimeData()->hasFormat( "text/uri-list" ) ) {
	 	event->acceptProposedAction();
    }
}

void LayerWaveDisplay::dragMoveEvent( QDragMoveEvent* event )
{
	event->accept();
}

void LayerWaveDisplay::dropEvent( QDropEvent* event )
{
    auto pEv = static_cast<DropEvent*>( event );

	const QMimeData* mimeData = pEv->mimeData();
	QString sText = pEv->mimeData()->text();

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
            // We only apply the first valid path provided.
			m_pComponentView->replaceLayer(
				m_pComponentView->getSelectedLayer(), filePaths.front()
			);
		}
	}
}
