#include "CfDNAPanelDesignDialog.h"
#include "ui_CfDNAPanelDesignDialog.h"
#include "GUIHelper.h"
#include <QMessageBox>
#include <QMenu>
#include <QDir>


CfDNAPanelDesignDialog::CfDNAPanelDesignDialog(const VariantList& variants, const SomaticReportConfiguration& somatic_report_configuration, const QString& processed_sample_name, const DBTable& processing_systems, QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::CfDNAPanelDesignDialog),
	variants_(variants),
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

	// set context menus for tables
	ui_->vars->setContextMenuPolicy(Qt::CustomContextMenu);
	ui_->genes->setContextMenuPolicy(Qt::CustomContextMenu);

	// fill processing system ComboBox
	ui_->cb_processing_system->fill(processing_systems, false);

	loadVariants();
	loadGenes();
}

CfDNAPanelDesignDialog::~CfDNAPanelDesignDialog()
{
	delete ui_;
}

void CfDNAPanelDesignDialog::loadVariants()
{
	// set dimensions
	ui_->vars->setRowCount(variants_.count());
	ui_->vars->setColumnCount(10);

	//create header
	int col_idx = 0;
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("select"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Select all variants which should be added to the cfDNA panel.");

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

	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("tumor_af"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Mutant allele frequency in tumor.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("tumor_dp"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Tumor Depth.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("normal_af"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Mutant allele frequency in normal.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("normal_dp"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Normal depth.");

	// get indices of report config
	QList<int> report_config_indices = somatic_report_configuration_.variantIndices(VariantType::SNVS_INDELS, false);

	// get af/dp indices
	int tumor_af_idx = variants_.annotationIndexByName("tumor_af");
	int tumor_dp_idx = variants_.annotationIndexByName("tumor_dp");
	int normal_af_idx = variants_.annotationIndexByName("normal_af");
	int normal_dp_idx = variants_.annotationIndexByName("normal_dp");


	// load filtered variant list
	int row_idx = 0;
	for (int i=0; i<variants_.count(); ++i)
	{
		const Variant& variant = variants_[i];
		int col_idx = 0;

		// filter variants by filter column
		if (variant.filters().length() != 0) continue;

		// filter variants by report config
		SomaticReportVariantConfiguration var_conf;
		if (report_config_indices.contains(i))
		{
			var_conf = somatic_report_configuration_.variantConfig(i, VariantType::SNVS_INDELS);
			if (var_conf.exclude_artefact) continue;
		}


		QTableWidgetItem* select_item = GUIHelper::createTableItem("");
		select_item->setFlags(select_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		select_item->setCheckState(Qt::Unchecked);

		ui_->vars->setItem(row_idx, col_idx++, select_item);

		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.chr().str()));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(QByteArray::number(variant.start())));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(QByteArray::number(variant.end())));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.ref()));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.obs()));

		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[tumor_af_idx]));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[tumor_dp_idx]));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[normal_af_idx]));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.annotations()[normal_dp_idx]));

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
}

void CfDNAPanelDesignDialog::selectAllVariants(bool deselect)
{
	for (int r = 0; r < ui_->vars->rowCount(); ++r)
	{
		ui_->vars->item(r, 0)->setCheckState((!deselect) ? Qt::Checked : Qt::Unchecked);
	}
}

void CfDNAPanelDesignDialog::selectAllGenes(bool deselect)
{
	for (int r = 0; r < ui_->genes->rowCount(); ++r)
	{
		ui_->genes->item(r, 0)->setCheckState((!deselect) ? Qt::Checked : Qt::Unchecked);
	}
}

void CfDNAPanelDesignDialog::createOutputFiles()
{

	VariantList selected_variants;

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
		}
	}


	// get all selected genes
	BedFile roi;
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
			annotations.append("GENE:" + gene_name.toUtf8());
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
		for (int i=0; i<kasp_variants.count(); i++)
		{
			BedLine& kasp_variant = kasp_variants[i];
			kasp_variant.annotations().append("Sample_Identifier:KASP_set2");
			roi.append(kasp_variant);
		}
	}


	// generate output VCF
	QString ref_genome = Settings::string("reference_genome", false);
	VcfFile vcf_file = VcfFile::convertGSvarToVcf(selected_variants, ref_genome);

	// generate bed file
	for (int i=0; i<vcf_file.count(); i++)
	{
		roi.append(BedLine(vcf_file[i].chr(), vcf_file[i].start(), vcf_file[i].end(), QByteArrayList() << "SNP_INDEL:" + vcf_file[i].ref() + ">" + vcf_file[i].altString()));
	}

	QString output_path = Settings::string("patient_specific_panel_folder", false);
	output_path += ui_->cb_processing_system->currentText() + "/";

	// create output folder if it not exists
	if (!QDir(output_path).exists()) QDir().mkdir(output_path);

	// check if panel already exists
	if (QFile::exists(output_path + processed_sample_name_ + ".vcf") || QFile::exists(output_path + processed_sample_name_ + ".vcf"))
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
	vcf_file.store(output_path + processed_sample_name_ + ".vcf");
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
