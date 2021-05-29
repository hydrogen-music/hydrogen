/*
 * Hydrogen
 * Copyright (C) 2021 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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

#ifndef COMMON_STRINGS_H
#define COMMON_STRINGS_H

#include <QString>
#include <QWidget>
#include <core/Object.h>

class CommonStrings : public QWidget, public H2Core::Object {
	H2_OBJECT
	Q_OBJECT

public:
	static const QString& getSmallSoloButton() { return m_sSmallSoloButton; }
	static const QString& getSmallMuteButton() { return m_sSmallMuteButton; }
	static const QString& getBigMuteButton() { return m_sBigMuteButton; }
	static const QString& getBypassButton() { return m_sBypassButton; }
	static const QString& getEditButton() { return m_sEditButton; }
	static const QString& getClearButton() { return m_sClearButton; }
	static const QString& getPlaybackTrackButton() { return m_sPlaybackTrackButton; }
	static const QString& getTimelineButton() { return m_sTimelineButton; }
	static const QString& getTimelineBigButton() { return m_sTimelineBigButton; }
	static const QString& getFXButton() { return m_sFXButton; }
	static const QString& getPeakButton() { return m_sPeakButton; }
	static const QString& getGeneralButton() { return m_sGeneralButton; }
	static const QString& getInstrumentButton() { return m_sInstrumentButton; }
	static const QString& getSoundLibraryButton() { return m_sSoundLibraryButton; }
	static const QString& getLayersButton() { return m_sLayersButton; }
	static const QString& getLoadLayerButton() { return m_sLoadLayerButton; }
	static const QString& getDeleteLayerButton() { return m_sDeleteLayerButton; }
	static const QString& getEditLayerButton() { return m_sEditLayerButton; }
	static const QString& getBeatCounterButton() { return m_sBeatCounterButton; }
	static const QString& getBeatCounterSetPlayButtonOff() { return m_sBeatCounterSetPlayButtonOff; }
	static const QString& getBeatCounterSetPlayButtonOn() { return m_sBeatCounterSetPlayButtonOn; }
	static const QString& getRubberbandButton() { return m_sRubberbandButton; }
	static const QString& getJackTransportButton() { return m_sJackTransportButton; }
	static const QString& getJackMasterButton() { return m_sJackMasterButton; }
	static const QString& getMixerButton() { return m_sMixerButton; }
	static const QString& getInstrumentRackButton() { return m_sInstrumentRackButton; }
	static const QString& getPatternModeButton() { return m_sPatternModeButton; }
	static const QString& getSongModeButton() { return m_sSongModeButton; }
	
private:
	static QString m_sSmallSoloButton;
	static QString m_sSmallMuteButton;
	static QString m_sBigMuteButton;
	static QString m_sBypassButton;
	static QString m_sEditButton;
	static QString m_sClearButton;
	static QString m_sPlaybackTrackButton;
	static QString m_sTimelineButton;
	static QString m_sTimelineBigButton;
	static QString m_sFXButton;
	static QString m_sPeakButton;
	static QString m_sGeneralButton;
	static QString m_sInstrumentButton;
	static QString m_sSoundLibraryButton;
	static QString m_sLayersButton;
	static QString m_sLoadLayerButton;
	static QString m_sEditLayerButton;
	static QString m_sDeleteLayerButton;
	static QString m_sBeatCounterButton;
	static QString m_sBeatCounterSetPlayButtonOff;
	static QString m_sBeatCounterSetPlayButtonOn;
	static QString m_sRubberbandButton;
	static QString m_sJackTransportButton;
	static QString m_sJackMasterButton;
	static QString m_sMixerButton;
	static QString m_sInstrumentRackButton;
	static QString m_sPatternModeButton;
	static QString m_sSongModeButton;
};
#endif
