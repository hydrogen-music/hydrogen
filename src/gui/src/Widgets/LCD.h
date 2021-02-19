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


#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

#include <vector>
#include <limits>

class LCDDigit : public QWidget, public H2Core::Object
{
    H2_OBJECT
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



class LCDDisplay : public QWidget, public H2Core::Object
{
    H2_OBJECT
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


///
/// Simple spinbox widget using an LCDDisplay
///
class LCDSpinBox : public QWidget, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT
	public:
		enum LCDSpinBoxType {
			INTEGER,
			INTEGER_MIN_OFF,
			FLOAT
		};

		/**
		 * Create a new LCDSpinBox
		 *
		 * @param pParent Parent widget
		 * @param lcdType Style of LCD characters to use
		 * @param nDigits Width of the display, in characters
		 * @param type Type of values used in the spinbox
		 * @param fMin Minimum value, -inf (default) to disable bounds checking
		 * @param fMax Maximum value, +inf (default) to disable bounds checking
		 */
		LCDSpinBox( QWidget *pParent, LCDDigit::LCDType lcdType, int nDigits, LCDSpinBoxType type, float fMin = -std::numeric_limits<float>::infinity(), float fMax = std::numeric_limits<float>::infinity());
		~LCDSpinBox();

		void setValue( float nValue );
		float getValue() {	return m_fValue;	}

		virtual void wheelEvent( QWheelEvent *ev );

	signals:
		void changed(LCDSpinBox *pRef);
		void spinboxClicked();

	public slots:
		void displayClicked( LCDDisplay *pRef );
		void upButtonClicked();
		void downButtonClicked();

	private:
		/**
		 * Increment the value, with bounds checking
		 */
		void incrementValue();
		/**
		 * Decrement the value, with bounds checking
		 */
		void decrementValue();

		LCDSpinBoxType m_type;
		LCDDisplay* m_pDisplay;

		float m_fValue;
		float m_fMinValue;
		float m_fMaxValue;
};


#endif
