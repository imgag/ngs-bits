#include "GeneInterpretabilityDialog.h"
#include "NGSD.h"
#include "GUIHelper.h"

GeneInterpretabilityDialog::GeneInterpretabilityDialog(QWidget* parent, QList<GeneInterpretabilityRegion> regions)
	: QDialog(parent)
	, ui_()
	, regions_(regions)
{
	ui_.setupUi(this);
	connect(ui_.calcualte_btn, SIGNAL(clicked(bool)), this, SLOT(calculate()));

	initTable();
}

void GeneInterpretabilityDialog::initTable()
{
	NGSD db;

	ui_.table->clear();

	//set column headers
	ui_.table->setColumnCount(regions_.count() + 3);
	int col = 0;
	ui_.table->setHorizontalHeaderItem(col++, GUIHelper::createTableItem("gene"));
	ui_.table->setHorizontalHeaderItem(col++, GUIHelper::createTableItem("relevant transcripts"));
	ui_.table->setHorizontalHeaderItem(col++, GUIHelper::createTableItem("transcripts region [bases]"));
	foreach(const GeneInterpretabilityRegion& reg, regions_)
	{
		ui_.table->setHorizontalHeaderItem(col++, GUIHelper::createTableItem(reg.name));
	}

	//resize
	GUIHelper::resizeTableCellWidths(ui_.table, 300);
}

void GeneInterpretabilityDialog::calculate()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		NGSD db;
		QList<BedFile> bed_files;

		//load genes from input
		GeneSet genes;
		QStringList lines = ui_.input->toPlainText().split('\n');
		foreach(QString line, lines)
		{
			line = line.trimmed();
			if (line.isEmpty() || line.startsWith("#")) continue;

			QStringList parts = line.split('\t');
			QString gene = parts[0].trimmed();
			if (gene.isEmpty()) continue;
			genes << gene.toLatin1();
		}

		//add one row per gene
		ui_.table->setRowCount(0);
		ui_.table->setRowCount(genes.count());
		int row = 0;
		foreach(QByteArray gene, genes)
		{
			QByteArray gene_approved = db.geneToApproved(gene);
			if (gene_approved!=gene) gene = gene_approved + " / " + gene;
			ui_.table->setItem(row++, 0, GUIHelper::createTableItem(gene));
		}
		GUIHelper::resizeTableCellWidths(ui_.table, 300);
		GUIHelper::resizeTableCellHeightsToFirst(ui_.table, 200);
		qApp->processEvents();

		//calculate
		row = 0;
		foreach(QByteArray gene, genes)
		{
			//get relevant transcripts
			QByteArray gene_approved = db.geneToApproved(gene);
			int gene_id = db.geneId(gene_approved);
			TranscriptList transcripts = db.relevantTranscripts(gene_id);

			//get target region
			QString mode = ui_.reg_exon_splice->isChecked() ? "exon" : "gene";
			bool exon_mode = mode=="exon";
			BedFile roi;
			foreach(const Transcript& t, transcripts)
			{
				roi.add(db.transcriptToRegions(t.name(), mode));
			}
			roi.merge();
			int bases_coding = exon_mode ? roi.baseCount() : -1;
			if (exon_mode) roi.extend(20);
			roi.merge();

			//gene base data
			int col = 1;
			ui_.table->setItem(row, col++, GUIHelper::createTableItem(transcripts.transcripts(gene_approved, true).join(", ")));
			QString base_count = QString::number(roi.baseCount());
			if (exon_mode) base_count += " (coding: "+QString::number(bases_coding)+")";
			ui_.table->setItem(row, col++, GUIHelper::createTableItem(base_count));

			//calcaulte intersect
			for (int i=0; i<regions_.count(); ++i)
			{
				//load target regions with needed the first time
				if (bed_files.count()<i+1)
				{
					bed_files.append(BedFile());
					bed_files[i].load(regions_[i].filename);
					bed_files[i].merge();
				}

				//calculate intersection
				BedFile tmp = roi;
				tmp.intersect(bed_files[i]);

				//show in table
				int ol = tmp.baseCount();
				QString ol_str = QString::number(ol);
				if (ol>0) ol_str += (" - " + QString::number(100.0*ol/roi.baseCount(), 'f', 2) + "%");
				ui_.table->setItem(row, col++, GUIHelper::createTableItem(ol_str));
			}

			qApp->processEvents();

			++row;
		}

		GUIHelper::resizeTableCellWidths(ui_.table, 300);

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, windowTitle());
	}
}
