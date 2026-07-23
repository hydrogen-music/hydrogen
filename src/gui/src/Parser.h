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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef PARSER_H
#define PARSER_H

#include <QString>

/** Reusable parser for provided command line arguments.
 *
 * In our current design we have to access the CLI arguments of the `hydrogen`
 * program at two different points (`h2cli` and `h2player` have other CLI
 * options and are not covered in here): 1. In #Reporter and 2. in the main
 * routine it spawns.
 *
 * Some CLI args, like the path to a custom log file, are important for the
 * #Reporter as well. But it can not access them through the child process but
 * has to retrieve it from the CLI args itself. Here it is crucial to a) perform
 * the parsing in the same way in order to avoid inconsistencies and b) keep the
 * parsing with the one in the main routine in sync as QCommandLineParser will
 * complain about arguments unknown to it. That's why we abstract it in a
 * separate class.
 *
 * Note that this is not a H2_OBJECT. It get's invoked way ahead of
 * #H2Core::Logger::bootstrap() and #H2Core::Base::bootstrap() and could not be
 * used with our usual logging macros etc.
 * */
class Parser {
	public:

		Parser();
		~Parser();

		bool parse( int argc, char* argv[] );

		const QString& getAudioDriver() const {
			return m_sAudioDriver; }
		const QString& getPlaylistFileName() const {
			return m_sPlaylistFileName; }
		const QString& getSongFileName() const {
			return m_sSongFileName; }
		const QString& getDrumkitToLoad() const {
			return m_sDrumkitToLoad; }

		const QString& getInstallDrumkitPath() const {
			return m_sInstallDrumkitPath; }

		unsigned getLogLevel() const {
			return m_logLevel; }
		const QString& getLogFile() const {
			return m_sLogFile; }
		bool getLogTimestamps() const {
			return m_bLogTimestamps; }
		bool getLogColors() const {return m_bLogColors; }

		const QString& getSysDataPath() const {
			return m_sSysDataPath; }
		const QString& getUsrDataPath() const {
			return m_sUsrDataPath; }
		const QString& getConfigFilePath() const {
			return m_sConfigFilePath; }
		const QString& getUiLayout() const {
			return m_sUiLayout; }
		int getOscPort() const {
			return m_nOscPort; }
		bool getNoSplashScreen() const {
			return m_bNoSplashScreen; }

		const QString& getShotList() const {
			return m_sShotList; }

		/** Connect-to-headless-engine endpoint (ADR 0016/0018/0032). When non-empty,
		 * the GUI connects to a headless engine at this IPC endpoint instead of
		 * creating its own authoritative engine + audio driver. */
		const QString& getConnectViaIpcEndpoint() const {
			return m_sConnectViaIpcEndpoint; }

		bool getNoReporter() const {
			return m_bNoReporter; }

		/** When set, the GUI quits immediately after it finished starting up
		 * (used by the startup smoke test to verify construction + teardown do
		 * not crash; ADR 0015/0016). */
		bool getQuitAfterStartup() const {
			return m_bQuitAfterStartup; }

	private:
		QString  m_sAudioDriver;
		QString  m_sPlaylistFileName;
		QString  m_sSongFileName;
		QString  m_sDrumkitToLoad;

		QString  m_sInstallDrumkitPath;

		unsigned m_logLevel;
		QString  m_sLogFile;
		bool     m_bLogTimestamps;
		bool     m_bLogColors;

		QString  m_sSysDataPath;
		QString  m_sUsrDataPath;
		QString  m_sConfigFilePath;
		QString  m_sUiLayout;
		int      m_nOscPort;
		bool     m_bNoSplashScreen;

		QString  m_sShotList;

		QString  m_sConnectViaIpcEndpoint;

		bool     m_bNoReporter;

		bool     m_bQuitAfterStartup;
};

#endif
