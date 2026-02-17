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

#ifndef DETAIL_SECTION
#define DETAIL_SECTION

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

#include <memory>
#include <vector>

namespace H2Core {
class Sample;
}

class DetailWaveDisplay;

/** Wrapper widget containing two #DetailWaveDisplay - one for the left and one
 * for the right channel of the sample.
 *
 * \ingroup docGUI*/
class DetailSection : public QWidget, public H2Core::Object<DetailSection> {
	H2_OBJECT( DetailSection )
	Q_OBJECT

   public:
	static constexpr int nWidth = 180;
	static constexpr int nHeight = 265;

	explicit DetailSection( QWidget* pParent );
	~DetailSection();

	void setSample( std::shared_ptr<H2Core::Sample> pNewSample );

	void setDetailSamplePosition(
		int nPosition,
		float fZoomFactor,
		const QString& sType
	);

   private:
	DetailWaveDisplay* m_pWaveDisplayL;
	DetailWaveDisplay* m_pWaveDisplayR;
};

#endif
