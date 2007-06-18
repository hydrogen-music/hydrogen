/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: LadspaFXSelector.cpp,v 1.7 2005/05/01 19:50:57 comix Exp $
 *
 */

#include "config.h"

#include "LadspaFXSelector.h"
#include "HydrogenApp.h"
#include "lib/Hydrogen.h"
#include "Skin.h"
#include "lib/Song.h"

#include "lib/fx/LadspaFX.h"
#include "qlistview.h"
#include "qpixmap.h"
#include "qlabel.h"
#include "qlistbox.h"
#include "qpushbutton.h"
#include "qstring.h"


LadspaFXSelector::LadspaFXSelector(int nLadspaFX) : LadspaFXSelector_UI( NULL ), Object( "LadspaFXSelector" )
{
//	infoLog( "INIT" );

	setMinimumSize(width(), height());
	setMaximumSize(width(), height());

	setCaption( trUtf8( "Select LADSPA FX" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	m_sSelectedPluginName = "";

	m_nameLbl->setText( QString("") );
	m_labelLbl->setText( QString("") );
	m_typeLbl->setText( QString("") );
	m_pIDLbl->setText( QString("") );
	m_pMakerLbl->setText( QString("") );
	m_pCopyrightLbl->setText( QString("") );
	m_pPluginsListBox->clear();
	m_pOkBtn->setEnabled(false);

	Song *pSong = (Hydrogen::getInstance() )->getSong();
#ifdef LADSPA_SUPPORT
	LadspaFX *pFX = pSong->getLadspaFX(nLadspaFX);
	if (pFX) {
		m_sSelectedPluginName = pFX->getPluginName();
	}
	buildLadspaGroups();


	LadspaFXGroup* pFXGroup = (HydrogenApp::getInstance())->getFXRootGroup();
	vector<LadspaFXInfo*> list = findPluginsInGroup( m_sSelectedPluginName, pFXGroup );
	for (uint i = 0; i < list.size(); i++) {
		m_pPluginsListBox->insertItem( list[i]->sName.c_str() );
	}
#endif

}



LadspaFXSelector::~LadspaFXSelector()
{
//	infoLog( "DESTROY" );
}


void LadspaFXSelector::buildLadspaGroups()
{
#ifdef LADSPA_SUPPORT
	m_pGroupsListView->clear();
	QListViewItem* pRootItem = new QListViewItem( m_pGroupsListView, 0 );
	pRootItem->setOpen( true );
	pRootItem->setText( 0, trUtf8("Groups") );

	LadspaFXGroup* pFXGroup = (HydrogenApp::getInstance())->getFXRootGroup();
	for (uint i = 0; i < pFXGroup->getChildGroups().size(); i++) {
		LadspaFXGroup *pNewGroup = (pFXGroup->getChildGroups())[i];
		addGroup( pRootItem, pNewGroup );
	}
#endif
}


#ifdef LADSPA_SUPPORT
void LadspaFXSelector::addGroup(QListViewItem *pItem, LadspaFXGroup *pGroup)
{
	QString sGroupName = QString(pGroup->getName().c_str());
	if (sGroupName == QString("Uncategorized")) {
		sGroupName = trUtf8("Uncategorized");
	}
	else if (sGroupName == QString("Categorized(LRDF)")) {
		sGroupName = trUtf8("Categorized (LRDF)");
	}
	QListViewItem* pNewItem = new QListViewItem( pItem, 0 );
	pNewItem->setOpen( true );
	pNewItem->setText( 0, sGroupName );

	for (uint i = 0; i < pGroup->getChildGroups().size(); i++) {
		LadspaFXGroup *pNewGroup = (pGroup->getChildGroups())[i];

		addGroup( pNewItem, pNewGroup );
	}
	for(uint i = 0; i < pGroup->getLadspaInfo().size(); i++) {
		LadspaFXInfo* pInfo = (pGroup->getLadspaInfo())[i];
		if (pInfo->sName == m_sSelectedPluginName) {
			m_pGroupsListView->setSelected(pNewItem, true);
			break;
		}
	}
}
#endif



string LadspaFXSelector::getSelectedFX()
{
	return m_sSelectedPluginName;
}


void LadspaFXSelector::pluginSelected()
{
#ifdef LADSPA_SUPPORT
	QString sSelected = m_pPluginsListBox->currentText();
	m_sSelectedPluginName = string(sSelected.latin1());


	vector<LadspaFXInfo*> pluginList = (HydrogenApp::getInstance())->getPluginList();
	for (uint i = 0; i < pluginList.size(); i++) {
		LadspaFXInfo *pFXInfo = pluginList[i];
		if (pFXInfo->sName == m_sSelectedPluginName ) {

			m_nameLbl->setText( QString( pFXInfo->sName.c_str() ) );
			m_labelLbl->setText( QString( pFXInfo->sLabel.c_str() ) );

			if ( ( pFXInfo->nIAPorts == 2 ) && ( pFXInfo->nOAPorts == 2 ) ) {		// Stereo plugin
				m_typeLbl->setText( trUtf8("Stereo") );
			}
			else if ( ( pFXInfo->nIAPorts == 1 ) && ( pFXInfo->nOAPorts == 1 ) ) {	// Mono plugin
				m_typeLbl->setText( trUtf8("Mono") );
			}
			else {
				// not supported
				m_typeLbl->setText( trUtf8("Not supported") );
			}

			m_pIDLbl->setText( QString( pFXInfo->sID.c_str() ) );
			m_pMakerLbl->setText( QString( pFXInfo->sMaker.c_str() ) );
			m_pCopyrightLbl->setText( QString( pFXInfo->sCopyright.c_str() ) );

			break;
		}
	}
	m_pOkBtn->setEnabled(true);
#endif
}



void LadspaFXSelector::groupSelected()
{
#ifdef LADSPA_SUPPORT
	m_pOkBtn->setEnabled(false);
	m_nameLbl->setText( QString("") );
	m_labelLbl->setText( QString("") );
	m_typeLbl->setText( QString("") );
	m_pIDLbl->setText( QString("") );
	m_pMakerLbl->setText( QString("") );
	m_pCopyrightLbl->setText( QString("") );

	QListViewItem* pItem = m_pGroupsListView->selectedItem();
	QString itemText = pItem->text(0);

	m_pPluginsListBox->clear();

	LadspaFXGroup* pFXGroup = (HydrogenApp::getInstance())->getFXRootGroup();

	vector<LadspaFXInfo*> list = findPluginsInGroup( string( itemText.latin1() ), pFXGroup );
	for (uint i = 0; i < list.size(); i++) {
		m_pPluginsListBox->insertItem( list[i]->sName.c_str() );
		if (list[i]->sName == m_sSelectedPluginName) {
			m_pPluginsListBox->setCurrentItem( i );
		}
	}
#endif
}


#ifdef LADSPA_SUPPORT
vector<LadspaFXInfo*> LadspaFXSelector::findPluginsInGroup( string sSelectedGroup, LadspaFXGroup *pGroup )
{
	vector<LadspaFXInfo*> list;

	if ( pGroup->getName() == sSelectedGroup ) {

		for (uint i = 0; i < pGroup->getLadspaInfo().size(); i++) {
			LadspaFXInfo *pInfo = (pGroup->getLadspaInfo())[i];
			list.push_back( pInfo );
		}
		return list;
	}
	else {
		for (uint i = 0; i < pGroup->getChildGroups().size(); i++) {
			LadspaFXGroup *pNewGroup = (pGroup->getChildGroups())[i];
			list = findPluginsInGroup( sSelectedGroup, pNewGroup );
			if (list.size() != 0) {
				return list;
			}
		}
	}

	return list;
}
#endif



