#include "SomaticReportDialog.h"
#include "NGSD.h"
#include "SomaticReportHelper.h"
#include "GlobalServiceProvider.h"
#include "Statistics.h"
#include "MainWindow.h"
#include "ClientHelper.h"
#include "IgvSessionManager.h"
#include <QMessageBox>
#include <QBuffer>

//struct holding reference data for tumor mutation burden (DOI:10.1186/s13073-017-0424-2)
struct tmbInfo
{
	QByteArray hpoterm;
	int cohort_count;
	double tmb_median;
	double tmb_max;
	QByteArray tumor_entity;

	static QList<tmbInfo> load(const QByteArray& file_name)
	{
		TSVFileStream in(file_name);

		int i_hpoterms = in.colIndex("HPO_TERMS",true);
		int i_count = in.colIndex("COUNT",true);
		int i_tmb_median = in.colIndex("TMB_MEDIAN",true);
		int i_tmb_max = in.colIndex("TMB_MAXIMUM",true);
		int i_tumor_entity = in.colIndex("TUMOR_ENTITY",true);

		QList<tmbInfo> out;
		while(!in.atEnd())
		{
			QByteArrayList current_line = in.readLine();
			tmbInfo tmp;
			tmp.hpoterm = current_line.at(i_hpoterms);
			tmp.cohort_count = current_line.at(i_count).toInt();
			tmp.tmb_median = current_line.at(i_tmb_median).toDouble();
			tmp.tmb_max = current_line.at(i_tmb_max).toDouble();
			tmp.tumor_entity = current_line.at(i_tumor_entity);
			out << tmp;
		}
		return out;
	}
	bool operator==(const tmbInfo& rhs) const
	{
		return hpoterm == rhs.hpoterm;
	}
};

