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

#include "AudioFileBrowser.h"
#include "../HydrogenApp.h"
#include "InstrumentEditor/InstrumentEditor.h"
#include "SampleWaveDisplay.h"
#include "../Widgets/Button.h"
#include "../Skin.h"

#include <hydrogen/Preferences.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/audio_engine.h>

#include <QModelIndex>
#include <QTreeWidget>
#include <QMessageBox>

using namespace H2Core;

const char* AudioFileBrowser::__class_name = "AudioFileBrowser";

AudioFileBrowser::AudioFileBrowser ( QWidget* pParent, bool bAllowMultiSelect, bool bShowInstrumentManipulationControls)
		: QDialog ( pParent )
		, Object ( __class_name )
{
	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( tr ( "Audio File Browser" ) );
	setFixedSize ( width(), height() );
	
	m_bAllowMultiSelect = bAllowMultiSelect;
	m_bShowInstrumentManipulationControls = bShowInstrumentManipulationControls;

	m_pDirModel = new QDirModel();
	m_pDirModel->setFilter( QDir::AllDirs | QDir::AllEntries | QDir::NoDotAndDotDot );
	m_pDirModel->setNameFilters( QStringList() << "*.ogg" << "*.OGG" << "*.wav" << "*.WAV" << "*.flac"<< "*.FLAC" << "*.aiff" << "*.AIFF"<< "*.au" << "*.AU" );
	m_pDirModel->setSorting( QDir::DirsFirst |QDir::Name );
	m_ModelIndex = m_pDirModel->index( QDir::currentPath() );
	
	m_pPlayBtn->setEnabled( false );
	m_pStopBtn->setEnabled( false );
	openBTN->setEnabled( false );

	m_pTree = new QTreeView( treeView );
	m_pTree->setModel( m_pDirModel );
	m_pTree->resize( 799, 310 );
	m_pTree->header()->resizeSection( 0, 405 );
	m_pTree->setAlternatingRowColors( true );
	m_pTree->setRootIndex( m_pDirModel->index( Preferences::get_instance()->__lastsampleDirectory ) );
	
	pathLineEdit->setText( Preferences::get_instance()->__lastsampleDirectory );
	m_pSampleFilename = "";
	m_pSelectedFile << "false" << "false";

	m_sEmptySampleFilename = Filesystem::empty_sample_path();

	m_pPathUptoolButton->setIcon( QIcon( Skin::getImagePath() + "/audiFileBrowser/go-up.png"));
	m_pPathUptoolButton->setToolTip( QString("Parent Folder"));
	m_pPathHometoolButton->setIcon( QIcon( Skin::getImagePath() + "/audiFileBrowser/go-home.png"));
	m_pPathHometoolButton->setToolTip( QString("Home"));

	m_pPlayBtn->setIcon( QIcon( Skin::getImagePath() + "/audiFileBrowser/player_play.png"));
	m_pPlayBtn->setToolTip( QString("Play selected"));
	m_pStopBtn->setIcon( QIcon( Skin::getImagePath() + "/audiFileBrowser/player_stop.png"));
	m_pStopBtn->setToolTip( QString("Stop"));

	m_pSampleWaveDisplay = new SampleWaveDisplay( waveformview );
	m_pSampleWaveDisplay->updateDisplay( m_sEmptySampleFilename );
	m_pSampleWaveDisplay->move( 3, 3 );

	playSamplescheckBox->setChecked( Preferences::get_instance()->__playsamplesonclicking );
	//get the kde or gnome environment variable for mouse double or single clicking
	m_SingleClick = false;
	getEnvironment();
	
	if( !m_bShowInstrumentManipulationControls ) {
		useNameCheckBox->hide();
		autoVelCheckBox->hide();
	}

	connect( m_pTree, SIGNAL( clicked( const QModelIndex&) ), SLOT( clicked( const QModelIndex& ) ) );
	connect( m_pTree, SIGNAL( doubleClicked( const QModelIndex&) ), SLOT( doubleClicked( const QModelIndex& ) ) );
	connect( pathLineEdit, SIGNAL( returnPressed() ), SLOT( updateModelIndex() ) );	
}



AudioFileBrowser::~AudioFileBrowser()
{
	Sample *pNewSample = Sample::load( m_sEmptySampleFilename );
	AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, 100 );
	INFOLOG ( "DESTROY" );
}


bool AudioFileBrowser::isFileSupported( QString filename )
{
	if 	(
		( filename.endsWith( ".ogg" ) ) ||
		( filename.endsWith( ".OGG" ) ) ||
		( filename.endsWith( ".wav" ) ) ||
		( filename.endsWith( ".WAV" ) ) ||
		( filename.endsWith( ".au" ) ) ||
		( filename.endsWith( ".AU" ) ) ||
		( filename.endsWith( ".aiff" ) ) ||
		( filename.endsWith( ".AIFF" ) ) ||
		( filename.endsWith( ".flac" ) ) ||
		( filename.endsWith( ".FLAC" ) )
		){
			return true;
		} else {
			return false;
		}
}


