/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#ifndef PATCH_BAY_H
#define PATCH_BAY_H


#include <map>
#include <memory>

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/DrumkitMap.h>

#include "Button.h"
#include "LCDCombo.h"
#include "LCDDisplay.h"

/** Custom widget to map instruments and components onto eachother when
 * switching drumkits.
 *
 * \ingroup docGUI docWidgets*/
class PatchBay : public QDialog, public H2Core::Object<PatchBay>
{
	H2_OBJECT(PatchBay)
	Q_OBJECT

public:
	/** Determines the number of columns and their content. */
	enum class Type {
		/** There will be a left column containing the instruments of the source
		 * kit, a right column with the ones from the target kit, a middle one
		 * containing the GIDs of all instruments, and two columns between them
		 * for drawing the established connection. */
		Instruments = 0,
		/** Just two columns containing the names of the provided components and
		 * their connection. Middle column is missing. */
		Components
	};

	PatchBay( QWidget* pParent,
			  std::shared_ptr<H2Core::Drumkit> pSourceDrumkit,
			  std::shared_ptr<H2Core::Drumkit> pTargetDrumkit,
			  Type type );
	~PatchBay();

private slots:
	void newType();

private:
	void addLeft( std::shared_ptr<H2Core::Instrument> pInstrument );
	void addRight( std::shared_ptr<H2Core::Instrument> pInstrument );

	LCDDisplay* createElement( const QString& sLabel );
	void drawConnections( QPainter &p );
	void drawConnection( QPainter &p, LCDDisplay* pLabel1, LCDDisplay* pLabel2 );

	int m_nFixedRowHeight = 20;

	QWidget* m_pLeftColumn;
	QVBoxLayout* m_pLeftColumnLayout;
	/** Maps the ID of an instrument to the widget representing it. */
	std::map<int, LCDDisplay*> m_leftColumn;
	QWidget* m_pLeftConnections;
	QWidget* m_pMiddleColumn;
	QVBoxLayout* m_pMiddleColumnLayout;
	/** Maps an instrument type to the widget representing it. */
	std::map<H2Core::DrumkitMap::Type, LCDDisplay*> m_midColumn;
	QWidget* m_pRightConnections;
	QWidget* m_pRightColumn;
	QVBoxLayout* m_pRightColumnLayout;
	/** Maps the ID of an instrument to the widget representing it. */
	std::map<int, LCDDisplay*> m_rightColumn;
	Button* m_pNewTypeButton;

	std::shared_ptr<H2Core::Drumkit> m_pSourceDrumkit;
	std::shared_ptr<H2Core::Drumkit> m_pTargetDrumkit;
	std::shared_ptr<H2Core::DrumkitMap> m_pSourceDrumkitMap;
	std::shared_ptr<H2Core::DrumkitMap> m_pTargetDrumkitMap;
	QStringList m_types;
	Type m_type;

	virtual void mouseMoveEvent( QMouseEvent *ev ) override;
	virtual void mousePressEvent( QMouseEvent *ev ) override;
	virtual void paintEvent( QPaintEvent *ev ) override;
};

/** Custom widget to map instruments and components onto eachother when
 * switching drumkits.
 *
 * \ingroup docGUI docWidgets*/
class NewTypeDialog : public QDialog, public H2Core::Object<NewTypeDialog>
{
	H2_OBJECT(NewTypeDialog)
	Q_OBJECT

public:
	NewTypeDialog( QWidget* pParent );
	~NewTypeDialog();

	H2Core::DrumkitMap::Type getType() const;

private:
	virtual void keyPressEvent( QKeyEvent *ev ) override;

	LCDCombo* m_pCombo;
};
#endif
