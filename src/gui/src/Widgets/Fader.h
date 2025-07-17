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


#ifndef FADER_H
#define FADER_H


#include <QtGui>
#include <QtWidgets>
#include <QSvgRenderer>

#include "WidgetWithInput.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

/** Custom fader widget.
 *
 * The meter, outline, and slider will be loaded from a SVG file while
 * the bars indicating the current value will be painted by Qt.
 *
 */
/** \ingroup docGUI docWidgets*/
class Fader : public WidgetWithInput, public H2Core::Object<Fader>
{
    H2_OBJECT(Fader)
	
public:
	enum class Type {
		Normal,
		/** Only used for the playback track in the SongEditorPanel*/
		Vertical,
		Master
	};
	
	Fader( QWidget *pParent, const Type& type,
		   const QString& sBaseToolTip, bool bUseIntSteps = false,
		   bool bWithoutKnob = false, float fMin = 0.0, float fMax = 1.0,
		   bool bModifyOnChange = true );
	~Fader();

	void setMaxPeak( float fMax );
	void setMinPeak( float fMin );

	void setPeak_L( float peak );
	float getPeak_L() const {	return m_fPeakValue_L;	}

	void setPeak_R( float peak );
	float getPeak_R() const {	return m_fPeakValue_R;	}

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

private:
	bool m_bWithoutKnob;
	Type m_type;
	QSvgRenderer* m_pBackground;
	QSvgRenderer* m_pKnob;

	float m_fPeakValue_L;
	float m_fPeakValue_R;
	float m_fMinPeak;
	float m_fMaxPeak;
	
	virtual void mouseMoveEvent(QMouseEvent *ev) override;
	virtual void mousePressEvent(QMouseEvent *ev) override;
	virtual void paintEvent(QPaintEvent *ev) override;
};
#endif
