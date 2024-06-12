#include "ReportDialog.h"
#include "GUIHelper.h"
#include "DiseaseInfoWidget.h"
#include "SampleDiseaseInfoWidget.h"
#include "DiagnosticStatusWidget.h"
#include "Settings.h"
#include <QTableWidgetItem>
#include <QMenu>
#include <QMessageBox>


ReportDialog::ReportDialog(QString ps, ReportSettings& settings, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, const RepeatLocusList& res, const TargetRegionInfo& roi, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, ps_(ps)
	, settings_(settings)
	, variants_(variants)
	, cnvs_(cnvs)
	, svs_(svs)
	, res_(res)
	, roi_(roi)
{
	ui_.setupUi(this);
	setWindowTitle(windowTitle() + ps);
	connect(ui_.report_type, SIGNAL(currentTextChanged(QString)), this, SLOT(updateVariantTable()));
	connect(ui_.meta_data_check_btn, SIGNAL(clicked(bool)), this, SLOT(checkMetaData()));
	connect(ui_.details_cov, SIGNAL(stateChanged(int)), this, SLOT(updateCoverageCheckboxStatus()));
	connect(this, SIGNAL(accepted()), this, SLOT(writeBackSettings()));

	initGUI();
	validateReportConfig();
}

void ReportDialog::checkMetaData()
{
	//clear
	ui_.meta_data_check_output->clear();

	//check
	QString ps_id = db_.processedSampleId(ps_);
	QHash<QString, QStringList> errors = db_.checkMetaData(ps_id, variants_, cnvs_, svs_, res_);

	//sort sample names and make current sample the first one
	QStringList sample_names = errors.keys();
	sample_names.sort();
	QString sample = db_.getSampleData(db_.sampleId(ps_)).name;
	sample_names.removeAll(sample);
	sample_names.prepend(sample);

	//show messages
	QStringList display_messages;
	foreach(QString sample_name, sample_names)
	{
		QStringList sample_messages = errors[sample_name];
		foreach(QString sample_message, sample_messages)
		{
			display_messages << sample_name + ": " + sample_message;
		}
	}
	if (display_messages.count()>0)
	{
		ui_.meta_data_check_output->setText("<font color='red'>" + display_messages.join("<br>") + "</font>");
	}

	//add edit menu entries
	QMenu* menu = new QMenu(this);
	for(int i=0; i<sample_names.count(); ++i)
	{
		QString sample_name = sample_names[i];

		if (i>0) menu->addSeparator();

		QAction* action = menu->addAction(sample_name + ": disease group/status");
		action->setData(sample_name);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(editDiseaseGroupStatus()));

		action = menu->addAction(sample_name + ": disease details (HPO, OMIM, Optha, ...)");
		action->setData(sample_name);
		connect(action, SIGNAL(triggered(bool)), this, SLOT(editDiseaseDetails()));

		if (ps_.startsWith(sample_name)) //only for current sample, because processed sample is needed
		{
			action = menu->addAction(sample_name + ": diagnostic status");
			action->setData(sample_name);
			connect(action, SIGNAL(triggered(bool)), this, SLOT(editDiagnosticStatus()));
		}
	}
	ui_.meta_data_edit_btn->setMenu(menu);

	//update button box
	activateOkButtonIfValid();
}

