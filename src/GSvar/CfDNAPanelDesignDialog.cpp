#include "CfDNAPanelDesignDialog.h"
#include "ui_CfDNAPanelDesignDialog.h"
#include "GUIHelper.h"
#include "LoginManager.h"
#include "NGSD.h"
#include <QMessageBox>
#include <QMenu>
#include <QDir>
#include <QPushButton>


CfDNAPanelDesignDialog::CfDNAPanelDesignDialog(const VariantList& variants, const FilterResult& filter_result, const SomaticReportConfiguration& somatic_report_configuration, const QString& processed_sample_name, const DBTable& processing_systems, QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::CfDNAPanelDesignDialog),
	variants_(variants),
	filter_result_(filter_result),
	somatic_report_configuration_(somatic_report_configuration),
	processed_sample_name_(processed_sample_name)
{

    // abort if no connection to NGSD
    if (!LoginManager::active())
    {
        GUIHelper::showMessage("No connection to the NGSD!", "You need access to the NGSD to design cfDNA panels!");
        this->close();
    }

    processed_sample_id_ = NGSD().processedSampleId(processed_sample_name_);


	// remove '?' entry
	setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

	ui_->setupUi(this);

	// connect signals and slots
	connect(ui_->buttonBox, SIGNAL(accepted()), this, SLOT(createOutputFiles()));
	connect(ui_->vars,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showVariantContextMenu(QPoint)));
	connect(ui_->genes,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showGeneContextMenu(QPoint)));
	connect(ui_->cb_hotspot_regions, SIGNAL(stateChanged(int)), this, SLOT(showHotspotRegions(int)));
	connect(ui_->vars,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(openVariantInIGV(QTableWidgetItem*)));
	connect(ui_->cb_processing_system,SIGNAL(currentTextChanged(QString)),this,SLOT(updateSystemSelection()));

	// set context menus for tables
	ui_->vars->setContextMenuPolicy(Qt::CustomContextMenu);
	ui_->genes->setContextMenuPolicy(Qt::CustomContextMenu);

	// fill processing system ComboBox
	ui_->cb_processing_system->fill(processing_systems, false);

	// set style of Error message
	ui_->l_error_message->setStyleSheet("QLabel{color: rgb(255, 0, 0);}");


	loadPreviousPanels();
	loadVariants();
	loadHotspotRegions();
	loadGenes();
}

CfDNAPanelDesignDialog::~CfDNAPanelDesignDialog()
{
	delete ui_;
}

void CfDNAPanelDesignDialog::loadPreviousPanels()
{
	// clear prev panel
	cfdna_panel_info_ = CfdnaPanelInfo();
	prev_vars_.clear();
	prev_genes_.clear();
	prev_kasp_ = true;
	prev_hotspots_.clear();

	QList<CfdnaPanelInfo> previous_panels = NGSD().cfdnaPanelInfo(processed_sample_id_);

    if(previous_panels.size() > 0)
	{
        QStringList panel_text;
		foreach (const CfdnaPanelInfo& panel, previous_panels)
        {
			panel_text.append("cfDNA panel for " + panel.processing_system  + " (" + panel.created_date.toString("dd.MM.yyyy") + " by " + panel.created_by + ")");
        }

		QComboBox* cfdna_panel_selector = new QComboBox(this);
		cfdna_panel_selector->addItems(panel_text);

		// create dlg
		QString message_text = "<br/>A personalized cfDNA panel file for the processed sample " + processed_sample_name_ + " already exists.<br/>"
				+ "<br/>Select the cfDNA panel which should be loaded or press 'cancel' to create a new panel:";
		auto dlg = GUIHelper::createDialog(cfdna_panel_selector, "Personalized cfDNA panel for " + processed_sample_name_ + " found", message_text, true);
		int btn = dlg->exec();
		if (btn!=1)
		{
			return;
		}
		cfdna_panel_info_ = previous_panels.at(cfdna_panel_selector->currentIndex());

		// load previous panel
		VcfFile prev_panel = NGSD().cfdnaPanelVcf(cfdna_panel_info_.id);
		for (int i = 0; i < prev_panel.count(); ++i)
		{
			const VcfLine& var = prev_panel.vcfLine(i);
			// create vcf pos string
			QString vcf_pos = var.chr().strNormalized(true) + ":" + QString::number(var.start()) + " " + var.ref() + ">" + var.altString();
			prev_vars_.insert(vcf_pos, false);
		}
		//load genes, KASP and hotspot regions
		BedFile prev_panel_regions =  NGSD().cfdnaPanelRegions(cfdna_panel_info_.id);
		prev_kasp_ = false;
		for (int i = 0; i < prev_panel_regions.count(); ++i)
		{
			const BedLine& bed_line = prev_panel_regions[i];
			const QByteArray& annotation = bed_line.annotations().at(0);
			//genes
			if (annotation.startsWith("gene:"))
			{
				prev_genes_.insert(annotation.split(':').at(1).trimmed());
			}
			//KASP
			else if (annotation.startsWith("SNP_for_sample_identification:KASP_set2"))
			{
				prev_kasp_ = true;
			}
			//hotspots
			else if (annotation.startsWith("hotspot_region:"))
			{
				prev_hotspots_.insert(bed_line.toString(true), false);
			}
		}

	}

	//activate KASP identifier
	ui_->cb_sample_identifier->setCheckState((prev_kasp_)?Qt::Checked:Qt::Unchecked);

	//preselect processing system
	ui_->cb_processing_system->setCurrentText(cfdna_panel_info_.processing_system);

	//update duplicate check
	updateSystemSelection();
}

