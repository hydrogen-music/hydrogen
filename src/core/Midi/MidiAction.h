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

#ifndef MIDI_ACTION_H
#define MIDI_ACTION_H

#include <QString>

#include <core/Helpers/Time.h>
#include <core/Object.h>

#include <chrono>
#include <memory>

/**
 * @class MidiAction
 *
 * @brief This class represents a midi action.
 *
 * This class represents actions which can be executed after a midi event
 * occurred. An example is the Type::Mute action, which mutes the outputs of
 * hydrogen.
 *
 * An action can be linked to an event. If this event occurs, the action gets
 * triggered. The handling of events takes place in #MidiInput.
 *
 * @author Sebastian Moors
 *
 * \ingroup docCore docMIDI */
class MidiAction : public H2Core::Object<MidiAction> {
	H2_OBJECT( MidiAction )
   public:
	static constexpr int nInvalidParameter = -10;
	static constexpr float fInvalidParameter = -999;

	enum class Type {
		BeatCounter,
		BpmCcRelative,
		BpmDecr,
		BpmFineCcRelative,
		BpmIncr,
		ClearPattern,
		ClearSelectedInstrument,
		CountIn,
		CountInPauseToggle,
		CountInStopToggle,
		EffectLevelAbsolute,
		EffectLevelRelative,
		FilterCutoffLevelAbsolute,
		GainLevelAbsolute,
		HumanizationSwingAbsolute,
		HumanizationSwingRelative,
		HumanizationTimingAbsolute,
		HumanizationTimingRelative,
		HumanizationVelocityAbsolute,
		HumanizationVelocityRelative,
		InstrumentPitch,
		LoadNextDrumkit,
		LoadPrevDrumkit,
		MasterVolumeAbsolute,
		MasterVolumeRelative,
		Mute,
		MuteToggle,
		NextBar,
		Null,
		PanAbsolute,
		PanAbsoluteSym,
		PanRelative,
		Pause,
		PitchLevelAbsolute,
		Play,
		PlaylistSong,
		PlaylistNextSong,
		PlaylistPrevSong,
		PlayPauseToggle,
		PlayStopToggle,
		PreviousBar,
		RecordExit,
		RecordReady,
		RecordStrobe,
		RecordStrobeToggle,
		RedoAction,
		SelectAndPlayPattern,
		SelectInstrument,
		SelectNextPattern,
		SelectNextPatternCcAbsolute,
		SelectNextPatternRelative,
		SelectOnlyNextPattern,
		SelectOnlyNextPatternCcAbsolute,
		Stop,
		StripMuteToggle,
		StripSoloToggle,
		StripVolumeAbsolute,
		StripVolumeRelative,
		TapTempo,
		TimingClockTick,
		ToggleMetronome,
		UndoAction,
		Unmute
	};
	static QString typeToQString( const Type& type );
	static Type parseType( const QString& sType );

	/** Can be ORed to define which parameters an action both can and must
	 * hold in order to be valid. */
	enum Requires {
		RequiresNone = 0x000,
		RequiresComponent = 0x001,
		RequiresFactor = 0x002,
		RequiresFx = 0x004,
		RequiresInstrument = 0x008,
		RequiresLayer = 0x010,
		RequiresPattern = 0x020,
		RequiresSong = 0x040,
	};
	static Requires requiresFromType( const Type& type );

	MidiAction( Type type, TimePoint timePoint = TimePoint() );
	MidiAction( const std::shared_ptr<MidiAction> pMidiAction );

	/** Similar to the copy constructor but instead of creating a true copy,
	 * the new action will be assigned a fresh time point. */
	static std::shared_ptr<MidiAction> from(
		const std::shared_ptr<MidiAction> pOther,
		TimePoint timePoint = TimePoint()
	);

	/** Prior to version 2.0 of Hydrogen all action parameters were stored as
	 * strings (in order to work around different parameter types, like int vs.
	 * float). This method can be used for compatibility to older config
	 * files. */
	static std::shared_ptr<MidiAction> fromQStrings(
		Type type,
		const QString& sParameter1,
		const QString& sParameter2,
		const QString& sParameter3
	);
	/** In versions of Hydrogen < 2.0 all parameters of MIDI actions were
	 * encoded as QStrings and stored as "parameter", "parameter2", and
	 * "parameter3" within the hydrogen.conf file. Regardless of the actual
	 * value of the parameter. Although it is less expressive and accessible for
	 * folks directly interacting with our XML files, we will keep this scheme
	 * for backward compatibility. */
	void toQStrings(
		QString* pParameter1,
		QString* pParameter2,
		QString* pParameter3
	) const;

	bool isNull() const;

