#include "DiseaseCourseWidget.h"
#include "ui_DiseaseCourseWidget.h"
#include "GUIHelper.h"
#include "Settings.h"
#include "GlobalServiceProvider.h"
#include "VariantHgvsAnnotator.h"
#include <QDir>
#include <QMessageBox>


DiseaseCourseWidget::DiseaseCourseWidget(const QString& tumor_sample_name, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::DiseaseCourseWidget),
	tumor_sample_name_(tumor_sample_name)
{
	ui_->setupUi(this);

	// abort if no connection to NGSD
	if (!LoginManager::active())
	{
		GUIHelper::showMessage("No connection to the NGSD!", "You need access to the NGSD to view the cfDNA samples!");
		close();
	}

	//link signal and slots
	connect(ui_->vars,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(VariantDoubleClicked(QTableWidgetItem*)));
	connect(ui_->btn_copyToClipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));

	createTableView();
}

DiseaseCourseWidget::~DiseaseCourseWidget()
{
	delete ui_;
}

void DiseaseCourseWidget::VariantDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;
	int row = item->row();
	int variant_idx = ui_->vars->verticalHeaderItem(row)->data(Qt::UserRole).toInt();

	const VcfLine& vcf_line = table_data_.lines.at(variant_idx).tumor_vcf_line;
	QString coords = vcf_line.chr().strNormalized(true) + ":" + QString::number(vcf_line.start());
	GlobalServiceProvider::gotoInIGV(coords, true);

	// add cfDNA BAM Files to IGV when executed for the first time
	if (!igv_initialized_)
	{
		foreach (const auto& cfdna_sample, table_data_.cfdna_samples)
		{
			QString bam = GlobalServiceProvider::database().processedSamplePath(cfdna_sample.ps_id, PathType::BAM).filename;
			GlobalServiceProvider::loadFileInIGV(bam, true);
		}
		igv_initialized_ = true;
	}


}

void DiseaseCourseWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->vars);
}