void ReportDialog::initGUI()
{
	//report types
	ui_.report_type->addItems(ReportVariantConfiguration::getTypeOptions());
	if (Settings::boolean("allow_report_with_all_types", true)) ui_.report_type->addItem("all");
	ui_.report_type->setCurrentIndex(0);

	//settings
	ui_.details_cov->setChecked(settings_.show_coverage_details);
	ui_.min_cov->setValue(settings_.min_depth);
	ui_.cov_based_on_complete_roi->setChecked(settings_.cov_based_on_complete_roi);
	ui_.cov_exon_padding->setValue(settings_.cov_exon_padding);
	ui_.depth_calc->setChecked(settings_.recalculate_avg_depth);
	ui_.omim_table->setChecked(settings_.show_omim_table);
	ui_.omim_table_one_only->setChecked(settings_.show_one_entry_in_omim_table);
	ui_.class_info->setChecked(settings_.show_class_details);
	ui_.language->setCurrentText(settings_.language);
	ui_.refseq_trans_names->setChecked(settings_.show_refseq_transcripts);

	//no ROI > no roi options
	if (!roi_.isValid())
	{
		ui_.details_cov->setChecked(false);
		ui_.details_cov->setEnabled(false);

		ui_.depth_calc->setChecked(false);
		ui_.depth_calc->setEnabled(false);

		ui_.cov_based_on_complete_roi->setChecked(false);
		ui_.cov_based_on_complete_roi->setEnabled(false);

		ui_.cov_exon_padding->setEnabled(false);
		ui_.cov_exon_padding_label->setEnabled(false);

		ui_.min_cov->setEnabled(false);
		ui_.min_cov_label->setEnabled(false);

		ui_.omim_table->setChecked(false);
		ui_.omim_table->setEnabled(false);
		ui_.omim_table_one_only->setChecked(false);
		ui_.omim_table_one_only->setEnabled(false);
	}

	//check box status
	updateCoverageCheckboxStatus();

	//meta data
	checkMetaData();
}