	int getValue() const;
	void setValue( int nValue );

	Requires getRequires() const;

	int getComponent() const;
	void setComponent( int nComponent );
	float getFactor() const;
	void setFactor( float fFactor );
	int getFx() const;
	void setFx( int nFx );
	int getInstrument() const;
	void setInstrument( int nInstrument );
	int getLayer() const;
	void setLayer( int nLayer );
	int getPattern() const;
	void setPattern( int nPattern );
	int getSong() const;
	void setSong( int nSong );

	const Type& getType() const;
	const TimePoint& getTimePoint() const;

	/**
	 * @returns whether the current MidiAction and @a pOther identically in all
	 *   member except of #m_nValue and #m_timePoint. If true, they are
	 *   associated with the same widget. The value will differ depending on the
	 *   incoming MIDI event.
	 */
	bool isEquivalentTo( const std::shared_ptr<MidiAction> pOther ) const;

	friend bool operator==( const MidiAction& lhs, const MidiAction& rhs )
	{
		return (
			lhs.m_type == rhs.m_type && lhs.m_nComponent == rhs.m_nComponent &&
			lhs.m_fFactor == rhs.m_fFactor && lhs.m_nFx == rhs.m_nFx &&
			lhs.m_nInstrument == rhs.m_nInstrument &&
			lhs.m_nLayer == rhs.m_nLayer && lhs.m_nPattern == rhs.m_nPattern &&
			lhs.m_nSong == rhs.m_nSong && lhs.m_nValue == rhs.m_nValue &&
			lhs.m_timePoint == rhs.m_timePoint
		);
	}
	friend bool operator!=( const MidiAction& lhs, const MidiAction& rhs )
	{
		return (
			lhs.m_type != rhs.m_type || lhs.m_nComponent != rhs.m_nComponent ||
			lhs.m_fFactor != rhs.m_fFactor || lhs.m_nFx != rhs.m_nFx ||
			lhs.m_nInstrument != rhs.m_nInstrument ||
			lhs.m_nLayer != rhs.m_nLayer || lhs.m_nPattern != rhs.m_nPattern ||
			lhs.m_nSong != rhs.m_nSong || lhs.m_nValue != rhs.m_nValue ||
			lhs.m_timePoint != rhs.m_timePoint
		);
	}
	friend bool operator==(
		std::shared_ptr<MidiAction> lhs,
		std::shared_ptr<MidiAction> rhs
	)
	{
		if ( lhs == nullptr || rhs == nullptr ) {
			return false;
		}
		return (
			lhs->m_type == rhs->m_type &&
			lhs->m_nComponent == rhs->m_nComponent &&
			lhs->m_fFactor == rhs->m_fFactor && lhs->m_nFx == rhs->m_nFx &&
			lhs->m_nInstrument == rhs->m_nInstrument &&
			lhs->m_nLayer == rhs->m_nLayer &&
			lhs->m_nPattern == rhs->m_nPattern &&
			lhs->m_nSong == rhs->m_nSong && lhs->m_nValue == rhs->m_nValue &&
			lhs->m_timePoint == rhs->m_timePoint
		);
	}
	friend bool operator!=(
		std::shared_ptr<MidiAction> lhs,
		std::shared_ptr<MidiAction> rhs
	)
	{
		if ( lhs == nullptr || rhs == nullptr ) {
			return true;
		}
		return (
			lhs->m_type != rhs->m_type ||
			lhs->m_nComponent != rhs->m_nComponent ||
			lhs->m_fFactor != rhs->m_fFactor || lhs->m_nFx != rhs->m_nFx ||
			lhs->m_nInstrument != rhs->m_nInstrument ||
			lhs->m_nLayer != rhs->m_nLayer ||
			lhs->m_nPattern != rhs->m_nPattern ||
			lhs->m_nSong != rhs->m_nSong || lhs->m_nValue != rhs->m_nValue ||
			lhs->m_timePoint != rhs->m_timePoint
		);
	}

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	Type m_type;
	int m_nValue;

	Requires m_requires;

	int m_nComponent;
	float m_fFactor;
	int m_nFx;
	int m_nInstrument;
	int m_nLayer;
	int m_nPattern;
	int m_nSong;

	TimePoint m_timePoint;
};

inline int MidiAction::getValue() const
{
	return m_nValue;
}

inline void MidiAction::setValue( int nValue )
{
	m_nValue = nValue;
}

inline MidiAction::Requires MidiAction::getRequires() const
{
	return m_requires;
}

inline const MidiAction::Type& MidiAction::getType() const
{
	return m_type;
}
inline const TimePoint& MidiAction::getTimePoint() const
{
	return m_timePoint;
}

#endif
