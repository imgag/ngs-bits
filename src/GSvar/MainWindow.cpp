#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "Settings.h"
#include "Exceptions.h"
#include "ChromosomalIndex.h"
#include "Log.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "GeneSet.h"
#include <QDir>
#include <QBitArray>
#include <QDesktopServices>
#include <QUrl>
#include <QTcpSocket>
#include <QTime>
#include "ExternalToolDialog.h"
#include "ReportDialog.h"
#include <QBrush>
#include <QFont>
#include <QInputDialog>
#include <QClipboard>
#include <QProgressBar>
#include <QToolButton>
#include <QMimeData>
#include <QSqlError>
#include <QBarSet>
#include <QBarSeries>
#include <QChart>
#include <QChartView>
#include <QBarCategoryAxis>

#include "ReportWorker.h"
#include "DBAnnotationWorker.h"
#include "ScrollableTextDialog.h"
#include "AnalysisStatusDialog.h"
#include "HttpHandler.h"
#include "ValidationDialog.h"
#include "ClassificationDialog.h"
#include "BasicStatistics.h"
#include "ApprovedGenesDialog.h"
#include "GeneInfoDialog.h"
#include "PhenoToGenesDialog.h"
#include "GenesToRegionsDialog.h"
#include "SubpanelDesignDialog.h"
#include "SubpanelArchiveDialog.h"
#include "IgvDialog.h"
#include "GapDialog.h"
#include "CnvWidget.h"
#include "RohWidget.h"
#include "GeneSelectorDialog.h"
#include "NGSHelper.h"
#include "XmlHelper.h"
#include "QCCollection.h"
#include "NGSDReannotationDialog.h"
#include "DiseaseInfoWidget.h"
#include "CandidateGeneDialog.h"
#include "TSVFileStream.h"
#include "LovdUploadDialog.h"
#include "OntologyTermCollection.h"
#include "ReportHelper.h"
#include "DiagnosticStatusOverviewDialog.h"
#include "SvWidget.h"
#include "VariantSampleOverviewDialog.h"
#include "SomaticReportConfiguration.h"
#include "ClinCnvList.h"
#include "SingleSampleAnalysisDialog.h"
#include "MultiSampleDialog.h"
#include "TrioDialog.h"
#include "SomaticDialog.h"
#include "Histogram.h"
#include "ProcessedSampleWidget.h"
#include "DBSelector.h"