void CfDNAPanelDesignDialog::loadVariants()
{
	// load reference
	FastaFileIndex genome_reference(Settings::string("reference_genome", false));


	// set dimensions
	ui_->vars->setRowCount(variants_.count());
	ui_->vars->setColumnCount(12);

	//create header
	int col_idx = 0;
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("select"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Select all variants which should be added to the cfDNA panel. Right click item to (de-)select all variants.");

	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("chr"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Chromosome the variant is located on.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("start"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Start position of the variant on the chromosome.\nFor insertions, the position of the base before the insertion is shown.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("end"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("End position of the the variant on the chromosome.\nFor insertions, the position of the base before the insertion is shown.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("ref"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Reference bases in the reference genome at the variant position.\n`-` in case of an insertion.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("obs"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Alternate bases observed in the sample.\n`-` in case of an deletion.");

	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("gene"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Affected gene list (comma-separated).");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("tumor_af"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Mutant allele frequency in tumor.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("tumor_dp"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Tumor Depth.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("normal_af"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Mutant allele frequency in normal.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("normal_dp"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Normal depth.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("info"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Additional variant info.");

	// get indices of report config
	QList<int> report_config_indices = somatic_report_configuration_.variantIndices(VariantType::SNVS_INDELS, false);

	// get af/dp indices
	int tumor_af_idx = variants_.annotationIndexByName("tumor_af");
	int tumor_dp_idx = variants_.annotationIndexByName("tumor_dp");
	int normal_af_idx = variants_.annotationIndexByName("normal_af");
	int normal_dp_idx = variants_.annotationIndexByName("normal_dp");
	int gene_idx = variants_.annotationIndexByName("gene");


	// load filtered variant list
	int row_idx = 0;
	for (int i=0; i<variants_.count(); ++i)
	{
		bool preselect = false;
		bool missing_in_cur_filter_set = false;

		const Variant& variant = variants_[i];

		// check if var is present in previous panel
		VariantVcfRepresentation vcf_rep = variant.toVCF(genome_reference);
		QString vcf_pos = vcf_rep.chr.str() + ":" + QString::number(vcf_rep.pos) + " " + vcf_rep.ref + ">" + vcf_rep.alt;
		if(prev_vars_.contains(vcf_pos))
		{
			preselect = true;
			prev_vars_[vcf_pos] = true;
		}

		// skip variants which are filtered out in the main window
		if (!filter_result_.flags()[i])
		{
			if(!preselect)
			{
				continue;
			}
			else
			{
				missing_in_cur_filter_set = true;
			}
		}



		// get report config for variant
		SomaticReportVariantConfiguration var_conf;
		if (report_config_indices.contains(i))
		{
			var_conf = somatic_report_configuration_.variantConfig(i, VariantType::SNVS_INDELS);
		}

		// create table
		int col_idx = 0;




		QTableWidgetItem* select_item = GUIHelper::createTableItem("");
		select_item->setFlags(select_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		select_item->setCheckState((preselect) ? Qt::Checked : Qt::Unchecked);


		ui_->vars->setItem(row_idx, col_idx++, select_item);

		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.chr().str()));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(QByteArray::number(variant.start())));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(QByteArray::number(variant.end())));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.ref()));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.obs()));

		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[gene_idx]));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[tumor_af_idx]));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[tumor_dp_idx]));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[normal_af_idx]));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[normal_dp_idx]));

		if (missing_in_cur_filter_set)
		{
			QTableWidgetItem* info_item = GUIHelper::createTableItem("from loaded cfDNA panel");
			info_item->setToolTip("This variant is part of the loaded cfDNA panel, but does not match the current filter settings.");
			ui_->vars->setItem(row_idx, col_idx++, info_item);
		}


		// vertical header
		QTableWidgetItem* item = GUIHelper::createTableItem(QByteArray::number(i+1));
		item->setData(Qt::UserRole, i); //store variant index in user data (for selection methods)
		if (report_config_indices.contains(i))
		{
			item->setIcon(QIcon(var_conf.showInReport() ? QPixmap(":/Icons/Report_add.png") : QPixmap(":/Icons/Report exclude.png")));
		}
		ui_->vars->setVerticalHeaderItem(row_idx, item);

		// increase row index
		row_idx++;
	}

	// resize after filling the table:
	ui_->vars->setRowCount(row_idx);

	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->vars, 150);

	// connect checkBoxes to update method
	connect(ui_->vars, SIGNAL(cellChanged(int,int)), this, SLOT(updateSelectedVariantCount()));

	// init selection label
	updateSelectedVariantCount();

	// check if all previous variants were found in VariantList
	QStringList missing_prev_vars;
	foreach (const QString& vcf_string, prev_vars_.keys())
	{
		if (!prev_vars_.value(vcf_string)) missing_prev_vars.append(vcf_string);
	}

	if(missing_prev_vars.size() > 0)
	{
		QMessageBox::warning(this, "Variant not found",
							 "The following variants are part of the loaded cfDNA panel, but are missing in the current variant list:\n\n"
							 + missing_prev_vars.join("\n"));
	}


}

