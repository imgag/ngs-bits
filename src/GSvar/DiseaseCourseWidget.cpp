#include "DiseaseCourseWidget.h"
#include "ui_DiseaseCourseWidget.h"
#include "GUIHelper.h"
#include "Settings.h"
#include <QDir>
#include <QMessageBox>

DiseaseCourseWidget::DiseaseCourseWidget(const QString& tumor_sample_name, QWidget *parent) :
	QWidget(parent),
	ui_(new Ui::DiseaseCourseWidget),
	tumor_sample_name_(tumor_sample_name)
{
	ui_->setupUi(this);

	if (!LoginManager::active()) THROW(DatabaseException, "Error: DiseaseCourseWidget requires access to the NGSD!");

	//link signal and slots
	connect(ui_->vars,SIGNAL(itemDoubleClicked(QTableWidgetItem*)),this,SLOT(VariantDoubleClicked(QTableWidgetItem*)));
	connect(ui_->btn_copyToClipboard,SIGNAL(clicked()),this,SLOT(copyToClipboard()));

	getCfDNASampleIds();
	loadVariantLists();
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

	const VcfLine& vcf_line = ref_column_.variants[row];
	QString coords = vcf_line.chr().strNormalized(true) + ":" + QString::number(vcf_line.start());
	emit openInIGV(coords);

	// add cfDNA BAM Files to IGV
	QStringList igv_commands;
	foreach (const cfDnaColumn& cf_dna, cf_dna_columns_)
	{
		QString ps_id = db_.processedSampleId(cf_dna.name);		
		QString bam = NGSD().processedSamplePath(ps_id, PathType::BAM);
		igv_commands << "load \"" + Helper::canonicalPath(bam) + "\"";
	}
	emit executeIGVCommands(igv_commands);
}

void DiseaseCourseWidget::copyToClipboard()
{
	GUIHelper::copyToClipboard(ui_->vars);
}

void DiseaseCourseWidget::getCfDNASampleIds()
{
	// get all same samples
	QString sample_id = db_.sampleId(tumor_sample_name_);
	QStringList same_sample_ids = db_.relatedSamples(sample_id, "same sample");
	same_sample_ids << sample_id; // add current sample id

	// get all related cfDNA samples
	QStringList cf_dna_sample_ids;
	foreach (QString cur_sample_id, same_sample_ids)
	{
		cf_dna_sample_ids.append(db_.relatedSamples(cur_sample_id, "tumor-cfDNA"));
	}

	// get corresponding processed sample
	cf_dna_ps_ids_.clear();
	foreach (QString cf_dna_sample, cf_dna_sample_ids)
	{
		SqlQuery query = db_.getQuery();
		query.exec("SELECT id FROM processed_sample WHERE sample_id=" + cf_dna_sample);
		while (query.next())
		{
			cf_dna_ps_ids_ << query.value(0).toString();
		}
	}
}

void DiseaseCourseWidget::loadVariantLists()
{
	// get processing systems from cfDNA samples
	QSet<QString> processing_systems;
	foreach (const QString& cf_dna_ps_id, cf_dna_ps_ids_)
	{
		processing_systems.insert(db_.getProcessingSystemData(db_.processingSystemIdFromProcessedSample(db_.processedSampleName(cf_dna_ps_id))).name_short);
	}
	if (processing_systems.size() > 1) THROW(ArgumentException, "Multiple processing systems used for cfDNA analysis. Cannot compare samples!");
	QString system_name = processing_systems.toList().at(0);

	// load cfDNA panel
	QList<CfdnaPanelInfo> cfdna_panels = db_.cfdnaPanelInfo(db_.processedSampleId(tumor_sample_name_), QString::number(db_.processingSystemId(system_name)));
	if (cfdna_panels.size() < 1) THROW(DatabaseException, "No matchin cfDNA panel for sample " + tumor_sample_name_ + " found in NGSD!");
	CfdnaPanelInfo cfdna_panel_info  = cfdna_panels.at(0);


	// create ref tumor column
	ref_column_.variants= db_.cfdnaPanelVcf(cfdna_panel_info.id);
	ref_column_.name = tumor_sample_name_;
	ref_column_.date = QDate::fromString(db_.getSampleData(db_.sampleId(tumor_sample_name_)).received, "dd.MM.yyyy");

	// create cfDNA column for each column
	cf_dna_columns_.clear();
	foreach (const QString& ps_id , cf_dna_ps_ids_)
	{
		cfDnaColumn cf_dna_column;
		cf_dna_column.name = db_.processedSampleName(ps_id);
		cf_dna_column.date = QDate::fromString(db_.getSampleData(db_.sampleId(cf_dna_column.name)).received, "dd.MM.yyyy");
		QString cfdna_vcf = db_.processedSamplePath(ps_id, PathType::VCF_CF_DNA);
		if (!QFile::exists(cfdna_vcf))
		{
			QMessageBox::warning(this, "File not found", "Could not find cfDNA VCF for processed Sample " + cf_dna_column.name + "! ");
		}
		else
		{
			// load variant list
			cf_dna_column.variants.load(cfdna_vcf);

			// create lookup table for each variant
			cf_dna_column.lookup_table.clear();
			for (int i=0; i<cf_dna_column.variants.count(); ++i)
			{
				const VcfLine& vcf_line = cf_dna_column.variants[i];
				cf_dna_column.lookup_table.insert(vcf_line.variantToString().toUtf8(), &vcf_line);
			}

			// add to list
			cf_dna_columns_.append(cf_dna_column);
		}
	}

	// sort vector by date
	std::sort(cf_dna_columns_.begin(), cf_dna_columns_.end());

}

