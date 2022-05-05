#include "AnalysisInformationWidget.h"
#include "NGSD.h"
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include <QFileInfo>
#include <QMessageBox>

AnalysisInformationWidget::AnalysisInformationWidget(QString ps_id, QWidget* parent)
	: QWidget(parent)
	, ui_()
	, ps_id_(ps_id)
{
	ui_.setupUi(this);

	updateGUI();
}

void AnalysisInformationWidget::updateGUI()
{
	try
	{
		NGSD db;
		QString ps = db.processedSampleName(ps_id_);
		QString sample_id = db.sampleId(ps);

		//analysis infos
		int last_job_id = db.lastAnalysisOf(ps_id_);
		if (last_job_id>0)
		{
			AnalysisJob job = db.analysisInfo(last_job_id);
			ui_.analysis->setText(job.type + " (" + job.args +")");

			if (!job.history.isEmpty())
			{
				QString text = job.history.first().timeAsString() + " - ";
				if (job.isRunning())
				{
					text += "<font color=red>[analysis running]</font>";
				}
				else
				{
					text += job.history.last().timeAsString();
				}
				ui_.analysis_date->setText(text);
			}
		}

		//file details
		SampleData sample_data = db.getSampleData(sample_id);
		ui_.table->setRowCount(4);
		if (sample_data.type.startsWith("DNA"))
		{
			ImportStatusGermline import_status = db.importStatus(ps_id_);

			//BAM
			FileLocation file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::BAM);
			ui_.table->setItem(0, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(0, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(0,1)->setTextColor(QColor(Qt::red));
			if (file.exists && sample_data.species=="human")
			{
				BamReader reader(file.filename);
				try
				{
					GenomeBuild build = reader.build();
					if (build!=GSvarHelper::build())
					{
						ui_.table->item(0,1)->setText(ui_.table->item(0,1)->text() + " (" + buildToString(build) + ")");
						ui_.table->item(0,1)->setTextColor(QColor(Qt::red));
					}
				}
				catch(...) {} //do nothing (genome build could not be determined)
			}
			ui_.table->setItem(0, 2, GUIHelper::createTableItem(QString::number(import_status.qc_terms) + " QC terms"));

			//small variants
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::GSVAR);
			ui_.table->setItem(1, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(1, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(1,1)->setTextColor(QColor(Qt::red));
			if (file.exists && sample_data.species=="human")
			{
				VariantList vl;
				vl.loadHeaderOnly(file.filename);
				if (vl.build()!=GSvarHelper::build())
				{
					ui_.table->item(1,1)->setText(ui_.table->item(1,1)->text() + " (" + buildToString(vl.build()) + ")");
					ui_.table->item(1,1)->setTextColor(QColor(Qt::red));
				}
			}
			ui_.table->setItem(1, 2, GUIHelper::createTableItem(QString::number(import_status.small_variants) + " small variants"));

			//CNVs
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::COPY_NUMBER_CALLS);
			ui_.table->setItem(2, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(2, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(2,1)->setTextColor(QColor(Qt::red));
			if (file.exists && sample_data.species=="human")
			{
				CnvList cnvs;
				cnvs.loadHeaderOnly(file.filename);
				QByteArray genome = cnvs.build();
				if (genome!="" && stringToBuild(genome)!=GSvarHelper::build())
				{
					ui_.table->item(1,1)->setText(ui_.table->item(1,1)->text() + " (" + genome + ")");
					ui_.table->item(1,1)->setTextColor(QColor(Qt::red));
				}
			}
			ui_.table->setItem(2, 2, GUIHelper::createTableItem(QString::number(import_status.cnvs) + " CNVs"));

			//SVs
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::STRUCTURAL_VARIANTS);
			ui_.table->setItem(3, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(3, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(3,1)->setTextColor(QColor(Qt::red));
			if (file.exists && sample_data.species=="human")
			{
				BedpeFile bedpe;
				bedpe.loadHeaderOnly(file.filename);
				QByteArray genome = bedpe.build();
				if (genome!="" && stringToBuild(genome)!=GSvarHelper::build())
				{
					ui_.table->item(3,1)->setText(ui_.table->item(3,1)->text() + " (" + genome + ")");
					ui_.table->item(3,1)->setTextColor(QColor(Qt::red));
				}
			}
			ui_.table->setItem(3, 2, GUIHelper::createTableItem(QString::number(import_status.svs) + " SVs"));

			GUIHelper::resizeTableCells(ui_.table);
		}
		else if (sample_data.type.startsWith("RNA"))
		{
			ui_.table->setRowCount(5);
			ImportStatusGermline import_status = db.importStatus(ps_id_);

			//BAM
			FileLocation file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::BAM);
			ui_.table->setItem(0, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(0, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(0,1)->setTextColor(QColor(Qt::red));
			if (file.exists && sample_data.species=="human")
			{
				BamReader reader(file.filename);
				try
				{
					GenomeBuild build = reader.build();
					if (build!=GSvarHelper::build())
					{
						ui_.table->item(0,1)->setText(ui_.table->item(0,1)->text() + " (" + buildToString(build) + ")");
						ui_.table->item(0,1)->setTextColor(QColor(Qt::red));
					}
				}
				catch(...) {} //do nothing (genome build could not be determined)
			}
			ui_.table->setItem(0, 2, GUIHelper::createTableItem(QString::number(import_status.qc_terms) + " QC terms"));

			//counts
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::COUNTS);
			ui_.table->setItem(1, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(1, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(1,1)->setTextColor(QColor(Qt::red));

			//expression
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::EXPRESSION);
			ui_.table->setItem(2, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(2, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(2,1)->setTextColor(QColor(Qt::red));

			//fusions
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::FUSIONS);
			ui_.table->setItem(3, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(3, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(3,1)->setTextColor(QColor(Qt::red));

			//splicing info
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::SPLICING_BED);
			ui_.table->setItem(4, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(4, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(4,1)->setTextColor(QColor(Qt::red));

			GUIHelper::resizeTableCells(ui_.table);
		}
		else if(sample_data.type.startsWith("cfDNA"))
		{
			ImportStatusGermline import_status = db.importStatus(ps_id_);

			//BAM
			FileLocation file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::BAM);
			ui_.table->setItem(0, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(0, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(0,1)->setTextColor(QColor(Qt::red));
			if (file.exists && sample_data.species=="human")
			{
				BamReader reader(file.filename);
				try
				{
					GenomeBuild build = reader.build();
					if (build!=GSvarHelper::build())
					{
						ui_.table->item(0,1)->setText(ui_.table->item(0,1)->text() + " (" + buildToString(build) + ")");
						ui_.table->item(0,1)->setTextColor(QColor(Qt::red));
					}
				}
				catch(...) {} //do nothing (genome build could not be determined)
			}
			ui_.table->setItem(0, 2, GUIHelper::createTableItem(QString::number(import_status.qc_terms) + " QC terms"));

			//small variants
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::GSVAR);
			ui_.table->setItem(1, 0,GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(1, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
			if (!file.exists) ui_.table->item(1,1)->setTextColor(QColor(Qt::red));
			if (file.exists && sample_data.species=="human")
			{
				VariantList vl;
				vl.loadHeaderOnly(file.filename);
				if (vl.build()!=GSvarHelper::build())
				{
					ui_.table->item(1,1)->setText(ui_.table->item(1,1)->text() + " (" + buildToString(vl.build()) + ")");
					ui_.table->item(1,1)->setTextColor(QColor(Qt::red));
				}
			}
			ui_.table->setItem(1, 2, GUIHelper::createTableItem(""));

			GUIHelper::resizeTableCells(ui_.table);
		}
		else
		{
			THROW(ProgrammingException, "Sample type '"+sample_data.type+"' not handled in analysis information widget!");
		}
	}
	catch (Exception e)
	{
		QMessageBox::warning(this, "Analysis information", "Error:\n" + e.message());
	}
}