void AudioFileBrowser::getEnvironment()
{
	QString desktopSession  = getenv("DESKTOP_SESSION");
//kde
	if( desktopSession == "kde" ) {
		QFile envfile( QDir::homePath() + "/.kde/share/config/kdeglobals");
	
		if ( !envfile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
			return;
		}
		
		QTextStream envin( &envfile );
		while ( !envin.atEnd() ) {
			QString envLine = envin.readLine();
			if( envLine == QString("SingleClick=true" ) ) {
				m_SingleClick = true;
				break;
			}		
		}
	}

//for gnome, xfce and all others we use double click as default
}



void AudioFileBrowser::keyPressEvent (QKeyEvent *ev)
{
	if( ev->modifiers()==Qt::ControlModifier && m_bAllowMultiSelect) {
		m_pTree->setSelectionMode( QAbstractItemView::MultiSelection );
		openBTN->setEnabled( true );
	}	
}



void AudioFileBrowser::keyReleaseEvent (QKeyEvent *ev)
{
	m_pTree->setSelectionMode( QAbstractItemView::SingleSelection );
}



void AudioFileBrowser::updateModelIndex()
{
	QString toRemove;
	QString newPath = pathLineEdit->text();

	if( QDir( newPath ).exists() ) {
		m_pTree->setRootIndex( m_pDirModel->index( newPath ) );
	} else {
		toRemove = newPath.section( '/', -1 );
//		QMessageBox::information ( this, "Hydrogen", newpath + toremove);
		newPath.replace( toRemove, "" );
		m_pTree->setRootIndex( m_pDirModel->index( newPath ) );
	}
}



void AudioFileBrowser::clicked( const QModelIndex& index )
{
	QString path = m_pDirModel->filePath( index );

	if( m_SingleClick ) {
		browseTree( index );
	}

	if( isFileSupported( path ) ) {
		browseTree( index );
	}
}



void AudioFileBrowser::doubleClicked( const QModelIndex& index )
{
	if(!m_SingleClick) {
		browseTree( index );
	}
}



void AudioFileBrowser::browseTree( const QModelIndex& index )
{

	QString path = m_pDirModel->filePath( index );
	pathLineEdit->setText( path );
	m_pSampleWaveDisplay->updateDisplay( m_sEmptySampleFilename );

	updateModelIndex(); //with this you have a navigation like konqueror

	if ( m_pDirModel->isDir( index ) ){
		m_pPlayBtn->setEnabled( false );
		openBTN->setEnabled( false );
		return;
	}

	QString name = path.section( '/', -1 );
	
	QString path2 = path;
	QString onlyPath = path;
	if ( name != "" ){
		onlyPath = path.replace( name, "" );
	}
		
	name = name.left( '.' );

	QString message = "Name: " + name;
	pathLineEdit->setText( onlyPath );

	QStringList path2List = path2.split("/");
	QString fleTxt = path2List.last();

	QApplication::setOverrideCursor(Qt::WaitCursor);

	if( isFileSupported( path2 ) )
	{

		filelineedit->setText( fleTxt );
		Sample *pNewSample = Sample::load( path2 );

		if ( pNewSample ) {
			m_pNBytesLable->setText( tr( "Size: %1 bytes" ).arg( pNewSample->get_size() / 2 ) );
			m_pSamplerateLable->setText( tr( "Samplerate: %1" ).arg( pNewSample->get_sample_rate() ) );
			float sec = ( float )( pNewSample->get_frames() / (float)pNewSample->get_sample_rate() );
			QString qsec;
			qsec = QString::asprintf( "%2.2f", sec );
			m_pLengthLable->setText( tr( "Sample length: " ) + qsec + tr( " s" ) );

			delete pNewSample;
			m_pSampleFilename = path2;

			m_pSampleWaveDisplay->updateDisplay( path2 );
			m_pPlayBtn->setEnabled( true );
			openBTN->setEnabled( true );

			//important this will only working correct if m_pSampleWaveDisplay->updateDisplay( file )
			//is ready with painting the wav file. else the playing sample get crackled sound!!
			if (playSamplescheckBox->isChecked()){
				if ( sec <= 600.00){
					on_m_pPlayBtn_clicked();
				}else
				{
					QMessageBox::information ( this, "Hydrogen", tr( "Please do not preview samples which are longer than 10 minutes!" )  );
				}
			}
			m_pNameLabel->setText( message );
		} else {
			openBTN->setEnabled( false );
			QMessageBox::information ( this, "Hydrogen", tr( "Unable to load that sample file." )  );
		}

	}else{
		m_pNameLabel->setText( tr( "Name:"));
		m_pNBytesLable->setText( tr( "Size:" ) );
		m_pSamplerateLable->setText( tr( "Samplerate:" ) );
		m_pLengthLable->setText( tr( "Sample length:" ) );
		m_pSampleWaveDisplay->updateDisplay( m_sEmptySampleFilename );
		m_pPlayBtn->setEnabled( false );
		m_pStopBtn->setEnabled( false );
		openBTN->setEnabled( false );
		m_pSampleFilename = "";
	}
	QApplication::restoreOverrideCursor();
}



