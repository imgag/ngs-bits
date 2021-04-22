#include "SomaticReportDialog.h"
#include "NGSD.h"
#include "SomaticReportHelper.h"
#include <QMessageBox>

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
		if(this->hpoterm == rhs.hpoterm) return true;
		return true;
	}
};

SomaticReportDialog::SomaticReportDialog(SomaticReportSettings &settings, const CnvList &cnvs, const VariantList& germl_variants, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, db_()
	, settings_(settings)
	, cnvs_(cnvs)
	, germl_variants_(germl_variants)
	, target_region_(settings.report_config.targetRegionName())
	, tum_cont_snps_(std::numeric_limits<double>::quiet_NaN())
	, tum_cont_max_clonality_(std::numeric_limits<double>::quiet_NaN())
	, tum_cont_histological_(std::numeric_limits<double>::quiet_NaN())
	, limitations_()
{

	ui_.setupUi(this);

	connect(ui_.report_type_rna, SIGNAL(clicked(bool)), this, SLOT(disableGUI()));
	connect(ui_.report_type_dna, SIGNAL(clicked(bool)), this, SLOT(enableGUI()));

	connect(ui_.report_type_rna, SIGNAL(clicked(bool)), this, SLOT(rnaSampleSelection()));
	connect(ui_.report_type_dna, SIGNAL(clicked(bool)), this, SLOT(rnaSampleSelection()));

	connect(ui_.include_cnv_burden, SIGNAL(stateChanged(int)), this, SLOT(cinState()));
	connect(ui_.limitations_check, SIGNAL(stateChanged(int)), this, SLOT(limitationState()));


	//Resolve tumor content estimate from NGSD
	QCCollection res = db_.getQCData(db_.processedSampleId(settings.tumor_ps, true));
	try
	{
		tum_cont_snps_ = Helper::toDouble(res.value("QC:2000054", true).asString());
	}
	catch(ArgumentException){} //nothing to do
	catch(TypeConversionException){} //nothing to do

	//Resolve histological tumor content (if available in NGSD)
	QList<SampleDiseaseInfo> disease_infos = db_.getSampleDiseaseInfo(db_.sampleId(settings.tumor_ps), "tumor fraction");
	for(const auto& entry : disease_infos)
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

	tum_cont_max_clonality_ = SomaticReportHelper::getCnvMaxTumorClonality(cnvs);

	//Load HPO terms from database
	QStringList hpos_ngsd;
	QList<SampleDiseaseInfo> details = db_.getSampleDiseaseInfo(db_.sampleId(settings_.tumor_ps));
	for(const auto& info : details)
	{
		if(info.type == "HPO term id")
		{
			hpos_ngsd << info.disease_info;
		}
	}

	QList<tmbInfo> hpo_tmbs = tmbInfo::load("://Resources/hpoterms_tmb.tsv");

	//Set Reference value proposals
	for(const auto& hpo_tmb : hpo_tmbs)
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
		if(settings_.report_config.tmbReferenceText() == ui_.tmb_reference->item(i, 1)->text())
		{
			old_entry_row = i;
			break;
		}
	}

	if(old_entry_row == -1) //Insert ref text from former report if not  in table
	{
		ui_.tmb_reference->insertRow(ui_.tmb_reference->rowCount());
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 0, new QTableWidgetItem("former report"));
		ui_.tmb_reference->setItem(ui_.tmb_reference->rowCount()-1, 1, new QTableWidgetItem( settings_.report_config.tmbReferenceText()) );
		ui_.tmb_reference->selectRow(ui_.tmb_reference->rowCount()-1);
	}
	else //set selection to former entry if already in table
	{
		if(settings_.report_config.tmbReferenceText() != "") ui_.tmb_reference->selectRow(old_entry_row);
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
	int i_class = germl_variants.annotationIndexByName("classification", true, false);
	int i_co_sp = germl_variants.annotationIndexByName("coding_and_splicing", true, false);

	BamReader bam_reader(db_.processedSamplePath(db_.processedSampleId(settings_.tumor_ps), PathType::BAM));
	FastaFileIndex fasta_idx(Settings::string("reference_genome"));

	QList<int> germl_indices_in_report = settings_.report_config.variantIndicesGermline();

	if(i_class != -1 && i_co_sp != -1)
	{
		for(int i=0; i<germl_variants_.count(); ++i)
		{
			bool ok = true;
			if(germl_variants[i].annotations()[i_class].trimmed().isEmpty() || germl_variants[i].annotations()[i_class].toInt(&ok) < 4)
			{
				continue;
			}

			if(!ok)
			{
				QMessageBox::warning(this, "Error processing control tissue variants", "Could not parse control tissue variant classification " + germl_variants[i].toString() +". Aborting control tissue variants.");
				break;
			}
			const Variant& snv = germl_variants_[i];


			//determine frequency of variant in tumor bam
			double freq_in_tum = bam_reader.getVariantDetails(fasta_idx, snv).frequency;
			double depth_in_tum = bam_reader.getVariantDetails(fasta_idx, snv).depth;

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

			ui_.germline_variants->setItem( row, 8, new QTableWidgetItem( QString(snv.annotations()[i_class])) );
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
	if(settings.report_config.limitations() != "")
	{
		ui_.limitations_text->setPlainText(settings.report_config.limitations());
		ui_.limitations_check->setChecked(true);
	}
	else
	{
		QString text = "Aufgrund der geringen DNA-Konzentration (x,y ng/µl) / DNA-Qualität / des geringen Tumorgehaltes / der Heterogenität der verwendeten Tumorprobe war";
		text += " nur eine eingeschränkte Detektion somatischer Varianten (Punktmutationen und Kopienzahlveränderungen / Kopienzahlveränderungen) möglich. ";
		text += "Es zeigte sich ein komplexes Bild, dass am ehesten mit einer Polysomie bzw. Monosomie mehrerer Chromosomen vereinbar ist. ";
		text += "Die Mutationslast und der MSI-Status sind nicht bestimmbar. Eine Wiederholung der DNA-Isolation aus Tumorgewebe war nicht möglich.\n\n";
		text += "Aufgrund der Allelfrequenz der einzelnen tumorspezifischen Varianten schätzen wir den Tumorgehalt der eingesandten Probe bei unter 10%. Aufgrund des geringen Tumorgehaltes der ";
		text += "verwendeten Tumorprobe war nur eine eingeschränkte Detektion somatischer Varianten (Punktmutationen und Kopienzahlveränderungen) möglich. Die Mutationslast und";
		text += " MSI-Status sind nicht bestimmbar. Die hier berichteten Varianten wurden vor allem durch eine Senkung des Detektionslimits der Allelfrequenz auf unter 5% detektiert ";
		text += " und wurden manuell überprüft. Eine Wiederholung der DNA-Isolation aus Tumorgewebe war nicht möglich.\n\n";
		text += "Es gibt Hinweise auf eine heterogene Probe mit niedrigem Anteil an Tumorzellen. Die Auswertung der Kopienzahlveränderungen ist in diesem Fall limitiert.";
		ui_.limitations_text->setPlainText(text);
	}



	//Update GUI
	if(BasicStatistics::isValidFloat(tum_cont_snps_))
	{
		ui_.include_max_tum_freq->setChecked(settings_.report_config.tumContentByMaxSNV());
		ui_.include_max_tum_freq->setText(ui_.include_max_tum_freq->text() + " ("  + QString::number(tum_cont_snps_, 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_tum_freq->setEnabled(false);
	}

	if(BasicStatistics::isValidFloat( tum_cont_max_clonality_))
	{
		ui_.include_max_clonality->setChecked(settings_.report_config.tumContentByClonality());
		ui_.include_max_clonality->setText(ui_.include_max_clonality->text() + " ("  + QString::number(tum_cont_max_clonality_ * 100., 'f', 1) +"%)");
	}
	else
	{
		ui_.include_max_clonality->setCheckable(false);
	}

	if(BasicStatistics::isValidFloat( tum_cont_histological_) && tum_cont_histological_ > 0.)
	{
		ui_.include_tum_content_histological->setChecked(settings_.report_config.tumContentByHistological());
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
		ui_.include_cnv_burden->setChecked(settings_.report_config.cnvBurden());
		ui_.include_cnv_burden->setText(ui_.include_cnv_burden->text() + " (" + QString::number(SomaticReportHelper::cnvBurden(cnvs_), 'f', 1)  + "%)");
	}
	else
	{
		ui_.include_cnv_burden->setCheckable(false);
	}

	//Set CIN options, depends on check state of cnv burden
	cinState();

	//Preselect remaining options
	ui_.include_msi_status->setChecked(settings_.report_config.msiStatus());
	ui_.fusions_detected->setChecked(settings_.report_config.fusionsDetected());
	if(Settings::boolean("debug_mode_enabled")) ui_.no_ngsd->setChecked(true);

	//index of hrd_score is equal to actual score value
	ui_.hrd_score->setCurrentIndex(settings_.report_config.hrdScore());


	//Load possible quality settings
	QStringList quality_entries = db_.getEnum("somatic_report_configuration", "quality");
	for(const auto& entry: quality_entries)
	{
		ui_.quality->addItem(entry);
	}

	//Set selected entry to old setting
	for(int i=0; i<ui_.quality->count();++i)
	{
		if(ui_.quality->itemText(i) == settings_.report_config.quality())
		{
			ui_.quality->setCurrentIndex(i);
			break;
		}
	}
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

	settings_.report_config.setTumContentByMaxSNV(ui_.include_max_tum_freq->isChecked());
	settings_.report_config.setTumContentByClonality(ui_.include_max_clonality->isChecked());
	settings_.report_config.setTumContentByHistological(ui_.include_tum_content_histological->isChecked());
	settings_.report_config.setMsiStatus(ui_.include_msi_status->isChecked());
	settings_.report_config.setCnvBurden(ui_.include_cnv_burden->isChecked());
	settings_.report_config.setFusionsDetected(ui_.fusions_detected->isChecked());

	//current index of hrd_score is identical to value!
	settings_.report_config.setHrdScore( ui_.hrd_score->currentIndex() );

	settings_.report_config.setQuality( ui_.quality->currentText() );

	if(ui_.tmb_reference->selectionModel()->selectedRows().count() == 1)
	{
		QString ref_text = ui_.tmb_reference->item(ui_.tmb_reference->selectionModel()->selectedRows()[0].row(), 1)->text();
		settings_.report_config.setTmbReferenceText(ref_text);
	}

	//get selected variants in germline sample
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

			settings_.report_config.setGermline(var_conf);
		}
		else
		{
			settings_.report_config.removeGermline(ui_.germline_variants->verticalHeaderItem(i)->text().toInt());
		}
	}

	//Resolve CIN settings
	settings_.report_config.setCinChromosomes(resolveCIN());


	if(ui_.limitations_check->isChecked())
	{
		settings_.report_config.setLimitations(ui_.limitations_text->toPlainText());
	}
	else
	{
		settings_.report_config.setLimitations("");
	}

}

SomaticReportDialog::report_type SomaticReportDialog::getReportType()
{
	if(ui_.report_type_dna->isChecked()) return report_type::DNA;
	else return report_type::RNA;
}

QString SomaticReportDialog::getRNAid()
{
	return ui_.rna_ids_for_report->currentText();
}

void SomaticReportDialog::enableChoiceReportType(bool enabled)
{
	ui_.report_type_label->setEnabled(enabled);
	ui_.report_type_dna->setEnabled(enabled);
	ui_.report_type_rna->setEnabled(enabled);
}

void SomaticReportDialog::cinState()
{
	if(ui_.include_cnv_burden->isChecked())
	{
		ui_.tabs->setTabEnabled(2, true);
		//preselect CIN chromosomes
		for(const auto& chr : settings_.report_config.cinChromosomes())
		{
			ui_.cin->findChild<QCheckBox*>(chr)->setChecked(true);
		}
	}
	else
	{

		ui_.tabs->setTabEnabled(2, false); //CIN tab


		for(const auto& checkbox : ui_.cin->findChildren<QCheckBox*>())
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
	if(ui_.limitations_check->isChecked()) ui_.limitations_text->setEnabled(true);
	else ui_.limitations_text->setEnabled(false);
}

QList<QString> SomaticReportDialog::resolveCIN()
{
	QList<QString> out = {};
	for(const auto& checkbox : ui_.cin->findChildren<QCheckBox*>())
	{
		if(checkbox->isChecked()) out << checkbox->text();
	}
	return out;
}

bool SomaticReportDialog::skipNGSD()
{
	return ui_.no_ngsd->isChecked();
}
