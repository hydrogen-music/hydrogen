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

#ifndef LCD_H
#define LCD_H

#include "config.h"

#include <QtGui>

#include <hydrogen/Object.h>

#include <vector>

class LCDDigit : public QWidget, public Object
{
	Q_OBJECT
	public:
		enum LCDType {
			SMALL_BLUE,
			SMALL_RED,
			LARGE_GRAY,
			SMALL_GRAY
		};

		LCDDigit( QWidget *pParent, LCDType type );
		~LCDDigit();

		void set( char ch );

		void setSmallRed();
		void setSmallBlue();

	signals:
		void digitClicked();

	private:
		static QPixmap *m_pSmallBlueFontSet;
		static QPixmap *m_pSmallRedFontSet;
		static QPixmap *m_pLargeGrayFontSet;
		static QPixmap *m_pSmallGrayFontSet;

		int m_nCol;
		int m_nRow;
		LCDType m_type;

		virtual void paintEvent(QPaintEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent* ev);
};



class LCDDisplay : public QWidget{
	Q_OBJECT
	public:
		LCDDisplay( QWidget * pParent, LCDDigit::LCDType type, int nDigits, bool leftAlign = false );
		~LCDDisplay();

		void setText( const QString& sMsg );
		QString getText() {	return m_sMsg;	}

		void setSmallRed();
		void setSmallBlue();

	public slots:
		void digitClicked();

	signals:
		void displayClicked( LCDDisplay* pRef );

	private:
		std::vector<LCDDigit*> m_pDisplay;
		QString m_sMsg;
		bool m_bLeftAlign;
};


class LCDSpinBox : public QWidget, public Object
{
	Q_OBJECT
	public:
		enum LCDSpinBoxType {
			INTEGER,
			FLOAT
		};

		LCDSpinBox( QWidget *pParent, int nDigits, LCDSpinBoxType type, int nMin = -1, int nMax = -1 );
		~LCDSpinBox();

		void setValue( float nValue );
		float getValue() {	return m_fValue;	}

		virtual void wheelEvent( QWheelEvent *ev );

		// richiamati da PlayerControl
		void upBtnClicked();
		void downBtnClicked();

	signals:
		void changed(LCDSpinBox *pRef);
		void spinboxClicked();

	public slots:
		void displayClicked( LCDDisplay *pRef );

	private:
		LCDSpinBoxType m_type;
		LCDDisplay* m_pDisplay;

		float m_fValue;
		int m_nMinValue;
		int m_nMaxValue;
};


#endif