void ReportDialog::updateVariantTable()
{
	//init
	ui_.vars->setRowCount(0);
	int row = 0;

	FastaFileIndex genome_idx(Settings::string("reference_genome"));

	//add small variants
	int geno_idx = variants_.getSampleHeader().infoByID(ps_).column_index;
	int gene_idx = variants_.annotationIndexByName("gene");
	int class_idx = variants_.annotationIndexByName("classification");
	foreach(int i, settings_.report_config->variantIndices(VariantType::SNVS_INDELS, true, type()))
	{
		Variant variant = variants_[i];
		const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::SNVS_INDELS, i);

		//manual curation
		if (var_conf.isManuallyCurated()) var_conf.updateVariant(variant, genome_idx, geno_idx);

		bool in_roi = true;
		if (roi_.isValid() && !roi_.regions.overlapsWith(variant.chr(), variant.start(), variant.end())) in_roi = false;

		QByteArray genotype = variant.annotations().at(geno_idx).trimmed();

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		addCheckBox(row, 0, in_roi && genotype!="wt", !in_roi)->setData(Qt::UserRole, i);
		addTableItem(row, 1, var_conf.report_type + (var_conf.causal ? " (causal)" : ""));
		addTableItem(row, 2, variantTypeToString(VariantType::SNVS_INDELS));
		addTableItem(row, 3, variant.toString(QChar(), 30) + " (" + genotype + ")");
		addTableItem(row, 4, variant.annotations().at(gene_idx));
		addTableItem(row, 5, variant.annotations().at(class_idx));
		++row;
	}


	//add CNVs
	foreach(int i, settings_.report_config->variantIndices(VariantType::CNVS, true, type()))
	{
		CopyNumberVariant cnv = cnvs_[i];
		const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::CNVS,i);

		//manual curation
		if (var_conf.isManuallyCurated()) var_conf.updateCnv(cnv, cnvs_.annotationHeaders(), db_);

		bool in_roi = true;
		if (roi_.isValid() && !roi_.regions.overlapsWith(cnv.chr(), cnv.start(), cnv.end())) in_roi = false;

		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		addCheckBox(row, 0, in_roi, !in_roi)->setData(Qt::UserRole, i);
		addTableItem(row, 1, var_conf.report_type + (var_conf.causal ? " (causal)" : ""));
		addTableItem(row, 2, variantTypeToString(VariantType::CNVS));
		addTableItem(row, 3, cnv.toStringWithMetaData() + " cn=" + QString::number(cnv.copyNumber(cnvs_.annotationHeaders())));
		addTableItem(row, 4, cnv.genes().join(", "));
		addTableItem(row, 5, var_conf.classification);
		++row;
	}

	//add SVs
	if (svs_.isValid())
	{
		//determine sample column index (for genotype extraction)
		int sv_sample_idx = 0;
		if (svs_.format() == BedpeFileFormat::BEDPE_GERMLINE_MULTI || svs_.format() == BedpeFileFormat::BEDPE_GERMLINE_TRIO)
		{
			try
			{
				sv_sample_idx = svs_.sampleHeaderInfo().infoByStatus(true).column_index - svs_.annotationIndexByName("FORMAT") - 1; //relativ to column after FORMAT
			}
			catch (...)
			{
				sv_sample_idx = -1;
			}

		}

		foreach(int i, settings_.report_config->variantIndices(VariantType::SVS, true, type()))
		{
			BedpeLine sv = svs_[i];
			const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::SVS,i);

			//manual curation
			if (var_conf.isManuallyCurated()) var_conf.updateSv(sv, svs_.annotationHeaders(), db_);

			QByteArray genotype = "n/a";
			if (sv_sample_idx!=-1) genotype = sv.genotypeHumanReadable(svs_.annotationHeaders(), false, sv_sample_idx);

			//check if variant is in ROI (if there is a ROI)
			bool in_roi = true;
			if (roi_.name!="")
			{
				in_roi = sv.affectedRegion().overlapsWith(roi_.regions);
			}

			ui_.vars->setRowCount(ui_.vars->rowCount()+1);
			addCheckBox(row, 0, in_roi && genotype!="0/0", !in_roi)->setData(Qt::UserRole, i);
			addTableItem(row, 1, var_conf.report_type + (var_conf.causal ? " (causal)" : ""));
			addTableItem(row, 2, variantTypeToString(VariantType::SVS));
			addTableItem(row, 3, sv.toString() + " (" + genotype + ")");
			addTableItem(row, 4, sv.genes(svs_.annotationHeaders()).join(", "));
			addTableItem(row, 5, var_conf.classification);
			++row;
		}
	}


	//add REs
	if (res_.isValid())
	{
		foreach(int i, settings_.report_config->variantIndices(VariantType::RES, true, type()))
		{
			RepeatLocus re = res_[i];
			const ReportVariantConfiguration& var_conf = settings_.report_config->get(VariantType::RES,i);

			//manual curation
			if (var_conf.isManuallyCurated()) var_conf.updateRe(re);

			//check if variant is in ROI (if there is a ROI)
			bool in_roi = true;
			if (roi_.name!="")
			{
				in_roi = roi_.regions.overlapsWith(re.region());
			}

			ui_.vars->setRowCount(ui_.vars->rowCount()+1);
			addCheckBox(row, 0, in_roi, !in_roi)->setData(Qt::UserRole, i);
			addTableItem(row, 1, var_conf.report_type + (var_conf.causal ? " (causal)" : ""));
			addTableItem(row, 2, variantTypeToString(VariantType::RES));
			addTableItem(row, 3, re.toString(true, true));
			addTableItem(row, 4, re.name());
			addTableItem(row, 5, var_conf.classification);
			++row;
		}
	}

	//add other causal variant
	OtherCausalVariant causal_variant = settings_.report_config->otherCausalVariant();

	if(!causal_variant.coordinates.isEmpty())
	{
		ui_.vars->setRowCount(ui_.vars->rowCount()+1);
		addCheckBox(row, 0, true, true)->setData(Qt::UserRole, -1);
		addTableItem(row, 1, "causal");
		addTableItem(row, 2, causal_variant.type);
		addTableItem(row, 3, causal_variant.coordinates);
		addTableItem(row, 4, causal_variant.gene);
		addTableItem(row, 5, "");
		addTableItem(row, 6, causal_variant.comment);
		++row;
	}

	//resize table cells
	GUIHelper::resizeTableCellWidths(ui_.vars);
	GUIHelper::resizeTableCellHeightsToFirst(ui_.vars);
}

void ReportDialog::updateCoverageCheckboxStatus()
{
	//enable coverage detail settings only if necessary
	if (roi_.isValid())
	{
		bool add_cov_details = ui_.details_cov->isChecked();
		ui_.min_cov->setEnabled(add_cov_details);
		ui_.min_cov_label->setEnabled(add_cov_details);
		ui_.depth_calc->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.depth_calc->setChecked(false);
		ui_.cov_based_on_complete_roi->setEnabled(add_cov_details);
		if (!add_cov_details) ui_.cov_based_on_complete_roi->setChecked(false);
		if (roi_.genes.isEmpty())
		{
			ui_.cov_exon_padding->setEnabled(false);
			ui_.cov_exon_padding_label->setEnabled(false);
		}
		else
		{
			ui_.cov_exon_padding->setEnabled(add_cov_details);
			ui_.cov_exon_padding_label->setEnabled(add_cov_details);
		}
	}
}

