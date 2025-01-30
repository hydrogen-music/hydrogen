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

#include <cstring>

#include "MixerSettingsDialog.h"
#include "HydrogenApp.h"
#include "MainForm.h"

#include "qmessagebox.h"
#include "qstylefactory.h"

#include <QPixmap>
#include <QFontDialog>

#include <core/MidiMap.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Lash/LashClient.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Translations.h>
#include "SongEditor/SongEditor.h"
#include "SongEditor/SongEditorPanel.h"


using namespace H2Core;

MixerSettingsDialog::MixerSettingsDialog(QWidget* parent)
 : QDialog( parent )
 {
	setupUi( this );

	setWindowTitle( tr( "Mixer Settings" ) );

	setMinimumSize( width(), height() );

	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	
	/* insert the items here so they work consistently no matter of their order in the menu (except the headings)
	 */

	// heading
	panLawComboBox->addItem( tr("------ Linear pan parameter ------"), QVariant( -10000 ) );
	qobject_cast< QStandardItemModel * >( panLawComboBox->model() )->item( 0 )->setEnabled( false );
	panLawComboBox->addItem( tr("Balance Law (0dB)"), QVariant( Sampler::LINEAR_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB)"), QVariant( Sampler::LINEAR_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB)"), QVariant( Sampler::LINEAR_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation)"),
																		QVariant( Sampler::LINEAR_CONST_K_NORM ) );
	panLawComboBox->insertSeparator(100);
	
	// heading	
	panLawComboBox->addItem( tr("------ Polar pan parameter ------"), QVariant( -10000 ) );
	qobject_cast< QStandardItemModel * >( panLawComboBox->model() )->item( 6 )->setEnabled( false );
	panLawComboBox->addItem( tr("Balance Law (0dB)"), QVariant( Sampler::POLAR_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB)"), QVariant( Sampler::POLAR_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB)"), QVariant( Sampler::POLAR_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation)"),
																		QVariant( Sampler::POLAR_CONST_K_NORM ) );
	panLawComboBox->insertSeparator(100);
	
	// heading
	panLawComboBox->addItem( tr("------ Ratio pan parameter ------"), QVariant( -10000 ) );
	qobject_cast< QStandardItemModel * >( panLawComboBox->model() )->item( 12 )->setEnabled( false );
	panLawComboBox->addItem( tr("Balance Law (0dB)"), QVariant( Sampler::RATIO_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB)"), QVariant( Sampler::RATIO_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB)"), QVariant( Sampler::RATIO_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation)"),
																		QVariant( Sampler::RATIO_CONST_K_NORM ) );
	panLawComboBox->insertSeparator(100);
	
	// heading
	panLawComboBox->addItem( tr("------ Quadratic pan parameter ------"), QVariant( -10000 ) );
	qobject_cast< QStandardItemModel * >( panLawComboBox->model() )->item( 18 )->setEnabled( false );
	panLawComboBox->addItem( tr("Balance Law (0dB)"), QVariant( Sampler::QUADRATIC_STRAIGHT_POLYGONAL ) );
	panLawComboBox->addItem( tr("Constant Power (-3dB)"), QVariant( Sampler::QUADRATIC_CONST_POWER ) );
	panLawComboBox->addItem( tr("Constant Sum (-6dB)"), QVariant( Sampler::QUADRATIC_CONST_SUM ) );
	panLawComboBox->addItem( tr("Constant k-Norm (Custom dB compensation)"),
																		QVariant( Sampler::QUADRATIC_CONST_K_NORM ) );

	panLawComboBox->setCurrentIndex( panLawComboBox->findData( pSong->getPanLawType() ) );
	panLawChanged(); // to hide custom dB SPL compensation
	panLawComboBox->setToolTip( tr("Relationship between the sound's apparent image position and the pan knob control"
								 																					) );

	connect(panLawComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT( panLawChanged() ));
	
	/** dB SPL Center Compensation, audio engineers friendly.
	 * (will be converted to the corresponding k assuming L^k + R^k = 1 )
	 */
	QValidator *validator = new QDoubleValidator( -10000., 0., 20, this );
	dBCompensationLineEdit->setValidator( validator );
	dBCompensationLineEdit->setText( QString( "%1" ).arg( -6.0206 / pSong->getPanLawKNorm() ) );

	adjustSize();
	setFixedSize( width(), height() );
}




MixerSettingsDialog::~MixerSettingsDialog()
{
	INFOLOG("~MIXER_SETTINGS_DIALOG");
}



void MixerSettingsDialog::on_cancelBtn_clicked()
{
	reject();
}


void MixerSettingsDialog::on_okBtn_clicked() {
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	bool bOk;
	
	// Pan Law settings
	pSong->setPanLawType( ( panLawComboBox->currentData() ).toInt( &bOk ) );
	
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
	pSong->setPanLawKNorm( - 6.0206 / fdBCenterCompensation );

	Hydrogen::get_instance()->setIsModified( true );

	accept();
}

void MixerSettingsDialog::panLawChanged(){ // hide/show some widgets
	bool bOk;
	int nPanLawType = ( panLawComboBox->currentData() ).toInt( &bOk);
	if (   nPanLawType == Sampler::LINEAR_CONST_K_NORM
		|| nPanLawType == Sampler::POLAR_CONST_K_NORM
		|| nPanLawType == Sampler::RATIO_CONST_K_NORM
		|| nPanLawType == Sampler::QUADRATIC_CONST_K_NORM
	   )
	{
		dBCompensationLineEdit->show();
		dBCompensationLbl->show();
	} else {
		dBCompensationLineEdit->hide();
		dBCompensationLbl->hide();
	}
}
