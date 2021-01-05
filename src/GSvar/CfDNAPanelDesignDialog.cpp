#include "CfDNAPanelDesignDialog.h"
#include "ui_CfDNAPanelDesignDialog.h"
#include "GUIHelper.h"
#include <QMessageBox>
#include <QMenu>
#include <QDir>


CfDNAPanelDesignDialog::CfDNAPanelDesignDialog(const VariantList& variants, const FilterResult& filter_result, const SomaticReportConfiguration& somatic_report_configuration, const QString& processed_sample_name, const DBTable& processing_systems, QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::CfDNAPanelDesignDialog),
	variants_(variants),
	filter_result_(filter_result),
	somatic_report_configuration_(somatic_report_configuration),
	processed_sample_name_(processed_sample_name)
{
	// remove '?' entry
	setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

	ui_->setupUi(this);

	// connect signals and slots
	connect(ui_->buttonBox, SIGNAL(accepted()), this, SLOT(createOutputFiles()));
	connect(ui_->vars,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showVariantContextMenu(QPoint)));
	connect(ui_->genes,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showGeneContextMenu(QPoint)));
	connect(ui_->cb_hotspot_regions, SIGNAL(stateChanged(int)), this, SLOT(showHotspotRegions(int)));
	connect(ui_->vars,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(openVariantInIGV(QTableWidgetItem*)));

	// set context menus for tables
	ui_->vars->setContextMenuPolicy(Qt::CustomContextMenu);
	ui_->genes->setContextMenuPolicy(Qt::CustomContextMenu);

	// fill processing system ComboBox
	ui_->cb_processing_system->fill(processing_systems, false);


	loadPreviousPanels(processing_systems);
	loadVariants();
	loadHotspotRegions();
	loadGenes();
}

CfDNAPanelDesignDialog::~CfDNAPanelDesignDialog()
{
	delete ui_;
}

