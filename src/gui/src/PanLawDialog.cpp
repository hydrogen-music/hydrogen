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

#include <cstring>

#include "Skin.h"
#include "PanLawDialog.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include "qmessagebox.h"
#include "qstylefactory.h"

#include <QPixmap>
#include <QFontDialog>

#include <core/MidiMap.h>
#include <core/Hydrogen.h>
#include <core/Preferences.h>
#include <core/Lash/LashClient.h>
#include <core/AudioEngine.h>
#include <core/Helpers/Translations.h>
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"


using namespace H2Core;

const char* PanLawDialog::__class_name = "PanLawDialog";

PanLawDialog::PanLawDialog(QWidget* parent)
 : QDialog( parent )
 , Object( __class_name )
{
	setupUi( this );

	setWindowTitle( tr( "Pan Law Settings" ) );

	setMinimumSize( width(), height() );

	Sampler* pSampler = AudioEngine::get_instance()->get_sampler();
	
	// insert the items here. They work consistently no matter of the order in this menu
	panLawComboBox->addItem( tr("Balance Law (0dB) - linear pan parameter"),	
													QVariant( pSampler->LINEAR_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB) - linear pan parameter"),
													QVariant( pSampler->LINEAR_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB) - linear pan parameter"),
													QVariant( pSampler->LINEAR_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation) - linear parameter"),
													QVariant( pSampler->LINEAR_CONST_K_NORM ) );
	panLawComboBox->insertSeparator(100);
	panLawComboBox->addItem( tr("Balance Law (0dB) - polar pan parameter"),
													QVariant( pSampler->POLAR_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB) - polar pan parameter"),
													QVariant( pSampler->POLAR_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB) - polar pan parameter"),
													QVariant( pSampler->POLAR_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation) - polar parameter"),
													QVariant( pSampler->POLAR_CONST_K_NORM ) );
	panLawComboBox->insertSeparator(100);
	panLawComboBox->addItem( tr("Balance Law (0dB) - ratio pan parameter"),
													QVariant( pSampler->RATIO_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB) - ratio pan parameter"),
													QVariant( pSampler->RATIO_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB) - ratio pan parameter"),
													QVariant( pSampler->RATIO_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation) - ratio parameter"),
													QVariant( pSampler->RATIO_CONST_K_NORM ) );
	panLawComboBox->insertSeparator(100);
	panLawComboBox->addItem( tr("Balance Law (0dB) - quadratic pan parameter"),
													QVariant( pSampler->QUADRATIC_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB) - quadratic pan parameter"),
													QVariant( pSampler->QUADRATIC_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB) - quadratic pan parameter"),
													QVariant( pSampler->QUADRATIC_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation) - quadratic parameter"),
													QVariant( pSampler->QUADRATIC_CONST_K_NORM ) );

	panLawComboBox->setCurrentIndex( panLawComboBox->findData( pSampler->getPanLawType() ) );
	panLawChanged(); // to hide dB SPL compensation
	connect(panLawComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT( panLawChanged() ));
	
	/** dB SPL Center Compensation, audio engineers friendly.
	 * (will be converted to the corresponding k assuming L^k + R^k = 1 )
	 */
	QValidator *validator = new QDoubleValidator( -10000., 0., 20, this );
	dBCompensationLineEdit->setValidator( validator );
	dBCompensationLineEdit->setText( QString( "%1" ).arg( -6.0206 / pSampler->getPanLawKNorm() ) );
	
}




PanLawDialog::~PanLawDialog()
{
	INFOLOG("~PAN_LAW_DIALOG");
}



void PanLawDialog::on_cancelBtn_clicked()
{
	reject();
}


void PanLawDialog::on_okBtn_clicked()
{	
	Sampler* pSampler = AudioEngine::get_instance()->get_sampler();
	bool bOk;

	pSampler->setPanLawType( ( panLawComboBox->currentData() ).toInt( &bOk ) );
	
	
	// allowing both point or comma decimal separator
	float fdBCenterCompensation = ( dBCompensationLineEdit->text() ).replace( ",", "." ).toFloat( &bOk );
	if ( !bOk ) { // this should not happen
		QMessageBox::information( this, "Hydrogen", tr( "dB Center Compensation rejected" ) );
		return;
	} else if ( fdBCenterCompensation > -0.01 ) {
	   /** reject small absolute values since computer approximation (k tends rapidly to infinity at 0)
		* and obviously reject positive values to not boost the center (compensation has the opposite aim).
		*/
		QMessageBox::information( this, "Hydrogen", tr( "dB Center Compensation must be less than -0.01" ) );
		return;
	}
	/** convert the dB Compensation to the corresponding exponent k: assuming constraint L^k + R^k = 1
	* For example -6.0206 dB <=> k = 1 <=> L + R = 1 (i.e. constant sum)
	*/
	pSampler->setPanLawKNorm( - 6.0206 / fdBCenterCompensation );

	accept();
}

void PanLawDialog::panLawChanged(){
	bool bOk;
	int nPanLawType = ( panLawComboBox->currentData() ).toInt( &bOk);
	Sampler* pSampler = AudioEngine::get_instance()->get_sampler();
	if (   nPanLawType == pSampler->LINEAR_CONST_K_NORM
		|| nPanLawType == pSampler->POLAR_CONST_K_NORM
		|| nPanLawType == pSampler->RATIO_CONST_K_NORM
		|| nPanLawType == pSampler->QUADRATIC_CONST_K_NORM
	   )
	{
		dBCompensationLineEdit->show();
		dBCompensationLbl->show();
	} else {
		dBCompensationLineEdit->hide();
		dBCompensationLbl->hide();
	}
}
