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

#ifndef LCDSPINBOX_H
#define LCDSPINBOX_H

#include <QtGui>
#include <QDoubleSpinBox>

#include <core/Basics/Event.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

/** Custom spin box.
 *
 * The QDoubleSpinBox is used for both integer and float values
 * instead of using QSpinBox as a base for the former. This is done to
 * keep the code concise. All internal values will be handled as
 * floats and only textFromValue() makes a difference.
 *
 * The LCDSpinBox::Kind was introduced to handle custom constraints
 * for the widgets concerning with the pattern size in the
 * PatternEditorPanel.
 *
 * Updating the font family in QDoubleSpinBox is not supported and
 * changing the font size (both via setFont()) yields erratic
 * results.
 */
/** \ingroup docGUI docWidgets*/
class LCDSpinBox : public QDoubleSpinBox, public H2Core::Object<LCDSpinBox>
{
    H2_OBJECT(LCDSpinBox)
	Q_OBJECT

public:

	enum class Type {
		Int,
		Double
	};

	enum class Kind {
		/** Behaves like QDoubleSpinBox.*/
		Default,
		/** The minimum value - a fractional one - can only be reached
		 * by entering it using the keyboard. up/down keys as well as
		 * mouse wheel increment the number by one and will stay at
		 * integer values per default.*/
		PatternSizeNumerator,
		/** Only a limited number of values is allowed.*/
		PatternSizeDenominator
	};

	enum Flag {
		None = 0x000,
		ModifyOnChange = 0x001,
		/** Used for MIDI channel spin boxes to indicate no MIDI in/output once the
		 * value is equal to `-1` */
		MinusOneAsOff = 0x002,
		/** Used for MIDI channel spin boxes to indicate MIDI input from all
		 * channels will be accepted once the value is equal to `-2` */
		MinusTwoAsAll = 0x004,
	};

	LCDSpinBox( QWidget *pParent, QSize size = QSize(), Type type = Type::Int,
				double fMin = 0.0, double fMax = 1.0, int = Flag::None );
	~LCDSpinBox();

	void setType( Type type );
	void setKind( Kind kind );
	void setSize( QSize size );

	virtual QValidator::State validate( QString &text, int &pos ) const override;
	
	bool getIsActive() const;
	void setIsActive( bool bIsActive );

	bool getIsHovered() const;

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
	void setValue( double fValue,
				   H2Core::Event::Trigger trigger =
				      H2Core::Event::Trigger::Default );

private slots:
		void valueChanged( double fNewValue );

signals:
	void slashKeyPressed();
	void valueChanged( int );
		/** A non-overloaded version of valueChanged() in order to use this
		 * signal with the version of connect() (Qt signal handling) which
		 * allows inline lambda functions. */
		void valueAdjusted();
	
private:
	double nextValueInPatternSizeDenominator( bool bUp, bool bAccelerated );
	void updateStyleSheet();

	QSize m_size;
	Type m_type;
	Kind m_kind;
		Flag m_flag;

	bool m_bEntered;
	bool m_bIsActive;

	virtual QString textFromValue( double fValue ) const override;
	virtual double valueFromText( const QString& sText ) const override;	
	virtual void paintEvent( QPaintEvent *ev ) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent( QEvent *ev ) override;
	virtual void wheelEvent( QWheelEvent *ev ) override;
	virtual void keyPressEvent( QKeyEvent *ev ) override;
	virtual void mousePressEvent(QMouseEvent *ev) override;
	virtual void mouseReleaseEvent( QMouseEvent *ev ) override;
	virtual void mouseMoveEvent(QMouseEvent *ev) override;

	virtual bool event( QEvent* ev ) override;
};

inline void LCDSpinBox::setKind( Kind kind ) {
	m_kind = kind;
}
inline bool LCDSpinBox::getIsActive() const {
	return m_bIsActive;
}
inline bool LCDSpinBox::getIsHovered() const {
	return m_bEntered;
}
#endif
