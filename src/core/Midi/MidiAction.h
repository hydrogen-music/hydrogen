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
* Each action has two independent parameters. The two parameters are optional
* and can be used to carry additional information, which mean only something to
* this very Action. They can have totally different meanings for other Actions.
* Example: parameter1 is the Mixer strip and parameter 2 a multiplier for the
* volume change on this strip
*
* @author Sebastian Moors
*
* \ingroup docCore docMIDI */
class MidiAction : public H2Core::Object<MidiAction> {
	H2_OBJECT(MidiAction)
	public:

		enum class Type {
			BeatCounter,
			BpmCcRelative,
			BpmDecr,
			BpmFineCcRelative,
			BpmIncr,
			ClearPattern,
			ClearSelectedInstrument,
			EffectLevelAbsolute,
			EffectLevelRelative,
			FilterCutoffLevelAbsolute,
			GainLevelAbsolute,
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

	MidiAction( Type type, TimePoint timePoint = TimePoint() );
	MidiAction( const std::shared_ptr<MidiAction> pMidiAction );

		/** Similar to the copy constructor but instead of creating a true copy,
		 * the new action will be assigned a fresh time point. */
		static std::shared_ptr<MidiAction> from(
			const std::shared_ptr<MidiAction> pOther,
			TimePoint timePoint = TimePoint() );

	bool isNull() const;

		const QString& getParameter1() const;
		void setParameter1( const QString& text );

		const QString& getParameter2() const;
		void setParameter2( const QString& text );

		const QString& getParameter3() const;
		void setParameter3( const QString& text );

		const QString& getValue() const;
		void setValue( const QString& text );

		const Type& getType() const;
		const TimePoint& getTimePoint() const;

	/**
	 * @returns whether the current MidiAction and @a pOther identically in all
	 *   member except of #m_sValue and #m_timePoint. If true, they are
	 *   associated with the same widget. The value will differ depending on the
	 *   incoming MIDI event.
	 */
	bool isEquivalentTo( const std::shared_ptr<MidiAction> pOther ) const;

	friend bool operator ==(const MidiAction& lhs, const MidiAction& rhs ) {
		return ( lhs.m_type == rhs.m_type &&
				 lhs.m_sParameter1 == rhs.m_sParameter1 &&
				 lhs.m_sParameter2 == rhs.m_sParameter2 &&
				 lhs.m_sParameter3 == rhs.m_sParameter3 &&
				 lhs.m_sValue == rhs.m_sValue &&
				 lhs.m_timePoint == rhs.m_timePoint );
	}
	friend bool operator !=(const MidiAction& lhs, const MidiAction& rhs ) {
		return ( lhs.m_type != rhs.m_type ||
				 lhs.m_sParameter1 != rhs.m_sParameter1 ||
				 lhs.m_sParameter2 != rhs.m_sParameter2 ||
				 lhs.m_sParameter3 != rhs.m_sParameter3 ||
				 lhs.m_sValue != rhs.m_sValue ||
				 lhs.m_timePoint != rhs.m_timePoint );
	}
	friend bool operator ==( std::shared_ptr<MidiAction> lhs,
							 std::shared_ptr<MidiAction> rhs ) {
		if ( lhs == nullptr || rhs == nullptr ) {
			return false;
		}
		return ( lhs->m_type == rhs->m_type &&
				 lhs->m_sParameter1 == rhs->m_sParameter1 &&
				 lhs->m_sParameter2 == rhs->m_sParameter2 &&
				 lhs->m_sParameter3 == rhs->m_sParameter3 &&
				 lhs->m_sValue == rhs->m_sValue &&
				 lhs->m_timePoint == rhs->m_timePoint );
	}
	friend bool operator !=( std::shared_ptr<MidiAction> lhs,
							 std::shared_ptr<MidiAction> rhs ) {
		if ( lhs == nullptr || rhs == nullptr ) {
			return true;
		}
		return ( lhs->m_type != rhs->m_type ||
				 lhs->m_sParameter1 != rhs->m_sParameter1 ||
				 lhs->m_sParameter2 != rhs->m_sParameter2 ||
				 lhs->m_sParameter3 != rhs->m_sParameter3 ||
				 lhs->m_sValue != rhs->m_sValue ||
				 lhs->m_timePoint != rhs->m_timePoint );
	}

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "",
						   bool bShort = true ) const override;

	private:
		Type m_type;
		QString m_sParameter1;
		QString m_sParameter2;
		QString m_sParameter3;
		QString m_sValue;
		TimePoint m_timePoint;
};

inline const QString& MidiAction::getParameter1() const {
	return m_sParameter1;
}

inline void MidiAction::setParameter1( const QString& text ) {
	m_sParameter1 = text;
}

inline const QString& MidiAction::getParameter2() const {
	return m_sParameter2;
}

inline void MidiAction::setParameter2( const QString& text ){
	m_sParameter2 = text;
}

inline const QString& MidiAction::getParameter3() const {
	return m_sParameter3;
}

inline void MidiAction::setParameter3( const QString& text ){
	m_sParameter3 = text;
}

inline const QString& MidiAction::getValue() const {
	return m_sValue;
}

inline void MidiAction::setValue( const QString& text ){
	m_sValue = text;
}

inline const MidiAction::Type& MidiAction::getType() const {
	return m_type;
}
inline const TimePoint& MidiAction::getTimePoint() const {
	return m_timePoint;
}

#endif