void CfDNAPanelDesignDialog::loadHotspotRegions()
{
	// open BED file
	BedFile hotspot_regions;
	hotspot_regions.load("://Resources/cfDNA_hotspot_regions.bed");

	// fill table

	// set dimensions
	ui_->hotspot_regions->setRowCount(hotspot_regions.count());
	ui_->hotspot_regions->setColumnCount(5);

	//create header
	int col_idx = 0;
	ui_->hotspot_regions->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("select"));
	ui_->hotspot_regions->horizontalHeaderItem(col_idx++)->setToolTip("Select all hotspot regions which should be added to the cfDNA panel.");

	ui_->hotspot_regions->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("chr"));
	ui_->hotspot_regions->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("start"));
	ui_->hotspot_regions->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("end"));
	ui_->hotspot_regions->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("gene"));

	// fill table
	for (int i = 0; i < hotspot_regions.count(); ++i)
	{
		int col_idx = 0;
		const BedLine& line = hotspot_regions[i];

		QTableWidgetItem* select_item = GUIHelper::createTableItem("");
		select_item->setFlags(select_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		// precheck if in loaded panel
		if (prev_hotspots_.contains(line.toString(true)))
		{
			select_item->setCheckState(Qt::Checked);
			prev_hotspots_[line.toString(true)] = true;

		}
		else
		{
			select_item->setCheckState(Qt::Unchecked);
		}

		// store file path in first cell
		select_item->setData(Qt::UserRole, i);

		ui_->hotspot_regions->setItem(i, col_idx++, select_item);

		ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(line.chr().strNormalized(true)));
		ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(QString::number(line.start())));
		ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(QString::number(line.end())));
		if (line.annotations().size() > 0) ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(line.annotations().at(0)));
	}

	// activate hotspot region view
	if (prev_hotspots_.size() > 0) ui_->cb_hotspot_regions->setCheckState(Qt::Checked);

	// check if all previous previous hotspot were found
	QStringList missing_prev_hotspots;
	foreach (const QString& region, prev_hotspots_.keys())
	{
		if (!prev_hotspots_.value(region)) missing_prev_hotspots.append(region);
	}

	if(missing_prev_hotspots.size() > 0)
	{
		QMessageBox::warning(this, "Hotspot region not found", QString() + "The following hotspot regions are part of the loaded cfDNA panel, "
							 + "but are missing in the current hotspot list \n(Maybe the hotspot list has been updated):\n\n"
							 + missing_prev_hotspots.join("\n"));
	}

	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->hotspot_regions, 250);

	// connect checkBoxes to update method
	connect(ui_->hotspot_regions, SIGNAL(cellChanged(int,int)), this, SLOT(updateSelectedHotspotCount()));

	// init selection label
	showHotspotRegions(ui_->cb_hotspot_regions->checkState());
	updateSelectedHotspotCount();
}

