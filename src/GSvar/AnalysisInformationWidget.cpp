#include "AnalysisInformationWidget.h"
#include "GlobalServiceProvider.h"
#include "GUIHelper.h"
#include "GSvarHelper.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QAction>

AnalysisInformationWidget::AnalysisInformationWidget(QString ps_id, QWidget* parent)
	: QWidget(parent)
	, ui_()
	, init_timer_(this, true)
	, ps_id_(ps_id)
{
	ui_.setupUi(this);

	QAction* action = new QAction(QIcon(":/Icons/CopyClipboard.png"), "Copy all", this);
	ui_.table->addAction(action);
	connect(action, SIGNAL(triggered(bool)), this, SLOT(copyTableToClipboard()));
}

void AnalysisInformationWidget::delayedInitialization()
{
	updateGUI();
}

void AnalysisInformationWidget::updateGUI()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		NGSD db;
		QString ps = db.processedSampleName(ps_id_);
		QString sample_id = db.sampleId(ps);
		QString rc_id = QString::number(db.reportConfigId(ps_id_));

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
		ui_.table->setRowCount(5);
		if (sample_data.type.startsWith("DNA"))
		{
			ImportStatusGermline import_status = db.importStatus(ps_id_);
			VariantCallingInfo call_info = db.variantCallingInfo(ps_id_);

			//BAM
			FileLocation file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::BAM);
			ui_.table->setItem(0, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(0, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(0,1)->setForeground(QBrush(QColor(Qt::red)));
			if (file.exists && sample_data.species=="human")
			{
                //color build if not matching GSvar build
				BamReader reader(file.filename);
                BamInfo info = reader.info();
                if (info.build!="" && info.build!=buildToString(GSvarHelper::build()))
                {
                    ui_.table->item(0,1)->setText(ui_.table->item(0,1)->text() + " (" + info.build + ")");
                    ui_.table->item(0,1)->setForeground(QBrush(QColor(Qt::red)));
				}

				//show mapper
				ui_.table->setItem(0, 4, GUIHelper::createTableItem(info.mapper + " " + info.mapper_version));
				//add BAM/CRAM infos as tooltip
				QString tooltip;
				tooltip += "file format: " + info.file_format + "\n";
				QString build = info.build;
				build += " masked:" + QString(info.false_duplications_masked ? "yes" : "no");
				build += " alt:" + QString(info.contains_alt_chrs ? "yes" : "no");
				tooltip += "build: " + build + "\n";
				tooltip += "paired-end: " + QString(info.paired_end ? "yes" : "no");
				ui_.table->item(0, 4)->setToolTip(tooltip);
			}
			ui_.table->setItem(0, 2, GUIHelper::createTableItem(QString::number(import_status.qc_terms) + " QC terms"));
			GUIHelper::resizeTableCellWidths(ui_.table);
			GUIHelper::resizeTableCellHeightsToFirst(ui_.table);

			//small variants
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::GSVAR);
			ui_.table->setItem(1, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(1, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(1,1)->setForeground(QBrush(QColor(Qt::red)));
			if (file.exists && sample_data.species=="human")
			{
				VariantList vl;
				vl.loadHeaderOnly(file.filename);
				if (vl.build()!=GSvarHelper::build())
				{
					ui_.table->item(1,1)->setText(ui_.table->item(1,1)->text() + " (" + buildToString(vl.build()) + ")");
                    ui_.table->item(1,1)->setForeground(QBrush(QColor(Qt::red)));
				}
			}
			ui_.table->setItem(1, 2, GUIHelper::createTableItem(QString::number(import_status.small_variants) + " small variants" + rcData(db, "report_configuration_variant", rc_id)));
			ui_.table->setItem(1, 3, GUIHelper::createTableItem(call_info.small_call_date));
			ui_.table->setItem(1, 4, GUIHelper::createTableItem(call_info.small_caller + " " + call_info.small_caller_version));
			ui_.table->setItem(1, 4, GUIHelper::createTableItem(call_info.small_caller + " " + call_info.small_caller_version));
			GUIHelper::resizeTableCellWidths(ui_.table);

			//CNVs
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::COPY_NUMBER_CALLS);
			ui_.table->setItem(2, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(2, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(2,1)->setForeground(QBrush(QColor(Qt::red)));
			if (file.exists && sample_data.species=="human")
			{
				CnvList cnvs;
				cnvs.loadHeaderOnly(file.filename);
				QByteArray genome = cnvs.build();
				if (genome!="" && stringToBuild(genome)!=GSvarHelper::build())
				{
					ui_.table->item(1,1)->setText(ui_.table->item(1,1)->text() + " (" + genome + ")");
                    ui_.table->item(1,1)->setForeground(QBrush(QColor(Qt::red)));
				}
			}
			ui_.table->setItem(2, 2, GUIHelper::createTableItem(QString::number(import_status.cnvs) + " CNVs" + rcData(db, "report_configuration_cnv", rc_id)));
			ui_.table->setItem(2, 3, GUIHelper::createTableItem(call_info.cnv_call_date));
			ui_.table->setItem(2, 4, GUIHelper::createTableItem(call_info.cnv_caller + " " + call_info.cnv_caller_version));
			GUIHelper::resizeTableCellWidths(ui_.table);

			//SVs
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::STRUCTURAL_VARIANTS);
			ui_.table->setItem(3, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(3, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(3,1)->setForeground(QBrush(QColor(Qt::red)));
			if (file.exists && sample_data.species=="human")
			{
				BedpeFile bedpe;
				bedpe.loadHeaderOnly(file.filename);
				QByteArray genome = bedpe.build();
				if (genome!="" && stringToBuild(genome)!=GSvarHelper::build())
				{
					ui_.table->item(3,1)->setText(ui_.table->item(3,1)->text() + " (" + genome + ")");
                    ui_.table->item(3,1)->setForeground(QBrush(QColor(Qt::red)));
				}
			}
			ui_.table->setItem(3, 2, GUIHelper::createTableItem(QString::number(import_status.svs) + " SVs" + rcData(db, "report_configuration_sv", rc_id)));
			ui_.table->setItem(3, 3, GUIHelper::createTableItem(call_info.sv_call_date));
			ui_.table->setItem(3, 4, GUIHelper::createTableItem(call_info.sv_caller + " " + call_info.sv_caller_version));
			GUIHelper::resizeTableCellWidths(ui_.table);

			//REs
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::REPEAT_EXPANSIONS);
			ui_.table->setItem(4, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(4, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(4,1)->setForeground(QBrush(QColor(Qt::red)));
			ui_.table->setItem(4, 2, GUIHelper::createTableItem(QString::number(import_status.res) + " REs" + rcData(db, "report_configuration_re", rc_id)));
			ui_.table->setItem(4, 3, GUIHelper::createTableItem(call_info.re_call_date));
			ui_.table->setItem(4, 4, GUIHelper::createTableItem(call_info.re_caller + " " + call_info.re_caller_version));
		}
		else if (sample_data.type.startsWith("RNA"))
		{
			ui_.table->setRowCount(5);
			ImportStatusGermline import_status = db.importStatus(ps_id_);

			//BAM
			FileLocation file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::BAM);
			ui_.table->setItem(0, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(0, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(0,1)->setForeground(QBrush(QColor(Qt::red)));
			if (file.exists && sample_data.species=="human")
			{
				BamReader reader(file.filename);
				try
				{
					GenomeBuild build = reader.build();
					if (build!=GSvarHelper::build())
					{
						ui_.table->item(0,1)->setText(ui_.table->item(0,1)->text() + " (" + buildToString(build) + ")");
                        ui_.table->item(0,1)->setForeground(QBrush(QColor(Qt::red)));
					}
				}
				catch(...) {} //do nothing (genome build could not be determined)
			}
			ui_.table->setItem(0, 2, GUIHelper::createTableItem(QString::number(import_status.qc_terms) + " QC terms"));

			//counts
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::COUNTS);
			ui_.table->setItem(1, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(1, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(1,1)->setForeground(QBrush(QColor(Qt::red)));

			//expression
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::EXPRESSION);
			ui_.table->setItem(2, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(2, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(2,1)->setForeground(QBrush(QColor(Qt::red)));

			//fusions
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::FUSIONS);
			ui_.table->setItem(3, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(3, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(3,1)->setForeground(QBrush(QColor(Qt::red)));

			//splicing info
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::SPLICING_BED);
			ui_.table->setItem(4, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(4, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(4,1)->setForeground(QBrush(QColor(Qt::red)));
		}
		else if(sample_data.type.startsWith("cfDNA"))
		{
			ImportStatusGermline import_status = db.importStatus(ps_id_);

			//BAM
			FileLocation file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::BAM);
			ui_.table->setItem(0, 0, GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(0, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(0,1)->setForeground(QBrush(QColor(Qt::red)));
			if (file.exists && sample_data.species=="human")
			{
				BamReader reader(file.filename);
				try
				{
					GenomeBuild build = reader.build();
					if (build!=GSvarHelper::build())
					{
						ui_.table->item(0,1)->setText(ui_.table->item(0,1)->text() + " (" + buildToString(build) + ")");
                        ui_.table->item(0,1)->setForeground(QBrush(QColor(Qt::red)));
					}
				}
				catch(...) {} //do nothing (genome build could not be determined)
			}
			ui_.table->setItem(0, 2, GUIHelper::createTableItem(QString::number(import_status.qc_terms) + " QC terms"));

			//small variants
			file = GlobalServiceProvider::database().processedSamplePath(ps_id_, PathType::GSVAR);
			ui_.table->setItem(1, 0,GUIHelper::createTableItem(file.fileName()));
			ui_.table->setItem(1, 1, GUIHelper::createTableItem(file.exists ? "yes" : "no"));
            if (!file.exists) ui_.table->item(1,1)->setForeground(QBrush(QColor(Qt::red)));
			if (file.exists && sample_data.species=="human")
			{
				VariantList vl;
				vl.loadHeaderOnly(file.filename);
				if (vl.build()!=GSvarHelper::build())
				{
					ui_.table->item(1,1)->setText(ui_.table->item(1,1)->text() + " (" + buildToString(vl.build()) + ")");
                    ui_.table->item(1,1)->setForeground(QBrush(QColor(Qt::red)));
				}
			}
			ui_.table->setItem(1, 2, GUIHelper::createTableItem(""));
		}
		else
		{
			THROW(ProgrammingException, "Sample type '"+sample_data.type+"' not handled in analysis information widget!");
		}

		GUIHelper::resizeTableCellWidths(ui_.table);
		GUIHelper::resizeTableCellHeightsToFirst(ui_.table);

		QApplication::restoreOverrideCursor();
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Analysis information");
	}
}

void AnalysisInformationWidget::copyTableToClipboard()
{
	GUIHelper::copyToClipboard(ui_.table, false);
}

QString AnalysisInformationWidget::rcData(NGSD& db, QString table, QString rc_id)
{
	QStringList infos;

	int c_vars_rc = db.getValue("SELECT count(*) FROM "+table+" WHERE report_configuration_id=" + rc_id).toInt();
	if (c_vars_rc>0)
	{
		infos << "report: " + QString::number(c_vars_rc);
		int c_vars_causal = db.getValue("SELECT count(*) FROM "+table+" WHERE causal=1 AND report_configuration_id=" + rc_id).toInt();
		if (c_vars_causal>0) infos << "causal: " + QString::number(c_vars_causal);
	}

	QString output;
	if (infos.count()>0)
	{
		output = " (" + infos.join(" ") + ")";
	}
	return output;
}
