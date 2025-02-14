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

#ifndef SONG_EDITOR_POSITION_RULER_H
#define SONG_EDITOR_POSITION_RULER_H

#include <memory>

#include <QtGui>
#include <QtWidgets>
#include <QList>

#include <core/Object.h>
#include <core/Timeline.h>
#include "../Widgets/WidgetWithScalableFont.h"

/** \ingroup docGUI*/
class SongEditorPositionRuler :  public QWidget,
								 protected WidgetWithScalableFont<8, 10, 12>,
								 public H2Core::Object<SongEditorPositionRuler>
{
    H2_OBJECT(SongEditorPositionRuler)
	Q_OBJECT

	public:
		explicit SongEditorPositionRuler( QWidget *parent );
		~SongEditorPositionRuler();	

		void setGridWidth( int width );

	static int tickToColumn( float fTick, int nGridWidth );
		static constexpr int m_nMinimumHeight = 50;
		void showTagWidget( int nColumn );
		void showBpmWidget( int nColumn );
		void updateEditor();
		void updatePosition();
		void updateSongSize();

	private:
		void createBackground();

		QTimer *			m_pTimer;
		int				m_nGridWidth;

	int m_nActiveBpmWidgetColumn;
	int m_nHoveredColumn;

	/** Indicated the part of the widget the cursor is hovering over.*/
	enum class HoveredRow {
		/** Cursor is not hovering the widget.*/
		None,
		/** Upper half until the lower end of the tempo marker
			text. */ 
		TempoMarker,
		/** Still part of the upper half, but only the last
			#m_nTagHeight pixels.*/
		Tag,
		/** Lower half*/
		Ruler
	};
	HoveredRow m_hoveredRow;

	/** Cached position of the playhead.*/
	float m_fTick;
	/** Cached and coarsed-grained position of the playhead.*/
	int m_nColumn;

	int m_nTagHeight;

	/** Area covering the length of the song columns.*/
	int m_nActiveColumns;

		QPixmap *			m_pBackgroundPixmap;
		bool 				m_bBackgroundInvalid;
		bool				m_bRightBtnPressed;
		
		virtual void mouseMoveEvent(QMouseEvent *ev) override;
		virtual void mousePressEvent( QMouseEvent *ev ) override;
		virtual void mouseReleaseEvent(QMouseEvent *ev) override;
		virtual void paintEvent( QPaintEvent *ev ) override;

	// virtual void enterEvent( QEvent* ev ) override;
	virtual void leaveEvent( QEvent* ev ) override;
	virtual bool event( QEvent* ev ) override;

	/** Calculates the position in pixel required to the painter for a
	 * particular @a nColumn of the grid.
	 *
	 * TODO: There needs to be some refactoring / common basis for
	 * song (and pattern) editor classes.
 	 */
	int columnToX( int nColumn ) const;
	int xToColumn( int nX ) const;

	void showToolTip( const QPoint& pos, const QPoint& globalPos );

	void drawTempoMarker( std::shared_ptr<const H2Core::Timeline::TempoMarker> pTempoMarker,
						  bool bEmphasize, QPainter& painter );
	/**
	 * @param pTempoMarker Associated #TempoMarker
	 * @param bEmphasize Whether the text of @a pTempoMarker will be
	 *   printed with bold font.
	 */
	QRect calcTempoMarkerRect( std::shared_ptr<const H2Core::Timeline::TempoMarker> pTempoMarker,
		bool bEmphasize = false ) const;

};

#endif
