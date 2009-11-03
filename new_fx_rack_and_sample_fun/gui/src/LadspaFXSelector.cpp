/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "LadspaFXSelector.h"
#include "HydrogenApp.h"
#include <hydrogen/hydrogen.h>
#include "Skin.h"
#include <hydrogen/Song.h>
#include <hydrogen/Preferences.h>

#include <hydrogen/fx/Effects.h>
#include <hydrogen/fx/LadspaFX.h>

using namespace std;
using namespace H2Core;

LadspaFXSelector::LadspaFXSelector(int nLadspaFX)
 : QDialog( NULL )
 , Object( "LadspaFXSelector" )
 , m_pCurrentItem( NULL )
{
	//INFOLOG( "INIT" );

	setupUi( this );

	setFixedSize( width(), height() );

	setWindowTitle( trUtf8( "Select LADSPA FX" ) );
	setWindowIcon( QPixmap( Skin::getImagePath() + "/icon16.png" ) );

	m_sSelectedPluginName = "";

	m_nameLbl->setText( QString("") );
	m_labelLbl->setText( QString("") );
	m_typeLbl->setText( QString("") );
	m_pIDLbl->setText( QString("") );
	m_pMakerLbl->setText( QString("") );
	m_pCopyrightLbl->setText( QString("") );
	m_pPluginsListBox->clear();
	m_pOkBtn->setEnabled(false);

	m_pGroupsListView->setHeaderLabels( QStringList( trUtf8( "Groups" ) ) );

#ifdef LADSPA_SUPPORT
	//Song *pSong = Hydrogen::get_instance()->getSong();
	LadspaFX *pFX = Effects::get_instance()->getLadspaFX(nLadspaFX);
	if (pFX) {
		m_sSelectedPluginName = pFX->getPluginName();
	}
	buildLadspaGroups();

	m_pGroupsListView->setItemHidden( m_pGroupsListView->headerItem(), true );


//	LadspaFXGroup* pFXGroup = LadspaFX::getLadspaFXGroup();
//	vector<LadspaFXInfo*> list = findPluginsInGroup( m_sSelectedPluginName, pFXGroup );
//	for (uint i = 0; i < list.size(); i++) {
//		m_pPluginsListBox->addItem( list[i]->m_sName.c_str() );
//	}
#endif

	connect( m_pPluginsListBox, SIGNAL( itemSelectionChanged () ), this, SLOT( pluginSelected() ) );
	pluginSelected();
// 	on_m_pGroupsListView_currentItemChanged( m_pGroupsListView->currentItem(), NULL );
}



LadspaFXSelector::~LadspaFXSelector()
{
	//INFOLOG( "DESTROY" );
}



void LadspaFXSelector::buildLadspaGroups()
{
#ifdef LADSPA_SUPPORT
	m_pGroupsListView->clear();

//	QTreeWidgetItem* pRootItem = new QTreeWidgetItem( );
// 	pRootItem->setText( 0, trUtf8("Groups") );
// 	m_pGroupsListView->addTopLevelItem( pRootItem );
// 	m_pGroupsListView->setItemExpanded( pRootItem, true );
	
	H2Core::LadspaFXGroup* pFXGroup = Effects::get_instance()->getLadspaFXGroup();
	for (uint i = 0; i < pFXGroup->getChildList().size(); i++) {
		H2Core::LadspaFXGroup *pNewGroup = ( pFXGroup->getChildList() )[ i ];
		addGroup( m_pGroupsListView, pNewGroup );
	}
	m_pGroupsListView->setCurrentItem( m_pCurrentItem );
#endif
}



#ifdef LADSPA_SUPPORT
void LadspaFXSelector::addGroup( QTreeWidget *parent, H2Core::LadspaFXGroup *pGroup )
{
	QTreeWidgetItem* pNewItem = new QTreeWidgetItem( parent );
	QFont f = pNewItem->font( 0 );
	f.setBold( true );
	pNewItem->setFont( 0, f );
	buildGroup( pNewItem, pGroup );
}

void LadspaFXSelector::addGroup( QTreeWidgetItem * parent, H2Core::LadspaFXGroup *pGroup )
{
	QTreeWidgetItem* pNewItem = new QTreeWidgetItem( parent );
	buildGroup( pNewItem, pGroup );
}

void LadspaFXSelector::buildGroup( QTreeWidgetItem *pNewItem, H2Core::LadspaFXGroup *pGroup )
{
	QString sGroupName = pGroup->getName();
	if (sGroupName == QString("Uncategorized")) {
		sGroupName = trUtf8("Alphabetic List");
	}
	else if (sGroupName == QString("Categorized(LRDF)")) {
		sGroupName = trUtf8("Categorized");
	}
	else if (sGroupName == QString("Recently Used")) {
		sGroupName = trUtf8("Recently Used");
	}
	pNewItem->setText( 0, sGroupName );


	for ( uint i = 0; i < pGroup->getChildList().size(); i++ ) {
		H2Core::LadspaFXGroup *pNewGroup = ( pGroup->getChildList() )[ i ];

		addGroup( pNewItem, pNewGroup );
	}
	for(uint i = 0; i < pGroup->getLadspaInfo().size(); i++) {
		H2Core::LadspaFXInfo* pInfo = (pGroup->getLadspaInfo())[i];
		if (pInfo->m_sName == m_sSelectedPluginName) {
			m_pCurrentItem = pNewItem;
			break;
		}
	}
}
#endif



