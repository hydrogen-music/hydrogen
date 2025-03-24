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

#ifndef COMPONENT_VIEW_H
#define COMPONENT_VIEW_H

#include <QtGui>
#include <QtWidgets>

#include <memory>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core {
	class InstrumentComponent;
}

class Button;
class ClickableLabel;
class LayerPreview;
class LCDCombo;
class LCDDisplay;
class Rotary;
class WaveDisplay;

/** Editor for all layers and settings contained in one
 * #H2Core::InstrumentComponent.
 *
 * \ingroup docGUI*/
class ComponentView : public QWidget,
					  protected WidgetWithScalableFont<10, 12, 14>,
					  public H2Core::Object<ComponentView>
{
	H2_OBJECT(ComponentView)
	Q_OBJECT

	public:
		static constexpr int nHeaderHeight = 27;
		static constexpr int nExpandedHeight = 420;

		explicit ComponentView( QWidget* pParent,
								std::shared_ptr<H2Core::InstrumentComponent> );
		~ComponentView();

		void updateView();

		bool getIsExpanded() const;
		void expand();
		void narrow();

		std::shared_ptr<H2Core::InstrumentComponent> getComponent() const;
		LayerPreview* getLayerPreview() const;
		int getSelectedLayer() const;

		void setComponent( std::shared_ptr<H2Core::InstrumentComponent> );
		void setSelectedLayer( int nLayer );

	public slots:
		void renameComponentAction();
		void showSampleEditor();

	private slots:
		void deleteComponent();
		void loadLayerBtnClicked();
		void removeLayerButtonClicked();
		void sampleSelectionChanged( int );
		void waveDisplayDoubleClicked( QWidget *pRef );


	private:
		virtual void mousePressEvent( QMouseEvent *event ) override;

		std::shared_ptr<H2Core::InstrumentComponent> m_pComponent;
		int m_nSelectedLayer;

		bool m_bIsExpanded;

		void updateActivation();
		void updateVisibility();

		Button* m_pShowLayersBtn;
		ClickableLabel* m_pComponentNameLbl;
		Button* m_pComponentSoloBtn;
		Button* m_pComponentMuteBtn;
		Rotary* m_pComponentGainRotary;

		LayerPreview *m_pLayerPreview;
		QScrollArea *m_pLayerScrollArea;

		Rotary *m_pLayerGainRotary;
		LCDDisplay *m_pLayerGainLCD;
		ClickableLabel* m_pLayerGainLbl;
		ClickableLabel* m_pCompoGainLbl;
		ClickableLabel* m_pLayerPitchLbl;
		ClickableLabel* m_pLayerPitchCoarseLbl;
		ClickableLabel* m_pLayerPitchFineLbl;

		Rotary *m_pLayerPitchCoarseRotary;
		Rotary *m_pLayerPitchFineRotary;

		LCDDisplay *m_pLayerPitchCoarseLCD;
		LCDDisplay *m_pLayerPitchFineLCD;

		//LCDCombo *__pattern_size_combo;
		LCDCombo *m_pSampleSelectionCombo;
		ClickableLabel* m_pSampleSelectionLbl;
		void setupSampleSelectionCombo();

		WaveDisplay *m_pWaveDisplay;

		Button *m_pLoadLayerBtn;
		Button *m_pRemoveLayerBtn;
		Button *m_pSampleEditorBtn;

		void setAutoVelocity();

		QMenu* m_pPopup;
		QAction* m_pDeleteAction;
};

inline bool ComponentView::getIsExpanded() const {
	return m_bIsExpanded;
}
inline std::shared_ptr<H2Core::InstrumentComponent> ComponentView::getComponent() const {
	return m_pComponent;
}
inline LayerPreview* ComponentView::getLayerPreview() const {
	return m_pLayerPreview;
}
inline int ComponentView::getSelectedLayer() const {
	return m_nSelectedLayer;
}

inline void ComponentView::setSelectedLayer( int nLayer ) {
	if ( m_nSelectedLayer != nLayer ) {
		m_nSelectedLayer = nLayer;
	}
}

#endif
