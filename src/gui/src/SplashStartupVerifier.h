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

#ifndef H2C_SPLASH_STARTUP_VERIFIER_H
#define H2C_SPLASH_STARTUP_VERIFIER_H

#include <QObject>

/**
 * \ingroup docGUI
 *
 * SplashStartupVerifier
 *
 * Out-of-process check that the splash screen actually becomes visible
 * during startup. QSplashScreen::show() does not paint synchronously:
 * the first paint only happens when the event loop (or an explicit
 * processEvents() pass) delivers the expose/paint events, and on
 * Wayland the compositor only shows the surface after that first
 * buffer commit. main() used to run the whole seconds-long startup
 * construction (engine, drumkit, MainForm) between show() and exec()
 * without a single event pass, so the splash never appeared at all.
 *
 * The defect is only observable against a live compositor, which a
 * headless CTest cannot provide (the offscreen platform exposes and
 * paints synchronously). The verifier therefore asserts the mechanism
 * that fixes it: an event pass while the splash is up, before the
 * heavy startup work. main() arms a zero-timer sentinel before the
 * splash exposure pump and captures whether it was delivered right
 * after the pump; only processing events in between can flip it in
 * time. Like PluginTransportGatingVerifier it turns the assertion
 * into an exit code a CTest can fail on (0 = splash got its event
 * pass, 1 = it did not).
 */
class SplashStartupVerifier : public QObject {
	Q_OBJECT

public:
	explicit SplashStartupVerifier( QObject* pParent = nullptr );
	~SplashStartupVerifier() override = default;

	/** Post the zero-timer sentinel. Must be called from main() after
	 * the splash was shown but BEFORE the exposure pump: the sentinel
	 * fires on the next event pass, so its delivery proves events were
	 * processed while the splash was up. */
	static void armSplashSentinel();

	/** Freeze the verdict from the sentinel state. Must be called from
	 * main() after the exposure pump but BEFORE the event loop
	 * starts — during exec() the sentinel would fire regardless of
	 * the defect. */
	static void captureSplashVerdict();

	/** Quit with the verdict. Scheduled via QTimer from main() so the
	 * startup event queue drains first. */
	void start();

private:
	/** Whether the sentinel zero-timer fired (set by the timer, read by
	 * captureSplashVerdict()). */
	static bool s_bSplashSentinelDelivered;

	/** Verdict frozen by captureSplashVerdict(); read by start(). */
	static bool s_bSplashVerdict;
};

#endif // H2C_SPLASH_STARTUP_VERIFIER_H