QT_CHARTS_USE_NAMESPACE

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, filter_widget_(new FilterDockWidget(this))
	, var_last_(-1)
	, sample_widget_(new SampleDetailsDockWidget(this))
	, var_widget_(new VariantDetailsDockWidget(this, preferred_transcripts_))
	, busy_dialog_(nullptr)
	, filename_()
	, db_annos_updated_(NO)
	, igv_initialized_(false)
	, last_report_path_(QDir::homePath())
{
	//setup GUI
	ui_.setupUi(this);
	setWindowTitle(QCoreApplication::applicationName());
	addDockWidget(Qt::RightDockWidgetArea, filter_widget_);
	addDockWidget(Qt::RightDockWidgetArea, sample_widget_);
	tabifyDockWidget(filter_widget_, sample_widget_);
	connect(sample_widget_, SIGNAL(showAlleleFrequencyHistogram()), this, SLOT(showAlleleFrequencyHistogram()));
	addDockWidget(Qt::BottomDockWidgetArea, var_widget_);
	connect(var_widget_, SIGNAL(jumbToRegion(QString)), this, SLOT(openInIGV(QString)));
	connect(var_widget_, SIGNAL(editVariantClassification()), this, SLOT(editVariantClassification()));
	connect(var_widget_, SIGNAL(editVariantValidation()), this, SLOT(editVariantValidation()));
	connect(var_widget_, SIGNAL(editVariantComment()), this, SLOT(editVariantComment()));
	connect(var_widget_, SIGNAL(showVariantSampleOverview()), this, SLOT(showVariantSampleOverview()));
	connect(ui_.tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

	//filter menu button
	auto filter_btn = new QToolButton();
	filter_btn->setIcon(QIcon(":/Icons/Filter.png"));
	filter_btn->setToolTip("Apply default variant filters.");
	filter_btn->setMenu(new QMenu());
	filter_btn->setPopupMode(QToolButton::InstantPopup);
	connect(filter_btn, SIGNAL(triggered(QAction*)), this, SLOT(applyFilter(QAction*)));
	foreach(QString filter_name, loadFilterNames())
	{
		if (filter_name=="---")
		{
			filter_btn->menu()->addSeparator();
		}
		else
		{
			filter_btn->menu()->addAction(filter_name);
		}
	}
	filter_btn->menu()->addSeparator();
	filter_btn->menu()->addAction(ui_.actionFiltersClear);
	filter_btn->menu()->addAction(ui_.actionFiltersClearWithROI);
	ui_.tools->insertWidget(ui_.actionReport, filter_btn);

	//signals and slots
	connect(ui_.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui_.vars, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(varsContextMenu(QPoint)));
	connect(filter_widget_, SIGNAL(filtersChanged()), this, SLOT(filtersChanged()));
	connect(filter_widget_, SIGNAL(filtersChanged()), filter_widget_, SLOT(raise()));
	connect(filter_widget_, SIGNAL(targetRegionChanged()), this, SLOT(resetAnnotationStatus()));
	connect(ui_.vars, SIGNAL(itemSelectionChanged()), this, SLOT(updateVariantDetails()));
	connect(&filewatcher_, SIGNAL(fileChanged()), this, SLOT(handleInputFileChange()));
	connect(ui_.vars, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(variantCellDoubleClicked(int, int)));
	connect(ui_.actionDesignSubpanel, SIGNAL(triggered()), this, SLOT(openSubpanelDesignDialog()));
	connect(filter_widget_, SIGNAL(phenotypeImportNGSDRequested()), this, SLOT(importPhenotypesFromNGSD()));
	connect(filter_widget_, SIGNAL(phenotypeSubPanelRequested()), this, SLOT(createSubPanelFromPhenotypeFilter()));

	//misc initialization
	filewatcher_.setDelayInSeconds(10);

	//if at home, use Patientenserver
	QString gsvar_report_folder = Settings::string("gsvar_report_folder");
	if (gsvar_report_folder!="" && QDir(gsvar_report_folder).exists())
	{
		last_report_path_ = gsvar_report_folder;
	}

	//delayed initialization
	connect(&delayed_init_timer_, SIGNAL(timeout()), this, SLOT(delayedInizialization()));
	delayed_init_timer_.setSingleShot(false);
	delayed_init_timer_.start(50);
}

void MainWindow::on_actionClose_triggered()
{
	loadFile("");
}

void MainWindow::on_actionIgvInit_triggered()
{
	igv_initialized_ = false;
}

void MainWindow::on_actionSV_triggered()
{
	if(filename_ == "") return;

	try
	{
		SvWidget* list = new SvWidget(filename_);
		auto dlg = GUIHelper::createDialog(list, "Structure variants");
		addModelessDialog(dlg);
		connect(list,SIGNAL(openSvInIGV(QString)),this,SLOT(openInIGV(QString)));
	}
	catch(FileParseException error)
	{
		QMessageBox::warning(this,"File Parse Exception",error.message());
	}
	catch(FileAccessException error)
	{
		QMessageBox::warning(this,"SV file not found",error.message());
	}
}

void MainWindow::on_actionCNV_triggered()
{
	if (filename_=="") return;

	//create list of genes with heterozygous variant hits
	GeneSet het_hit_genes;
	int i_genes = variants_.annotationIndexByName("gene", true, false);
	QString geno_column = variants_.type()==GERMLINE_TRIO ? processedSampleName() : "genotype";
	int i_genotype = variants_.annotationIndexByName(geno_column, true, false);
	if (i_genes!=-1 && i_genotype!=-1)
	{
		for (int i=0; i<variants_.count(); ++i)
		{
			if (ui_.vars->isRowHidden(i)) continue;
			if (variants_[i].annotations()[i_genotype]!="het") continue;

			GeneSet genes = GeneSet::createFromText(variants_[i].annotations()[i_genes], ',');
			foreach(const QByteArray& gene, genes)
			{
				het_hit_genes.insert(gene);
			}
		}
	}
	else if (variants_.type()!=SOMATIC_PAIR)
	{
		QMessageBox::information(this, "Invalid variant list", "Column 'genotype' or 'gene' not found in variant list. Cannot apply compound-heterozygous filter based on variants!");
	}

	if (Settings::boolean("NGSD_enabled", true))
	{
		NGSD db;
		het_hit_genes = db.genesToApproved(het_hit_genes);
	}

	CnvWidget* list = new CnvWidget(filename_, filter_widget_, het_hit_genes);

	connect(list, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	auto dlg = GUIHelper::createDialog(list, "Copy-number variants");
	addModelessDialog(dlg);
}

void MainWindow::on_actionROH_triggered()
{
	if (filename_=="" || variants_.type()==GERMLINE_MULTISAMPLE) return;

	//trios special handling
	QString filename = filename_;
	if (variants_.type()==GERMLINE_TRIO)
	{
		//show ROHs of child (index)
		QString child = variants_.getSampleHeader().infoByStatus(true).column_name;
		QString trio_folder = QFileInfo(filename_).path();
		QString project_folder = QFileInfo(trio_folder).path();
		filename = project_folder + "/Sample_" + child + "/" + child + ".GSvar";

		//UPDs
		QString upd_file = trio_folder + "/trio_upd.tsv";
		if (!QFile::exists(upd_file))
		{
			QMessageBox::warning(this, "UPD detection", "The UPD file is missing!\n" + upd_file);
		}
		else
		{
			QStringList upd_data = Helper::loadTextFile(upd_file, false, QChar::Null, true);
			if (upd_data.count()>1)
			{
				QPlainTextEdit* text_edit = new QPlainTextEdit(this);
				text_edit->setReadOnly(true);
				QStringList headers = upd_data[0].split("\t");
				for (int r=1; r<upd_data.count(); ++r)
				{
					QStringList parts = upd_data[r].split("\t");
					QString line = parts[0] + ":" + parts[1] + "-" + parts[2];
					for(int c=3 ; c<parts.count(); ++c)
					{
						line += " " + headers[c] + "=" + parts[c];
					}
					text_edit->appendPlainText(line);
				}
				text_edit->setMinimumSize(800, 100);
				auto dlg = GUIHelper::createDialog(text_edit, "UPD(s) detected!");
				dlg->exec();
			}
		}
	}

	RohWidget* list = new RohWidget(filename, filter_widget_);
	connect(list, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	auto dlg = GUIHelper::createDialog(list, "Runs of homozygosity");
	addModelessDialog(dlg);
}

void MainWindow::on_actionGeneSelector_triggered()
{
	if (filename_=="") return;

	//show dialog
	QString sample_folder = QFileInfo(filename_).absolutePath();
	GeneSelectorDialog dlg(sample_folder, processedSampleName(), this);
	connect(&dlg, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	if (dlg.exec())
	{
		//copy report to clipboard
		QString report = dlg.report();
		QApplication::clipboard()->setText(report);

		//show message
		if (QMessageBox::question(this, "Gene selection report", "Gene selection report was copied to clipboard.\nDo you want to open the sub-panel design dialog for selected genes?")==QMessageBox::Yes)
		{
			openSubpanelDesignDialog(dlg.genesForVariants());
		}
	}
}

void MainWindow::on_actionNGSDAnnotation_triggered()
{
	if (variants_.count()==0) return;

	//check disease information present
	NGSD db;
	QString sample_id = db.sampleId(processedSampleName(), false);
	if (sample_id!="")
	{
		DiseaseInfoWidget* widget = new DiseaseInfoWidget(sample_id, this);
		auto dlg = GUIHelper::createDialog(widget, "Disease information", "", true);
		if (widget->diseaseInformationMissing() && dlg->exec()==QDialog::Accepted)
		{
			db.setSampleDiseaseData(sample_id, widget->diseaseGroup(), widget->diseaseStatus());
		}
	}

	//show NGSD annotation dialog
	NGSDReannotationDialog dlg(filter_widget_->targetRegion(), this);
	if (!dlg.exec()) return;

	//show busy dialog
	busy_dialog_ = new BusyDialog("Database annotation", this);

	//start worker
	DBAnnotationWorker* worker = new DBAnnotationWorker(processedSampleName(), variants_, busy_dialog_, dlg.roiFile(), dlg.maxAlleleFrequency());
	connect(worker, SIGNAL(finished(bool)), this, SLOT(databaseAnnotationFinished(bool)));
	worker->start();
}

void MainWindow::on_actionGeneVariantInfo_triggered()
{
	CandidateGeneDialog dlg(this);

	dlg.exec();
}

void MainWindow::on_actionOpenSampleFolder_triggered()
{
	if (filename_=="") return;

	QDesktopServices::openUrl(QFileInfo(filename_).absolutePath());

}

void MainWindow::on_actionOpenSampleQcFiles_triggered()
{
	if (filename_=="") return;

	QStringList files = Helper::findFiles(QFileInfo(filename_).absolutePath(), "*.qcML", false);
	foreach(QString file, files)
	{
		QDesktopServices::openUrl(file);
	}
}

void MainWindow::on_actionPublishVariantInLOVD_triggered()
{
	LovdUploadDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionDiagnosticStatusOverview_triggered()
{
	DiagnosticStatusOverviewDialog* dlg = new DiagnosticStatusOverviewDialog(this);
	connect(dlg, SIGNAL(openProcessedSample(QString)), this, SLOT(openProcessedSampleFromNGSD(QString)));
	addModelessDialog(QSharedPointer<QDialog>(dlg));
}

void MainWindow::on_actionReanalyze_triggered()
{
	if (filename_=="") return;

	SampleHeaderInfo header_info = variants_.getSampleHeader();

	QList<AnalysisJobSample> samples;
	if (variants_.type()==GERMLINE_SINGLESAMPLE)
	{
		SingleSampleAnalysisDialog dlg(this);
		samples << AnalysisJobSample {header_info[0].id, ""};
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			foreach(const AnalysisJobSample& sample,  dlg.samples())
			{
				NGSD().queueAnalysis("single sample", dlg.highPriority(), dlg.arguments(), QList<AnalysisJobSample>() << sample);
			}
		}
	}
	else if (variants_.type()==GERMLINE_MULTISAMPLE)
	{
		MultiSampleDialog dlg(this);
		foreach(const SampleInfo& info, header_info)
		{
			samples << AnalysisJobSample {info.id, info.isAffected() ? "affected" : "control"};
		}
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			NGSD().queueAnalysis("multi sample", dlg.highPriority(), dlg.arguments(), dlg.samples());
		}
	}
	else if (variants_.type()==GERMLINE_TRIO)
	{
		TrioDialog dlg(this);
		foreach(const SampleInfo& info, header_info)
		{
			if(info.isAffected())
			{
				samples << AnalysisJobSample {info.id, "child"};
			}
			else
			{
				samples << AnalysisJobSample {info.id, info.gender()=="male" ? "father" : "mother"};
			}
		}
		dlg.setSamples(samples);
		if (dlg.exec()==QDialog::Accepted)
		{
			NGSD().queueAnalysis("trio", dlg.highPriority(), dlg.arguments(), dlg.samples());
		}
	}
	else if (variants_.type()==SOMATIC_PAIR)
	{
		SomaticDialog dlg(this);
		foreach(const SampleInfo& info, header_info)
		{
			samples << AnalysisJobSample {info.id, info.isTumor() ? "tumor" : "normal"};
		}
		dlg.setSamples(samples);

		if (dlg.exec()==QDialog::Accepted)
		{
			NGSD().queueAnalysis("somatic", dlg.highPriority(), dlg.arguments(), dlg.samples());
		}
	}
}

void MainWindow::delayedInizialization()
{
	if (!isVisible()) return;
	if (!delayed_init_timer_.isActive()) return;
	delayed_init_timer_.stop();

	//initialize LOG file
	if (QFile::exists(Log::fileName()) && !Helper::isWritable(Log::fileName()))
	{
		QMessageBox::warning(this, "GSvar log file not writable", "The log file '" + Log::fileName() + "' is not writable.\nPlease inform your administrator!");
		close();
		return;
	}
	Log::setFileEnabled(true);
	Log::appInfo();

	//load from INI file (if a valid INI file - otherwise restore INI file)
	if (Settings::string("igv_genome").trimmed().isEmpty())
	{
		Settings::restoreBackup();
		if (Settings::string("igv_genome").trimmed().isEmpty())
		{
			QMessageBox::warning(this, "GSvar INI file empty", "The ini file '" + Settings::fileName() + "' is empty.\nPlease inform your administrator!");
			close();
			return;
		}
	}
	else
	{
		Settings::createBackup();
	}

	updateRecentFilesMenu();
	updateIGVMenu();
	updateNGSDSupport();

	//preferred transcripts
	preferred_transcripts_ = loadPreferredTranscripts();

	//load imprinting gene list
	QStringList lines = Helper::loadTextFile(":/Resources/imprinting_genes.tsv", true, '#', true);
	foreach(const QString& line, lines)
	{
		QStringList parts = line.split("\t");
		if (parts.count()==2)
		{
			imprinting_genes_ << parts[0].toLatin1();
		}
	}

	//load command line argument
	if (QApplication::arguments().count()>=2)
	{
		loadFile(QApplication::arguments().at(1));
	}
}

void MainWindow::handleInputFileChange()
{
	QMessageBox::information(this, "GSvar file changed", "The input file changed.\nIt is reloaded now!");
	loadFile(filename_);
}

void MainWindow::variantCellDoubleClicked(int row, int /*col*/)
{
	openInIGV(variants_[row].toString());
}

void MainWindow::openInIGV(QString region)
{
	QStringList init_commands;
	if (!igv_initialized_)
	{
		IgvDialog dlg(this);

		//sample VCF
        QString folder = QFileInfo(filename_).absolutePath();
		QStringList files = Helper::findFiles(folder, "*_var_annotated.vcf.gz", false);
        if (files.count()==1)
        {
			QString name = QFileInfo(files[0]).baseName().replace("_var_annotated", "");
			dlg.addFile(name, "VCF", files[0], ui_.actionIgvSample->isChecked());
        }

		//sample BAM file(s)
		QList<IgvFile> bams = getBamFiles();
		if (bams.empty()) return;
		foreach(const IgvFile& file, bams)
		{
			dlg.addFile(file.id, file.type, file.filename, true);
		}

		//reference BAM
		QString ref = filter_widget_->referenceSample();
		if (ref!="")
		{
			dlg.addFile("reference sample", "BAM", ref, true);
		}

		//sample CNV file(s)
		QList<IgvFile> segs = getSegFilesCnv();
		foreach(const IgvFile& file, segs)
		{
			dlg.addFile(file.id, file.type, file.filename, true);
		}

		//sample BAF file(s)
		QList<IgvFile> bafs = getIgvFilesBaf();
		foreach(const IgvFile& file, bafs)
		{
			dlg.addFile(file.id, file.type, file.filename, true);
		}

		//target region
		QString roi = filter_widget_->targetRegion();
		if (roi!="")
		{
			dlg.addFile("target region track", "BED", roi, true);
		}

		//sample low-coverage
		files = Helper::findFiles(folder, "*_lowcov.bed", false);
        if (files.count()==1)
        {
			dlg.addFile("low-coverage regions track", "BED", files[0], ui_.actionIgvLowcov->isChecked());
		}

		//amplicon file (of processing system)
		try
		{
			NGSD db;
			QString processed_sample_id = db.processedSampleId(filename_);
			ProcessingSystemData system_data = db.getProcessingSystemData(processed_sample_id, true);
			QString amplicons = system_data.target_file.left(system_data.target_file.length()-4) + "_amplicons.bed";
			if (QFile::exists(amplicons))
			{
				dlg.addFile("amplicons track (of processing system)", "BED", amplicons, true);
			}
		}
		catch(...)
		{
			//nothing to do here
		}

		//custom tracks
		QList<QAction*> igv_actions = ui_.menuTracks->findChildren<QAction*>();
		foreach(QAction* action, igv_actions)
		{
			QString text = action->text();
			if (!text.startsWith("custom track:")) continue;
			dlg.addFile(text, "custom track", action->toolTip().replace("custom track:", "").trimmed(), action->isChecked());
		}

		//execute dialog
		if (!dlg.exec()) return;

		if (dlg.initializationAction()==IgvDialog::INIT)
		{
			QStringList files_to_load = dlg.filesToLoad();
			init_commands.append("new");
			init_commands.append("genome " + Settings::string("igv_genome"));

			//load non-BAM files
			foreach(QString file, files_to_load)
			{
				if (!file.endsWith(".bam"))
				{
					init_commands.append("load \"" + QDir::toNativeSeparators(file) + "\"");
				}
			}

			//collapse tracks
			init_commands.append("collapse");

			//load BAM files
			foreach(QString file, files_to_load)
			{
				if (file.endsWith(".bam"))
				{
					init_commands.append("load \"" + QDir::toNativeSeparators(file) + "\"");
				}
			}

			igv_initialized_ = true;
		}
		else if (dlg.initializationAction()==IgvDialog::SKIP_SESSION)
		{
			igv_initialized_ = true;
		}
		else if (dlg.initializationAction()==IgvDialog::SKIP_ONCE)
		{
			//nothing to do there
		}
	}

	//send commands to IGV
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	try
	{
		//init
		foreach(QString command, init_commands)
		{
			executeIGVCommand(command);
		}

		//jump
		executeIGVCommand("goto " + region);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Error while sending command to IGV:", e.message());
		igv_initialized_ = false;
	}
	QApplication::restoreOverrideCursor();
}

void MainWindow::editVariantClassification()
{
	int var_curr = currentVariantIndex();
	if (var_curr==-1) return;

	Variant& variant = variants_[var_curr];

	try
	{
		//add variant if missing
		NGSD db;
		if (db.variantId(variant, false)=="")
		{
			db.addVariant(variant, variants_);
		}

		//execute dialog
		ClassificationDialog dlg(this, variant);
		if (dlg.exec())
		{
			//update DB
			ClassificationInfo class_info = dlg.classificationInfo();
			db.setClassification(variant, class_info);

			//update variant table
			int i_class = variants_.annotationIndexByName("classification", true, true);
			variant.annotations()[i_class] = class_info.classification.replace("n/a", "").toLatin1();
			int i_class_comment = variants_.annotationIndexByName("classification_comment", true, true);
			variant.annotations()[i_class_comment] = class_info.comments.toLatin1();

			//update details widget and filtering
			var_widget_->updateVariant(variants_, var_curr);
			variantListChanged();
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::editVariantValidation()
{
	int var_curr = currentVariantIndex();
	if (var_curr==-1) return;

	Variant& variant = variants_[var_curr];

	try
	{
		int i_quality = variants_.annotationIndexByName("quality", true, true);
		ValidationDialog dlg(this, filename_, variant, i_quality);

		if (dlg.exec()) //update DB
		{
			NGSD().setValidationStatus(filename_, variant, dlg.info());

			//update variant table
			QByteArray status = dlg.info().status.toLatin1();
			if (status=="true positive") status = "TP";
			if (status=="false positive") status = "FP";
			int i_validation = variants_.annotationIndexByName("validation", true, true);
			variant.annotations()[i_validation] = status;

			//update details widget and filtering
			var_widget_->updateVariant(variants_, var_curr);
			variantListChanged();
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::editVariantComment()
{
	int var_curr = currentVariantIndex();
	if (var_curr==-1) return;

	Variant& variant = variants_[var_curr];

	try
	{
		//add variant if missing
		NGSD db;
		if (db.variantId(variant, false)=="")
		{
			db.addVariant(variant, variants_);
		}

		bool ok = true;
		QByteArray text = QInputDialog::getMultiLineText(this, "Variant comment", "Text: ", db.comment(variant), &ok).toLatin1();

		if (ok)
		{
			//update DB
			db.setComment(variant, text);

			//get annotation text (from NGSD to get comments of other samples as well)
			VariantList tmp;
			tmp.append(variant);
			db.annotate(tmp, processedSampleName());
			text = tmp[0].annotations()[tmp.annotationIndexByName("comment", true, true)];

			//update datastructure (if comment column is present)
			int col_index = variants_.annotationIndexByName("comment", true, false);
			if (col_index!=-1)
			{
				variant.annotations()[col_index] = text;
			}

			//update GUI (if column is present)
			int gui_index = guiColumnIndex("comment");
			if (gui_index!=-1)
			{
				QTableWidgetItem* item = ui_.vars->item(var_curr, gui_index);
				if (item==nullptr)
				{
					ui_.vars->setItem(var_curr, gui_index, createTableItem(""));
				}
				item->setText(text);
				var_widget_->updateVariant(variants_, var_curr);
			}
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::showVariantSampleOverview()
{
	int var_curr = currentVariantIndex();
	if (var_curr==-1) return;

	try
	{
		VariantSampleOverviewDialog dlg(variants_[var_curr], this);
		dlg.exec();
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::showAlleleFrequencyHistogram()
{
	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO)
	{
		QMessageBox::information(this, "Allele frequency histogram", "This functionality is only available for germline single sample and germline trio analysis.");
		return;
	}

	//create histogram
	Histogram hist(0.0, 1.0, 0.05);
	int col = guiColumnIndex("quality");
	for (int row=0; row<=ui_.vars->rowCount(); ++row)
	{
		QTableWidgetItem* item = ui_.vars->item(row, col);
		if (item==nullptr) continue;

		QStringList parts = item->text().split(';');
		foreach(const QString& part, parts)
		{
			if (part.startsWith("AF="))
			{
				bool ok;
				QString value_str = part.mid(3);
				if (type==GERMLINE_TRIO) value_str = value_str.split(',')[0];
				double value = value_str.toDouble(&ok);
				if (ok)
				{
					hist.inc(value, true);
				}
			}
		}
	}

	//create chart
	QBarSet* set = new QBarSet("Allele frequency");
	for(int bin=0; bin<hist.binCount(); ++bin)
	{
		set->append(hist.binValue(bin, true));
	}
	QBarSeries* series = new QBarSeries();
	series->append(set);
	QChart* chart = new QChart();
	chart->addSeries(series);
	chart->legend()->setVisible(false);
	chart->createDefaultAxes();
	chart->axisY()->setTitleText("%");
	QBarCategoryAxis* x_axis = new QBarCategoryAxis();
	for(int bin=0; bin<hist.binCount(); ++bin)
	{
		double start = hist.startOfBin(bin);
		x_axis->append(QString::number(start, 'f', 2) + "-" + QString::number(start+hist.binSize(), 'f', 2));
	}
	x_axis->setTitleText("Allele frequency");
	x_axis->setLabelsAngle(90);
	chart->setAxisX(x_axis);

	//show chart
	QChartView* view = new QChartView(chart);
	view->setRenderHint(QPainter::Antialiasing);
	view->setMinimumSize(800, 600);
	auto dlg = GUIHelper::createDialog(view, "Allele frequency histogram");
	dlg->exec();
}

QString MainWindow::targetFileName() const
{
	if (filter_widget_->targetRegion()=="") return "";

	QString output = "_" + QFileInfo(filter_widget_->targetRegion()).fileName();
	output.remove(".bed");
	output.remove(QRegExp("_[0-9_]{4}_[0-9_]{2}_[0-9_]{2}"));
	return output;
}

int MainWindow::guiColumnIndex(QString column) const
{
	for(int i=0; i<ui_.vars->columnCount(); ++i)
	{
		if (ui_.vars->horizontalHeaderItem(i)->text()==column)
		{
			return i;
		}
	}

	return -1;
}

QString MainWindow::processedSampleName()
{
	QString filename = QFileInfo(filename_).baseName();


	if (variants_.type()==SOMATIC_PAIR)
	{
		return filename.split("-")[0];
	}
	else if (variants_.type()==GERMLINE_TRIO)
	{

		return variants_.getSampleHeader().infoByStatus(true).column_name;
	}

	return filename;
}

QString MainWindow::sampleName()
{
	QString ps_name = processedSampleName();
	return (ps_name + "_").split('_')[0];
}

int MainWindow::currentVariantIndex()
{
	auto ranges = ui_.vars->selectedRanges();
	if (ranges.count()!=1 || ranges[0].rowCount()!=1)
	{
		return -1;
	}

	return ranges[0].topRow();
}

QStringList MainWindow::loadFilterNames() const
{
	QStringList output;

	QString filename = QCoreApplication::applicationDirPath() + QDir::separator() + QCoreApplication::applicationName().replace(".exe","") + "_filters.ini";
	foreach(QString line, Helper::loadTextFile(filename, true, QChar::Null, true))
	{
		if (line.startsWith("#"))
		{
			output << line.mid(1);
		}
	}

	return output;
}

FilterCascade MainWindow::loadFilter(QString name) const
{
	FilterCascade output;


	QString filename = QCoreApplication::applicationDirPath() + QDir::separator() + QCoreApplication::applicationName().replace(".exe","") + "_filters.ini";
	QStringList filter_file = Helper::loadTextFile(filename, true, QChar::Null, true);

	bool in_filter = false;
	foreach(QString line, filter_file)
	{
		if (line.startsWith("#"))
		{
			in_filter = (line == "#"+name);
		}
		else if (in_filter)
		{
			QStringList parts = line.trimmed().split('\t');
			QString name = parts[0];
			output.add(FilterFactory::create(name, parts.mid(1)));
		}
	}

	return output;
}

QString MainWindow::processedSampleUserInput()
{
	//create
	DBSelector* selector = new DBSelector(this);
	NGSD db;
	selector->fill(db.createTable("processed_sample", "SELECT ps.id, CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM sample s, processed_sample ps WHERE ps.sample_id=s.id"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select processed sample", "processed sample:", true);
	if (dlg->exec()==QDialog::Rejected) return "";

	return db.processedSampleName(selector->getId());
}

void MainWindow::addModelessDialog(QSharedPointer<QDialog> dlg, bool maximize)
{
	if (maximize)
	{
		dlg->showMaximized();
	}
	else
	{
		dlg->show();
	}
	modeless_dialogs_.append(dlg);

	//we always clean up when we add another dialog.
	//Like that, only one dialog can be closed and not destroyed at the same time.
	cleanUpModelessDialogs();
}

void MainWindow::cleanUpModelessDialogs()
{
	for (int i=modeless_dialogs_.count()-1; i>=0; --i)
	{
		if (modeless_dialogs_[i]->isHidden())
		{
			modeless_dialogs_.removeAt(i);
		}
	}
}

void MainWindow::importPhenotypesFromNGSD()
{
	QList<Phenotype> phenotypes;

	//load from NGSD
	NGSD db;
	QList<SampleDiseaseInfo> disease_info = db.getSampleDiseaseInfo(db.sampleId(filename_), "HPO term id");
	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		try
		{
			phenotypes << db.phenotypeByAccession(entry.disease_info.toLatin1());
		}
		catch(Exception& e)
		{
			qDebug() << e.message();
		}
	}

	filter_widget_->setPhenotypes(phenotypes);
}

void MainWindow::createSubPanelFromPhenotypeFilter()
{
	//convert phenotypes to genes
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	NGSD db;
	GeneSet genes;
	foreach(const Phenotype& pheno, filter_widget_->phenotypes())
	{
		genes << db.phenotypeToGenes(pheno, true);
	}
	QApplication::restoreOverrideCursor();

	//open dialog
	openSubpanelDesignDialog(genes);
}

QStringList MainWindow::geneInheritanceMissing(QBitArray selected)
{
	//get set of genes
	int genes_index = variants_.annotationIndexByName("gene", true, true);
	QSet<QByteArray> genes;
	for (int i=0; i<variants_.count(); ++i)
	{
		if(selected[i])
		{
			foreach(QByteArray gene, variants_[i].annotations()[genes_index].split(','))
			{
				genes.insert(gene.trimmed());
			}
		}
	}

	//check if inheritance is missing
	QStringList output;
	NGSD db;
	foreach(QByteArray gene, genes)
	{
		QString inheritance = db.geneInfo(gene).inheritance;
		if (inheritance=="n/a")
		{
			output.append(gene);
		}
	}

	return output;
}

void MainWindow::on_actionOpen_triggered()
{
	//get file to open
	QString path = Settings::path("path_variantlists");
	QString filename = QFileDialog::getOpenFileName(this, "Open variant list", path, "GSvar files (*.GSvar);;All files (*.*)");
	if (filename=="") return;

	//update data
	loadFile(filename);
}

void MainWindow::on_actionOpenByName_triggered()
{
	QString ps_name = processedSampleUserInput();
	if (ps_name.isEmpty()) return;
	openProcessedSampleFromNGSD(ps_name);
}

void MainWindow::openProcessedSampleFromNGSD(QString processed_sample_name)
{
	//convert name to file
	try
	{
		NGSD db;
		QString processed_sample_id = db.processedSampleId(processed_sample_name);
		QString file = db.processedSamplePath(processed_sample_id, NGSD::GSVAR);

		//check if this is a somatic tumor sample
		QString ps_id = db.processedSampleId(processed_sample_name);
		QString normal_sample = db.normalSample(ps_id);
		if (normal_sample!="")
		{
			QString gsvar_somatic = file.split("Sample_")[0]+ "/" + "Somatic_" + processed_sample_name + "-" + normal_sample + "/" + processed_sample_name + "-" + normal_sample + ".GSvar";
			if (QMessageBox::question(this, "Tumor sample", "The sample is the tumor sample of a tumor-normal pair.\nDo you want to open the somatic variant list?")==QMessageBox::Yes)
			{
				file = gsvar_somatic;
			}
		}

		if (!QFile::exists(file))
		{
			QMessageBox::warning(this, "GSvar file missing", "The GSvar file does not exist:\n" + file);
			return;
		}

		loadFile(file);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Open processed sample from NGSD", e.message());
	}
}

QTableWidgetItem* MainWindow::createTableItem(const QByteArray& text, bool clear) const
{
	//no text > no item (optimization for WGS - empty items are big and take a lot of RAM)
	if (text.isEmpty()) return nullptr;

	//use cache to prevent creating the same QString over and over again.
	//because of implicit sharing, the cache does not use much memory (QByteArrays are in VariantList and QStrings are in QTableWidget)
	static QHash<QByteArray, QString> cache;
	if (clear) cache.clear();
	if (!cache.contains(text))
	{
		cache.insert(text, QString(text));
	}
	return new QTableWidgetItem(cache[text]);
}

void MainWindow::checkMendelianErrorRate(double cutoff_perc)
{
	QString output = "";
	try
	{
		SampleHeaderInfo infos = variants_.getSampleHeader();
		int col_c = infos.infoByStatus(true).column_index;

		foreach(const SampleInfo& info, infos)
		{
			if (info.isAffected()) continue;

			int errors = 0;
			int autosomal = 0;

			int col_p = info.column_index;

			for (int i=0; i<variants_.count(); ++i)
			{
				const Variant& v = variants_[i];
				if (!v.chr().isAutosome()) continue;
				++autosomal;

				QString geno_c = v.annotations()[col_c];
				QString geno_p = v.annotations()[col_p];

				if ((geno_p=="hom" && geno_c=="wt") || (geno_p=="wt" && geno_c=="hom")) ++errors;
			}

			double percentage = 100.0 * errors / autosomal;
			if (percentage>cutoff_perc)
			{
				output = "Amount of variants with mendelian error between " + infos.infoByStatus(true).column_name + " and " + info.column_name + " too high:\n" + QString::number(errors) + "/" + QString::number(autosomal) + " ~ " + QString::number(percentage, 'f', 2) + "%";
				break;
			}
		}
	}
	catch (Exception& e)
	{
		output = "Mendelian error rate calulation not possible:\n" + e.message();
	}

	if (!output.isEmpty())
	{
		QMessageBox::warning(this, "Medelian error rate", output);
	}
}

void MainWindow::openProcessedSampleTab(QString ps_name)
{
	QString ps_id;
	try
	{
		ps_id = NGSD().processedSampleId(ps_name);
	}
	catch (DatabaseException e)
	{
		GUIHelper::showMessage("NGSD error", "The processed sample database ID could not be determined for '"  + ps_name + "'!\nError message: " + e.message());
		return;
	}

	ProcessedSampleWidget* widget = new ProcessedSampleWidget(this, ps_id);
	int index = ui_.tabs->addTab(widget, QIcon(":/Icons/NGSD_sample.png"), ps_name);
	ui_.tabs->setCurrentIndex(index);
	connect(widget, SIGNAL(openProcessedSampleTab(QString)), this, SLOT(openProcessedSampleTab(QString)));
}

void MainWindow::closeTab(int index)
{
	if (index==0)
	{
		int res = QMessageBox::question(this, "Close file?", "Do you want to close the current sample?", QMessageBox::Yes, QMessageBox::No);
		if (res==QMessageBox::Yes)
		{
			loadFile("");
		}
	}
	else
	{
		QWidget* widget = ui_.tabs->widget(index);
		ui_.tabs->removeTab(index);
		widget->deleteLater();
	}
}

void MainWindow::on_actionChangeLog_triggered()
{
	QDesktopServices::openUrl(QUrl("https://github.com/imgag/ngs-bits/tree/master/doc/GSvar/changelog.md"));
}

void MainWindow::loadFile(QString filename)
{
	//reset GUI and data structures
	setWindowTitle(QCoreApplication::applicationName());
	filter_widget_->reset(true);
	filename_ = "";
	filewatcher_.clearFile();
	db_annos_updated_ = NO;
	igv_initialized_ = false;
	ui_.vars->setRowCount(0);
	ui_.vars->setColumnCount(0);

	ui_.tabs->setCurrentIndex(0);
	while (ui_.tabs->count()>1)
	{
		closeTab(ui_.tabs->count()-1);
	}

	if (filename=="")
	{
		sample_widget_->clear();
		return;
	}

	//load data
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	try
	{
		variants_.load(filename);

		filter_widget_->setValidFilterEntries(variants_.filters().keys());

		//update data structures
		Settings::setPath("path_variantlists", filename);
		filename_ = filename;
		filewatcher_.setFile(filename);

		//update GUI
		setWindowTitle(QCoreApplication::applicationName() + " - " + filename);
		ui_.statusBar->showMessage("Loaded variant list with " + QString::number(variants_.count()) + " variants.");

		variantListChanged();
		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Error", "Loading the file '" + filename + "' or displaying the contained variants failed!\nError message:\n" + e.message());
		loadFile("");
		return;
	}

	//update recent files (before try block to remove non-existing files from the recent files menu)
	addToRecentFiles(filename);

	//warn if no 'filter' column is present
	QStringList errors;
	if (variants_.annotationIndexByName("filter", true, false)==-1)
	{
		errors << "column 'filter' missing";
	}
	if (variants_.getSampleHeader(false).empty())
	{
		errors << "sample header missing";
	}
	if (!errors.empty())
	{
		QMessageBox::warning(this, "Outdated GSvar file", "The GSvar file contains the following error(s):\n  -" + errors.join("\n  -") + "\n\nTo ensure that GSvar works as expected, re-run the analysis starting from annotation!");
	}

	//update sample info dialog
	sample_widget_->refresh(processedSampleName());
	sample_widget_->raise();

	//update variant details widget
	try
	{
		var_widget_->setLabelTooltips(variants_);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Outdated GSvar file", "The GSvar file contains the following error:\n" + e.message() + "\n\nTo ensure that GSvar works as expected, re-run the analysis starting from annotation!");
	}

	if (variants_.type()==GERMLINE_TRIO)
	{
		checkMendelianErrorRate();
	}
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About " + QCoreApplication::applicationName(), QCoreApplication::applicationName()+ " " + QCoreApplication::applicationVersion()+ "\n\nA free viewing and filtering tool for genomic variants.\n\nInstitute of Medical Genetics and Applied Genomics\nUniversity Hospital Tübingen\nGermany\n\nMore information at:\nhttps://github.com/imgag/ngs-bits");
}

void MainWindow::on_actionResize_triggered()
{
	GUIHelper::resizeTableCells(ui_.vars, 200);

	//set mimumn width of chr, start, end
	if (ui_.vars->columnWidth(0)<42)
	{
		ui_.vars->setColumnWidth(0, 42);
	}
	if (ui_.vars->columnWidth(1)<62)
	{
		ui_.vars->setColumnWidth(1, 62);
	}
	if (ui_.vars->columnWidth(2)<62)
	{
		ui_.vars->setColumnWidth(2, 62);
	}

	//restrict REF/ALT column width
	for (int i=3; i<=4; ++i)
	{
		if (ui_.vars->columnWidth(i)>80)
		{
			ui_.vars->setColumnWidth(i, 80);
		}
	}
}

void MainWindow::on_actionResizeCustom_triggered()
{
	GUIHelper::resizeTableCells(ui_.vars, 50);

	//set mimumn width of chr, start, end
	if (ui_.vars->columnWidth(0)<42)
	{
		ui_.vars->setColumnWidth(0, 42);
	}
	if (ui_.vars->columnWidth(1)<62)
	{
		ui_.vars->setColumnWidth(1, 62);
	}
	if (ui_.vars->columnWidth(2)<62)
	{
		ui_.vars->setColumnWidth(2, 62);
	}

	//big
	int size_big = 400;
	int index = guiColumnIndex("OMIM");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_big);

	//medium
	int size_med = 100;
	index = guiColumnIndex("genotype");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("gene");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("variant_type");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("filter");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("ClinVar");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("HGMD");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("NGSD_hom");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("NGSD_het");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("NGSD_group");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("classification");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
	index = guiColumnIndex("gene_info");
	if (index!=-1) ui_.vars->setColumnWidth(index, size_med);
}

void MainWindow::on_actionReport_triggered()
{
	if (variants_.count()==0) return;

	//check if this is a germline or somatic
	AnalysisType type = variants_.type();
	if (type==SOMATIC_PAIR)
	{
		generateReportSomaticRTF();
	}
	else if (type==GERMLINE_SINGLESAMPLE)
	{
		generateReport();
	}
	else
	{
		QMessageBox::information(this, "Report error", "Report not supported for this type of analysis!");
	}
}

void MainWindow::generateReportSomaticRTF()
{
	//check CGI columns are present
	if(variants_.annotationIndexByName("CGI_driver_statement", true, false)<0 || variants_.annotationIndexByName("CGI_gene_role", true, false)<0)
	{
		QMessageBox::warning(this,"Somatic report", "Report cannot be created because GSVar-file does not contain CGI-data.");
		return;
	}

	//load CNVs
	ClinCnvList cnvs;
	try
	{
		QString filename_cnv = QFileInfo(filename_).filePath().split('.')[0] + "_clincnv.tsv";
		cnvs.load(filename_cnv);
	}
	catch(Exception& error)
	{
		QMessageBox::warning(this, "Error loading CNV file", error.message()+"\n\nContinuing without CNVs!");
		cnvs.clear();
	}

	//Configure report (CNVs)
	if(!cnvs.isEmpty())
	{
		SomaticReportConfiguration configReport(cnvs, this);
		configReport.setWindowFlags(Qt::Window);
		connect(&configReport, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
		if(!configReport.exec()) return;
		cnvs = configReport.getSelectedCNVs();
	}

	//get RTF file name
	QString file_rep = QFileDialog::getSaveFileName(this, "Store report file", last_report_path_ + "/" + QFileInfo(filename_).baseName() + "_report_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf", "RTF files (*.rtf);;All files(*.*)");
	if (file_rep=="") return;
	//generate report
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);
		ReportHelper report(filename_, cnvs, filter_widget_->filters(),filter_widget_->targetRegion());

		//Generate RTF
		QString temp_filename = Helper::tempFileName(".rtf");
		report.writeRtf(temp_filename);
		ReportWorker::validateAndCopyReport(temp_filename, file_rep, false, true);

		//Generate files for QBIC upload
		report.germlineSnvForQbic();
		report.somaticSnvForQbic();
		report.germlineCnvForQbic();
		report.somaticCnvForQbic();
		report.somaticSvForQbic();
		report.metaDataForQbic();

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& error)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Error while creating report", error.message());
		return;
	}
	catch(...)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Error while creating report", "No error message!");
		return;
	}

	//open report
	if (QMessageBox::question(this, "Report", "Report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
	{
		QDesktopServices::openUrl(file_rep);
	}
}

void MainWindow::generateReport()
{
	//check if required NGSD annotations are present
	if (variants_.annotationIndexByName("classification", true, false)==-1
	 || variants_.annotationIndexByName("NGSD_hom", true, false)==-1
	 || variants_.annotationIndexByName("NGSD_het", true, false)==-1
	 || variants_.annotationIndexByName("comment", true, false)==-1
	 || variants_.annotationIndexByName("gene_info", true, false)==-1)
	{
		GUIHelper::showMessage("Error", "Cannot generate report without complete NGSD annotation!\nPlease re-annotate NGSD information first!");
		return;
	}

	//check if NGSD annotations are up-to-date
	QDateTime mod_date = QFileInfo(filename_).lastModified();
	if (db_annos_updated_==NO && mod_date < QDateTime::currentDateTime().addDays(-42))
	{
		if (QMessageBox::question(this, "NGSD annotations outdated", "NGSD annotation data is older than six weeks!\nDo you want to continue with annotations from " + mod_date.toString("yyyy-MM-dd") + "?")==QMessageBox::No)
		{
			return;
		}
	}

	//check disease information
	if (Settings::boolean("NGSD_enabled", true))
	{
		NGSD db;
		QString sample_id = NGSD().sampleId(filename_, false);
		if (sample_id!="")
		{
			DiseaseInfoWidget* widget = new DiseaseInfoWidget(sample_id, this);
			auto dlg = GUIHelper::createDialog(widget, "Disease information", "", true);
			if (widget->diseaseInformationMissing() && dlg->exec()==QDialog::Accepted)
			{
				db.setSampleDiseaseData(sample_id, widget->diseaseGroup(), widget->diseaseStatus());
			}
		}
	}

	//determine visible variants
	QBitArray visible(variants_.count(), true);
	for (int i=0; i<variants_.count(); ++i)
	{
		if(ui_.vars->isRowHidden(i))
		{
			visible.setBit(i, false);
		}
	}

	//check if inheritance information is set for all genes in NGSD
	QStringList missing = geneInheritanceMissing(visible);
	int max_shown = 50;
	QString missing_text = missing.mid(0, max_shown).join(", ");
	if (missing.count()>max_shown)
	{
		missing_text += ", ... (" + QString::number(missing.count()) + " genes overall)";
	}
	if (!missing.empty() && QMessageBox::question(this, "Report", "Gene inheritance information is missing for these genes:\n\n" + missing_text + "\n\nDo you want to continue?")==QMessageBox::No)
	{
		return;
	}

	//show report dialog
	ReportDialog dialog(filename_, this);
	dialog.addVariants(variants_, visible);
	dialog.setTargetRegionSelected(filter_widget_->targetRegion()!="");
	if (!dialog.exec()) return;
	ReportSettings settings = dialog.settings();

	//get export file name
	QString base_name = processedSampleName();
	QString file_rep = QFileDialog::getSaveFileName(this, "Export report file", last_report_path_ + "/" + base_name + targetFileName() + "_report_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (file_rep=="") return;
	last_report_path_ = QFileInfo(file_rep).absolutePath();

	//get BAM file name if necessary
	QString bam_file = "";
	QList<IgvFile> bams = getBamFiles();
	if (bams.empty()) return;
	bam_file = bams.first().filename;

	//update diagnostic status
	NGSD db;
	db.setDiagnosticStatus(db.processedSampleId(filename_), settings.diag_status);
	//show busy dialog
	busy_dialog_ = new BusyDialog("Report", this);
	busy_dialog_->init("Generating report", false);

	//start worker in new thread
	ReportWorker* worker = new ReportWorker(base_name, bam_file, filter_widget_->targetRegion(), variants_, filter_widget_->filters(), preferred_transcripts_, dialog.settings(), getLogFiles(), file_rep);
	connect(worker, SIGNAL(finished(bool)), this, SLOT(reportGenerationFinished(bool)));
	worker->start();
}

void MainWindow::reportGenerationFinished(bool success)
{
	delete busy_dialog_;

	//show result info box
	ReportWorker* worker = qobject_cast<ReportWorker*>(sender());
	if (success)
	{
		if (QMessageBox::question(this, "Report", "Report generated successfully!\nDo you want to open the report in your browser?")==QMessageBox::Yes)
		{
			QDesktopServices::openUrl(worker->getReportFile());
		}
	}
	else
	{
		QMessageBox::warning(this, "Error", "Report generation failed:\n" + worker->errorMessage());
	}

	//clean
	worker->deleteLater();
}

void MainWindow::databaseAnnotationFinished(bool success)
{
	delete busy_dialog_;

	//show result info box
	DBAnnotationWorker* worker = qobject_cast<DBAnnotationWorker*>(sender());
	if (success)
	{
		db_annos_updated_ = worker->targetRegionOnly() ? ROI : YES;
		variantListChanged();
	}
	else
	{
		QMessageBox::warning(this, "Error", "Database annotation failed:\n" + worker->errorMessage());
	}

	//update variant details widget
	try
	{
		var_widget_->setLabelTooltips(variants_);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Outdated GSvar file", "The GSvar file contains the following error:\n" + e.message() + "\n\nTo ensure that GSvar works as expected, re-run the analysis starting from annotation!");
	}

	//clean
	worker->deleteLater();
}

void MainWindow::on_actionOpenProcessedSampleTabs_triggered()
{
	if (filename_=="") return;

	if (variants_.type()==GERMLINE_SINGLESAMPLE)
	{
		openProcessedSampleTab(processedSampleName());
	}
	else
	{
		SampleHeaderInfo infos = variants_.getSampleHeader();
		foreach(const SampleInfo& info, infos)
		{
			openProcessedSampleTab(info.column_name);
		}
	}
}

void MainWindow::on_actionOpenProcessedSampleTabByName_triggered()
{
	QString ps_name = processedSampleUserInput();
	if (ps_name.isEmpty()) return;

	openProcessedSampleTab(ps_name);
}

void MainWindow::on_actionGenderXY_triggered()
{
	ExternalToolDialog dialog("Determine gender", "xy", this);
	dialog.exec();
}

void MainWindow::on_actionGenderHet_triggered()
{
	ExternalToolDialog dialog("Determine gender", "hetx", this);
	dialog.exec();
}

void MainWindow::on_actionGenderSRY_triggered()
{
	ExternalToolDialog dialog("Determine gender", "sry", this);
	dialog.exec();
}

void MainWindow::on_actionStatisticsBED_triggered()
{
	ExternalToolDialog dialog("BED file information", "", this);
	dialog.exec();
}

void MainWindow::on_actionSampleSimilarityTSV_triggered()
{
	ExternalToolDialog dialog("Sample similarity", "variant list", this);
	dialog.exec();
}

void MainWindow::on_actionSampleSimilarityBAM_triggered()
{
	ExternalToolDialog dialog("Sample similarity", "bam", this);
	dialog.exec();
}

void MainWindow::on_actionSampleAncestry_triggered()
{
	ExternalToolDialog dialog("Sample ancestry", "", this);
	dialog.exec();
}

void MainWindow::on_actionAnalysisStatus_triggered()
{
	auto dlg = new AnalysisStatusDialog(0);
	dlg->setWindowFlags(Qt::Window);
	addModelessDialog(QSharedPointer<QDialog>(dlg), true);
}

void MainWindow::on_actionGapsLookup_triggered()
{
	if (filename_=="") return;

	//get gene name from user
	QString gene = QInputDialog::getText(this, "Display gaps", "Gene:");
	if (gene=="") return;

	//locate report(s)
	QString folder = QFileInfo(filename_).absolutePath();
	QStringList reports = Helper::findFiles(folder, "*_lowcov.bed", false);

	//abort if no report is found
	if (reports.count()==0)
	{
		GUIHelper::showMessage("Error", "Could not detect low-coverage BED file in folder '" + folder + "'.");
		return;
	}

	//select report
	QString report = "";
	if (reports.count()==1)
	{
		report = reports[0];
	}
	else
	{
		bool ok = true;
		report = QInputDialog::getItem(this, "Select low-coverage BED file", "Files", reports, 0, false, &ok);
		if (!ok) return;
	}

	//look up data in report
	QStringList output;
	QStringList lines = Helper::loadTextFile(report, true);
	foreach(QString line, lines)
	{
		QStringList parts = line.split('\t');
		if(parts.count()==4 && parts[3].contains(gene, Qt::CaseInsensitive))
		{
			output.append(line);
		}
	}

	//show output
	QTextEdit* edit = new QTextEdit();
	edit->setText(output.join("\n"));
	edit->setMinimumWidth(500);
	edit->setWordWrapMode(QTextOption::NoWrap);
	edit->setReadOnly(true);
	auto dlg = GUIHelper::createDialog(edit, "Gaps of gene '" + gene + "' from low-coverage BED file '" + report + "':");
	dlg->exec();
}

void MainWindow::on_actionGapsRecalculate_triggered()
{
	if (filename_=="") return;

	//check for ROI file
	QString roi_file = filter_widget_->targetRegion();
	if (roi_file=="")
	{
		QMessageBox::warning(this, "Gaps error", "No target region filter set!");
		return;
	}
	BedFile roi;
	roi.load(roi_file);
	roi.merge();

	//check for BAM file
	QList<IgvFile> bams = getBamFiles();
	if (bams.empty()) return;
	QString bam_file = bams.first().filename;

	//load genes list file
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	GeneSet genes;
	QString genes_file = roi_file.left(roi_file.size()-4) + "_genes.txt";
	if (QFile::exists(genes_file))
	{
		genes = GeneSet::createFromFile(genes_file);
	}

	//prepare dialog
	QString sample_name = QFileInfo(bam_file).fileName().replace(".bam", "");
	GapDialog dlg(this, sample_name, roi_file);
	dlg.process(bam_file, roi, genes);
	QApplication::restoreOverrideCursor();

	//show dialog
	connect(&dlg, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	if (dlg.exec())
	{
		QString report = dlg.report();
		QApplication::clipboard()->setText(report);
		QMessageBox::information(this, "Gap report", "Gap report was copied to clipboard.");

		//write report file to transfer folder
		QString gsvar_gap_transfer = Settings::string("gsvar_gap_transfer");
		if (gsvar_gap_transfer!="")
		{
			QString file_rep = gsvar_gap_transfer + "/" + QFileInfo(bam_file).baseName() + targetFileName() + "_gaps_" + QDate::currentDate().toString("yyyyMMdd") + ".txt";
			Helper::storeTextFile(file_rep, report.split("\n"));
		}
	}
}

void MainWindow::on_actionExportVCF_triggered()
{
	//create BED file with 15 flanking bases around variants
	BedFile roi;
	for(int i=0; i<variants_.count(); ++i)
	{
		if (ui_.vars->isRowHidden(i)) continue;
		roi.append(BedLine(variants_[i].chr(), variants_[i].start()-15, variants_[i].end()+15));
	}
	roi.merge();

	//load original VCF
	QString orig_name = filename_;
	orig_name.replace(".GSvar", "_var_annotated.vcf.gz");
	if (!QFile::exists(orig_name))
	{
		GUIHelper::showMessage("VCF export error", "Could not find original VCF file '" + orig_name + "'!");
		return;
	}
	VariantList orig_vcf;
	orig_vcf.load(orig_name, VCF_GZ, &roi);
	ChromosomalIndex<VariantList> orig_idx(orig_vcf);

	//create new VCF
	VariantList output;
	output.copyMetaData(orig_vcf);
	for(int i=0; i<variants_.count(); ++i)
	{
		if (ui_.vars->isRowHidden(i)) continue;
		int hit_count = 0;
		const Variant& v = variants_[i];
		QVector<int> matches = orig_idx.matchingIndices(v.chr(), v.start()-10, v.end()+10);
		foreach(int index, matches)
		{
			const Variant& v2 = orig_vcf[index];
			if (v.isSNV()) //SNV
			{
				if (v.start()==v2.start() && v.obs()==v2.obs())
				{
					output.append(v2);
					++hit_count;
				}
			}
			else if (v.ref()=="-") //insertion
			{
				if (v.start()==v2.start() && v2.ref().count()==1 && v2.obs().mid(1)==v.obs())
				{
					output.append(v2);
					++hit_count;
				}
			}
			else if (v.obs()=="-") //deletion
			{
				if (v.start()-1==v2.start() && v2.obs().count()==1 && v2.ref().mid(1)==v.ref())
				{
					output.append(v2);
					++hit_count;
				}
			}
			else //complex
			{
				if (v.start()==v2.start() && v2.obs()==v.obs() && v2.ref()==v.ref())
				{
					output.append(v2);
					++hit_count;
				}
			}
		}
		if (hit_count!=1)
		{
			THROW(ProgrammingException, "Found " + QString::number(hit_count) + " matching variants for " + v.toString() + " in VCF file. Exactly one expected!");
		}
	}

	//store to VCF file
	QString file_name = filename_;
	file_name.replace(".GSvar", "_var_export.vcf");
	file_name = QFileDialog::getSaveFileName(this, "Export VCF", file_name, "VCF (*.vcf);;All files (*.*)");
	if (file_name!="")
	{
		output.store(file_name, VCF);
	}
}

void MainWindow::on_actionExportGSvar_triggered()
{
	//create new VCF
	VariantList output;
	output.copyMetaData(variants_);
	for(int i=0; i<variants_.count(); ++i)
	{
		if (!ui_.vars->isRowHidden(i))
		{
			output.append(variants_[i]);
		}
	}

	//store to VCF file
	QString file_name = filename_;
	file_name.replace(".GSvar", "_export.GSvar");
	file_name = QFileDialog::getSaveFileName(this, "Export GSvar", file_name, "GSvar (*.gsvar);;All files (*.*)");
	if (file_name!="")
	{
		output.store(file_name, TSV);
	}
}

void MainWindow::on_actionPreferredTranscripts_triggered()
{
	//update from settings INI
	QDateTime preferred_transcripts_last_modified = QFileInfo(Settings::fileName()).lastModified();
	preferred_transcripts_ = loadPreferredTranscripts();

	//show dialog
	QString text = "<pre>";
	for (auto it=preferred_transcripts_.cbegin(); it!=preferred_transcripts_.cend(); ++it)
	{
		text += it.key() + "\t" + it.value().join(", ") + "\n";
	}
	text += "</pre>";
	QTextEdit* edit = new QTextEdit(text);
	edit->setMinimumHeight(600);
	edit->setMinimumWidth(500);
	QSharedPointer<QDialog> dlg = GUIHelper::createDialog(edit, "Preferred transcripts list", "", true);

	//abort on cancel
	if (dlg->exec()!=QDialog::Accepted) return;

	//parse file
	QMap<QString, QStringList> preferred_transcripts_new;
	QStringList lines = edit->toPlainText().split("\n");
	foreach(QString line, lines)
	{
		line = line.trimmed();
		if (line.isEmpty() || line.startsWith("#")) continue;

		QStringList parts = line.trimmed().split('\t');
		if (parts.count()!=2)
		{
			QMessageBox::warning(this, "Invalid preferred transcript line", "Found line that does not contain two tab-separated colmnns:\n" + line + "\n\nAborting!");
			return;
		}

		//check gene
		QByteArray gene = parts[0].toLatin1().trimmed();
		NGSD db;
		int gene_id = db.geneToApprovedID(gene);
		if(gene_id==-1)
		{
			QMessageBox::warning(this, "Invalid preferred transcript line", "Gene name '" + gene + "' is not a HGNC-approved name!\n\nAborting!");
			return;
		}
		gene = db.geneSymbol(gene_id);

		//remove version number if present (NM_000543.3 => NM_000543.)
		QStringList transcripts = parts[1].split(",");
		foreach(QString transcript, transcripts)
		{
			transcript = transcript.trimmed();
			if (transcript.isEmpty()) continue;
			if (transcript.contains("."))
			{
				transcript = transcript.left(transcript.lastIndexOf('.'));
			}

			preferred_transcripts_new[gene].append(transcript);
		}
	}

	//prevent writing if the INI file changed while showing the dialog
	if (preferred_transcripts_last_modified!=QFileInfo(Settings::fileName()).lastModified())
	{
		QMessageBox::warning(this, "Cannot write preferred transcripts", "Perferred transcripts were changed by another GSvar instance.\nPlease re-do your changes!");
		return;
	}

	//store in INI file
	QMap<QString, QVariant> tmp;
	for(auto it=preferred_transcripts_new.cbegin(); it!=preferred_transcripts_new.cend(); ++it)
	{
		tmp.insert(it.key(), it.value());
	}
	Settings::setMap("preferred_transcripts", tmp);

	//update in-memory copy of preferred transcripts
	preferred_transcripts_ = preferred_transcripts_new;
}

void MainWindow::on_actionOpenDocumentation_triggered()
{
	QDesktopServices::openUrl(QUrl("https://github.com/imgag/ngs-bits/tree/master/doc/GSvar/index.md"));
}

void MainWindow::on_actionConvertHgnc_triggered()
{
	ApprovedGenesDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionGeneInfo_triggered()
{
	QString symbol = QInputDialog::getText(this, "Gene information", "symbol").trimmed();
	if (symbol.isEmpty()) return;

	GeneInfoDialog dlg(symbol.toLatin1(), this);
	dlg.exec();
}

void MainWindow::on_actionPhenoToGenes_triggered()
{
	try
	{
		PhenoToGenesDialog dlg(this);
		dlg.exec();
	}
	catch (DatabaseException& e)
	{
		QMessageBox::warning(this, "Database error", e.message());
	}
}

void MainWindow::on_actionGenesToRegions_triggered()
{
	GenesToRegionsDialog dlg(this);
	dlg.exec();
}

void MainWindow::openSubpanelDesignDialog(const GeneSet& genes)
{
	SubpanelDesignDialog dlg(this);
	dlg.setGenes(genes);

	dlg.exec();

	if (dlg.lastCreatedSubPanel()!="")
	{
		//update target region list
		filter_widget_->loadTargetRegions();

		//optinally use sub-panel as target regions
		if (QMessageBox::question(this, "Use sub-panel?", "Do you want to set the sub-panel as target region?")==QMessageBox::Yes)
		{
			filter_widget_->setTargetRegion(dlg.lastCreatedSubPanel());
		}
	}

}

void MainWindow::on_actionArchiveSubpanel_triggered()
{
	SubpanelArchiveDialog dlg(this);
	dlg.exec();
	if (dlg.changedSubpanels())
	{
		filter_widget_->loadTargetRegions();
	}
}

void MainWindow::on_actionCopy_triggered()
{
	copyToClipboard(false);
}

void MainWindow::on_actionCopySplit_triggered()
{
	copyToClipboard(true);
}

void MainWindow::copyToClipboard(bool split_quality)
{
	// Data to be copied is not selected en bloc
	if (ui_.vars->selectedRanges().count()!=1 && !split_quality)
	{
		//Create 2d list with empty QStrings (size equal to QTable in Main Window)
		QList< QList<QString> > data;
		for(int r=0;r<ui_.vars->rowCount();++r)
		{
			QList<QString> line;
			for(int c=0;c<ui_.vars->columnCount();++c)
			{
				line.append("");
			}
			data.append(line);
		}

		//Fill data with non-empty entries from QTable in Main Window
		QBitArray empty_columns;
		empty_columns.fill(true,ui_.vars->columnCount());
		QList<QTableWidgetItem*> all_items = ui_.vars->selectedItems();
		foreach(QTableWidgetItem* item,all_items)
		{
			if(!item->text().isEmpty())
			{
				data[item->row()][item->column()] = item->text();
				empty_columns[item->column()] = false;
			}
		}

		//Remove empty columns
		for(int c=ui_.vars->columnCount()-1;c>=0;--c)
		{
			if(empty_columns[c])
			{
				for(int r=0;r<ui_.vars->rowCount();++r)
				{
					data[r].removeAt(c);
				}
			}
		}

		//Remove empty rows
		for(int r=ui_.vars->rowCount()-1;r>=0;--r)
		{
			bool row_is_empty = true;
			for(int c=0;c<data[r].count();++c)
			{
				if(!data[r][c].isEmpty())
				{
					row_is_empty = false;
					break;
				}
			}
			if(row_is_empty) data.removeAt(r);
		}

		QString text = "";
		for(int r=0;r<data.count();++r)
		{
			for(int c=0;c<data[r].count();++c)
			{
				text.append(data[r][c]);
				if(c<data[r].count()-1) text.append("\t");
			}
			text.append("\n");
		}
		QApplication::clipboard()->setText(text);

		return;
	}

	QTableWidgetSelectionRange range = ui_.vars->selectedRanges()[0];

	//check quality column is present
	QStringList quality_keys;
	quality_keys << "QUAL" << "DP" << "AF" << "MQM" << "TRIO"; //if modified, also modify quality_values!!!
	int qual_index = -1;
	if (split_quality)
	{
		qual_index = guiColumnIndex("quality");
		if (qual_index==-1)
		{
			QMessageBox::warning(this, "Copy to clipboard", "Column with index 6 has other name than quality. Aborting!");
			return;
		}
	}
	

	//copy header
	QString selected_text = "";
	if (range.rowCount()!=1)
	{
		selected_text += "#";
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");
			if (split_quality && col==qual_index)
			{
				selected_text.append(quality_keys.join('\t'));
			}
			else
			{
				selected_text.append(ui_.vars->horizontalHeaderItem(col)->text());
			}
		}
	}

	//copy rows
	for (int row=range.topRow(); row<=range.bottomRow(); ++row)
	{
		//skip filtered-out rows
		if (ui_.vars->isRowHidden(row)) continue;

		if (selected_text!="") selected_text.append("\n");
		for (int col=range.leftColumn(); col<=range.rightColumn(); ++col)
		{
			if (col!=range.leftColumn()) selected_text.append("\t");

			QTableWidgetItem* item = ui_.vars->item(row, col);
			if (item==nullptr) continue;

			if (split_quality && col==qual_index)
			{
				QStringList quality_values;
				for(int i=0; i<quality_keys.count(); ++i) quality_values.append("");
				QStringList entries = item->text().split(';');
				foreach(const QString& entry, entries)
				{
					QStringList key_value = entry.split('=');
					if (key_value.count()!=2)
					{
						QMessageBox::warning(this, "Copy to clipboard", "Cannot split quality entry '" + entry + "' into key and value. Aborting!");
						return;
					}
					int index = quality_keys.indexOf(key_value[0]);
					if (index==-1)
					{
						QMessageBox::warning(this, "Copy to clipboard", "Unknown quality entry '" + key_value[0] + "'. Aborting!");
						return;
					}

					quality_values[index] = key_value[1];
				}
				selected_text.append(quality_values.join('\t'));
			}
			else
			{
				selected_text.append(item->text().replace('\n',' ').replace('\r',' '));
			}
		}
	}

	QApplication::clipboard()->setText(selected_text);
}

QString MainWindow::nobr()
{
	return "<p style='white-space:pre; margin:0; padding:0;'>";
}

void MainWindow::uploadtoLovd(int variant_index, int variant_index2)
{
	//(1) prepare data as far as we can (no RefSeq transcript data is available)
	LovdUploadData data;

	//sample name
	data.processed_sample = processedSampleName();

	//gender
	NGSD db;
	QString sample_id = db.sampleId(data.processed_sample);
	data.gender = db.getSampleData(sample_id).gender;

	//phenotype(s)
	QList<SampleDiseaseInfo> disease_info = db.getSampleDiseaseInfo(sample_id, "HPO term id");
	foreach(const SampleDiseaseInfo& entry, disease_info)
	{
		data.phenos << db.phenotypeByAccession(entry.disease_info.toLatin1(), false);
	}

	//data 1st variant
	const Variant& variant = variants_[variant_index];
	data.variant = variant;
	int genotype_index = variants_.annotationIndexByName("genotype");
	data.genotype = variant.annotations()[genotype_index];
	FastaFileIndex idx(Settings::string("reference_genome"));
	data.hgvs_g = variant.toHGVS(idx);
	int classification_index = variants_.annotationIndexByName("classification");
	data.classification = variant.annotations()[classification_index];

	//data 2nd variant (comp-het)
	if (variant_index2!=-1)
	{
		const Variant& variant2 = variants_[variant_index2];
		data.variant2 = variant2;
		data.genotype2 = variant2.annotations()[genotype_index];
		data.hgvs_g2 = variant2.toHGVS(idx);
		data.classification2 = variant2.annotations()[classification_index];
	}

	// (2) show dialog
	LovdUploadDialog dlg(this);
	dlg.setData(data);
	dlg.exec();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e)
{
	if (!e->mimeData()->hasFormat("text/uri-list")) return;
	if (e->mimeData()->urls().count()!=1) return;
	QUrl url = e->mimeData()->urls().at(0);
	if (!url.isLocalFile()) return;

	QString filename = url.toLocalFile();
	if (QFile::exists(filename) && filename.endsWith(".GSvar"))
	{
		e->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent* e)
{
	loadFile(e->mimeData()->urls().first().toLocalFile());
	e->accept();
}

void MainWindow::variantListChanged()
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	QTime timer;
	timer.start();

	//update variant details widget
	var_last_ = -1;

	//resize
	ui_.vars->setRowCount(variants_.count());
	ui_.vars->setColumnCount(5 + variants_.annotations().count());

	//header
	ui_.vars->setHorizontalHeaderItem(0, createTableItem("chr", true));
	ui_.vars->horizontalHeaderItem(0)->setToolTip("Chromosome of variant");
	ui_.vars->setHorizontalHeaderItem(1, createTableItem("start"));
	ui_.vars->horizontalHeaderItem(1)->setToolTip("Genomic start position of variant");
	ui_.vars->setHorizontalHeaderItem(2, createTableItem("end"));
	ui_.vars->horizontalHeaderItem(2)->setToolTip("Genomic end position of variant");
	ui_.vars->setHorizontalHeaderItem(3, createTableItem("ref"));
	ui_.vars->horizontalHeaderItem(3)->setToolTip("Reference genome sequence");
	ui_.vars->setHorizontalHeaderItem(4, createTableItem("obs"));
	ui_.vars->horizontalHeaderItem(4)->setToolTip("Sequence observed in the sample");
	for (int i=0; i<variants_.annotations().count(); ++i)
	{
		QString anno = variants_.annotations()[i].name();
		QTableWidgetItem* header = new QTableWidgetItem(anno);

		//additional descriptions for filter column
		QString add_desc = "";
		if (anno=="filter")
		{
			auto it = variants_.filters().cbegin();
			while (it!=variants_.filters().cend())
			{
				add_desc += "\n - "+it.key() + ": " + it.value();
				++it;
			}
		}

		//additional descriptions and color for genotype columns
		SampleHeaderInfo sample_data = variants_.getSampleHeader(false);
		foreach(const SampleInfo& info, sample_data)
		{
			if (info.column_name==anno)
			{
				auto it = info.properties.cbegin();
				while(it != info.properties.cend())
				{
					add_desc += "\n - " + it.key() + ": " + it.value();

					if (info.isAffected())
					{
						header->setForeground(QBrush(Qt::darkRed));
					}

					++it;
				}
			}
		}

		QString header_desc = variants_.annotationDescriptionByName(anno, false, false).description();
		header->setToolTip(header_desc + add_desc);
		ui_.vars->setHorizontalHeaderItem(i+5, header);
	}

	//content
	int i_genes = variants_.annotationIndexByName("gene", true, false);
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, false);
	int i_validation = variants_.annotationIndexByName("validation", true, false);
	int i_classification = variants_.annotationIndexByName("classification", true, false);
	int i_comment = variants_.annotationIndexByName("comment", true, false);
	int i_ihdb_hom = variants_.annotationIndexByName("NGSD_hom", true, false);
	int i_ihdb_het = variants_.annotationIndexByName("NGSD_het", true, false);
	int i_clinvar = variants_.annotationIndexByName("ClinVar", true, false);
	int i_hgmd = variants_.annotationIndexByName("HGMD", true, false);
	for (int i=0; i<variants_.count(); ++i)
	{
		const Variant& row = variants_[i];
		ui_.vars->setItem(i, 0, createTableItem(row.chr().str()));
		if (!row.chr().isAutosome())
		{
			ui_.vars->item(i,0)->setBackgroundColor(Qt::yellow);
			ui_.vars->item(i,0)->setToolTip("Not autosome");
		}
		ui_.vars->setItem(i, 1, createTableItem(QByteArray::number(row.start())));
		ui_.vars->setItem(i, 2, createTableItem(QByteArray::number(row.end())));
		ui_.vars->setItem(i, 3, createTableItem(row.ref()));
		ui_.vars->setItem(i, 4, createTableItem(row.obs()));
		bool is_warning_line = false;
		bool is_notice_line = false;
		bool is_ok_line = false;
		for (int j=0; j<row.annotations().count(); ++j)
		{
			const QByteArray& anno = row.annotations().at(j);
			QTableWidgetItem* item = createTableItem(anno);

			//warning
			if (j==i_co_sp && anno.contains(":HIGH:"))
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}
			else if (j==i_classification && (anno=="3" || anno=="M"))
			{
				item->setBackgroundColor(QColor(255, 135, 60)); //orange
				is_notice_line = true;
			}
			else if (j==i_classification && (anno=="4" || anno=="5"))
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}
			else if (j==i_clinvar && anno.contains("pathogenic")) //matches "pathogenic" and "likely pathogenic"
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}
			else if (j==i_hgmd && anno.contains("CLASS=DM")) //matches both "DM" and "DM?"
			{
				item->setBackgroundColor(Qt::red);
				is_warning_line = true;
			}

			//non-pathogenic
			if (j==i_classification && (anno=="0" || anno=="1" || anno=="2"))
			{
				item->setBackgroundColor(Qt::green);
				is_ok_line = true;
			}

			//highlighed
			if (j==i_validation && anno.contains("TP"))
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_comment && anno!="")
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_ihdb_hom && anno=="0")
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_ihdb_het && anno=="0")
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_clinvar && anno.contains("(confirmed)"))
			{
				item->setBackgroundColor(Qt::yellow);
			}
			else if (j==i_genes)
			{
				bool hit = false;
				if (anno.contains(','))
				{
					 hit = imprinting_genes_.intersectsWith(GeneSet::createFromText(anno, ','));
				}
				else
				{
					hit = imprinting_genes_.contains(anno);
				}
				if (hit)
				{
					item->setBackgroundColor(Qt::yellow);
					item->setToolTip("Imprinting gene");
				}
			}

			ui_.vars->setItem(i, 5+j, item);
		}

		//mark vertical header - warning (red), notice (orange)
		if (is_notice_line && !is_ok_line)
		{
			QTableWidgetItem* item = createTableItem(QByteArray::number(i+1));
			item->setForeground(QBrush(QColor(255, 135, 60)));
			QFont font;
			font.setWeight(QFont::Bold);
			item->setFont(font);
			ui_.vars->setVerticalHeaderItem(i, item);
		}
		else if (is_warning_line && !is_ok_line)
		{
			QTableWidgetItem* item = createTableItem(QByteArray::number(i+1));
			item->setForeground(QBrush(Qt::red));
			QFont font;
			font.setWeight(QFont::Bold);
			item->setFont(font);
			ui_.vars->setVerticalHeaderItem(i, item);
		}
		else
		{
			QTableWidgetItem* item = ui_.vars->takeVerticalHeaderItem(i);
			if (item) delete(item);
		}
	}

	//resize cells
	on_actionResize_triggered();

	QApplication::restoreOverrideCursor();

	Log::perf("Initializing variant table took ", timer);

	//re-filter in case some relevant columns changed
	filtersChanged();
}

void MainWindow::varsContextMenu(QPoint pos)
{
	QList<QTableWidgetSelectionRange> ranges = ui_.vars->selectedRanges();
	if (ranges.count()!=1 || ranges[0].rowCount()!=1) return;
	int row = ranges[0].topRow();

	//init
	bool ngsd_enabled = Settings::boolean("NGSD_enabled", true);
	const Variant& variant = variants_[row];
	int i_gene = variants_.annotationIndexByName("gene", true, true);
	QStringList genes = QString(variant.annotations()[i_gene]).split(',', QString::SkipEmptyParts);
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
	QList<VariantTranscript> transcripts = variant.transcriptAnnotations(i_co_sp);
	int i_dbsnp = variants_.annotationIndexByName("dbSNP", true, true);

	//create contect menu
	QMenu menu(ui_.vars);
	QMenu* sub_menu = nullptr;
	if (!genes.isEmpty())
	{
		sub_menu = menu.addMenu("Gene info");
		foreach(QString g, genes)
		{
			sub_menu->addAction(g);
		}
		sub_menu->setEnabled(ngsd_enabled);
	}

	//HGMD
	sub_menu = menu.addMenu(QIcon("://Icons/HGMD.png"), "HGMD");
	foreach(QString g, genes)
	{
		sub_menu->addAction(g);
	}

	//OMIM
	sub_menu = menu.addMenu(QIcon("://Icons/OMIM.png"), "OMIM");
	foreach(QString g, genes)
	{
		sub_menu->addAction(g);
	}

	//GeneCards
	sub_menu = menu.addMenu(QIcon("://Icons/GeneCards.png"), "GeneCards");
	foreach(QString g, genes)
	{
		sub_menu->addAction(g);
	}

	//Google
	sub_menu = menu.addMenu(QIcon("://Icons/Google.png"), "Google");
	foreach(QString g, genes)
	{
		sub_menu->addAction("gene: " + g);
	}
	foreach(const VariantTranscript& trans, transcripts)
	{
		QAction* action = sub_menu->addAction("variant: " + trans.gene + " " + trans.id + " " + trans.hgvs_c + " " + trans.hgvs_p);
		if (preferred_transcripts_.value(trans.gene).contains(trans.id))
		{
			QFont font = action->font();
			font.setBold(true);
			action->setFont(font);
		}
	}

	//PrimerDesign
	QAction* action = menu.addAction(QIcon("://Icons/WebService.png"), "PrimerDesign");
	action->setEnabled(Settings::string("PrimerDesign")!="");

	//Alamut
	if (Settings::string("Alamut")!="")
	{
		sub_menu = menu.addMenu(QIcon("://Icons/Alamut.png"), "Alamut");

		//BAM
		if (variants_.type()==GERMLINE_SINGLESAMPLE)
		{
			sub_menu->addAction("BAM");
		}

		//genomic location
		QString loc = variant.chr().str() + ":" + QByteArray::number(variant.start());
		loc.replace("chrMT", "chrM");
		sub_menu->addAction(loc);
		sub_menu->addAction(loc + variant.ref() + ">" + variant.obs());

		//genes
		foreach(QString g, genes)
		{
			sub_menu->addAction(g);
		}
		sub_menu->addSeparator();

		//transcripts
		foreach(const VariantTranscript& transcript, transcripts)
		{
			if  (transcript.id!="" && transcript.hgvs_c!="")
			{
				QAction* action = sub_menu->addAction(transcript.id + ":" + transcript.hgvs_c + " (" + transcript.gene + ")");

				//highlight preferred transcripts
				if (preferred_transcripts_.value(transcript.gene).contains(transcript.id))
				{
					QFont font = action->font();
					font.setBold(true);
					action->setFont(font);
				}
			}
		}
	}

	//UCSC
	menu.addAction(QIcon("://Icons/UCSC.png"), "Open in UCSC Genome Browser");

	//LOVD upload
	sub_menu = menu.addMenu(QIcon("://Icons/LOVD.png"), "LOVD");
	sub_menu->addAction("Find in LOVD");
	action = sub_menu->addAction("Publish in LOVD");
	action->setEnabled(ngsd_enabled);

	//MitoMap
	if (variant.chr().isM())
	{
		menu.addAction(QIcon("://Icons/MitoMap.png"), "Open in MitoMap");
	}

	//SysID
	sub_menu = menu.addMenu(QIcon(":/Icons/SysID.png"), "SysID");
	foreach(QString g, genes)
	{
		sub_menu->addAction(g);
	}

	//varsome
	menu.addAction(QIcon("://Icons/VarSome.png"), "VarSome");

	//Execute menu
	action = menu.exec(ui_.vars->viewport()->mapToGlobal(pos));
	if (!action) return;
	QMenu* parent_menu = qobject_cast<QMenu*>(action->parent());

	QByteArray text = action->text().toLatin1();

	if (text=="PrimerDesign")
	{
		try
		{
			QString url = Settings::string("PrimerDesign")+"/index.php?user="+Helper::userName()+"&sample="+sampleName()+"&chr="+variant.chr().str()+"&start="+QString::number(variant.start())+"&end="+QString::number(variant.end())+"";
			QDesktopServices::openUrl(QUrl(url));
		}
		catch (Exception& e)
		{
			GUIHelper::showMessage("NGSD error", "Error while processing'"  + filename_ + "'!\nError message: " + e.message());
			return;
		}
	}
	else if (text=="Open in UCSC Genome Browser")
	{
		QDesktopServices::openUrl(QUrl("http://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=" + variant.chr().str()+":"+QString::number(variant.start()-20)+"-"+QString::number(variant.end()+20)));
	}
	else if (text=="Find in LOVD")
	{
		QDesktopServices::openUrl(QUrl("https://databases.lovd.nl/shared/variants#search_chromosome=" + variant.chr().strNormalized(false)+"&search_VariantOnGenome/DNA=g." + QString::number(variant.start())));
	}
	else if (text=="Open in MitoMap")
	{
		QDesktopServices::openUrl(QUrl("https://www.mitomap.org/cgi-bin/search_allele?starting="+QString::number(variant.start())+"&ending="+QString::number(variant.end())));
	}
	else if (text=="Publish in LOVD")
	{
		try
		{
			//comp-het (one selection with at least two rows)
			if (ui_.vars->selectedRanges().count()==1 && ui_.vars->selectedRanges()[0].rowCount()>=2)
			{
				int index1 = ui_.vars->selectedRanges()[0].topRow();
				int index2 = ui_.vars->selectedRanges()[0].bottomRow();
				uploadtoLovd(index1, index2);
			}
			//comp-het (two selections with one row each)
			else if (ui_.vars->selectedRanges().count()==2 && ui_.vars->selectedRanges()[0].rowCount()==1 && ui_.vars->selectedRanges()[1].rowCount()==1)
			{
				int index1 = ui_.vars->selectedRanges()[0].topRow();
				int index2 = ui_.vars->selectedRanges()[1].topRow();
				uploadtoLovd(index1, index2);
			}
			else
			{
				uploadtoLovd(row);
			}
		}
		catch (Exception& e)
		{
			GUIHelper::showMessage("LOVD upload error", "Error while uploading variant to LOVD: " + e.message());
			return;
		}
	}
	else if (parent_menu && parent_menu->title()=="Gene info")
	{
		GeneInfoDialog dlg(text, this);
		dlg.exec();
	}
	else if (parent_menu && parent_menu->title()=="HGMD")
	{
		QDesktopServices::openUrl(QUrl("https://portal.biobase-international.com/hgmd/pro/gene.php?gene=" + text));
	}
	else if (parent_menu && parent_menu->title()=="OMIM")
	{
		QDesktopServices::openUrl(QUrl("https://omim.org/search/?search=" + text));
	}
	else if (parent_menu && parent_menu->title()=="GeneCards")
	{
		QDesktopServices::openUrl(QUrl("http://www.genecards.org/cgi-bin/carddisp.pl?gene=" + text));
	}
	else if (parent_menu && parent_menu->title()=="Alamut")
	{
		//documentation of the alamut API:
		// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.9/accessing.html
		// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.11/Alamut-HTTP.html
		// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.9/programmatic-access.html
		QStringList parts = action->text().split(" ");
		if (parts.count()>=1)
		{
			QString value = parts[0];
			if (value=="BAM")
			{
				QList<IgvFile> bams = getBamFiles();
				if (bams.empty()) return;
				value = "BAM<" + bams.first().filename;
			}

			try
			{
				HttpHandler(HttpHandler::NONE).getHttpReply(Settings::string("Alamut")+"/show?request="+value);
			}
			catch (Exception& e)
			{
				QMessageBox::warning(this, "Communication with Alamut failed!", e.message());
			}
		}
	}
	else if (parent_menu && parent_menu->title()=="Google")
	{
		QByteArray query;
		QByteArrayList parts = text.split(' ');
		QByteArray type = parts[0].trimmed();
		QByteArray gene = parts[1].trimmed();

		//gene + phenotype query
		if (type == "gene:")
		{
			query = gene + " AND (mutation";
			foreach(const Phenotype& pheno, filter_widget_->phenotypes())
			{
				query += " OR \"" + pheno.name() + "\"";
			}
			query += ")";
		}

		//variant query
		if (type == "variant:")
		{
			QByteArray hgvs_c = parts[3].trimmed();
			QByteArray hgvs_p = parts[4].trimmed();
			query = gene + " AND (\"" + hgvs_c.mid(2) + "\" OR \"" + hgvs_c.mid(2).replace(">", "->") + "\" OR \"" + hgvs_c.mid(2).replace(">", "-->") + "\" OR \"" + hgvs_c.mid(2).replace(">", "/") + "\"";
			if (hgvs_p!="")
			{
				query += " OR \"" + hgvs_p.mid(2) + "\"";
			}
			QByteArray dbsnp = variant.annotations()[i_dbsnp].trimmed();
			if (dbsnp!="")
			{
				query += " OR \"" + dbsnp + "\"";
			}
			query += ")";
		}

		QDesktopServices::openUrl(QUrl("https://www.google.com/search?q=" + query.replace("+", "%2B").replace(' ', '+')));
	}
	else if (parent_menu && parent_menu->title()=="SysID")
	{
		QDesktopServices::openUrl(QUrl("https://sysid.cmbi.umcn.nl/search?search=" + text));
	}
	else if (text=="VarSome")
	{
		QString ref = variant.ref();
		ref.replace("-", "");
		QString obs = variant.obs();
		obs.replace("-", "");
		QString var = variant.chr().str() + "-" + QString::number(variant.start()) + "-" +  ref + "-" + obs;
		QString genome = variant.chr().isM() ? "hg38" : "hg19";
		QDesktopServices::openUrl(QUrl("https://varsome.com/variant/" + genome + "/" + var));
	}
}

void MainWindow::updateVariantDetails()
{
	int var_current = currentVariantIndex();
	if (var_current==-1) //no several variant => clear
	{
		var_widget_->clear();
	}
	else if (var_current!=var_last_) //update variant details (if changed)
	{
		var_widget_->updateVariant(variants_, var_current);
	}

	var_last_ = var_current;
}

void MainWindow::applyFilter(QAction* action)
{
	if (action==nullptr) return;

	QString text = action->text();
	if (text=="Clear filters")
	{
		filter_widget_->reset(false);
	}
	else if (text=="Clear filters and target region")
	{
		filter_widget_->reset(true);
	}
	else
	{
		filter_widget_->setFilters(text, loadFilter(text));
	}
}

void MainWindow::executeIGVCommand(QString command)
{
	//connect
	QAbstractSocket socket(QAbstractSocket::UnknownSocketType, this);
	int igv_port = Settings::integer("igv_port", 60151);
	QString igv_host = Settings::string("igv_host", "127.0.0.1");
	socket.connectToHost(igv_host, igv_port);
	if (!socket.waitForConnected(1000))
	{
		THROW(Exception, "Could not connect to IGV at host " + igv_host + " and port " + QString::number(igv_port) + ".\nPlease start IGV and enable the remote control port:\nView => Preferences => Advanced => Enable port");
	}

	//execute command
	socket.write((command + "\n").toLatin1());
	socket.waitForReadyRead(180000); // 3 min timeout (trios can be slow)
	QString answer = socket.readAll();
	if (answer.trimmed()!="OK")
	{
		THROW(Exception, "Could not execute IGV command '" + command + "'.\nAnswer: " + answer);
	}

	//disconnect
	socket.disconnectFromHost();
}

QStringList MainWindow::getLogFiles()
{
	QDir data_dir(QFileInfo(filename_).path());
	QStringList output = data_dir.entryList(QStringList("*_log?_*.log"),  QDir::Files);

	for(int i=0; i<output.count(); ++i)
	{
		output[i] = data_dir.absolutePath() + "/" + output[i];
	}

	return output;
}

QList<IgvFile> MainWindow::getBamFiles()
{
	QList<IgvFile> output;

	QString sample_folder = QFileInfo(filename_).path();
	QString project_folder = QFileInfo(sample_folder).path();

	SampleHeaderInfo data = variants_.getSampleHeader();
	foreach(const SampleInfo& info, data)
	{
		QString bam1 = sample_folder + "/" + info.id + ".bam";
		QString bam2 = project_folder + "/Sample_" + info.id + "/" + info.id + ".bam";
		if (QFile::exists(bam1))
		{
			output << IgvFile{info.id, "BAM" , bam1};
		}
		else if (QFile::exists(bam2))
		{
			output << IgvFile{info.id, "BAM" , bam2};
		}
		else
		{
			QMessageBox::warning(this, "Missing BAM file!", "Could not find BAM file at one of the default locations:\n" + bam1 + "\n" + bam2);
			output.clear();
			return output;
		}
	}

	return output;
}

QList<IgvFile> MainWindow::getSegFilesCnv()
{
	QList<IgvFile> output;

	if (variants_.type()==SOMATIC_PAIR)
	{
		//tumor-normal SEG file
		QString segfile = filename_.left(filename_.length()-6) + "_cnvs.seg";
		QString pair = QFileInfo(filename_).baseName();
		output << IgvFile{pair + " (copy number)", "CNV" , segfile};

		QString covfile = filename_.left(filename_.length()-6) + "_cov.seg";
		output << IgvFile{pair + " (coverage)","CNV",covfile};

		//germline SEG file
		QString basename = QFileInfo(filename_).baseName().left(filename_.length()-6);
		if (basename.contains("-"))
		{
			QString tumor_ps_name = basename.split("-")[1];
			QString pair_folder = QFileInfo(filename_).path();
			QString project_folder = QFileInfo(pair_folder).path();
			segfile = project_folder + "/Sample_" + tumor_ps_name + "/" + tumor_ps_name + "_cnvs.seg";
			output << IgvFile{tumor_ps_name, "CNV" , segfile};
		}
	}
	else
	{
		QList<IgvFile> tmp = getBamFiles();
		foreach(const IgvFile& file, tmp)
		{
			QString segfile = file.filename.left(file.filename.length()-4) + "_cnvs.seg";
			if (QFile::exists(segfile))
			{
				output << IgvFile{file.id, "CNV" , segfile};
			}
		}
	}

	return output;
}

QList<IgvFile> MainWindow::getIgvFilesBaf()
{
	QList<IgvFile> output;

	if (variants_.type()==SOMATIC_PAIR)
	{
		QString segfile = filename_.left(filename_.length()-6) + "_bafs.igv";
		QString pair = QFileInfo(filename_).baseName();
		output << IgvFile{pair, "BAF" , segfile};
	}
	else
	{
		QList<IgvFile> tmp = getBamFiles();
		foreach(const IgvFile& file, tmp)
		{
			QString segfile = file.filename.left(file.filename.length()-4) + "_bafs.igv";
			if (QFile::exists(segfile))
			{
				output << IgvFile{file.id, "BAF" , segfile};
			}
		}
	}

	return output;
}

void MainWindow::filtersChanged()
{
	if (filename_=="") return;

	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	try
	{
		//apply main filter
		QTime timer;
		timer.start();

		const FilterCascade& filter_cascade = filter_widget_->filters();
		FilterResult filter_result = filter_cascade.apply(variants_, false);
		filter_widget_->markFailedFilters();

		Log::perf("Applying annotation filter took ", timer);
		timer.start();

		//roi file name changed => update ROI
		QString roi = filter_widget_->targetRegion();
		if (roi!=last_roi_filename_)
		{
			last_roi_filename_ = "";
			last_roi_.clear();

			if (roi!="")
			{
				last_roi_.load(roi);
				last_roi_.merge();
				last_roi_filename_ = roi;
			}

			Log::perf("Updating target region filter took ", timer);
			timer.start();
		}

		//roi filter
		if (roi!="")
		{
			FilterRegions::apply(variants_, last_roi_, filter_result);
			Log::perf("Applying target region filter took ", timer);
			timer.start();
		}

		//gene filter
		GeneSet genes_filter = filter_widget_->genes();
		if (!genes_filter.isEmpty())
		{
			FilterGenes filter;
			filter.setStringList("genes", genes_filter.toStringList());
			filter.apply(variants_, filter_result);
			Log::perf("Applying gene filter took ", timer);
			timer.start();
		}

		//text filter
		QByteArray text = filter_widget_->text();
		if (!text.isEmpty())
		{
			FilterAnnotationText filter;
			filter.setString("term", text);
			filter.setString("action", "FILTER");
			filter.apply(variants_, filter_result);
			Log::perf("Applying text filter took ", timer);
			timer.start();
		}

		//target region filter
		BedLine region = BedLine::fromString(filter_widget_->region());
		if (region.isValid())
		{
			BedFile tmp;
			tmp.append(region);
			FilterRegions::apply(variants_, tmp, filter_result);
			Log::perf("Applying region filter took ", timer);
			timer.start();
		}

		//phenotype selection changed => update ROI
		const QList<Phenotype>& phenos = filter_widget_->phenotypes();
		if (phenos!=last_phenos_)
		{
			last_phenos_ = phenos;

			//convert phenotypes to genes
			NGSD db;
			GeneSet pheno_genes;
			foreach(const Phenotype& pheno, phenos)
			{
				pheno_genes << db.phenotypeToGenes(pheno, true);
			}

			//convert genes to ROI
			last_phenos_roi_ = db.genesToRegions(pheno_genes, Transcript::ENSEMBL, "gene", true);
			last_phenos_roi_.extend(5000);
			last_phenos_roi_.merge();
			Log::perf("Updating phenotype filter took ", timer);
			timer.start();
		}

		//phenotype filter
		if (!last_phenos_.isEmpty())
		{
			FilterRegions::apply(variants_, last_phenos_roi_, filter_result);
			Log::perf("Applying phenotype filter took ", timer);
			timer.start();
		}

		//update GUI
		timer.start();
		ui_.vars->setUpdatesEnabled(false);
		for(int i=0; i<variants_.count(); ++i)
		{
			if (filter_result.flags()[i])
			{
				ui_.vars->showRow(i);
			}
			else
			{
				ui_.vars->hideRow(i);
			}
		}
		ui_.vars->setUpdatesEnabled(true);
		Log::perf("Applying filter results to GUI took ", timer);

		//update status bar
		QString status = QString::number(filter_result.countPassing()) + " of " + QString::number(variants_.count()) + " variants passed filters.";
		ui_.statusBar->showMessage(status);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog !");
	}

	QApplication::restoreOverrideCursor();
}

void MainWindow::resetAnnotationStatus()
{
	if (db_annos_updated_==ROI)
	{
		db_annos_updated_ = NO;
	}
}

void MainWindow::addToRecentFiles(QString filename)
{
	//update settings
	QStringList recent_files = Settings::stringList("recent_files");
	recent_files.removeAll(filename);
	if (QFile::exists(filename))
	{
		recent_files.prepend(filename);
	}
	while (recent_files.size() > 10)
	{
		recent_files.removeLast();
	}
	Settings::setStringList("recent_files", recent_files);

	//update GUI
	updateRecentFilesMenu();
}


void MainWindow::updateRecentFilesMenu()
{
	QStringList recent_files = Settings::stringList("recent_files");

	QMenu* menu = new QMenu();
	foreach(const QString& file, recent_files)
	{
		menu->addAction(file, this, SLOT(openRecentFile()));
	}
	ui_.actionRecent->setMenu(menu);
}

void MainWindow::updateIGVMenu()
{
	QStringList entries = Settings::stringList("igv_menu");
	if (entries.count()==0)
	{
		ui_.menuTracks->addAction("No custom entries in INI file!");
	}
	else
	{
		foreach(QString entry, entries)
		{
			QStringList parts = entry.trimmed().split("\t");
			if(parts.count()!=3) continue;
			QAction* action = ui_.menuTracks->addAction("custom track: " + parts[0]);
			action->setCheckable(true);
			action->setChecked(parts[1]=="1");
			action->setToolTip(parts[2]);
		}
	}
}

QMap<QString, QStringList> MainWindow::loadPreferredTranscripts() const
{
	QMap<QString, QStringList> output;

	QMap<QString, QVariant> tmp = Settings::map("preferred_transcripts");
	for(auto it=tmp.cbegin(); it!=tmp.cend(); ++it)
	{
		output[it.key()] = it.value().toStringList();
	}

	return output;
}

void MainWindow::updateNGSDSupport()
{
	bool ngsd_enabled = Settings::boolean("NGSD_enabled", true);
	bool target_file_folder_set = Settings::string("target_file_folder_windows")!="" && Settings::string("target_file_folder_linux")!="";

	//toolbar
	ui_.actionReport->setEnabled(ngsd_enabled);
	ui_.actionOpenProcessedSampleTabs->setEnabled(ngsd_enabled);
	ui_.actionOpenProcessedSampleTabByName->setEnabled(ngsd_enabled);
	ui_.actionNGSDAnnotation->setEnabled(ngsd_enabled);
	ui_.actionAnalysisStatus->setEnabled(ngsd_enabled);
	ui_.actionReanalyze->setEnabled(ngsd_enabled);
	ui_.actionSampleInformation->setEnabled(ngsd_enabled);
	ui_.actionGapsRecalculate->setEnabled(ngsd_enabled);
	ui_.actionGeneSelector->setEnabled(ngsd_enabled);
	ui_.actionDiagnosticStatusOverview->setEnabled(ngsd_enabled);

	//tools menu
	ui_.actionOpenByName->setEnabled(ngsd_enabled);
	ui_.actionGeneInfo->setEnabled(ngsd_enabled);
	ui_.actionGenesToRegions->setEnabled(ngsd_enabled);
	ui_.actionGeneVariantInfo->setEnabled(ngsd_enabled);
	ui_.actionPhenoToGenes->setEnabled(ngsd_enabled);
	ui_.actionConvertHgnc->setEnabled(ngsd_enabled);
	ui_.actionDesignSubpanel->setEnabled(ngsd_enabled);
	ui_.actionDesignSubpanel->setEnabled(target_file_folder_set);
}

void MainWindow::openRecentFile()
{
	QAction* action = qobject_cast<QAction*>(sender());
	loadFile(action->text());
}

