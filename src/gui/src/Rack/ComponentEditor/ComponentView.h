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

#include "LayerPreview.h"
#include "../../Widgets/Rotary.h"
#include "../../Widgets/WidgetWithScalableFont.h"

namespace H2Core {
	class InstrumentComponent;
}

class Button;
class ClickableLabel;
class InlineEdit;
class LCDCombo;
class LCDDisplay;
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
		static constexpr int nWidth = 290;

		static constexpr int nVerticalSpacing = 5;
		static constexpr int nHeaderHeight = 28;
		static constexpr int nHeaderSpacing = 1;
		static constexpr int nSeparatorHeight = 1;
		static constexpr int nToolBarHeight = 28;
		static constexpr int nButtonWidth = 21;
		static constexpr int nButtonHeight = 24;
		static constexpr int nExpansionButtonWidth = 14;
		static constexpr int nSampleSelectionHeight = 18;
		static constexpr int nLayerPreviewHeight =
			LayerPreview::nHeader +
			LayerPreview::nLayerHeight * 17; // max layers
		static constexpr int nWaveDisplayHeight = 58;
		static constexpr int nLabelHeight = 10;
		static constexpr int nMargin = 7;
		static constexpr int nExpandedHeight =
			ComponentView::nVerticalSpacing * 7 +
			ComponentView::nHeaderHeight +
			ComponentView::nToolBarHeight +
			ComponentView::nLayerPreviewHeight +
			ComponentView::nWaveDisplayHeight +
			Rotary::nHeight +
			ComponentView::nLabelHeight +
			ComponentView::nMargin;

		explicit ComponentView( QWidget* pParent,
								std::shared_ptr<H2Core::InstrumentComponent> );
		~ComponentView();

		void updateColors();
		void updateIcons();
		void updateStyleSheet();
		void updateView();

		void accountForScrollbar( bool bScrollBarVisible );

		int getExpandedHeight() const;
		bool getIsExpanded() const;
		void expand();
		void collapse();

		std::shared_ptr<H2Core::InstrumentComponent> getComponent() const;
		LayerPreview* getLayerPreview() const;
		int getSelectedLayer() const;

		void replaceLayer( int nLayer );
		void setComponent( std::shared_ptr<H2Core::InstrumentComponent> );
		void setLayers(
			QStringList filePaths,
			bool bRenameInstrument,
			bool bAutoVelocity
		);
		void setSelectedLayer( int nLayer );

	public slots:
		void showSampleEditor();

	signals:
		void expandedOrCollapsed();

	private slots:
		void deleteComponent();
		void addNewLayer();
		void removeLayerButtonClicked();
		void sampleSelectionChanged( int );
		void waveDisplayDoubleClicked( QWidget *pRef );


	private:
		std::shared_ptr<H2Core::InstrumentComponent> m_pComponent;
		int m_nSelectedLayer;

		bool m_bIsExpanded;

		void updateActivation();
		void updatePitchDisplay();
		void updateVisibility();

		QVBoxLayout* m_pVBoxMainLayout;

		QPushButton* m_pShowLayersBtn;
		InlineEdit* m_pInlineEdit;
		ClickableLabel* m_pComponentNameLbl;

		QWidget* m_pSeparatorComponent;
		QVBoxLayout* m_pSeparatorLayout;

		QToolBar* m_pToolBarComponent;
		QAction* m_pNewComponentAction;
		QAction* m_pDuplicateComponentAction;
		QAction* m_pDeleteComponentAction;
		Button* m_pComponentSoloBtn;
		Button* m_pComponentMuteBtn;
		Rotary* m_pComponentGainRotary;

		QWidget* m_pComponentWidget;
		QWidget* m_pLayerWidget;

		LayerPreview *m_pLayerPreview;

		QToolBar* m_pToolBarLayer;
		QAction* m_pNewLayerAction;
		QAction* m_pReplaceLayerAction;
		QAction* m_pDuplicateLayerAction;
		QAction* m_pDeleteLayerAction;
		QAction* m_pEditLayerAction;
		Button* m_pLayerSoloBtn;
		Button* m_pLayerMuteBtn;
		Rotary *m_pLayerGainRotary;

		LCDDisplay *m_pLayerPitchLCD;
		ClickableLabel* m_pLayerPitchLbl;
		Rotary *m_pLayerPitchCoarseRotary;
		ClickableLabel* m_pLayerPitchCoarseLbl;
		Rotary *m_pLayerPitchFineRotary;
		ClickableLabel* m_pLayerPitchFineLbl;

		//LCDCombo *__pattern_size_combo;
		LCDCombo *m_pSampleSelectionCombo;
		ClickableLabel* m_pSampleSelectionLbl;
		void setupSampleSelectionCombo();

		WaveDisplay *m_pWaveDisplay;
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