void CfDNAPanelDesignDialog::loadGenes()
{
	// get all genes from NGSD
	genes_=NGSD().cfdnaGenes();

	// create table

	// set dimensions
	ui_->genes->setRowCount(genes_.length());
	ui_->genes->setColumnCount(4);

	//create header
	int col_idx = 0;
	ui_->genes->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("select"));
	ui_->genes->horizontalHeaderItem(col_idx++)->setToolTip("Check all genes which should be added to the cfDNA panel.");

	ui_->genes->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("gene"));
	ui_->genes->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("region"));
	ui_->genes->setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("file date"));

	// fill table
	int r = 0;
	foreach (const CfdnaGeneEntry& entry, genes_)
	{
		QTableWidgetItem* select_item = GUIHelper::createTableItem("");
		select_item->setFlags(select_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		// precheck if in loaded panel
		if (prev_genes_.contains(entry.gene_name))
		{
			select_item->setCheckState(Qt::Checked);
		}
		else
		{
			select_item->setCheckState(Qt::Unchecked);
		}


		// store file path in first cell
		select_item->setData(Qt::UserRole, r);

		ui_->genes->setItem(r, 0, select_item);
		ui_->genes->setItem(r, 1, GUIHelper::createTableItem(entry.gene_name));
		ui_->genes->setItem(r, 2, GUIHelper::createTableItem(BedLine(entry.chr, entry.start, entry.end).toString(true)));
		ui_->genes->setItem(r, 3, GUIHelper::createTableItem(entry.date.toString("dd.MM.yyyy")));
		r++;
	}

	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->genes, 150);
}

void CfDNAPanelDesignDialog::selectAllVariants(bool deselect)
{
	for (int r = 0; r < ui_->vars->rowCount(); ++r)
	{
		ui_->vars->item(r, 0)->setCheckState((!deselect) ? Qt::Checked : Qt::Unchecked);
	}
}

void CfDNAPanelDesignDialog::selectAllHotspotRegions(bool deselect)
{
	for (int r = 0; r < ui_->hotspot_regions->rowCount(); ++r)
	{
		ui_->hotspot_regions->item(r, 0)->setCheckState((!deselect) ? Qt::Checked : Qt::Unchecked);
	}
}

void CfDNAPanelDesignDialog::selectAllGenes(bool deselect)
{
	for (int r = 0; r < ui_->genes->rowCount(); ++r)
	{
		ui_->genes->item(r, 0)->setCheckState((!deselect) ? Qt::Checked : Qt::Unchecked);
	}
}

void CfDNAPanelDesignDialog::updateSelectedVariantCount()
{
	int selected_variant_count = 0;
	// get all selected variants
	for (int r = 0; r < ui_->vars->rowCount(); ++r)
	{
		if (ui_->vars->item(r, 0)->checkState() == Qt::Checked)
		{
			selected_variant_count++;
		}
	}

	ui_->l_count_variables->setText(QString::number(selected_variant_count) + " / " + QString::number(ui_->vars->rowCount()));
}

void CfDNAPanelDesignDialog::updateSelectedHotspotCount()
{
	int selected_hotspot_count = 0;
	// get all selected hotspots
	for (int r = 0; r < ui_->hotspot_regions->rowCount(); ++r)
	{
		if (ui_->hotspot_regions->item(r, 0)->checkState() == Qt::Checked)
		{
			selected_hotspot_count++;
		}
	}

	ui_->l_count_hotspot_regions->setText(QString::number(selected_hotspot_count) + " / " + QString::number(ui_->hotspot_regions->rowCount()));
}

