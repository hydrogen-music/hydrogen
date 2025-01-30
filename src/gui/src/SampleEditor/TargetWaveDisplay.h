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

#ifndef TARGET_WAVE_DISPLAY
#define TARGET_WAVE_DISPLAY

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Basics/Sample.h>
#include <memory>

class SampleEditor;

namespace H2Core
{
	class InstrumentLayer;
	class EnvelopePoint;
}

/** \ingroup docGUI*/
class TargetWaveDisplay :  public QWidget,  public H2Core::Object<TargetWaveDisplay>
{
	H2_OBJECT(TargetWaveDisplay)
	Q_OBJECT

	public:
		explicit TargetWaveDisplay(QWidget* pParent);
		~TargetWaveDisplay();

		enum EnvelopeEditMode {
			VELOCITY = 0,
			PAN = 1
		};

		void updateDisplay( std::shared_ptr<H2Core::InstrumentLayer> pLayer );
		void updateDisplayPointer();
		void paintLocatorEventTargetDisplay( int pos, bool last_event);
		virtual void paintEvent(QPaintEvent *ev) override;
		H2Core::Sample::PanEnvelope* get_pan() { return &m_PanEnvelope; }
		H2Core::Sample::VelocityEnvelope* get_velocity() { return &m_VelocityEnvelope; }

	private:
		QPixmap m_Background;

		QString m_sSampleName;
		QString m_sInfo;

		int m_nX;
		int m_nY;
		int m_nLocator;

		int *m_pPeakData_Left;
		int *m_pPeakData_Right;

		bool m_UpdatePosition;
		EnvelopeEditMode m_EditMode;

		int m_nSnapRadius;

		virtual void mouseMoveEvent(QMouseEvent *ev) override;
		virtual void mousePressEvent(QMouseEvent *ev) override;
		virtual void mouseReleaseEvent(QMouseEvent *ev) override;

		virtual void updateMouseSelection(QMouseEvent *ev);
		virtual void updateEnvelope();

		H2Core::Sample::PanEnvelope m_PanEnvelope;
		H2Core::Sample::VelocityEnvelope m_VelocityEnvelope;

		int m_nSelectedEnvelopePoint;
};

#endif
