#include "CfDNAPanelDesignDialog.h"
#include "ui_CfDNAPanelDesignDialog.h"
#include <QMessageBox>
#include <GUIHelper.h>
#include <QDir>

CfDNAPanelDesignDialog::CfDNAPanelDesignDialog(const VariantList& variants, const SomaticReportConfiguration& somatic_report_configuration, const QString& processed_sample_name, const QString& system_name, QWidget *parent) :
	QDialog(parent),
	ui_(new Ui::CfDNAPanelDesignDialog),
	variants_(variants),
	processed_sample_name_(processed_sample_name),
	system_name_(system_name),
	somatic_report_configuration_(somatic_report_configuration)
{
	// remove '?' entry
	setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));

	ui_->setupUi(this);

	// connect signals and slots
	connect(ui_->buttonBox, SIGNAL(accepted()), this, SLOT(createOutputFiles()));


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
	ui_->vars->setColumnCount(9);

	//create header
	ui_->vars->setHorizontalHeaderItem(0, GUIHelper::createTableItem("chr"));
	ui_->vars->horizontalHeaderItem(0)->setToolTip("Chromosome the variant is located on.");
	ui_->vars->setHorizontalHeaderItem(1, GUIHelper::createTableItem("start"));
	ui_->vars->horizontalHeaderItem(1)->setToolTip("Start position of the variant on the chromosome.\nFor insertions, the position of the base before the insertion is shown.");
	ui_->vars->setHorizontalHeaderItem(2, GUIHelper::createTableItem("end"));
	ui_->vars->horizontalHeaderItem(2)->setToolTip("End position of the the variant on the chromosome.\nFor insertions, the position of the base before the insertion is shown.");
	ui_->vars->setHorizontalHeaderItem(3, GUIHelper::createTableItem("ref"));
	ui_->vars->horizontalHeaderItem(3)->setToolTip("Reference bases in the reference genome at the variant position.\n`-` in case of an insertion.");
	ui_->vars->setHorizontalHeaderItem(4, GUIHelper::createTableItem("obs"));
	ui_->vars->horizontalHeaderItem(4)->setToolTip("Alternate bases observed in the sample.\n`-` in case of an deletion.");

	ui_->vars->setHorizontalHeaderItem(5, GUIHelper::createTableItem("tumor_af"));
	ui_->vars->horizontalHeaderItem(5)->setToolTip("Mutant allele frequency in tumor.");
	ui_->vars->setHorizontalHeaderItem(6, GUIHelper::createTableItem("tumor_dp"));
	ui_->vars->horizontalHeaderItem(6)->setToolTip("Tumor Depth.");
	ui_->vars->setHorizontalHeaderItem(7, GUIHelper::createTableItem("normal_af"));
	ui_->vars->horizontalHeaderItem(7)->setToolTip("Mutant allele frequency in normal.");
	ui_->vars->setHorizontalHeaderItem(8, GUIHelper::createTableItem("normal_dp"));
	ui_->vars->horizontalHeaderItem(8)->setToolTip("Normal depth.");

	// get indices of report config
	QList<int> report_config_indices = somatic_report_configuration_.variantIndices(VariantType::SNVS_INDELS, false);

	// get af/dp indices
	int tumor_af_idx = variants_.annotationIndexByName("tumor_af");
	int tumor_dp_idx = variants_.annotationIndexByName("tumor_dp");
	int normal_af_idx = variants_.annotationIndexByName("normal_af");
	int normal_dp_idx = variants_.annotationIndexByName("normal_dp");


	// load filtered variant list
	int r = 0;
	for (int i=0; i<variants_.count(); ++i)
	{
		const Variant& variant = variants_[i];

		// filter variants by filter column
		if (variant.filters().length() != 0) continue;

		// filter variants by report config
		SomaticReportVariantConfiguration var_conf;
		if (report_config_indices.contains(i))
		{
			var_conf = somatic_report_configuration_.variantConfig(i, VariantType::SNVS_INDELS);
			if (var_conf.exclude_artefact) continue;
		}


		QTableWidgetItem* chr_item = GUIHelper::createTableItem(variant.chr().str());
		chr_item->setFlags(chr_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		chr_item->setCheckState(Qt::Unchecked);

		ui_->vars->setItem(r, 0, chr_item);
		ui_->vars->setItem(r, 1, GUIHelper::createTableItem(QByteArray::number(variant.start())));
		ui_->vars->setItem(r, 2, GUIHelper::createTableItem(QByteArray::number(variant.end())));
		ui_->vars->setItem(r, 3, GUIHelper::createTableItem(variant.ref()));
		ui_->vars->setItem(r, 4, GUIHelper::createTableItem(variant.obs()));

		ui_->vars->setItem(r, 5, GUIHelper::createTableItem(variant.annotations()[tumor_af_idx]));
		ui_->vars->setItem(r, 6, GUIHelper::createTableItem(variant.annotations()[tumor_dp_idx]));
		ui_->vars->setItem(r, 7, GUIHelper::createTableItem(variant.annotations()[normal_af_idx]));
		ui_->vars->setItem(r, 8, GUIHelper::createTableItem(variant.annotations()[normal_dp_idx]));

		// vertical header
		QTableWidgetItem* item = GUIHelper::createTableItem(QByteArray::number(i+1));
		item->setData(Qt::UserRole, i); //store variant index in user data (for selection methods)
		if (report_config_indices.contains(i))
		{
			item->setIcon(QIcon(var_conf.showInReport() ? QPixmap(":/Icons/Report_add.png") : QPixmap(":/Icons/Report exclude.png")));
		}
		ui_->vars->setVerticalHeaderItem(r, item);

		// increase row index
		r++;
	}

	// resize after filling the table:
	ui_->vars->setRowCount(r);

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
	ui_->genes->setColumnCount(3);

	// create header
	ui_->genes->setHorizontalHeaderItem(0, GUIHelper::createTableItem("gene"));
	ui_->genes->setHorizontalHeaderItem(1, GUIHelper::createTableItem("region"));
	ui_->genes->setHorizontalHeaderItem(2, GUIHelper::createTableItem("file date"));

	// fill table
	int r = 0;
	foreach (const GeneEntry& entry, genes_)
	{
		QTableWidgetItem* gene_item = GUIHelper::createTableItem(entry.gene_name);
		gene_item->setFlags(gene_item->flags() | Qt::ItemIsUserCheckable); // add checkbox
		gene_item->setCheckState(Qt::Unchecked);

		// store file path in first cell
		gene_item->setData(Qt::UserRole, entry.file_path);

		ui_->genes->setItem(r, 0, gene_item);
		ui_->genes->setItem(r, 1, GUIHelper::createTableItem(BedLine(entry.chr, entry.start, entry.end).toString(true)));
		ui_->genes->setItem(r, 2, GUIHelper::createTableItem(entry.date.toString("dd.MM.yyyy")));
		r++;
	}
}