void AudioFileBrowser::on_m_pPlayBtn_clicked()
{

	if( QFile( m_pSampleFilename ).exists() == false ) {
		return;
	}
	
	m_pStopBtn->setEnabled( true );
	
	Sample *pNewSample = Sample::load( m_pSampleFilename );
	if ( pNewSample ) {
		assert(pNewSample->get_sample_rate() != 0);
		
		int length = ( ( pNewSample->get_frames() / pNewSample->get_sample_rate() + 1) * 100 );
		AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, length );
	}
}



void AudioFileBrowser::on_m_pStopBtn_clicked()
{
	Sample *pNewSample = Sample::load( m_sEmptySampleFilename );
	AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, 100 );
	m_pStopBtn->setEnabled( false );
}



void AudioFileBrowser::on_cancelBTN_clicked()
{
	Preferences::get_instance()->__lastsampleDirectory = pathLineEdit->text();
	m_pSelectedFile << "false" << "false" << "";
	reject();
}



void AudioFileBrowser::on_openBTN_clicked()
{
	if( m_pTree->selectionModel()->selectedRows().size() > 0) {
		QList<QModelIndex>::iterator i;
		QList<QModelIndex> list = m_pTree->selectionModel()->selectedRows();

		for (i = list.begin(); i != list.end(); ++i) {
			QString path2 = (*i).data().toString();
			if( isFileSupported( path2 ) ){
				QString path = pathLineEdit->text();
				
				if(! path.endsWith("/")) {
					path = path + "/";
				}
				
				QString act_filename = path + path2;
				m_pSelectedFile << act_filename ;
			}
		}
	}

	Preferences::get_instance()->__lastsampleDirectory = pathLineEdit->text();
	accept();
}



void AudioFileBrowser::on_playSamplescheckBox_clicked()
{
	Preferences::get_instance()->__playsamplesonclicking = playSamplescheckBox->isChecked();
}



QStringList AudioFileBrowser::getSelectedFiles()
{
	if ( useNameCheckBox->isChecked() ) {
		m_pSelectedFile[0] = "true";
	}
	if ( autoVelCheckBox->isChecked() ) {
		m_pSelectedFile[1] = "true";
	}
	return m_pSelectedFile;
}



void AudioFileBrowser::on_m_pPathHometoolButton_clicked()
{

	QString path = pathLineEdit->text();
	QStringList pathlist = path.split("/");

	while( path != QDir::rootPath() ){

		if( pathlist.isEmpty () ) {
			break;
		}
		
		pathlist.removeLast();
		QString updir = pathlist.join("/");

		pathLineEdit->setText( updir );
		m_pTree->setRootIndex( m_pDirModel->index( updir ) );
		m_pTree->collapse( m_pDirModel->index( updir  ) );
		m_pTree->setExpanded( m_pDirModel->index(updir), false  );
		path = pathLineEdit->text();
	}

	pathLineEdit->setText( QDir::homePath() );
	m_pTree->setRootIndex( m_pDirModel->index( QDir::homePath() ) );

	m_pTree->collapse( m_pDirModel->index( QDir::homePath())  );
}



void AudioFileBrowser::on_m_pPathUptoolButton_clicked()
{
	QString path = pathLineEdit->text();
	QStringList pathlist = path.split("/");

	if( pathlist.isEmpty () ) {
		return;
	}

	if( path.endsWith( "/" ) ) {
		pathlist.removeLast();
		QString tmpupdir = pathlist.join("/");
		m_pTree->setRootIndex( m_pDirModel->index( tmpupdir ) );
		m_pTree->collapse( m_pDirModel->index( tmpupdir  ) );
		m_pTree->setExpanded( m_pDirModel->index( tmpupdir ), false  );
	}

	pathlist.removeLast();

	QString updir = pathlist.join("/");
	if ( updir == "" ) {
		pathLineEdit->setText( QString("/") );
	} else {
		pathLineEdit->setText( updir );
	}

	m_pTree->setRootIndex( m_pDirModel->index( updir ) );
	m_pTree->collapse( m_pDirModel->index( updir  ) );
	m_pTree->setExpanded( m_pDirModel->index(updir), false  );
}



void AudioFileBrowser::on_hiddenCB_clicked()
{
	if ( hiddenCB->isChecked() ) {
		m_pDirModel->setFilter( QDir::AllDirs | QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden );
	} else {
		m_pDirModel->setFilter( QDir::AllDirs | QDir::AllEntries | QDir::NoDotAndDotDot );
		m_pTree->setRootIndex( m_pDirModel->index( pathLineEdit->text() ) );
	}
}
