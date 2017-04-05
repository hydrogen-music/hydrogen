/*
 * Hydrogen
 * Copyright(c) 2002-2016 by the Hydrogen Team 
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

#include "InfoBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

#include "../Skin.h"

InfoBar::InfoBar( QWidget *parent )
	: QWidget( parent )
{
	setBackgroundColor();
	createLayout();
	createIcon();
	createLabel();
	createCloseButton();
}


void InfoBar::setBackgroundColor()
{
	QPalette pal = palette();
	pal.setColor( QPalette::Window, QColor( 97, 165, 249 ) );
	setAutoFillBackground( true );
	setPalette( pal );
}


void InfoBar::createLayout()
{
	m_pLayout = new QHBoxLayout();
	m_pLayout->setContentsMargins( 2, 2, 2, 2 );
	setLayout( m_pLayout );
}


void InfoBar::createIcon()
{
	QLabel *icon = new QLabel();
	icon->setPixmap( QPixmap( Skin::getImagePath() + "/warning.png" ) );
	m_pLayout->addWidget( icon );
}


void InfoBar::createLabel()
{
	m_pLabel = new QLabel();
	m_pLayout->addWidget( m_pLabel, 1 );

	QFont font = m_pLabel->font();
	font.setPointSize( 11 );
	m_pLabel->setFont( font );
}


void InfoBar::createCloseButton()
{
	QPushButton *close = new QPushButton();
	QIcon closeIcon = style()->standardIcon( QStyle::SP_TitleBarCloseButton );
	close->setIcon( closeIcon );
	close->setFlat( true );
	m_pLayout->addWidget( close );

	QObject::connect(close, SIGNAL(clicked()), this, SLOT(hide()));
}


void InfoBar::setTitle(const QString &title)
{
	m_sTitle = title;
	updateText();
}


void InfoBar::setText(const QString &str)
{
	m_sText = str;
	updateText();
}


void InfoBar::updateText()
{
	auto html = QString("<html><b>%1</b><br/>%2</html>").arg(m_sTitle).arg(m_sText);
	m_pLabel->setText(html);
}


QPushButton *InfoBar::addButton( const QString &label )
{
	QPushButton *button = new QPushButton();
	button->setText( label );
	m_pLayout->insertWidget( m_pLayout->count() - 1,  button );
	m_buttons.push_back(button);
	return button;
}


void InfoBar::reset()
{
	m_sTitle = "";
	m_sText = "";
	updateText();

	for (QPushButton *button : m_buttons) {
		delete button;
	}
	m_buttons.clear();
}