void ReportDialog::validateReportConfig()
{
	int rc_id = db_.reportConfigId(db_.processedSampleId(ps_));
	if (rc_id==-1) return;

	VariantList missing_small_variants;
	CnvList missing_cnvs;
	BedpeFile missing_svs;

	//SNVs/InDels
	QList<int> small_variant_ids = db_.getValuesInt("SELECT variant_id FROM report_configuration_variant WHERE report_configuration_id=:0", QString::number(rc_id));
	foreach (int var_id, small_variant_ids)
	{
		//match report variant against variant list
		Variant var = db_.variant(QString::number(var_id));
		bool match_found = false;
		for (int i=0; i<variants_.count(); ++i)
		{
			if (var==variants_[i])
			{
				match_found = true;
				break;
			}
		}
		if (!match_found) missing_small_variants.append(var);
	}

	//CNVs
	QList<int> cnv_ids = db_.getValuesInt("SELECT cnv_id FROM report_configuration_cnv WHERE report_configuration_id=:0", QString::number(rc_id));
	foreach (int var_id, cnv_ids)
	{
		//match report variant against variant list
		CopyNumberVariant cnv = db_.cnv(var_id);
		bool match_found = false;
		for (int i=0; i<cnvs_.count(); ++i)
		{
			if (cnvs_[i].hasSamePosition(cnv))
			{
				match_found = true;
				break;
			}
		}
		if (!match_found) missing_cnvs.append(cnv);
	}

	//SVs
	foreach (StructuralVariantType type, QList<StructuralVariantType>() << StructuralVariantType::DEL << StructuralVariantType::DUP << StructuralVariantType::INS << StructuralVariantType::INV << StructuralVariantType::BND)
	{
		QString sv_table = db_.svTableName(type);
		QList<int> sv_ids = db_.getValuesInt("SELECT " + sv_table + "_id FROM report_configuration_sv WHERE report_configuration_id=:0 AND " + sv_table + "_id IS NOT NULL", QString::number(rc_id));
		foreach (int var_id, sv_ids)
		{
			//match report variant against variant list
			BedpeLine sv = db_.structuralVariant(var_id, type, svs_);
			if (svs_.findMatch(sv, true, false) == -1) missing_svs.append(sv);
		}
	}

	QStringList message_text;
	if (missing_small_variants.count() > 0)
	{
		message_text << "SNVs/Indels: ";
		for (int i = 0; i < missing_small_variants.count(); ++i)
		{
			message_text << "\t" + missing_small_variants[i].toString();
		}
		message_text << "";
	}
	if (missing_cnvs.count() > 0)
	{
		message_text << "CNVs: ";
		for (int i = 0; i < missing_cnvs.count(); ++i)
		{
			message_text << "\t" + missing_cnvs[i].toString();
		}
		message_text << "";
	}
	if (missing_svs.count() > 0)
	{
		message_text << "SVs: ";
		for (int i = 0; i < missing_svs.count(); ++i)
		{
			message_text << "\t" + missing_svs[i].toString();
		}
		message_text << "";
	}

	if (message_text.size() > 0)
	{
		QMessageBox::warning(this, "Missing variants in variant list", QString("The following variants are part of the current report configuration, but they are missing in the loaded variant list ")
					 + "and as a result will not be part of the report. \n\n" + message_text.join("\n"));
	}

}

void ReportDialog::editDiseaseGroupStatus()
{
	QAction* action = qobject_cast<QAction*>(sender());
	QString sample = action->data().toString();
	QString sample_id = db_.sampleId(sample);

	//get disease group/status
	DiseaseInfoWidget* widget = new DiseaseInfoWidget(sample, sample_id, this);
	auto dlg = GUIHelper::createDialog(widget, "Disease information of '" + sample + "'", "", true);
	if (dlg->exec() != QDialog::Accepted) return;

	//update
	db_.setSampleDiseaseData(sample_id, widget->diseaseGroup(), widget->diseaseStatus());
	checkMetaData();
}

