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

#ifndef FOOTER_H
#define FOOTER_H

#include <QtGui>
#include <QtWidgets>
#include <chrono>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

class StatusMessageDisplay;

/** Buttom-most widget in the main gui. If will show a bunch of textual
 * information, like the status messages - including the history of previous
 * messages when clicking it - CPU usage, and the number of encountered XRuns.
 *
 * \ingroup docGUI*/
class Footer : public QWidget,
			   protected WidgetWithScalableFont<5, 6, 7>,
			   public EventListener,
			   public H2Core::Object<Footer> {
    H2_OBJECT(Footer)
	Q_OBJECT
public:

		static constexpr int nHeight = 23;
		static constexpr std::chrono::seconds cpuTimeout{ 1 };
		static constexpr int nCpuLoadWarningThreshold = 90;

		explicit Footer(QWidget *parent);
		~Footer();

		void showStatusBarMessage( const QString& msg, const QString& sCaller = "" );

		void driverChangedEvent() override;
		void XRunEvent() override;

public slots:
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

private slots:
		void updateCpuLoad();

private:
		void updateStyleSheet();
		void updateXRuns();

		StatusMessageDisplay* m_pStatusMessageDisplay;

		QWidget* m_pCpuGroup;
		QLabel* m_pCpuLabel;

		QWidget* m_pXRunGroup;
		QLabel* m_pXRunLabel;

		int m_nXRuns;
		bool m_bCpuLoadWarning;
};


#endif