void CfDNAPanelDesignDialog::openVariantInIGV(QTableWidgetItem* item)
{
	if (item==nullptr) return;
	int r = item->row();

	// get variant index
	bool ok;
	int var_idx = ui_->vars->verticalHeaderItem(r)->data(Qt::UserRole).toInt(&ok);
	if (!ok) THROW(ProgrammingException, "Variant table row header user data '" + ui_->vars->verticalHeaderItem(r)->data(Qt::UserRole).toString() + "' is not an integer!");

	const Variant& var = variants_[var_idx];
	QString coords = var.chr().strNormalized(true) + ":" + QString::number(var.start()) + "-" + QString::number(var.end());
	emit openInIGV(coords);
}

void CfDNAPanelDesignDialog::updateSystemSelection()
{
	// skip on loaded panels
	if ((cfdna_panel_info_.id != -1) && (ui_->cb_processing_system->currentText().toUtf8() == cfdna_panel_info_.processing_system))
	{
		ui_->l_error_message->setVisible(false);
		ui_->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
		return;
	}

	// get processing system id
	int sys_id = NGSD().processingSystemId(ui_->cb_processing_system->currentText());

	bool panel_exists = (NGSD().cfdnaPanelInfo(processed_sample_id_, QString::number(sys_id)).size() > 0);

	// (de-)activate OK button and error message
	ui_->l_error_message->setVisible(panel_exists);
	ui_->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!panel_exists);
}

void CfDNAPanelDesignDialog::createOutputFiles()
{

	VariantList selected_variants;
	int variant_count = 0;

	// copy header
	selected_variants.copyMetaData(variants_);

	// get all selected variants
	for (int r = 0; r < ui_->vars->rowCount(); ++r)
	{
		if (ui_->vars->item(r, 0)->checkState() == Qt::Checked)
		{
			// get variant index
			bool ok;
			int var_idx = ui_->vars->verticalHeaderItem(r)->data(Qt::UserRole).toInt(&ok);
			if (!ok) THROW(ProgrammingException, "Variant table row header user data '" + ui_->vars->verticalHeaderItem(r)->data(Qt::UserRole).toString() + "' is not an integer!");

			selected_variants.append(variants_[var_idx]);
			variant_count++;
		}
	}

	// get all selected hotspot regions
	BedFile roi;
	if (ui_->cb_hotspot_regions->checkState() == Qt::Checked)
	{
		for (int r = 0; r < ui_->hotspot_regions->rowCount(); ++r)
		{
			if (ui_->hotspot_regions->item(r, 0)->checkState() == Qt::Checked)
			{
				// parse entry
				Chromosome chr = Chromosome(ui_->hotspot_regions->item(r, 1)->text());
				int start  = Helper::toInt(ui_->hotspot_regions->item(r, 2)->text(), "start", QString::number(r));
				int end = Helper::toInt(ui_->hotspot_regions->item(r, 3)->text(), "end", QString::number(r));
				QByteArrayList annotations;
				annotations << "hotspot_region:" + ui_->hotspot_regions->item(r, 4)->text().toUtf8();
				roi.append(BedLine(chr, start, end, annotations));
				variant_count++;
			}
		}
	}

	// get all selected genes
	for (int r = 0; r < ui_->genes->rowCount(); ++r)
	{
		if (ui_->genes->item(r, 0)->checkState() == Qt::Checked)
		{
			int idx = ui_->genes->item(r, 0)->data(Qt::UserRole).toInt();
			QString gene_name = ui_->genes->item(r, 1)->text();

			// add to overall gene list
			QByteArrayList annotations;
			annotations.append("gene:" + gene_name.toUtf8());
			const BedFile& bed = genes_.at(idx).bed;
			for (int i = 0; i < bed.count(); ++i)
			{
				roi.append(BedLine(bed[i].chr(), bed[i].start(), bed[i].end(), annotations));
			}
		}
	}

	// get KASP variants
	if (ui_->cb_sample_identifier->isChecked())
	{
		BedFile kasp_variants;
		kasp_variants.load("://Resources/KASP_set2_pad5.bed");
		kasp_variants.clearAnnotations();
		variant_count += kasp_variants.count();
		for (int i=0; i<kasp_variants.count(); i++)
		{
			BedLine& kasp_variant = kasp_variants[i];
			kasp_variant.annotations().append("SNP_for_sample_identification:KASP_set2");
			roi.append(kasp_variant);
		}
	}

	// generate output VCF
	QString ref_genome = Settings::string("reference_genome", false);
	VcfFile vcf_file = VcfFile::convertGSvarToVcf(selected_variants, ref_genome);

	// generate bed file
	for (int i=0; i<vcf_file.count(); i++)
	{
		roi.append(BedLine(vcf_file[i].chr(), vcf_file[i].start(), vcf_file[i].end(), QByteArrayList() << "patient_specific_somatic_variant:" + vcf_file[i].ref() + ">" + vcf_file[i].altString()));
	}


	// check number of selected variants
	if (variant_count < 25)
	{
		int btn = QMessageBox::information(this, "Too few variants selected", "Only " + QByteArray::number(variant_count)
										   + " variants selected. A cfDNA panel should contain at least 25 variants. \nDo you want to continue?",
										   QMessageBox::Yes, QMessageBox::Cancel);
		if (btn!=QMessageBox::Yes)
		{
			return;
		}
	}

	// store variant list in data base
	roi.sort();

	cfdna_panel_info_.tumor_id = processed_sample_id_.toInt();
	cfdna_panel_info_.created_by = LoginManager::userName().toUtf8();
	cfdna_panel_info_.created_date = QDate::currentDate();
	cfdna_panel_info_.processing_system = ui_->cb_processing_system->currentText().toUtf8();

	NGSD().storeCfdnaPanel(cfdna_panel_info_, roi.toText().toUtf8(), vcf_file.toText());


	emit accept();
}