void CfDNAPanelDesignDialog::loadPreviousPanels(const DBTable& processing_systems)
{
	QStringList previous_panel_files;
	for (int i = 0; i < processing_systems.rowCount(); ++i)
	{
		// check if previous panel exists
		QString processing_system = processing_systems.row(i).value(0);
		QString output_path = Settings::string("patient_specific_panel_folder", false) + processing_system + "/" +  processed_sample_name_ + ".vcf";
		if(QFile::exists(output_path)) previous_panel_files.append(output_path);
	}

	if(previous_panel_files.size() > 0)
	{

		QString message_text = "A personalized cfDNA panel file for the processed sample " + processed_sample_name_ + " already exists.\n";
		int load_previous_panel = QMessageBox::information(this, "cfDNA panel found", message_text + "Would you like to load the previous panel?\n\n"
														   + previous_panel_files.join("\n"), QMessageBox::Yes, QMessageBox::Cancel);
		if (load_previous_panel!=QMessageBox::Yes)
		{
			return;
		}
		QString selected_panel;
		if(previous_panel_files.size() > 1)
		{
			QComboBox* vcf_file_selector = new QComboBox(this);
			vcf_file_selector->addItems(previous_panel_files);

			// create dlg
			auto dlg = GUIHelper::createDialog(vcf_file_selector, "Select cfDNA panel", "Select the cfDNA panel which should be loaded:", true);
			int btn = dlg->exec();
			if (btn!=1)
			{
				return;
			}
			selected_panel = vcf_file_selector->currentText();
		}
		else
		{
			selected_panel = previous_panel_files.at(0);
		}
		// load previous panel
		VcfFile prev_panel;
		prev_panel.load(selected_panel);
		for (int i = 0; i < prev_panel.count(); ++i)
		{
			const VcfLine& var = prev_panel.vcfLine(i);
			// create vcf pos string
			QString vcf_pos = var.chr().strNormalized(true) + ":" + QString::number(var.pos()) + " " + var.ref() + ">" + var.altString();
			prev_vars_.insert(vcf_pos, false);
		}


	}
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
		QStringList vcf_str_list = variant.toVCF(genome_reference).split('\t');
		QString vcf_pos = vcf_str_list[0].trimmed() + ":" + vcf_str_list[1].trimmed() + " " + vcf_str_list[3].trimmed() + ">" +vcf_str_list[4].trimmed();
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


//		// filter variants by filter column
//		if (variant.filters().length() != 0) continue;

		// get report config for variant
		SomaticReportVariantConfiguration var_conf;
		if (report_config_indices.contains(i))
		{
			var_conf = somatic_report_configuration_.variantConfig(i, VariantType::SNVS_INDELS);
			//exclude artifacts
//			if (var_conf.exclude_artefact) continue;
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
		GUIHelper::showMessage("Variant not found",
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
		select_item->setCheckState(Qt::Unchecked);

		// store file path in first cell
		select_item->setData(Qt::UserRole, i);

		ui_->hotspot_regions->setItem(i, col_idx++, select_item);

		ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(line.chr().strNormalized(true)));
		ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(QString::number(line.start())));
		ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(QString::number(line.end())));
		if (line.annotations().size() > 0) ui_->hotspot_regions->setItem(i, col_idx++, GUIHelper::createTableItem(line.annotations().at(0)));
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
	// get all bed files in the genes folder
	QDir gene_folder(Settings::string("patient_specific_panel_folder", false) + "genes/");
	QStringList bed_file_paths = gene_folder.entryList(QStringList() << "*.bed" << "*.BED", QDir::Files);

	// extract info
	foreach (const QString& file_name, bed_file_paths)
	{
		GeneEntry gene_entry;
		gene_entry.file_path = gene_folder.absolutePath() + "/" + file_name;
		QString base_name = QFileInfo(file_name).baseName();
		QStringList parts = base_name.split("_");
		if(parts.length() != 2)
		{
			QMessageBox::warning(this, "Invalid File", "Invalid BED file name '" + file_name + "' found in gene folder. Skipping file.");
			continue;
		}
		gene_entry.gene_name = parts.at(0).trimmed();
		gene_entry.date = QDate::fromString(parts.at(1).trimmed(), "yyyy-MM-dd");

		// load BED file to get gene start and end
		BedFile bed_file;
		bed_file.load(gene_entry.file_path);
		gene_entry.chr = bed_file[0].chr();
		gene_entry.start = bed_file[0].start();
		gene_entry.end = bed_file[bed_file.count()-1].end();

		// add gene bed file to list
		genes_.append(gene_entry);

	}

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
	foreach (const GeneEntry& entry, genes_)
	{
		QTableWidgetItem* select_item = GUIHelper::createTableItem("");
		select_item->setFlags(select_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		select_item->setCheckState(Qt::Unchecked);

		// store file path in first cell
		select_item->setData(Qt::UserRole, entry.file_path);

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
			QString file_path = ui_->genes->item(r, 0)->data(Qt::UserRole).toString();
			QString gene_name = ui_->genes->item(r, 1)->text();

			// load single gene bed file
			BedFile gene;
			gene.load(file_path);
			gene.clearAnnotations();

			// add to overall gene list
			QByteArrayList annotations;
			annotations.append("gene:" + gene_name.toUtf8());
			for (int i = 0; i < gene.count(); ++i)
			{
				roi.append(BedLine(gene[i].chr(), gene[i].start(), gene[i].end(), annotations));
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

	QString output_path = Settings::string("patient_specific_panel_folder", false);
	output_path += ui_->cb_processing_system->currentText() + "/";

	// create output folder if it not exists
	if (!QDir(output_path).exists()) QDir().mkdir(output_path);

	// check if panel already exists
	if (QFile::exists(output_path + processed_sample_name_ + ".vcf") || QFile::exists(output_path + processed_sample_name_ + ".bed"))
	{
		int btn = QMessageBox::information(this, "Panel file already exists", "A personalized cfDNA panel file for the processed sample "
										   + processed_sample_name_ + " already exists.\nWould you like to overide the previous panel?",
										   QMessageBox::Yes, QMessageBox::Cancel);
		if (btn!=QMessageBox::Yes)
		{
			return;
		}
	}

	// store variant list
	vcf_file.store(output_path + processed_sample_name_ + ".vcf", false, 0);
	roi.sort();
	roi.store(output_path + processed_sample_name_ + ".bed");

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

