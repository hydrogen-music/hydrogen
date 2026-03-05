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

#ifndef TARGET_SECTION
#define TARGET_SECTION

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <memory>

class SampleEditor;
class SampleEnvelope;
class WaveDisplay;

namespace H2Core {
class InstrumentLayer;
}  // namespace H2Core

/** \ingroup docGUI*/
class TargetSection : public QWidget, public H2Core::Object<TargetSection> {
	H2_OBJECT( TargetSection )
	Q_OBJECT

   public:
	/** Do only change these value with care. They are hardcoded in Sample.cpp
	 * and incorporated in the values of previous .h2song files. */
	static constexpr int nHeight = 91;
	static constexpr int nWidth = 841;

	explicit TargetSection( SampleEditor* pParent );
	~TargetSection();

	void setEnabled( bool bEnabled );
	void setLayer( std::shared_ptr<H2Core::InstrumentLayer> pLayer );

   private:
	WaveDisplay* m_pWaveDisplayL;
	WaveDisplay* m_pWaveDisplayR;
	SampleEnvelope* m_pSampleEnvelope;

	bool m_bEnabled;
};

inline void TargetSection::setEnabled( bool bEnabled )
{
	m_bEnabled = bEnabled;
}

#endif