void ReportDialog::editDiseaseDetails()
{
	QAction* action = qobject_cast<QAction*>(sender());
	QString sample = action->data().toString();
	QString sample_id = db_.sampleId(sample);

	//get disease details
	SampleDiseaseInfoWidget* widget = new SampleDiseaseInfoWidget(sample, this);
	widget->setDiseaseInfo(db_.getSampleDiseaseInfo(sample_id));
	auto dlg = GUIHelper::createDialog(widget, "Sample disease details of '" + sample + "'", "", true);
	if (dlg->exec() != QDialog::Accepted) return;

	//update
	db_.setSampleDiseaseInfo(sample_id, widget->diseaseInfo());
	checkMetaData();
}

void ReportDialog::editDiagnosticStatus()
{
	QString ps_id = db_.processedSampleId(ps_);
	DiagnosticStatusWidget* widget = new DiagnosticStatusWidget(this);
	widget->setStatus(db_.getDiagnosticStatus(ps_id));
	auto dlg = GUIHelper::createDialog(widget, "Diagnostic status of '" + ps_, "'", true);
	if (dlg->exec()!=QDialog::Accepted) return;

	db_.setDiagnosticStatus(ps_id, widget->status());
	checkMetaData();
}

QTableWidgetItem* ReportDialog::addTableItem(int row, int col, QString text)
{
	QTableWidgetItem* item = new QTableWidgetItem();

	item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
	item->setText(text);

	ui_.vars->setItem(row, col, item);

	return item;
}

QTableWidgetItem*ReportDialog::addCheckBox(int row, int col, bool is_checked, bool check_state_editable)
{
	QTableWidgetItem* item = new QTableWidgetItem();

	if (is_checked)
	{
		item->setCheckState(Qt::Checked);
	}
	else
	{
		item->setCheckState(Qt::Unchecked);
	}

	if (check_state_editable)
	{
		item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsUserCheckable);
	}
	else
	{
		item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
		item->setToolTip("Variants inside the target region are always seleted.\nVariants with genotype 'wt' cannot be selected.");
	}

	ui_.vars->setItem(row, col, item);

	return item;
}

void ReportDialog::writeBackSettings()
{
	settings_.selected_variants.clear();
	settings_.select_other_causal_variant = false;
	for (int r=0; r<ui_.vars->rowCount(); ++r)
	{
		if (ui_.vars->item(r, 0)->checkState()!=Qt::Checked) continue;

		int index = ui_.vars->item(r, 0)->data(Qt::UserRole).toInt();
		if(index != -1)
		{
			VariantType type = stringToVariantType(ui_.vars->item(r, 2)->text());
			settings_.selected_variants << qMakePair(type, index);
		}
		else
		{
			// other causal variant
			settings_.select_other_causal_variant = true;
		}
	}

	settings_.show_coverage_details = ui_.details_cov->isChecked();
	settings_.min_depth = ui_.min_cov->value();
	settings_.cov_based_on_complete_roi = ui_.cov_based_on_complete_roi->isChecked();
	settings_.cov_exon_padding = ui_.cov_exon_padding->value();
	settings_.recalculate_avg_depth = ui_.depth_calc->isChecked();
	settings_.show_omim_table = ui_.omim_table->isChecked();
	settings_.show_one_entry_in_omim_table = ui_.omim_table_one_only->isChecked();
	settings_.show_class_details = ui_.class_info->isChecked();
	settings_.language = ui_.language->currentText();
	settings_.show_refseq_transcripts = ui_.refseq_trans_names->isChecked();
}

void ReportDialog::activateOkButtonIfValid()
{
	ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	ui_.buttonBox->setToolTip("");

	//meta data ok?
	if (!ui_.meta_data_check_output->text().trimmed().isEmpty())
	{
		ui_.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui_.buttonBox->setToolTip("Correct meta data errors to continue!");
		return;
	}

}