void CfDNAPanelDesignDialog::showVariantContextMenu(QPoint pos)
{
	// create menu
	QMenu menu(ui_->vars);
	QAction* a_select_all = menu.addAction("Select all variants");
	QAction* a_deselect_all = menu.addAction("Deselect all variants");
	QAction* a_open_igv = menu.addAction("Open variant in IGV");
	// execute menu
	QAction* action = menu.exec(ui_->vars->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;
	// react
	if (action == a_select_all)
	{
		selectAllVariants(false);
	}
	else if (action == a_deselect_all)
	{
		selectAllVariants(true);
	}
	else if (action == a_open_igv)
	{
		openVariantInIGV(ui_->vars->itemAt(pos));
	}
	else
	{
		THROW(ProgrammingException, "Invalid menu action in context menu selected!")
	}
}

void CfDNAPanelDesignDialog::showHotspotContextMenu(QPoint pos)
{
	// create menu
	QMenu menu(ui_->hotspot_regions);
	QAction* a_select_all = menu.addAction("Select all hotspot regions");
	QAction* a_deselect_all = menu.addAction("Deselect all hotspot regions");
	// execute menu
	QAction* action = menu.exec(ui_->hotspot_regions->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;
	// react
	if (action == a_select_all)
	{
		selectAllHotspotRegions(false);
	}
	else if (action == a_deselect_all)
	{
		selectAllHotspotRegions(true);
	}
	else
	{
		THROW(ProgrammingException, "Invalid menu action in context menu selected!")
	}
}

void CfDNAPanelDesignDialog::showGeneContextMenu(QPoint pos)
{
	// create menu
	QMenu menu(ui_->genes);
	QAction* a_select_all = menu.addAction("Select all genes");
	QAction* a_deselect_all = menu.addAction("Deselect all genes");
	// execute menu
	QAction* action = menu.exec(ui_->genes->viewport()->mapToGlobal(pos));
	if (action == nullptr) return;
	// react
	if (action == a_select_all)
	{
		selectAllGenes(false);
	}
	else if (action == a_deselect_all)
	{
		selectAllGenes(true);
	}
	else
	{
		THROW(ProgrammingException, "Invalid menu action in context menu selected!")
	}
}

void CfDNAPanelDesignDialog::showHotspotRegions(int state)
{
	ui_->hotspot_regions->setVisible(state == Qt::Checked);
	ui_->l_count_hotspot_regions->setVisible(state == Qt::Checked);
}