void DiseaseCourseWidget::createTableView()
{
	// clear old table
	ui_->vars->clear();


	// set dimensions
	ui_->vars->setColumnCount(7 + cf_dna_columns_.length());
	ui_->vars->setRowCount(ref_column_.variants.count());

	int col_idx = 0;

	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("chr"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Chromosome the variant is located on.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("pos"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Position of the variant on the chromosome.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("ref"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Reference bases in the reference genome at the variant position.\n`-` in case of an insertion.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("obs"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Alternate bases observed in the sample.\n`-` in case of an deletion.");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("gene"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Affected gene list (comma-separated).");
	ui_->vars->setHorizontalHeaderItem(col_idx, GUIHelper::createTableItem("coding and splicing"));
	ui_->vars->horizontalHeaderItem(col_idx++)->setToolTip("Coding and splicing details (Gene, ENST number, type, impact, exon/intron number, HGVS.c, HGVS.p, Pfam domain).");

	// set header for sample
	ui_->vars-> setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem(ref_column_.name + "\n(" + ref_column_.date.toString("dd.MM.yyyy") + ")"));

	// set cfDNA header
	foreach (const cfDnaColumn& cf_dna_column, cf_dna_columns_)
	{
		ui_->vars-> setHorizontalHeaderItem(col_idx++, GUIHelper::createTableItem(cf_dna_column.name + "\n(" + cf_dna_column.date.toString("dd.MM.yyyy") + ")"));
	}


	for (int i=0; i<ref_column_.variants.count(); ++i)
	{
		const VcfLine& variant = ref_column_.variants[i];
		col_idx = 0;

		ui_->vars->setItem(i, col_idx++, GUIHelper::createTableItem(variant.chr().str()));
		ui_->vars->setItem(i, col_idx++, GUIHelper::createTableItem(QByteArray::number(variant.start())));
		ui_->vars->setItem(i, col_idx++, GUIHelper::createTableItem(variant.ref()));
		ui_->vars->setItem(i, col_idx++, GUIHelper::createTableItem(variant.alt(0)));
		ui_->vars->setItem(i, col_idx++, GUIHelper::createTableItem(variant.info("gene", false)));
		ui_->vars->setItem(i, col_idx, GUIHelper::createTableItem(variant.info("coding_and_splicing", false)));
		ui_->vars->item(i, col_idx++)->setToolTip(variant.info("coding_and_splicing", false).replace(",", "\n"));

		// show tumor af of ref
		ui_->vars->setItem(i, col_idx++, GUIHelper::createTableItem(variant.info("tumor_af")));

		//rightAlign number columns
		ui_->vars->item(i, 1)->setTextAlignment(Qt::AlignRight);
		ui_->vars->item(i, 6)->setTextAlignment(Qt::AlignRight);

		// get tumor af for each cfDNA sample
		QByteArray key = variant.variantToString().toUtf8();
		foreach (const cfDnaColumn& cf_dna_column, cf_dna_columns_)
		{
			// get variant
			if (cf_dna_column.lookup_table.contains(key))
			{
				const VcfLine* cf_dna_variant = cf_dna_column.lookup_table.value(key);
				double alt_count = Helper::toDouble(cf_dna_variant->formatValueFromSample("Alt_Count"), "Alt_Count", QString::number(i));
				double depth = Helper::toDouble(cf_dna_variant->formatValueFromSample("DP"), "DP", QString::number(i));
				double p_value = Helper::toDouble(cf_dna_variant->info("PValue"), "PValue", QString::number(i));
				double cf_dna_af = (depth != 0)? alt_count/depth : 0.0;

				// generate table item with tool tip
				QTableWidgetItem* cfdna_item = GUIHelper::createTableItem(QString::number(cf_dna_af, 'f', 5));
				cfdna_item->setToolTip("Alt. count:\t" + QString::number(alt_count, 'f', 0).rightJustified(9, ' ')
									+ "\nDepth:     \t" + QString::number(depth, 'f', 0).rightJustified(7, ' ')
									+ "\np-value:   \t" + QString::number(p_value, 'f', 4).rightJustified(6, ' '));
				cfdna_item->setTextAlignment(Qt::AlignRight);
				ui_->vars->setItem(i, col_idx++, cfdna_item);
			}
			else
			{
				ui_->vars->setItem(i, col_idx++, GUIHelper::createTableItem("not detected"));
			}
		}

	}

	// optimize cell sizes
	GUIHelper::resizeTableCells(ui_->vars, 250);

	// set row height to fixed value
	for (int i=0; i<ui_->vars->rowCount(); ++i) ui_->vars->setRowHeight(i, 25);

}