QString LadspaFXSelector::getSelectedFX()
{
	return m_sSelectedPluginName;
}


void LadspaFXSelector::pluginSelected()
{
#ifdef LADSPA_SUPPORT
	//INFOLOG( "[pluginSelected]" );
	//
        
	if ( m_pPluginsListBox->selectedItems().isEmpty() ) return;

	QString sSelected = m_pPluginsListBox->currentItem()->text();
	m_sSelectedPluginName = sSelected;


	std::vector<H2Core::LadspaFXInfo*> pluginList = Effects::get_instance()->getPluginList();
	for (uint i = 0; i < pluginList.size(); i++) {
		H2Core::LadspaFXInfo *pFXInfo = pluginList[i];
		if (pFXInfo->m_sName == m_sSelectedPluginName ) {

			m_nameLbl->setText(  pFXInfo->m_sName );
			m_labelLbl->setText( pFXInfo->m_sLabel );

			if ( ( pFXInfo->m_nIAPorts == 2 ) && ( pFXInfo->m_nOAPorts == 2 ) ) {		// Stereo plugin
				m_typeLbl->setText( trUtf8("Stereo") );
			}
			else if ( ( pFXInfo->m_nIAPorts == 1 ) && ( pFXInfo->m_nOAPorts == 1 ) ) {	// Mono plugin
				m_typeLbl->setText( trUtf8("Mono") );
			}
			else {
				// not supported
				m_typeLbl->setText( trUtf8("Not supported") );
			}

			m_pIDLbl->setText( pFXInfo->m_sID );
			m_pMakerLbl->setText( pFXInfo->m_sMaker );
			m_pCopyrightLbl->setText( pFXInfo->m_sCopyright );

			break;
		}
	}
	m_pOkBtn->setEnabled(true);
#endif
}



void LadspaFXSelector::on_m_pGroupsListView_currentItemChanged( QTreeWidgetItem * currentItem, QTreeWidgetItem * previous )
{
	UNUSED( previous );
#ifdef LADSPA_SUPPORT
	//INFOLOG( "new selection: " + currentItem->text(0).toLocal8Bit().constData() );

	m_pOkBtn->setEnabled(false);
	m_nameLbl->setText( QString("") );
	m_labelLbl->setText( QString("") );
	m_typeLbl->setText( QString("") );
	m_pIDLbl->setText( QString("") );
	m_pMakerLbl->setText( QString("") );
	m_pCopyrightLbl->setText( QString("") );

	// nothing was selected
	if ( currentItem == NULL ) {
		return;
	}
	
	if ( currentItem->childCount() ) {
		currentItem->setExpanded( true );
	}

	QString itemText = currentItem->text( 0 );

	m_pPluginsListBox->clear(); // ... Why not anyway ? Jakob Lund

// 	while( m_pPluginsListBox->count() != 0) {    // NOTE commented out:
// 		m_pPluginsListBox->takeItem( 0 );    // This way of clearing the list causes multiple signal emissions,
// 	}                                            // each time calling pluginSelected().  Jakob.

	H2Core::LadspaFXGroup* pFXGroup = Effects::get_instance()->getLadspaFXGroup();

	std::vector<H2Core::LadspaFXInfo*> pluginList = findPluginsInGroup( itemText, pFXGroup );
	
	int selectedIndex = -1;
	for (int i = 0; i < (int)pluginList.size(); i++) {
		//INFOLOG( "adding plugin: " + pluginList[ i ]->m_sName );
		m_pPluginsListBox->addItem( pluginList[ i ]->m_sName );
		if ( pluginList[ i ]->m_sName == m_sSelectedPluginName ) {
			selectedIndex = i;
		}
	}
	if ( selectedIndex >= 0 )
		m_pPluginsListBox->setCurrentRow( selectedIndex );
#endif
}


#ifdef LADSPA_SUPPORT
std::vector<H2Core::LadspaFXInfo*> LadspaFXSelector::findPluginsInGroup( const QString& sSelectedGroup, H2Core::LadspaFXGroup *pGroup )
{
	//INFOLOG( "group: " + sSelectedGroup );
	vector<H2Core::LadspaFXInfo*> list;

	if ( pGroup->getName() == sSelectedGroup ) {
		//INFOLOG( "found..." );
		for ( uint i = 0; i < pGroup->getLadspaInfo().size(); ++i ) {
			H2Core::LadspaFXInfo *pInfo = ( pGroup->getLadspaInfo() )[i];
			list.push_back( pInfo );
		}
		return list;
	}
	else {
		//INFOLOG( "not found...searching in the child groups" );
		for ( uint i = 0; i < pGroup->getChildList().size(); ++i ) {
			H2Core::LadspaFXGroup *pNewGroup = ( pGroup->getChildList() )[ i ];
			list = findPluginsInGroup( sSelectedGroup, pNewGroup );
			if (list.size() != 0) {
				return list;
			}
		}
	}

	//WARNINGLOG( "[findPluginsInGroup] no group found ('" + sSelectedGroup + "')" );
	return list;
}
#endif