SomaticReportDialog::SomaticReportDialog(QString project_filename, SomaticReportSettings &settings, const CnvList& cnvs, const VariantList& germl_variants, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, settings_(settings)
	, germl_variants_(germl_variants)
	, target_region_(settings.report_config->targetRegionName())
	, tum_cont_snps_(std::numeric_limits<double>::quiet_NaN())
	, tum_cont_max_clonality_(std::numeric_limits<double>::quiet_NaN())
	, tum_cont_histological_(std::numeric_limits<double>::quiet_NaN())
	, limitations_()
	, project_filename_(project_filename)
{

	cnvs_ = SomaticReportSettings::filterCnvs(cnvs, settings_);


	ui_.setupUi(this);

	connect(ui_.report_type_rna, SIGNAL(clicked(bool)), this, SLOT(disableGUI()));
	connect(ui_.report_type_cfdna, SIGNAL(clicked(bool)), this, SLOT(disableGUI()));
	connect(ui_.report_type_dna, SIGNAL(clicked(bool)), this, SLOT(enableGUI()));

	connect(ui_.report_type_rna, SIGNAL(clicked(bool)), this, SLOT(rnaSampleSelection()));
	connect(ui_.report_type_cfdna, SIGNAL(clicked(bool)), this, SLOT(rnaSampleSelection()));
	connect(ui_.report_type_dna, SIGNAL(clicked(bool)), this, SLOT(rnaSampleSelection()));

	connect(ui_.include_cnv_burden, SIGNAL(stateChanged(int)), this, SLOT(cinState()));
	connect(ui_.limitations_check, SIGNAL(stateChanged(int)), this, SLOT(limitationState()));
	connect(ui_.quality_no_abnormalities, SIGNAL(stateChanged(int)), this, SLOT(qualityState()));

	connect( ui_.label_hint_igv_screenshot_available, SIGNAL(linkActivated(QString)), this, SLOT(createIgvScreenshot()) );


	//Resolve tumor content estimate from NGSD
	QCCollection res = db_.getQCData(db_.processedSampleId(settings.tumor_ps, true));
	try
	{
		tum_cont_snps_ = res.value("QC:2000054", true).asDouble();
	}
	catch(ArgumentException){} //nothing to do
	catch(TypeConversionException){} //nothing to do

	//Resolve histological tumor content (if available in NGSD)
	QList<SampleDiseaseInfo> disease_infos = db_.getSampleDiseaseInfo(db_.sampleId(settings.tumor_ps), "tumor fraction");
	foreach(const auto& entry, disease_infos)
	{
		if(entry.type == "tumor fraction")
		{
			try
			{
				tum_cont_histological_ = Helper::toDouble(entry.disease_info);
			}
			catch(ArgumentException) {} //Nothing to do here
			break;
		}
	}

	tum_cont_max_clonality_ = SomaticReportHelper::getCnvMaxTumorClonality(cnvs_);

	//Load HPO terms from database
	QStringList hpos_ngsd;
	QList<SampleDiseaseInfo> details = db_.getSampleDiseaseInfo(db_.sampleId(settings_.tumor_ps));
	foreach(const auto& info, details)
	{
		if(info.type == "HPO term id")
		{
			hpos_ngsd << info.disease_info;
		}
	}

	QList<tmbInfo> hpo_tmbs = tmbInfo::load("://Resources/hpoterms_tmb.tsv");

	//Set Reference value proposals
	foreach(const auto& hpo_tmb, hpo_tmbs)
	{
		if( !hpos_ngsd.contains(hpo_tmb.hpoterm) ) continue;

		QTableWidgetItem *disease = new QTableWidgetItem(QString(hpo_tmb.tumor_entity));
		QTableWidgetItem *tmb_text = new QTableWidgetItem("Median: " + QString::number(hpo_tmb.tmb_median,'f', 2).replace(".",",") + " Var/Mbp, Maximum: " + QString::number(hpo_tmb.tmb_max,'f',2).replace(".",",") + " Var/Mbp");

		ui_.tmb_reference->insertRow(ui_.tmb_reference->rowCount());
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 0, disease);
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 1, tmb_text);
		ui_.tmb_reference->item(ui_.tmb_reference->rowCount()-1, 0)->setFlags(ui_.tmb_reference->item(ui_.tmb_reference->rowCount()-1, 0)->flags() & ~Qt::ItemIsEditable);
	}

	//load former TMB ref entry (stored in somatic settings from NGSD)
	int old_entry_row = -1;
	for(int i=0; i<ui_.tmb_reference->rowCount(); ++i)
	{
		if(settings_.report_config->tmbReferenceText() == ui_.tmb_reference->item(i, 1)->text())
		{
			old_entry_row = i;
			break;
		}
	}

	if(old_entry_row == -1) //Insert ref text from former report if not  in table
	{
		ui_.tmb_reference->insertRow(ui_.tmb_reference->rowCount());
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 0, new QTableWidgetItem("former report"));
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 1, new QTableWidgetItem( settings_.report_config->tmbReferenceText()) );
		ui_.tmb_reference->selectRow(ui_.tmb_reference->rowCount()-1);
	}
	else //set selection to former entry if already in table
	{
		if(settings_.report_config->tmbReferenceText() != "") ui_.tmb_reference->selectRow(old_entry_row);
		else if(ui_.tmb_reference->rowCount() >= 3) ui_.tmb_reference->selectRow(2); //First entry that referes to a suggested ref value
	}

	ui_.tmb_reference->item(0,0)->setFlags(ui_.tmb_reference->item(0,0)->flags() & ~Qt::ItemIsEditable);//set "none" item non-editable
	ui_.tmb_reference->item(0,1)->setFlags(ui_.tmb_reference->item(0,1)->flags() & ~Qt::ItemIsEditable);//set "no value text" item non-editable
	ui_.tmb_reference->item(1,0)->setFlags(ui_.tmb_reference->item(1,0)->flags() & ~Qt::ItemIsEditable);
	ui_.tmb_reference->resizeRowsToContents();
	ui_.tmb_reference->setColumnWidth(1,500);
	ui_.tmb_reference->setRowCount(ui_.tmb_reference->rowCount());
	ui_.tmb_reference->setEditTriggers(QAbstractItemView::DoubleClicked);


	//load control tissue snps (NGSD class 4/5 only) into widget
	int i_co_sp = germl_variants.annotationIndexByName("coding_and_splicing", true, false);

	BamReader bam_reader(GlobalServiceProvider::database().processedSamplePath(db_.processedSampleId(settings_.tumor_ps), PathType::BAM).filename);
	FastaFileIndex fasta_idx(Settings::string("reference_genome"));

	QList<int> germl_indices_in_report = settings_.report_config->variantIndicesGermline();

	if(i_co_sp != -1)
	{
		QMap<QString, ClassificationInfo> class_map = db_.getAllClassifications();

		for(int i=0; i<germl_variants_.count(); ++i)
		{
			const Variant& snv = germl_variants_[i];

			QString key = snv.chr().strNormalized(true) + ":" + QString::number(snv.start()) + "-" + QString::number(snv.end()) + " " + snv.ref() + ">" + snv.obs();

			if (! class_map.contains(key) || (class_map[key].classification != "4" && class_map[key].classification != "5")) continue;

			//determine frequency of variant in tumor bam
			VariantDetails variant_details = bam_reader.getVariantDetails(fasta_idx, snv, false);
			double freq_in_tum = variant_details.frequency;
			double depth_in_tum = variant_details.depth;

			ui_.germline_variants->insertRow(ui_.germline_variants->rowCount());

			const int row = ui_.germline_variants->rowCount()-1;

			ui_.germline_variants->setVerticalHeaderItem( row, new QTableWidgetItem(QString::number(i)) ); //variant index

			//preselect variants contained in report configuration
			QTableWidgetItem *check = new QTableWidgetItem();
			if(germl_indices_in_report.contains(i)) check->setCheckState(Qt::Checked);
			else check->setCheckState(Qt::Unchecked);
			ui_.germline_variants->setItem( row, 0, check );


			ui_.germline_variants->setItem( row, 1, new QTableWidgetItem( QString(snv.chr().strNormalized(true))) );
			ui_.germline_variants->setItem( row, 2, new QTableWidgetItem( QString::number(snv.start())) );
			ui_.germline_variants->setItem( row, 3, new QTableWidgetItem( QString::number(snv.end())) );
			ui_.germline_variants->setItem( row, 4, new QTableWidgetItem( QString(snv.ref())) );
			ui_.germline_variants->setItem( row, 5, new QTableWidgetItem( QString(snv.obs())) );
			ui_.germline_variants->setItem( row, 6, new QTableWidgetItem( QString::number(freq_in_tum, 'f', 2) ) );
			ui_.germline_variants->setItem( row, 7, new QTableWidgetItem( QString::number(depth_in_tum) ) );

			ui_.germline_variants->setItem( row, 8, new QTableWidgetItem( class_map[key].classification ) );
			ui_.germline_variants->item( row, 8)->setBackground(Qt::red);

			ui_.germline_variants->setItem( row, 9, new QTableWidgetItem( QString(snv.annotations()[i_co_sp])) );
		}

		ui_.germline_variants->setRowCount(ui_.germline_variants->rowCount());
		ui_.germline_variants->resizeColumnsToContents();

		//disable editable
		for(int i=0; i<ui_.germline_variants->rowCount(); ++i)
		{
			for(int j = 0; j<ui_.germline_variants->columnCount(); ++j)
			{
				ui_.germline_variants->item(i,j)->setFlags(ui_.germline_variants->item(i,j)->flags() & ~Qt::ItemIsEditable);
			}
		}
		if(ui_.germline_variants->rowCount() == 0) ui_.tabs->setTabEnabled(1, false);
	}
	else
	{
		ui_.tabs->setTabEnabled(1, false);
	}

	//limitations
	if(settings.report_config->limitations() != "")
	{
		ui_.limitations_text->setPlainText(settings.report_config->limitations());
		ui_.limitations_check->setChecked(true);
	}
	else
	{
		QString text = "Es gibt Hinweise auf eine heterogene Probe. Die Auswertung der Kopienzahlveränderungen ist bei Tumorsubpopulationen mit niedrigem Anteil an Tumorzellen limitiert.\n\n";
		text += "Die durchgeführte Sequenzierung der Tumor-DNA zeigte zudem eine Vermischung mit einer in unserem Labor nicht bekannten Probe mit einem Anteil von unter 10 %. Eine Auswertung der somatischen Tumordiagnostik war daher nur für Varianten mit höherer Klonalität möglich.\n\n";
		text += "Aufgrund der reduzierten Qualität des Tumorblocks konnte nur eine geringen DNA-Menge gewonnen werden. Aufgrund der daraus resultierenden niedrigen Sequenziertiefe (durchschnittl. 75x) und der detektierten Heterogenität der verwendeten Tumorprobe war nur eine eingeschränkte Detektion somatischer Punktmutationen und Kopienzahlveränderungen, der Mutationslast und auch Mikrosatelliteninstabilität möglich.\n";
		ui_.limitations_text->setPlainText(text);
	}



	//Update GUI
	ui_.include_msi_status->setText(ui_.include_msi_status->text() + " (" + (!BasicStatistics::isValidFloat(settings.get_msi_value(db_)) ? "n/a" : QByteArray::number(settings.get_msi_value(db_),'f',2)) + ")");

	if(BasicStatistics::isValidFloat(tum_cont_snps_))
	{
		ui_.include_max_tum_freq->setChecked(settings_.report_config->includeTumContentByMaxSNV());
		ui_.include_max_tum_freq->setText(ui_.include_max_tum_freq->text() + " ("  + QString::number(tum_cont_snps_, 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_tum_freq->setEnabled(false);
	}

	if(BasicStatistics::isValidFloat( tum_cont_max_clonality_))
	{
		ui_.include_max_clonality->setChecked(settings_.report_config->includeTumContentByClonality());
		ui_.include_max_clonality->setText(ui_.include_max_clonality->text() + " ("  + QString::number(tum_cont_max_clonality_ * 100., 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_clonality->setCheckable(false);
		ui_.include_max_clonality->setEnabled(false);
	}

	if(BasicStatistics::isValidFloat( tum_cont_histological_) && tum_cont_histological_ > 0.)
	{
		ui_.include_tum_content_histological->setChecked(settings_.report_config->includeTumContentByHistological());
		ui_.include_tum_content_histological->setText(ui_.include_tum_content_histological->text() + " (" + QString::number(tum_cont_histological_, 'f', 1)+"%)");
	}
	else
	{
		ui_.include_tum_content_histological->setCheckable(false);
		ui_.include_tum_content_histological->setEnabled(false);
	}

	if(target_region_ != "")
	{
		ui_.target_bed_path->setText(target_region_);
	}
	else
	{
		ui_.target_bed_path->setText("not set");
	}

	if(SomaticReportHelper::cnvBurden(cnvs_) > 0.01)
	{
		ui_.include_cnv_burden->setChecked(settings_.report_config->cnvBurden());
		ui_.include_cnv_burden->setText(ui_.include_cnv_burden->text() + " (" + QString::number(SomaticReportHelper::cnvBurden(cnvs_), 'f', 1)  + "%)");
	}
	else
	{
		ui_.include_cnv_burden->setCheckable(false);
		ui_.include_cnv_burden->setEnabled(false);
	}

	ui_.include_mutation_burden->setChecked(settings_.report_config->includeMutationBurden());

	if(cnvs_.count() > 0)
	{
		ui_.cnv_loh_count->setText( QString::number(settings_.report_config->cnvLohCount()) );
		ui_.cnv_tai_count->setText( QString::number(settings_.report_config->cnvTaiCount()) );
		ui_.cnv_lst_count->setText( QString::number(settings_.report_config->cnvLstCount()) );
	}


	//Set CIN options, depends on check state of cnv burden
	cinState();

	//Preselect remaining options
	ui_.include_msi_status->setChecked(settings_.report_config->msiStatus());
	ui_.fusions_detected->setChecked(settings_.report_config->fusionsDetected());
	if(Settings::boolean("debug_mode_enabled")) ui_.no_ngsd->setChecked(true);

	//Load possible quality settings
	QStringList quality_entries = db_.getEnum("somatic_report_configuration", "quality");

	foreach(const auto& entry, quality_entries)
	{
		if (entry == "no abnormalities") continue;
		ui_.quality_list->addItem(entry);
	}

	//Set selected entry to old setting
	QListWidgetItem* item = 0;
	for(int i=0; i<ui_.quality_list->count();++i)
	{
		item = ui_.quality_list->item(i);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

		if(settings_.report_config->quality().contains(ui_.quality_list->item(i)->text()))
		{
			ui_.quality_list->item(i)->setCheckState(Qt::CheckState::Checked);
		}
		else
		{
			ui_.quality_list->item(i)->setCheckState(Qt::CheckState::Unchecked);
		}
	}

	if (settings_.report_config->quality().count() == 1 && settings_.report_config->quality()[0] == "no abnormalities")
	{
		ui_.quality_no_abnormalities->setChecked(true);
	}


	//Load possible HRD statements
	foreach(const auto& entry, db_.getEnum("somatic_report_configuration", "hrd_statement"))
	{
		ui_.hrd_statement->addItem(entry);
	}
	//Preselect  entry to old setting
	for(int i=0; i<ui_.hrd_statement->count(); ++i)
	{
		if(ui_.hrd_statement->itemText(i) == settings_.report_config->hrdStatement())
		{
			ui_.hrd_statement->setCurrentIndex(i);
			break;
		}
	}

	updateIgvText();
}

void SomaticReportDialog::disableGUI()
{
	for(int i=0; i<ui_.tabs->count(); ++i)
	{
		ui_.tabs->setTabEnabled(i, false);
	}
}

void SomaticReportDialog::enableGUI()
{
	for(int i=0; i<ui_.tabs->count(); ++i)
	{
		ui_.tabs->setTabEnabled(i, true);
	}
}

void SomaticReportDialog::rnaSampleSelection()
{
	if(ui_.report_type_rna->isChecked()) ui_.rna_ids_for_report->setEnabled(true);
	else ui_.rna_ids_for_report->setEnabled(false);
}

void SomaticReportDialog::writeBackSettings()
{
	if(getReportType() == RNA) return; //No report configuration for RNA samples

	settings_.report_config->setIncludeTumContentByMaxSNV(ui_.include_max_tum_freq->isChecked());
	settings_.report_config->setIncludeTumContentByClonality(ui_.include_max_clonality->isChecked());
	settings_.report_config->setIncludeTumContentByHistological(ui_.include_tum_content_histological->isChecked());
	settings_.report_config->setIncludeTumContentByEstimated(ui_.include_tum_content_estimated->isChecked());
	settings_.report_config->setTumContentByEstimated(ui_.tum_content_estimated->value());

	settings_.report_config->setMsiStatus(ui_.include_msi_status->isChecked());
	settings_.report_config->setCnvBurden(ui_.include_cnv_burden->isChecked());
	settings_.report_config->setIncludeMutationBurden(ui_.include_mutation_burden->isChecked());
	settings_.report_config->setFusionsDetected(ui_.fusions_detected->isChecked());

	//current index of hrd_score is identical to value!
	QStringList quality;
	if (ui_.quality_no_abnormalities->isChecked())
	{
		quality.append(ui_.quality_no_abnormalities->text());
	}
	else
	{
		for(int i=0; i< ui_.quality_list->count(); i++)
		{
			QListWidgetItem* item = ui_.quality_list->item(i);
			if (item->checkState() == Qt::Checked)
			{
				quality.append(item->text());
			}
		}
	}
	settings_.report_config->setQuality(quality);

	settings_.report_config->setHrdStatement( ui_.hrd_statement->currentText() );

	if(ui_.tmb_reference->selectionModel()->selectedRows().count() == 1)
	{
		QString ref_text = ui_.tmb_reference->item(ui_.tmb_reference->selectionModel()->selectedRows()[0].row(), 1)->text();
		settings_.report_config->setTmbReferenceText(ref_text);
	}

	//get selected variants in germline sample - clear current and replace by the selected
	settings_.report_config->clearGermlineVariantConfigurations();

	for(int i=0; i<ui_.germline_variants->rowCount(); ++i)
	{
		if(ui_.germline_variants->item(i,0)->checkState() == Qt::Checked)
		{
			SomaticReportGermlineVariantConfiguration var_conf;
			var_conf.variant_index = ui_.germline_variants->verticalHeaderItem(i)->text().toInt();

			try
			{
				var_conf.tum_freq = Helper::toDouble(ui_.germline_variants->item( i, 6 )->text());
			}
			catch(ArgumentException)
			{
				var_conf.tum_freq = std::numeric_limits<double>::quiet_NaN();
			}

			try
			{
				var_conf.tum_depth = Helper::toDouble( ui_.germline_variants->item(i, 7)->text() );
			}
			catch(ArgumentException)
			{
				var_conf.tum_depth = std::numeric_limits<double>::quiet_NaN();
			}

			settings_.report_config->addGermlineVariantConfiguration(var_conf);
		}
	}

	//Resolve CIN settings
	settings_.report_config->setCinChromosomes(resolveCIN());


	if(ui_.limitations_check->isChecked())
	{
		settings_.report_config->setLimitations(ui_.limitations_text->toPlainText());
	}
	else
	{
		settings_.report_config->setLimitations("");
	}

	//IGV data
	if(GlobalServiceProvider::fileLocationProvider().getSomaticIgvScreenshotFile().exists)
	{
		QImage picture;
		if (GlobalServiceProvider::fileLocationProvider().isLocal())
		{
			picture = QImage(GlobalServiceProvider::fileLocationProvider().getSomaticIgvScreenshotFile().filename);
		}
		else
		{
			try
			{
				QByteArray response = HttpHandler(true).get(GlobalServiceProvider::fileLocationProvider().getSomaticIgvScreenshotFile().filename);
				if (!response.isEmpty()) picture.loadFromData(response);
			}
			catch (Exception& e)
			{
				QMessageBox::warning(this, "Could not retrieve the screenshot from the server", e.message());
			}
		}

		if( (uint)picture.width() > 1200 ) picture = picture.scaledToWidth(1200, Qt::TransformationMode::SmoothTransformation);
		if( (uint)picture.height() > 1200 ) picture = picture.scaledToHeight(1200, Qt::TransformationMode::SmoothTransformation);

		QByteArray png_data = "";

		if(!picture.isNull())
		{
			QBuffer buffer(&png_data);
			buffer.open(QIODevice::WriteOnly);
			if(picture.save(&buffer, "PNG"))
			{
				settings_.igv_snapshot_png_hex_image = png_data.toHex();
				settings_.igv_snapshot_width = picture.width();
				settings_.igv_snapshot_height = picture.height();
			}
		}
	}
}

SomaticReportDialog::report_type SomaticReportDialog::getReportType()
{
	if(ui_.report_type_dna->isChecked()) return report_type::DNA;
	else if (ui_.report_type_cfdna->isChecked()) return report_type::cfDNA;
	else return report_type::RNA;
}

QString SomaticReportDialog::getRNAid()
{
	return ui_.rna_ids_for_report->currentText();
}

void SomaticReportDialog::enableChoiceRnaReportType(bool enabled)
{
	ui_.report_type_label->setEnabled(enabled);
	ui_.report_type_dna->setEnabled(enabled);
	ui_.report_type_rna->setEnabled(enabled);
}

void SomaticReportDialog::enableChoicecfDnaReportType(bool enabled)
{
	ui_.report_type_label->setEnabled(enabled);
	ui_.report_type_dna->setEnabled(enabled);
	ui_.report_type_cfdna->setEnabled(enabled);
}

void SomaticReportDialog::cinState()
{
	if(ui_.include_cnv_burden->isChecked())
	{
		ui_.tabs->setTabEnabled(2, true);
		//preselect CIN chromosomes
		for(const auto& chr : settings_.report_config->cinChromosomes())
		{
			ui_.cin->findChild<QCheckBox*>(chr)->setChecked(true);
		}
	}
	else
	{

		ui_.tabs->setTabEnabled(2, false); //CIN tab


		foreach(const auto& checkbox, ui_.cin->findChildren<QCheckBox*>())
		{
			checkbox->setChecked(false);
		}
	}
}

void SomaticReportDialog::setRNAids(const QStringList& rna_ids)
{
	ui_.rna_ids_for_report->addItems(rna_ids);
}

void SomaticReportDialog::limitationState()
{
	ui_.limitations_text->setEnabled(ui_.limitations_check->isChecked());
}

void SomaticReportDialog::qualityState()
{
	ui_.quality_list->setEnabled(! ui_.quality_no_abnormalities->isChecked());
	if (ui_.quality_no_abnormalities->isChecked())
	{
		//uncheck all other options:
		for(int i=0; i<ui_.quality_list->count();++i)
		{
			ui_.quality_list->item(i)->setCheckState(Qt::CheckState::Unchecked);
		}
	}
}

void SomaticReportDialog::createIgvScreenshot()
{
    updateIgvText();

    //create commands
	QStringList commands;

	commands << "genome " + Settings::path("igv_genome");
	for(auto& loc : GlobalServiceProvider::fileLocationProvider().getBafFiles(false))
	{
		if (!loc.exists) continue;
		if (!loc.id.contains("somatic")) continue;

		commands  << "load " + Helper::canonicalPath(loc.filename);
	}

	FileLocation loc = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCoverageFile();
	if (loc.exists) commands << "load " + Helper::canonicalPath(loc.filename);

	loc = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCallFile();
	if (loc.exists) commands << "load " + Helper::canonicalPath(loc.filename);

    FileLocation screenshot_file_location = GlobalServiceProvider::fileLocationProvider().getSomaticIgvScreenshotFile();
	if (screenshot_file_location.exists)
	{
		QMessageBox::Button b = QMessageBox::warning(this, "IGV screenshot", "IGV screenshot already exists. Do you want to recreate it?", QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
		if (b != QMessageBox::Yes) return;
	}

    QString screenshot_filename = Helper::canonicalPath(screenshot_file_location.filename);

    if (!GlobalServiceProvider::fileLocationProvider().isLocal())
	{
        // For client-server mode we create a temporary file locally first
        screenshot_filename = QDir(QDir::tempPath()).filePath(QUrl(screenshot_filename).fileName());
	}

	commands << "maxPanelHeight 400";
    commands << "snapshot " + screenshot_filename;

	//create screenshot
	try
	{
		IGVSession& session = IgvSessionManager::get(0);
		session.execute(commands, false);
		session.waitForDone();

		if (!QFileInfo::exists(screenshot_filename))
        {
			THROW(Exception, "IGV screenshot was not created!")
        }
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "IGV screenshot", "Could not create IGV screenshot. Error message: " + e.message());
		return;
	}

	// Upload screenshot to the server, if the client-server mode is activated
	if (!GlobalServiceProvider::fileLocationProvider().isLocal())
	{
		QList<QString> filename_parts = project_filename_.split("/");
		if (filename_parts.size()<=3)
		{
			QMessageBox::warning(this, "Processed sample error", "Could not find find the location of the processed sample!");
			return;
		}

        QHttpMultiPart* multipart_form = new QHttpMultiPart(QHttpMultiPart::FormDataType);
		QHttpPart binary_form_data;
		binary_form_data.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/png"));
        binary_form_data.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\"; filename=\"" + QFileInfo(screenshot_filename).fileName() + "\""));

		QFile* file = new QFile(screenshot_filename);
		file->open(QIODevice::ReadOnly);
		binary_form_data.setBodyDevice(file);
        file->setParent(multipart_form);
		multipart_form->append(binary_form_data);

		try
        {
            HttpHandler(true).post(ClientHelper::serverApiUrl() + "upload?ps_url_id=" + QUrl(filename_parts[filename_parts.size()-2].toUtf8()).toEncoded() + "&token=" + LoginManager::userToken(), multipart_form);
		}
		catch (Exception& e)
		{
			QMessageBox::warning(this, "File upload failed", e.message());
		}
	}

	updateIgvText();
}

QList<QString> SomaticReportDialog::resolveCIN()
{
	QList<QString> out = {};
	foreach(const auto& checkbox, ui_.cin->findChildren<QCheckBox*>())
	{
		if(checkbox->isChecked()) out << checkbox->text();
	}
	return out;
}

bool SomaticReportDialog::skipNGSD()
{
	return ui_.no_ngsd->isChecked();
}


void SomaticReportDialog::updateIgvText()
{
	if(GlobalServiceProvider::fileLocationProvider().getSomaticIgvScreenshotFile().exists)
	{
		ui_.label_hint_igv_screenshot_available->setText("<span style=\"color:#000000;\">available</span> <a href='bal'>[recreate]</a>");
	}
	else
	{
		ui_.label_hint_igv_screenshot_available->setText("<span style=\"color:#ff0000;\">not available</span> <a href='bal'>[create]</a>");
	}
}
