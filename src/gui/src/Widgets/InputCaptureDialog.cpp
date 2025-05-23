/*
 * Hydrogen
 * Copyright(c) 2008-2025 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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

#include "InputCaptureDialog.h"

InputCaptureDialog::InputCaptureDialog( QWidget* pParent, const QString& sTitle,
										const QString& sLabel, const Type& type,
										float fMin, float fMax )
	: QDialog( pParent ),
	  m_sTitle( sTitle ),
	  m_sLabel( sLabel ),
	  m_type( type ),
	  m_fMin( fMin ),
	  m_fMax( fMax )
 {

	setWindowTitle( tr( "InputCaptureDialog" ) );

	QVBoxLayout* pVBoxLayout = new QVBoxLayout( this );
	setLayout( pVBoxLayout );

	if ( ! m_sTitle.isEmpty() ) {
		m_pLabelTitle = new QLabel( sTitle, this );
		m_pLabelTitle->setAlignment( Qt::AlignCenter );
		m_pLabelTitle->setContentsMargins( 5, 5, 5, 5 );
		m_pLabelTitle->setStyleSheet( "QLabel { font-weight: bold; }" );
		pVBoxLayout->addWidget( m_pLabelTitle );
	}

	m_pLabel = new QLabel( sLabel, this );
	m_pLabel->setAlignment( Qt::AlignCenter );
	pVBoxLayout->addWidget( m_pLabel );

	m_pLineEdit = new QLineEdit();
	pVBoxLayout->addWidget( m_pLineEdit );

	if ( m_type == Type::IntMidi ) {
		m_fMin = 0;
		m_fMax = 127;
	}
	
	m_pLabelBounds = new QLabel( QString( "[%1,%2]" ).arg( m_fMin ).arg( m_fMax ), this );
	m_pLabelBounds->setAlignment( Qt::AlignCenter );
	pVBoxLayout->addWidget( m_pLabelBounds );

}

InputCaptureDialog::~InputCaptureDialog() {
}

QString InputCaptureDialog::text() const {
	return m_pLineEdit->text();
}

void InputCaptureDialog::keyPressEvent( QKeyEvent* ev ) {
	int nKey = ev->key();
	if ( nKey == Qt::Key_Enter || nKey == Qt::Key_Return ) {
		// Sanity checks to ensure a proper output
		bool bOk;
		if ( m_type == Type::Int || m_type == Type::IntMidi ) {
			const int nRes = m_pLineEdit->text().toInt( &bOk, 10 );
			if ( ! bOk || nRes < static_cast<int>(m_fMin) ||
				 nRes > static_cast<int>(m_fMax) ) {
				ERRORLOG( QString( "Invalid integer input [%1] with bounds [%2,%3]" )
						  .arg( m_pLineEdit->text() )
						  .arg( static_cast<int>( m_fMin ) )
						  .arg( static_cast<int>( m_fMax ) ) );
				reject();
			}
		}
		else if ( m_type == Type::Float ) {
			const float fRes = m_pLineEdit->text().toFloat( &bOk );
			if ( ! bOk || fRes < m_fMin || fRes > m_fMax ) {
				ERRORLOG( QString( "Invalid float input [%1] with bounds [%2,%3]" )
						  .arg( m_pLineEdit->text() ).arg( m_fMin )
						  .arg( m_fMax ) );
				reject();
			}
		}
			
		accept();
	}
	else if ( nKey == Qt::Key_Escape ) {
		reject();
	}
		
}
