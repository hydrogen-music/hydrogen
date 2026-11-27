/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#include "SplashStartupVerifier.h"

#include <core/Logger.h>
#include <core/Object.h>

#include <QCoreApplication>
#include <QTimer>

bool SplashStartupVerifier::s_bSplashSentinelDelivered = false;
bool SplashStartupVerifier::s_bSplashVerdict = false;

SplashStartupVerifier::SplashStartupVerifier( QObject* pParent )
	: QObject( pParent ) {
}

void SplashStartupVerifier::armSplashSentinel()
{
	// A zero-timer fires on the next event pass — whenever that
	// happens. The verdict is frozen before exec() by
	// captureSplashVerdict(), so only an event pass during the splash
	// phase of startup (the exposure pump in main()) counts.
	QTimer::singleShot( 0, []() {
		SplashStartupVerifier::s_bSplashSentinelDelivered = true;
	} );
}

void SplashStartupVerifier::captureSplashVerdict()
{
	s_bSplashVerdict = s_bSplashSentinelDelivered;
}

void SplashStartupVerifier::start()
{
	if ( ! s_bSplashVerdict ) {
		___ERRORLOG( "Splash startup verification: the splash screen did "
					 "not receive an event pass during startup — it never "
					 "became visible" );
		QCoreApplication::exit( 1 );
		return;
	}

	___INFOLOG( "Splash startup verification passed" );
	QCoreApplication::quit();
}