void CfDNAPanelDesignDialog::createOutputFiles()
{

	selected_variants_.clear();

	// copy header
	selected_variants_.copyMetaData(variants_);

	// get all selected variants
	for (int r = 0; r < ui_->genes->rowCount(); ++r)
	{
		if (ui_->genes->item(r, 0)->checkState() == Qt::Checked)
		{
			// get variant index
			bool ok;
			int var_idx = ui_->vars->verticalHeaderItem(r)->data(Qt::UserRole).toInt(&ok);
			if (!ok) THROW(ProgrammingException, "Variant table row header user data '" + ui_->vars->verticalHeaderItem(r)->data(Qt::UserRole).toString() + "' is not an integer!");

			selected_variants_.append(variants_[var_idx]);
		}
	}


	// get all selected genes
	BedFile roi;
	for (int r = 0; r < ui_->genes->rowCount(); ++r)
	{
		if (ui_->genes->item(r, 0)->checkState() == Qt::Checked)
		{
			QString file_path = ui_->genes->item(r, 0)->data(Qt::UserRole).toString();
			QString gene_name = ui_->genes->item(r, 0)->text();

			// load single gene bed file
			BedFile gene;
			gene.load(file_path);
			gene.clearAnnotations();

			// add to overall gene list
			QByteArrayList annotations;
			annotations << "GENE" << gene_name.toUtf8();
			for (int i = 0; i < gene.count(); ++i)
			{
				roi.append(BedLine(gene[i].chr(), gene[i].start(), gene[i].end(), annotations));
			}
		}
	}


	// generate bed file
	for (int i=0; i<selected_variants_.count(); i++)
	{
		BedLine bed_line = BedLine(selected_variants_[i].chr(), selected_variants_[i].start(), selected_variants_[i].end(), QByteArrayList() << "SNP_INDEL" << selected_variants_[i].ref() + ">" + selected_variants_[i].obs());
		roi.append(bed_line);
	}

	QString output_path = Settings::string("patient_specific_panel_folder", false);
	output_path += system_name_ + "/";

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
			emit reject();
			return;
		}
	}
	// store variant list
	QString ref_genome = Settings::string("reference_genome", false);
	selected_variants_.storeAsVCF(output_path + processed_sample_name_ + ".vcf", ref_genome, 0);
	roi.sort();
	roi.store(output_path + processed_sample_name_ + ".bed");

	emit accept();
}
