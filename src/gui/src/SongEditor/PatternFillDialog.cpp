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
 *
 */

#include "PatternFillDialog.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCDSpinBox.h"

#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

PatternFillDialog::PatternFillDialog( QWidget* pParent, FillRange* pFillRange)
	: QDialog( pParent )
	, m_pFillRange( pFillRange )
{
	setWindowTitle( tr( "Fill with selected pattern" ) );

	const auto pPref = H2Core::Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	const QSize buttonSize( 70, 24 );
	const QSize spinBoxSize( buttonSize.width() * 2.5 / 2, buttonSize.height() );

	auto pMainLayout = new QVBoxLayout( this );
	setLayout( pMainLayout );

	// Range spin boxes
	auto pRangeWidget = new QWidget( this );
	pMainLayout->addWidget( pRangeWidget );
	auto pRangeLayout = new QHBoxLayout( pRangeWidget );
	pRangeLayout->setContentsMargins( 0, 0, 0, 0 );
	pRangeLayout->setSpacing( 2 );
	pRangeWidget->setLayout( pRangeLayout );

	m_pFromSpinBox = new LCDSpinBox(
		this, spinBoxSize, LCDSpinBox::Type::Int, /*min*/ 0, /*max*/ 1,
		/*modifyOnChange*/ false, /*minusOneAsOff*/ false );
	m_pFromSpinBox->setFocus();
	m_pFromSpinBox->setFocusPolicy( Qt::StrongFocus );
	pRangeLayout->addWidget( m_pFromSpinBox );

	m_pToSpinBox = new LCDSpinBox(
		this, spinBoxSize, LCDSpinBox::Type::Int, /*min*/ 1,
		/*max*/ pPref->getMaxBars(), /*modifyOnChange*/ false,
		/*minusOneAsOff*/ false );
	m_pToSpinBox->setFocusPolicy( Qt::StrongFocus );
	pRangeLayout->addWidget( m_pToSpinBox );

	// We need to ensure `from` is always smaller or equal than `to`.
	connect( m_pFromSpinBox, &LCDSpinBox::valueAdjusted, [=]() {
		m_pToSpinBox->setMinimum( m_pFromSpinBox->value() );
	} );
	connect( m_pToSpinBox, &LCDSpinBox::valueAdjusted, [=]() {
		m_pFromSpinBox->setMaximum( m_pToSpinBox->value() );
	} );

	// Action buttons
	auto pButtonsWidget = new QWidget( this );
	pMainLayout->addWidget( pButtonsWidget );
	auto pButtonsLayout = new QHBoxLayout( pButtonsWidget );
	pButtonsLayout->setContentsMargins( 0, 0, 0, 0 );
	pButtonsLayout->setSpacing( 2 );
	pButtonsWidget->setLayout( pButtonsLayout );

	auto pClearButton = new Button(
		this, buttonSize, Button::Type::Push, "",
		pCommonStrings->getButtonClear() );
	pClearButton->setFocusPolicy( Qt::StrongFocus );
	connect( pClearButton, &QPushButton::clicked, [=]() {
		if ( m_pFillRange != nullptr ) {
			m_pFillRange->nFrom = m_pFromSpinBox->value();
			m_pFillRange->nTo = m_pToSpinBox->value();
			m_pFillRange->bInsert = false;
		}
		accept();
	} );
	pButtonsLayout->addWidget( pClearButton );

	auto pFillButton = new Button(
		this, buttonSize, Button::Type::Push, "",
		pCommonStrings->getButtonFill() );
	pFillButton->setFocusPolicy( Qt::StrongFocus );
	connect( pFillButton, &QPushButton::clicked, [=]() {
		if ( m_pFillRange != nullptr ) {
			m_pFillRange->nFrom = m_pFromSpinBox->value();
			m_pFillRange->nTo = m_pToSpinBox->value();
			m_pFillRange->bInsert = true;
		}
		accept();
	} );
	pButtonsLayout->addWidget( pFillButton );

	auto pCancelButton = new Button(
		this, buttonSize, Button::Type::Push, "",
		pCommonStrings->getButtonCancel() );
	pCancelButton->setFocusPolicy( Qt::StrongFocus );
	connect( pCancelButton, &QPushButton::clicked, [=]() {
		reject();
	} );
	pButtonsLayout->addWidget( pCancelButton );
}

PatternFillDialog::~PatternFillDialog() {
}