void DiseaseCourseWidget::createTableView()
{
	//get variant infos
	QStringList errors;
	table_data_ = GSvarHelper::cfdnaTable(tumor_sample_name_, errors, false);
	if(errors.length() > 0) QMessageBox::warning(this, "Error creating cfDNA table", "During creation of the cfDNA table the following error(s) occurred:\n\t" + errors.join("\n\t"));

	// clear old table
	ui_->vars->clear();

	// set dimensions
	ui_->vars->setColumnCount(7 + table_data_.cfdna_samples.length()*4);
	ui_->vars->setRowCount(table_data_.lines.length());

	int col_idx = 0;

	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("chr", Qt::AlignBottom));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Chromosome the variant is located on.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("pos", Qt::AlignBottom));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Position of the variant on the chromosome.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("ref", Qt::AlignBottom));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Reference bases in the reference genome at the variant position.\n`-` in case of an insertion.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("obs", Qt::AlignBottom));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Alternate bases observed in the sample.\n`-` in case of an deletion.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("gene", Qt::AlignBottom));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Affected gene list (comma-separated).");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("coding and splicing", Qt::AlignBottom));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Coding and splicing details (Gene, ENST number, type, impact, exon/intron number, HGVS.c, HGVS.p, Pfam domain).");

	// set header for sample
	ui_->vars-> setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem(table_data_.tumor_sample.name + "\n(" + table_data_.tumor_sample.date.toString("dd.MM.yyyy") + ")\nAllele frequency", Qt::AlignBottom));

	// set cfDNA header
	foreach (const auto& cfdna_sample, table_data_.cfdna_samples)
	{
		ui_->vars-> setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem(cfdna_sample.name + "\n(" + cfdna_sample.date.toString("dd.MM.yyyy") + ")\nAllele fequency", Qt::AlignBottom));
		ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("multi-UMI allele frequency");
		ui_->vars-> setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("Alt count", Qt::AlignBottom));
		ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("multi-UMI alternative counts");
		ui_->vars-> setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("Depth", Qt::AlignBottom));
		ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("multi-UMI depth (multi-UMI alt reads + multi-UMI ref reads)");
		ui_->vars-> setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem("p-value", Qt::AlignBottom));
	}

	int row_idx = 0;
	for (int i = 0; i < table_data_.lines.length(); ++i)
	{
		const auto& line = table_data_.lines.at(i);
		const VcfLine& variant = line.tumor_vcf_line;
		//skip ID SNPs
		if (variant.id().contains("ID")) continue;

		col_idx = 0;
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.chr().str()));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.start()));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.ref()));
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.alt(0)));

		//update gene and coding_and_splicing column with live annotation
		GeneSet genes;
		QList<QStringList> coding_splicing_raw = GSvarHelper::annotateCodingAndSplicing(variant, genes, true, 5000);
		QStringList coding_splicing_collapsed;
		foreach (const QStringList& entry, coding_splicing_raw)
		{
			coding_splicing_collapsed << entry.join(":");
		}

		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(genes.toStringList().join(",")));
		ui_->vars->setItem(row_idx, col_idx, GUIHelper::createTableItem(coding_splicing_collapsed.join(", ")));
		ui_->vars->item(row_idx, col_idx++)->setToolTip(coding_splicing_collapsed.join("\n").replace(":", " "));

		//store variant index (e.g. for IGV)
		ui_->vars->setVerticalHeaderItem(row_idx, new QTableWidgetItem());
		ui_->vars->verticalHeaderItem(row_idx)->setData(Qt::UserRole, i);

		// show tumor af of ref
		ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(variant.info("tumor_af")));

		//rightAlign number columns
		ui_->vars->item(row_idx, 1)->setTextAlignment(Qt::AlignRight);
		ui_->vars->item(row_idx, 6)->setTextAlignment(Qt::AlignRight);

		//fill table
		foreach (const auto& cfdna_entry, line.cfdna_columns)
		{
			double cfdna_af = cfdna_entry.multi_af;
			int alt_count = cfdna_entry.multi_alt;
			int depth = cfdna_entry.multi_ref + alt_count;
			double p_value = cfdna_entry.p_value;

			if(std::isnan(cfdna_af))
			{
				//special handling: VCF not found
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem("file not found!", Qt::AlignTop|Qt::AlignRight));
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(""));
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(""));
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(""));
			}
			else
			{
				//default case
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(cfdna_af, 5));
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(alt_count));
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(depth));
				ui_->vars->setItem(row_idx, col_idx++, GUIHelper::createTableItem(p_value,  4));
			}



		}

		row_idx++;

	}
	ui_->vars->setRowCount(row_idx);



	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->vars, 250);

	// set row height to fixed value
	for (int i=0; i<ui_->vars->rowCount(); ++i) ui_->vars->setRowHeight(i, 25);


	//fill MRD table

	// clear old table
	ui_->mrd->clear();

	// set dimensions
	ui_->mrd->setColumnCount(table_data_.cfdna_samples.length());
	ui_->mrd->setRowCount(6);

	// set header
	for (int col_idx = 0; col_idx < table_data_.cfdna_samples.length(); ++col_idx)
	{
		ui_->mrd->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem(table_data_.cfdna_samples.at(col_idx).name));
	}
	ui_->mrd->setVerticalHeaderItem(0, GUIHelper::createTableItem("MRD log10:"));
	ui_->mrd->setVerticalHeaderItem(1, GUIHelper::createTableItem("MRD p-value:"));
	ui_->mrd->setVerticalHeaderItem(2, GUIHelper::createTableItem("Depth:"));
	ui_->mrd->setVerticalHeaderItem(3, GUIHelper::createTableItem("Alt:"));
	ui_->mrd->setVerticalHeaderItem(4, GUIHelper::createTableItem("Mean AF:"));
	ui_->mrd->setVerticalHeaderItem(5, GUIHelper::createTableItem("Median AF:"));

	// fill table
	for (int row_idx = 0; row_idx < 6; ++row_idx)
	{
		for (int col_idx = 0; col_idx < table_data_.cfdna_samples.length(); ++col_idx)
		{
			if(table_data_.mrd_tables.at(col_idx).rowCount() > 0)
			{
				ui_->mrd->setItem(row_idx, col_idx, GUIHelper::createTableItem(table_data_.mrd_tables.at(col_idx).row(0).at(row_idx), Qt::AlignRight));
				continue;
			}
			ui_->mrd->setItem(row_idx, col_idx, GUIHelper::createTableItem(""));
		}
	}

	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->mrd, 250);

}
