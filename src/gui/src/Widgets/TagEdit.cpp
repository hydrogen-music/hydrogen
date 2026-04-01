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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "TagEdit.h"

#include "../Skin.h"

#include <core/Globals.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

TagEdit::TagEdit( QWidget* pParent ) : QWidget( pParent )
{
	setFocusPolicy( Qt::ClickFocus );

	auto pMainLayout = new QVBoxLayout();
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	pMainLayout->setSpacing( 0 );
	setLayout( pMainLayout );

	auto pScrollArea = new QScrollArea( this );
	pScrollArea->setObjectName( "TagEditScrollArea" );
	pScrollArea->setWidgetResizable( true );
	pScrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	pMainLayout->addWidget( pScrollArea );

	auto pVBoxLayout = new QVBoxLayout();
	pVBoxLayout->setContentsMargins(
		TagEdit::nMargin, TagEdit::nMargin, TagEdit::nMargin, TagEdit::nMargin
	);
	pScrollArea->setLayout( pVBoxLayout );

	m_pTable = new QTableWidget( pScrollArea );
	pVBoxLayout->addWidget( m_pTable );

	m_pTable->setContentsMargins( 0, 0, 0, 0 );
	m_pTable->setColumnCount( 2 );
	m_pTable->setRowCount( 1 );
	m_pTable->setSelectionMode( QAbstractItemView::NoSelection );
	m_pTable->horizontalHeader()->hide();
	m_pTable->verticalHeader()->hide();
	m_pTable->setColumnWidth( 1, TagEdit::nIconWidth );
	m_pTable->horizontalHeader()->setSectionResizeMode(
		0, QHeaderView::Stretch
	);
	m_pTable->horizontalHeader()->setSectionResizeMode( 1, QHeaderView::Fixed );

	auto pDummyWidget = new QWidget( m_pTable );
	m_pTable->setCellWidget( 0, 0, pDummyWidget );

	// Create the bottom-most row. This one should never be removed.
	QString sIconPath( Skin::getSvgImagePath() );
	if ( H2Core::Preferences::get_instance()
			 ->getInterfaceTheme()
			 ->m_iconColor == H2Core::InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	}
	else {
		sIconPath.append( "/icons/black/" );
	}

	// No handler connection in here. We just read the content of all text edits
	// on getTags().

	auto pAddRowButton = new QToolButton( m_pTable );
	pAddRowButton->setFocusPolicy( Qt::NoFocus );
	pAddRowButton->setFixedSize(
		QSize( TagEdit::nButtonWidth, TagEdit::nButtonWidth )
	);
	pAddRowButton->setIcon( QIcon( sIconPath + "new.svg" ) );
	pAddRowButton->setIconSize(
		QSize( TagEdit::nIconWidth, TagEdit::nIconWidth )
	);
	connect( pAddRowButton, &QPushButton::clicked, [=]() { addRow( "" ); } );
	m_pTable->setCellWidget( 0, 1, pAddRowButton );

	pScrollArea->setMinimumHeight(
		TagEdit::nMinimumRows * m_pTable->rowHeight( 0 ) + 6
	);

	updateStyleSheet();
}

TagEdit::~TagEdit()
{
}

QStringList TagEdit::getTags() const
{
	QStringList tags;
	for ( int ii = 0; ii < m_pTable->rowCount() - 1; ++ii ) {
		auto pTextEdit =
			dynamic_cast<QLineEdit*>( m_pTable->cellWidget( ii, 0 ) );
		if ( pTextEdit != nullptr && !pTextEdit->text().isEmpty() ) {
			tags << pTextEdit->text();
		}
	}

	return std::move( tags );
}

void TagEdit::setTags( const QStringList& tags )
{
	// Start with a clean table.
	if ( m_pTable->rowCount() > 1 ) {
		for ( int ii = m_pTable->rowCount() - 2; ii >= 0; --ii ) {
			removeRow( ii );
		}
	}

	for ( const auto& ssTag : tags ) {
		addRow( ssTag );
	}
}

void TagEdit::addRow( const QString& sText )
{
	const int nNewRow = m_pTable->rowCount() - 1;
	m_pTable->insertRow( nNewRow );

	QString sIconPath( Skin::getSvgImagePath() );
	if ( H2Core::Preferences::get_instance()
			 ->getInterfaceTheme()
			 ->m_iconColor == H2Core::InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	}
	else {
		sIconPath.append( "/icons/black/" );
	}

	auto pLineEdit = new QLineEdit( m_pTable );
	pLineEdit->setText( sText );
	m_pTable->setCellWidget( nNewRow, 0, pLineEdit );
	// No handler connection in here. We just read the content of all text edits
	// on getTags().

	auto pDeleteRowButton = new QToolButton( m_pTable );
	pDeleteRowButton->setFocusPolicy( Qt::NoFocus );
	pDeleteRowButton->setFixedSize(
		QSize( TagEdit::nButtonWidth, TagEdit::nButtonWidth )
	);
	pDeleteRowButton->setIcon( QIcon( sIconPath + "bin.svg" ) );
	pDeleteRowButton->setIconSize(
		QSize( TagEdit::nIconWidth, TagEdit::nIconWidth )
	);
	connect( pDeleteRowButton, &QPushButton::clicked, [=]() {
		int nRow = -1;
		for ( int nnRow = 0; nnRow < m_pTable->rowCount(); ++nnRow ) {
			if ( pDeleteRowButton == m_pTable->cellWidget( nnRow, 1 ) ) {
				nRow = nnRow;
				break;
			}
		}
		if ( nRow < 0 || nRow >= m_pTable->columnCount() ) {
			ERRORLOG(
				QString(
					"Delete button associated with incorrect row [%1] [0,%2)"
				)
					.arg( nRow )
					.arg( m_pTable->columnCount() )
			);
			return;
		}
		removeRow( nRow );
	} );
	m_pTable->setCellWidget( nNewRow, 1, pDeleteRowButton );
	// We always keep the bottom-most row.
}

void TagEdit::removeRow( int nIndex )
{
	// We always keep the bottom-most row.
	if ( nIndex < 0 || nIndex >= m_pTable->rowCount() ) {
		ERRORLOG( QString( "Index [%1] out of bound [0, %2]" )
					  .arg( nIndex )
					  .arg( m_pTable->rowCount() ) );
		return;
	}
	else if ( nIndex == m_pTable->rowCount() - 1 ) {
		ERRORLOG( "The bottom-most row can not be removed." );
	}

	m_pTable->removeRow( nIndex );
}

void TagEdit::updateStyleSheet()
{
	const QColor backgroundColor =
		H2Core::Preferences::get_instance()->getColorTheme()->m_midLightColor;

	setStyleSheet( QString( "                   \
QTableWidget {                                  \
    background-color: %1;                       \
    selection-background-color: %1;             \
}                                               \
" )
					   .arg( backgroundColor.name() )
					   .append( Skin::getToolButtonStyle( backgroundColor ) ) );
}
