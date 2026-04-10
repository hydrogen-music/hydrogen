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

#include "SoundLibraryTree.h"

#include <QMimeData>

#include <core/SoundLibrary/SoundLibraryInfo.h>

#include "SoundLibraryPanel.h"
#include "../../Compatibility/MouseEvent.h"

using namespace H2Core;

SoundLibraryTree::SoundLibraryTree(
	SoundLibraryPanel* pParent,
	Filesystem::Artifact artifact,
	bool bStandAlone
)
	: QTreeWidget( pParent ),
	  m_pSoundLibraryPanel( pParent ),
	  m_artifact( artifact ),
	  m_bStandAlone( bStandAlone )
{
	setAlternatingRowColors( true );
	setRootIsDecorated( false );
	headerItem()->setHidden( true );

	connect( this, &QTreeWidget::currentItemChanged, [&]() {
		m_pSoundLibraryPanel->updateDetailView();
	} );

}

void SoundLibraryTree::mousePressEvent( QMouseEvent* event )
{
	//	INFOLOG( "[mousePressEvent]" );
	QTreeWidget::mousePressEvent( event );

	auto pEv = static_cast<MouseEvent*>( event );

	if ( event->button() == Qt::RightButton ) {
		emit rightClicked( pEv->globalPosition().toPoint() );
	}
	else if ( event->button() == Qt::LeftButton ) {
		emit leftClicked( pEv->globalPosition().toPoint() );
	}
}

void SoundLibraryTree::mouseMoveEvent( QMouseEvent* pEvent )
{
	if ( m_bStandAlone ) {
		return;
	}

    // Initialize drag and drop events
	if ( !( pEvent->buttons() & Qt::LeftButton ) ) {
		return;
	}

	if ( currentItem() == nullptr ) {
		return;
	}

	auto it = m_registry.find( currentItem() );
	if ( it == m_registry.end() || it->second == nullptr ) {
		return;
	}

	const QString sMimeText =
		QString( "drag %1::%2" )
			.arg( Filesystem::ArtifactToQString( m_artifact ) )
			.arg( it->second->getPath() );

	auto pDrag = new QDrag( this );
	auto pMimeData = new QMimeData;
	pMimeData->setText( sMimeText );
	pDrag->setMimeData( pMimeData );
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
}
