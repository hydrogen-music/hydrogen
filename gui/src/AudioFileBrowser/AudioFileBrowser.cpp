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
#include "../widgets/Button.h"

#include <hydrogen/data_path.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/sample.h>
#include <hydrogen/audio_engine.h>

#include <QModelIndex>
#include <QTreeWidget>
#include <QMessageBox>

using namespace H2Core;
using namespace std;

AudioFileBrowser::AudioFileBrowser ( QWidget* pParent )
		: QDialog ( pParent )
		, Object ( "AudioFileBrowser" )
{

	setupUi ( this );
	INFOLOG ( "INIT" );
	setWindowTitle ( trUtf8 ( "Audio File Browser" ) );
	setFixedSize ( width(), height() );
	installEventFilter( this );


        model = new QDirModel();
	model->setFilter( QDir::AllDirs | QDir::AllEntries | QDir::NoDotAndDotDot );
	model->setNameFilters( QStringList() << "*.wav" << "*.WAV" << "*.flac"<< "*.FLAC" << "*.aiff" << "*.AIFF"<< "*.au" << "*.AU" );
	model->setSorting( QDir::DirsFirst |QDir::Name );
	__index = model->index( QDir::currentPath() );
	
	m_pPlayBtn->setEnabled( false );
	m_pStopBtn->setEnabled( false );
	openBTN->setEnabled( false );

        tree = new QTreeView( treeView );
        tree->setModel( model );
	tree->resize( 799, 310 );
	tree->header()->resizeSection( 0, 405 );
	tree->setAlternatingRowColors( true );
	tree->setRootIndex( model->index( Preferences::getInstance()->__lastsampleDirectory ) );
	

	pathLineEdit->setText( Preferences::getInstance()->__lastsampleDirectory );
	m_psamplefilename = "";	
	m_pselectedFile << "false" << "false";

	sEmptySampleFilename = DataPath::get_data_path() + "/emptySample.wav";

	m_pSampleWaveDisplay = new SampleWaveDisplay( waveformview );
	m_pSampleWaveDisplay->updateDisplay( sEmptySampleFilename );
	m_pSampleWaveDisplay->move( 3, 3 );

	playSamplescheckBox->setChecked( Preferences::getInstance()->__playsamplesonclicking );

	connect( tree, SIGNAL( clicked( const QModelIndex&) ), SLOT( clicked( const QModelIndex& ) ) );
	connect( pathLineEdit, SIGNAL( returnPressed() ), SLOT( updateModelIndex() ) );
	
}


AudioFileBrowser::~AudioFileBrowser()
{
	Sample *pNewSample = Sample::load( sEmptySampleFilename );
	AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, 100 );
	INFOLOG ( "DESTROY" );
}

void AudioFileBrowser::keyPressEvent (QKeyEvent *ev){
	
	if( ev->modifiers()==Qt::ControlModifier ){
		tree->setSelectionMode( QAbstractItemView::MultiSelection );
		openBTN->setEnabled( true );
	}
	
}

void AudioFileBrowser::keyReleaseEvent (QKeyEvent *ev){

	tree->setSelectionMode( QAbstractItemView::SingleSelection );

}

void AudioFileBrowser::updateModelIndex()
{
	QString toremove = "";
	QString newpath = pathLineEdit->text();

	if( QDir( newpath ).exists() ){
		tree->setRootIndex( model->index( newpath ) );
	}else
	{
		toremove = newpath.section( '/', -1 );
//		QMessageBox::information ( this, "Hydrogen", newpath + toremove);
		newpath.replace( toremove, "" );
		tree->setRootIndex( model->index( newpath ) );
	}

}

void AudioFileBrowser::clicked( const QModelIndex& index )
{

	QString path = model->filePath( index );
	pathLineEdit->setText( path );
	filelineedit->setText( path );
	m_pSampleWaveDisplay->updateDisplay( sEmptySampleFilename );

	updateModelIndex(); //with this you have a navigation like konqueror 

	if ( model->isDir( index ) ){
		m_pPlayBtn->setEnabled( false );
		return;
	}

	QString name = path.section( '/', -1 ); 
	
	QString path2 = path;
	QString onlypath = path;
	if ( name != "" ){
		onlypath = path.replace( name, "" );
	}
		
	name = name.left( '.' );

	QString message = "Name: " + name;
	filelineedit->setText( path2 );
	pathLineEdit->setText( onlypath );

	


	if 	(
		( path2.endsWith( ".wav" ) ) ||
		( path2.endsWith( ".WAV" ) ) ||
		( path2.endsWith( ".au" ) ) ||
		( path2.endsWith( ".AU" ) ) ||
		( path2.endsWith( ".aiff" ) ) ||
		( path2.endsWith( ".AIFF" ) ) ||
		( path2.endsWith( ".flac" ) ) ||
		( path2.endsWith( ".FLAC" ) )
		) {


			Sample *pNewSample = Sample::load( path2 );
			if ( pNewSample ) {
				m_pNBytesLable->setText( trUtf8( "Size: %1 bytes" ).arg( pNewSample->get_size() / 2 ) );
				m_pSamplerateLable->setText( trUtf8( "Samplerate: %1" ).arg( pNewSample->get_sample_rate() ) );
				float sec = ( float )( pNewSample->get_n_frames() / (float)pNewSample->get_sample_rate() );
				QString qsec = "";
				qsec.sprintf( "%2.2f", sec );
				m_pLengthLable->setText( trUtf8( "Samplelength: " ) + qsec + trUtf8( " s" ) );
				
				delete pNewSample;
				m_psamplefilename = path2;

				
				if (playSamplescheckBox->isChecked()){
					if ( sec <= 600.00){
						on_m_pPlayBtn_clicked();
					}else
					{
						QMessageBox::information ( this, "Hydrogen", trUtf8( "No clicking audio preview for samples longer than 10 minutes!" )  );
					}
				}

				m_pSampleWaveDisplay->updateDisplay( path2 );
				m_pPlayBtn->setEnabled( true );
				openBTN->setEnabled( true );

			}
		
			m_pNameLabel->setText( message );
		}else{
			m_pNameLabel->setText( trUtf8( "Name:"));
			m_pNBytesLable->setText( trUtf8( "Size:" ) );
			m_pSamplerateLable->setText( trUtf8( "Samplerate:" ) );
			m_pLengthLable->setText( trUtf8( "Samplelength:" ) );
			m_pSampleWaveDisplay->updateDisplay( sEmptySampleFilename );
			m_pPlayBtn->setEnabled( false );
			m_pStopBtn->setEnabled( false );
			openBTN->setEnabled( false );
			m_psamplefilename = "";
		}
}



void AudioFileBrowser::on_m_pPlayBtn_clicked()
{

	if( QFile( m_psamplefilename ).exists() == false )
		return;
	m_pStopBtn->setEnabled( true );
	Sample *pNewSample = Sample::load( m_psamplefilename );
	if ( pNewSample ){
		int length = ( ( pNewSample->get_n_frames() / pNewSample->get_sample_rate() + 1) * 100 );
		AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, length );
	}
}


void AudioFileBrowser::on_m_pStopBtn_clicked()
{
	Sample *pNewSample = Sample::load( sEmptySampleFilename );
	AudioEngine::get_instance()->get_sampler()->preview_sample( pNewSample, 100 );
	m_pStopBtn->setEnabled( false );
}



void AudioFileBrowser::on_cancelBTN_clicked()
{
	Preferences::getInstance()->__lastsampleDirectory = pathLineEdit->text();
	m_pselectedFile << "false" << "false" << "";
	reject();

}



void AudioFileBrowser::on_openBTN_clicked()
{
	if( tree->selectionModel()->selectedIndexes().size() / 4 > 0){

		QList<QModelIndex>::iterator i;
		QList<QModelIndex> list = tree->selectionModel()->selectedIndexes();	


    		for (i = list.begin(); i != list.end(); ++i){
			QString path2 = (*i).data().toString();
			if 	(
			( path2.endsWith( ".wav" ) ) ||
			( path2.endsWith( ".WAV" ) ) ||
			( path2.endsWith( ".au" ) ) ||
			( path2.endsWith( ".AU" ) ) ||
			( path2.endsWith( ".aiff" ) ) ||
			( path2.endsWith( ".AIFF" ) ) ||
			( path2.endsWith( ".flac" ) ) ||
			( path2.endsWith( ".FLAC" ) )
			) {
				QString path = pathLineEdit->text();
				QString act_filename = path + path2;
				m_pselectedFile << act_filename ;
		
			}
			++i;++i;++i;
		}
	}

	/*
	if 	(
		( ( filelineedit->text().endsWith( ".wav" ) ) ||
		( filelineedit->text().endsWith( ".WAV" ) ) ||
		( filelineedit->text().endsWith( ".au" ) ) ||
		( filelineedit->text().endsWith( ".AU" ) ) ||
		( filelineedit->text().endsWith( ".aiff" ) ) ||
		( filelineedit->text().endsWith( ".AIFF" ) ) ||
		( filelineedit->text().endsWith( ".flac" ) ) ||
		( filelineedit->text().endsWith( ".FLAC" ) ) ) &&
		( QFile( filelineedit->text() ).exists() == true )
		) {		
			m_pselectedFile[2] = filelineedit->text();
			
		}else
		{
			m_pselectedFile << "false" << "false" << "";
		}
	*/
	Preferences::getInstance()->__lastsampleDirectory = pathLineEdit->text();
	accept();
}



void AudioFileBrowser::on_playSamplescheckBox_clicked()
{
	Preferences::getInstance()->__playsamplesonclicking = playSamplescheckBox->isChecked();
}



QStringList AudioFileBrowser::selectedFile()
{
	if ( useNameCheckBox->isChecked() ){
		 m_pselectedFile[0] = "true";
	}
	if ( autoVelCheckBox->isChecked() ){
		 m_pselectedFile[1] = "true";
	}
	return m_pselectedFile;
}



void AudioFileBrowser::on_m_pPathHometoolButton_clicked()
{
	pathLineEdit->setText( QDir::homePath() );
	tree->setRootIndex( model->index( QDir::homePath() ) );
}



void AudioFileBrowser::on_m_pPathUptoolButton_clicked()
{
	QString toremove = "";
	QString path = pathLineEdit->text();

	if ( path.length() <=1 ) 
		return;

	if ( path.endsWith( '/') ) {
		toremove = path.section( '/', -2 );
	}else
	{
		toremove = path.section( '/', -1 );
	}
	
	QString updir =  path.replace( toremove, "" );
	pathLineEdit->setText( updir );
	tree->setRootIndex( model->index( updir ) );
	tree->collapse( model->index( updir  ) );
}



void AudioFileBrowser::on_hiddenCB_clicked()
{
	if ( hiddenCB->isChecked() ){
	 	model->setFilter( QDir::AllDirs | QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden );
	}else
	{
	 	model->setFilter( QDir::AllDirs | QDir::AllEntries | QDir::NoDotAndDotDot );
		tree->setRootIndex( model->index( pathLineEdit->text() ) );
	}
}
