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
#include "ReportWorker.h"
#include "DBAnnotationWorker.h"
#include "SampleInformationDialog.h"
#include "ScrollableTextDialog.h"
#include "TrioDialog.h"
#include "HttpHandler.h"
#include "ValidationDialog.h"
#include "ClassificationDialog.h"
#include "BasicStatistics.h"
#include "ApprovedGenesDialog.h"
#include "GeneInfoDialog.h"
#include "PhenoToGenesDialog.h"
#include "GenesToRegionsDialog.h"
#include "VariantFilter.h"
#include "SubpanelDesignDialog.h"
#include "SubpanelArchiveDialog.h"
#include "IgvDialog.h"
#include "GapDialog.h"
#include "CnvWidget.h"
#include "GeneSelectorDialog.h"
#include "LovdUploadFile.h"
#include "PhenotypeSelector.h"
#include "NGSHelper.h"
#include "XmlHelper.h"
#include "QCCollection.h"
#include "MultiSampleDialog.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, filter_widget_(new FilterDockWidget(this))
	, var_last_(-1)
	, var_widget_(new VariantDetailsDockWidget(this))
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
	filter_widget_->raise();
	addDockWidget(Qt::BottomDockWidgetArea, var_widget_);
	var_widget_->raise();
	connect(var_widget_, SIGNAL(jumbToRegion(QString)), this, SLOT(openInIGV(QString)));

	//annotation menu button
	auto anno_btn = new QToolButton();
	anno_btn->setIcon(QIcon(":/Icons/Database.png"));
	anno_btn->setToolTip("Re-annotate variant list with frequency information and comments from NGSD.");
	anno_btn->setMenu(new QMenu());
	anno_btn->menu()->addAction(ui_.actionDatabase);
	connect(ui_.actionDatabase, SIGNAL(triggered(bool)), this, SLOT(annotateVariantsComplete()));
	anno_btn->menu()->addAction(ui_.actionDatabaseROI);
	connect(ui_.actionDatabaseROI, SIGNAL(triggered(bool)), this, SLOT(annotateVariantsROI()));
	anno_btn->setPopupMode(QToolButton::InstantPopup);
	ui_.tools->insertWidget(ui_.actionReport, anno_btn);

    //filter menu button
    auto filter_btn = new QToolButton();
    filter_btn->setIcon(QIcon(":/Icons/Filter.png"));
	filter_btn->setToolTip("Apply default variant filters.");
	filter_btn->setMenu(new QMenu());
    filter_btn->menu()->addAction(ui_.actionFiltersGermline);
    connect(ui_.actionFiltersGermline, SIGNAL(triggered(bool)), this, SLOT(applyDefaultFiltersGermline()));
	filter_btn->menu()->addAction(ui_.actionFiltersTrio);
	connect(ui_.actionFiltersTrio, SIGNAL(triggered(bool)), this, SLOT(applyDefaultFiltersTrio()));
	filter_btn->menu()->addAction(ui_.actionFiltersMultiSample);
	connect(ui_.actionFiltersMultiSample, SIGNAL(triggered(bool)), this, SLOT(applyDefaultFiltersMultiSample()));
    filter_btn->menu()->addAction(ui_.actionFiltersSomatic);
    connect(ui_.actionFiltersSomatic, SIGNAL(triggered(bool)), this, SLOT(applyDefaultFiltersSomatic()));
    filter_btn->menu()->addAction(ui_.actionFiltersClear);
    connect(ui_.actionFiltersClear, SIGNAL(triggered(bool)), this, SLOT(clearFilters()));
    filter_btn->setPopupMode(QToolButton::InstantPopup);
    ui_.tools->insertWidget(ui_.actionReport, filter_btn);

	//signals and slots
	connect(ui_.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui_.vars, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(varsContextMenu(QPoint)));
	connect(filter_widget_, SIGNAL(filtersChanged()), this, SLOT(filtersChanged()));
	connect(filter_widget_, SIGNAL(targetRegionChanged()), this, SLOT(resetAnnoationStatus()));
	connect(ui_.vars, SIGNAL(itemSelectionChanged()), this, SLOT(updateVariantDetails()));
	connect(&filewatcher_, SIGNAL(fileChanged()), this, SLOT(handleInputFileChange()));
	connect(ui_.vars, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(variantDoubleClicked(QTableWidgetItem*)));
	connect(ui_.actionDesignSubpanel, SIGNAL(triggered()), this, SLOT(openSubpanelDesignDialog()));

	//misc initialization
	filewatcher_.setDelayInSeconds(10);

	//if at home, use Patientenserver
	if (QDir("Q:\\TRANSFER\\Auf-Patientenserver\\").exists())
	{
		last_report_path_ = "Q:\\TRANSFER\\Auf-Patientenserver\\";
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

void MainWindow::on_actionCNV_triggered()
{
	CnvWidget* list = new CnvWidget(filename_);
	list->setGenesFilter(filter_widget_->genes());
	list->setRoiFilter(filter_widget_->targetRegion());
	connect(list, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	auto dlg = GUIHelper::showWidgetAsDialog(list, "Copy-number variants", false, false);
	addModelessDialog(dlg);
}

void MainWindow::on_actionGeneSelector_triggered()
{
	if (filename_=="") return;

	//show dialog
	QString sample_folder = QFileInfo(filename_).absolutePath();
	QString sample_name = QFileInfo(filename_).baseName();
	GeneSelectorDialog dlg(sample_folder, sample_name, this);
	connect(&dlg, SIGNAL(openRegionInIGV(QString)), this, SLOT(openInIGV(QString)));
	if (dlg.exec())
	{
		//copy report to clipboard
		QString report = dlg.report();
		QApplication::clipboard()->setText(report);

		//show message
		if (QMessageBox::question(this, "Gene selection report", "Gene selection report was copied to clipboard.\nDo you want to open the sub-panel design dialog for selected genes?")==QMessageBox::Yes)
		{
			QStringList genes = dlg.genesForVariants();
			openSubpanelDesignDialog(genes);
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

	//load from INI file
	if (Settings::allKeys().count()<5)
	{
		Settings::restoreBackup();
		if (Settings::allKeys().count()<5)
		{
			QMessageBox::warning(this, "GSvar ini file empty", "The ini file '" + Settings::fileName() + "' is empty.\nPlease inform your administrator!");
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
	updatePreferredTranscripts();
	updateNGSDSupport();

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

void MainWindow::variantDoubleClicked(QTableWidgetItem* item)
{
	if (item==nullptr) return;

	openInIGV(variants_[item->row()].toString());
}

void MainWindow::openInIGV(QString region)
{
	QStringList init_commands;
	if (!igv_initialized_)
	{
		IgvDialog dlg(this);

		//sample VCF
        QString folder = QFileInfo(filename_).absolutePath();
        QStringList files = Helper::findFiles(folder,"*_var_annotated.vcf.gz", false);
        if (files.count()==1)
        {
			dlg.addFile("variants (VCF)", files[0], ui_.actionIgvSample->isChecked());
        }

		//sample BAM file(s)
		QMap<QString, QString> bams = getBamFiles();
		if (bams.empty()) return;

		auto it = bams.cbegin();
		while(it!=bams.cend())
		{
			dlg.addFile(it.key() + " (BAM)", it.value(), true);
			++it;
		}

		//reference BAM
		QString ref = filter_widget_->referenceSample();
		if (ref!="")
		{
			dlg.addFile("reads reference (BAM)", ref, true);
		}

		//sample CNVs
		files = Helper::findFiles(folder,"*_cnvs.seg", false);
		if (files.count()==1)
		{
			dlg.addFile("CNVs track", files[0], true);
		}

		//target region
		QString roi = filter_widget_->targetRegion();
		if (roi!="")
		{
			dlg.addFile("target region track", roi, true);
		}

		//sample low-coverage
        files = Helper::findFiles(folder,"*_lowcov.bed", false);
        if (files.count()==1)
        {
			dlg.addFile("low-coverage regions track", files[0], ui_.actionIgvLowcov->isChecked());
		}

		//amplicon file (of processing system)
		try
		{
			QString ps_roi = NGSD().getProcessingSystem(filename_, NGSD::FILE);
			QString amplicons = ps_roi.left(ps_roi.length()-4) + "_amplicons.bed";
			if (QFile::exists(amplicons))
			{
				dlg.addFile("amplicons track (of processing system)", amplicons, true);
			}
		}
		catch(...)
		{
			//nothing to do here
		}

		//custom tracks
		dlg.addSeparator();
		QList<QAction*> igv_actions = ui_.menuTracks->findChildren<QAction*>();
		foreach(QAction* action, igv_actions)
		{
			QString text = action->text();
			if (!text.startsWith("custom track:")) continue;
			dlg.addFile(text, action->toolTip(), action->isChecked());
		}

		//execute dialog
		if (dlg.exec())
		{
			QStringList files_to_load = dlg.filesToLoad();
			init_commands.append("new");
			init_commands.append("genome 1kg_v37");

			//load non-BAM files
			foreach(QString file, files_to_load)
			{
				if (!file.endsWith(".bam"))
				{
					init_commands.append("load \"" + file + "\"");
				}
			}

			//collapse tracks
			init_commands.append("collapse");

			//load BAM files
			foreach(QString file, files_to_load)
			{
				if (file.endsWith(".bam"))
				{
					init_commands.append("load \"" + file + "\"");
				}
			}

			igv_initialized_ = true;
		}
		else //skipped
		{
			if (dlg.skipForSession()) igv_initialized_ = true;
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

void MainWindow::addModelessDialog(QSharedPointer<QDialog> ptr)
{
	modeless_dialogs_.append(ptr);

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
	QString filename = QFileDialog::getOpenFileName(this, "Open variant list", path, "Supported formats (*.GSvar;*.tsv);; GSvar files (*.GSvar);;TSV files (*.tsv);;All files (*.*)");
	if (filename=="") return;

	//update data
	loadFile(filename);
}

void MainWindow::on_actionOpenNGSD_triggered()
{
	//get processed sample name
	QString ps_name = QInputDialog::getText(this, "Open processed sample from NGSD", "processed sample name:").trimmed();
	if (ps_name=="") return;

	//convert name to file
	try
	{
		QString file = NGSD().processedSamplePath(ps_name, NGSD::GSVAR);
		loadFile(file);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Open processed sample from NGSD", e.message());
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
	filter_widget_->reset(true, false);
	filename_ = "";
	filewatcher_.clearFile();
	db_annos_updated_ = NO;
	igv_initialized_ = false;
	ui_.vars->setRowCount(0);
	ui_.vars->setColumnCount(0);

	if (filename=="") return;

	//update recent files (before try block to remove non-existing files from the recent files menu)
	addToRecentFiles(filename);

	//load data
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	try
	{
		variants_.load(filename);
		filter_widget_->setFilterColumns(variants_.filters());

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
	}

	//warn if no 'filter' column is present
	if (variants_.annotationIndexByName("filter", true, false)==-1)
	{
		QMessageBox::warning(this, "Error: 'filter' column missing", "GSvar file does not contains the required 'filter' column.\nRun reanalysis starting from annotation using the sample information dialog!");
	}
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About " + QCoreApplication::applicationName(), QCoreApplication::applicationName()+ " " + QCoreApplication::applicationVersion()+ "\n\nA free viewing and filtering tool for genomic variants.\n\nInstitute of Medical Genetics and Applied Genomics\nUniversity Hospital Tübingen\nGermany\n\nMore information at:\nhttps://github.com/imgag/ngs-bits");
}

void MainWindow::on_actionResize_triggered()
{
	ui_.vars->resizeColumnsToContents();
	ui_.vars->resizeRowsToContents();

	//limit column width
	for (int i=0; i<ui_.vars->columnCount(); ++i)
	{
		int max = (i<6) ? 80 : 200;
		if (ui_.vars->columnWidth(i)>max)
		{
			ui_.vars->setColumnWidth(i, max);
		}
	}
}

void MainWindow::on_actionReport_triggered()
{
	if (variants_.count()==0) return;

	//check if this is a germline or somatic
	VariantListType type = getType();
	if (type==SOMATIC_PAIR)
	{
		generateReportSomatic();
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

void MainWindow::generateReportSomatic()
{
	QString roi_file = filter_widget_->targetRegion();
	if (roi_file=="")
	{
		QMessageBox::warning(this, "Somatic report", "Report cannot be created because target region is not set.");
		return;
	}

	//init
	NGSD db;
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
	int i_snpq = variants_.annotationIndexByName("snp_q", true, true);
	int i_tum_af = variants_.annotationIndexByName("tumor_af", true, true);
	int i_tum_dp = variants_.annotationIndexByName("tumor_dp", true, true);
	int i_nor_af = variants_.annotationIndexByName("normal_af", true, true);
	int i_nor_dp = variants_.annotationIndexByName("normal_dp", true, true);
	int i_cosm = variants_.annotationIndexByName("COSMIC", true, true);

	//determine tumor/normal processed sample name
	QString base_name = QFileInfo(filename_).baseName();
	QStringList parts = base_name.replace("-", "_").replace(".", "_").split("_");
	QString tumor = parts[0] + "_" + parts[1];
	QString normal = parts[2] + "_" + parts[3];

	//report header
	QString temp_filename = Helper::tempFileName(".html");
	QSharedPointer<QFile> outfile = Helper::openFileForWriting(temp_filename);
	QTextStream stream(outfile.data());
	ReportWorker::writeHtmlHeader(stream, tumor + "-" + normal);

	stream << "<h4>Technischer Report zur bioinformatischen Analyse</h4>" << endl;
	stream << "<p>Probe Tumor: " << tumor << " (" << db.getExternalSampleName(tumor) << ")" << endl;
	stream << "<br />Probe Normal: " << normal << " (" << db.getExternalSampleName(normal) << ")" << endl;
	stream << "<br />Prozessierungssystem: " << db.getProcessingSystem(tumor, NGSD::LONG) << endl;
	stream << "<br />Datum: " << QDate::currentDate().toString("dd.MM.yyyy") << endl;
	stream << "<br />User: " << Helper::userName() << endl;
	stream << "<br />Analysesoftware: "  << QCoreApplication::applicationName() << " " << QCoreApplication::applicationVersion() << endl;
	stream << "</p>" << endl;

	//QC statistics
	stream << "<p><b>Statistik Sequenzierung:</b>" << endl;
	QCCollection stats = db.getQCData(tumor);
	for (int i=0; i<stats.count(); ++i)
	{
		if (stats[i].accession()=="QC:2000025" || stats[i].accession()=="QC:2000030")
		{
			stream << "<br />Tumor - " << stats[i].name() << ": " << stats[i].asString();
		}
	}
	stats = db.getQCData(normal);
	for (int i=0; i<stats.count(); ++i)
	{
		if (stats[i].accession()=="QC:2000025" || stats[i].accession()=="QC:2000030")
		{
			stream << "<br />Nomal - " << stats[i].name() << ": " << stats[i].asString();
		}
	}
	stream << "</p>" << endl;

	//somatic variants
	stream << "<p><b>Varianten:</b>" << endl;
	QList<int> indices;
	for (int i=0; i<variants_.count(); ++i)
	{
		if (ui_.vars->isRowHidden(i)) continue;
		indices.append(i);
	}
	stream << "<br />Variantenzahl: " << indices.count() << endl;

	stream << "<table>" << endl;
	stream << "<tr> <td><b>Position</b></td> <td><b>Ref</b></td> <td><b>Obs</b></td> <td><b>Quality</b></td> <td><b><nobr>Tumor AF/DP</nobr></b></td> <td><b><nobr>Normal AF/DP</nobr></b></td> <td><b>COSMIC</b></td> <td><b>Details</b></td> </tr>" << endl;
	foreach(int i, indices)
	{
		const Variant& variant = variants_[i];
		stream << "<tr>" << endl;
		stream << "<td><nobr>" << variant.chr().str() << ":" << variant.start() << "-" << variant.end() << "</nobr></td><td>" << variant.ref() << "</td><td>" << variant.obs() << "</td>";
		stream << "<td>" << variant.annotations().at(i_snpq) << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_tum_af)  << "/" << variant.annotations().at(i_tum_dp) << "</td>" << endl;
		stream << "<td>" << variant.annotations().at(i_nor_af)  << "/" << variant.annotations().at(i_nor_dp) << "</td>" << endl;
		QByteArray tmp = variant.annotations().at(i_cosm);
		tmp.replace(",", "");
		stream << "<td>" << tmp << "</td>" << endl;
		tmp = variant.annotations().at(i_co_sp);
		tmp.replace(",", " ");
		tmp.replace(">", "&gt;");
		tmp.replace("&", "&name;");
		stream << "<td>" << tmp << "</td>" << endl;
		stream << "</tr>" << endl;
	}
	stream << "</table>" << endl;
	stream << "Position = chromosomale Position der Variante (hg19), Ref = Wildtyp-Allel, Obs = mutiertes Allel, AF / DF = Anteil der Variante an allen Reads (Allelfrequenz) / gesamte Sequenziertiefe, COSMIC = Catalogue of Somatic Mutations in Cancer, Details: Auswirkung der Variante auf cDNA- bzw. Proteinebene inkl. Transkript ID" << endl;
	stream << "</p>" << endl;

	//CNV table
	BedFile roi = BedFile();
	roi.load(roi_file);
	ChromosomalIndex<BedFile> roi_idx(roi);
	stream << "<p><b>CNVs:</b>" << endl;
	stream << "<table>" << endl;
	QStringList file = Helper::loadTextFile(QString(filename_).replace(".GSvar", "_cnvs.tsv"), true);
	foreach(QString line, file)
	{
		if (line=="" || line.startsWith("##")) continue;

		if (line.startsWith("#")) //header
		{
			stream << "<tr><td><b>" << line.replace("\t", "</b></td><td><b>") << "</b></td></tr>" << endl;
		}
		else //contents
		{
			QStringList parts = line.split("\t");
			if (roi_idx.matchingIndex(parts[0], parts[1].toInt(), parts[2].toInt())!=-1)
			{
				stream << "<tr><td>" << line.replace("\t", "</td><td>") << "</td></tr>" << endl;
			}
		}
	}
	stream << "</table>" << endl;

	//CNV loss/gain genes list
	QStringList genes_loss;
	QStringList genes_gain;
	foreach(QString line, file)
	{
		if (line=="" || line.startsWith("#")) continue;

		QStringList parts = line.split("\t");
		if (roi_idx.matchingIndex(parts[0], parts[1].toInt(), parts[2].toInt())!=-1)
		{
			QStringList genes = parts[9].split(',');
			QStringList cns = parts[6].split(',');
			if (cns[0].toInt()>2)
			{
				genes_gain << genes;
			}
			else
			{
				genes_loss << genes;
			}
		}
	}
	genes_loss.sort();
	genes_loss.removeDuplicates();
	stream << "<br /> Gene mit Deletion: " << genes_loss.join(", ") << endl;
	genes_gain.sort();
	genes_gain.removeDuplicates();
	stream << "<br /> Gene mit Amplifikation: " << genes_gain.join(", ") << endl;
	stream << "</p>" << endl;

	//gaps
	stream << "<p><b>Lückenstatistik:</b>" << endl;
	stream << "<br />Zielregion: " << QFileInfo(roi_file).fileName();
	GeneSet genes = GeneSet::createFromFile(roi_file.left(roi_file.size()-4) + "_genes.txt");
	if (!genes.isEmpty())
	{
		stream << "<br />Zielregion Gene (" << QString::number(genes.count()) << "): " << genes.join(", ") << endl;
	}
	stream << "<br />Zielregion Regionen: " << roi.count();
	stream << "<br />Zielregion Basen: " << roi.baseCount();

	BedFile roi_inter;
	roi_inter.load(db.getProcessingSystem(tumor, NGSD::FILE));
	roi_inter.intersect(roi);
	if (roi_inter.baseCount()!=roi.baseCount())
	{
		QString message = "Gaps cannot be calculated because the selected target region is larger than the processing system target region:";
		BedFile roi_missing;
		roi_missing.load(roi_file);
		roi_missing.subtract(roi_inter);
		for (int i=0; i<std::min(10, roi_missing.count()); ++i)
		{
			message += "\n" + roi_missing[i].toString(true);
		}
		QMessageBox::warning(this, "Invalid target region", message);
		return;
	}

	QString low_cov_file = filename_;
	low_cov_file.replace(".GSvar", "_stat_lowcov.bed");
	BedFile low_cov;
	low_cov.load(low_cov_file);
	low_cov.intersect(roi);
	stream << "<br />Lücken Regionen: " << low_cov.count();
	stream << "<br />Lücken Basen: " << low_cov.baseCount() << " (" << QString::number(100.0 * low_cov.baseCount()/roi.baseCount(), 'f', 2) << "%)";


	//annotate low-coverage regions with gene names
	for(int i=0; i<low_cov.count(); ++i)
	{
		BedLine& line = low_cov[i];
		GeneSet genes = db.genesOverlapping(line.chr(), line.start(), line.end(), 20); //extend by 20 to annotate splicing regions as well
		line.annotations().append(genes.join(", "));
	}

	//group by gene name
	QHash<QByteArray, BedFile> grouped;
	for (int i=0; i<low_cov.count(); ++i)
	{
		QList<QByteArray> genes = low_cov[i].annotations()[0].split(',');
		foreach(QByteArray gene, genes)
		{
			gene = gene.trimmed();

			//skip non-gene regions
			// - remains of VEGA database in old HaloPlex designs
			// - SNPs for sample identification
			if (gene=="") continue;

			grouped[gene].append(low_cov[i]);
		}
	}

	stream << "<table>" << endl;
	stream << "<tr><td><b>Gen</b></td><td><b>L&uuml;cken</b></td><td><b>Chromosom</b></td><td><b>Koordinaten (hg19)</b></td></tr>" << endl;
	for (auto it=grouped.cbegin(); it!=grouped.cend(); ++it)
	{
		stream << "<tr> <td>" << endl;
		const BedFile& gaps = it.value();
		QString chr = gaps[0].chr().strNormalized(true);;
		QStringList coords;
		for (int i=0; i<gaps.count(); ++i)
		{
			coords << QString::number(gaps[i].start()) + "-" + QString::number(gaps[i].end());
		}
		stream << it.key() << "</td><td>" << gaps.baseCount() << "</td><td>" << chr << "</td><td>" << coords.join(", ") << endl;

		stream << "</td> </tr>" << endl;
	}
	stream << "</table>" << endl;
	stream << "</p>" << endl;

	//close stream
	ReportWorker::writeHtmlFooter(stream);
	outfile->close();

	//validate/store
	QString file_rep = QFileDialog::getSaveFileName(this, "Export report file", last_report_path_ + "/" + base_name + "_report_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	ReportWorker::validateAndCopyReport(temp_filename, file_rep);

	//show result info box
	if (QMessageBox::question(this, "Report", "Report generated successfully!\nDo you want to open the report in your browser?")==QMessageBox::Yes)
	{
		QDesktopServices::openUrl(file_rep);
	}
}

void MainWindow::generateReport()
{
	//check if NGSD annotations are present
	if (variants_.annotationIndexByName("classification", true, false)==-1
	 || variants_.annotationIndexByName("ihdb_allsys_hom", true, false)==-1
	 || variants_.annotationIndexByName("ihdb_allsys_het", true, false)==-1
	 || variants_.annotationIndexByName("comment", true, false)==-1
	 || variants_.annotationIndexByName("gene_info", true, false)==-1)
	{
		GUIHelper::showMessage("Error", "Cannot generate report without complete NGSD annotation!\nPlease re-annotate NGSD information first!");
		return;
	}

	//check if NGSD annotations are up-to-date
	QString mod_date = QFileInfo(filename_).lastModified().toString("yyyy-MM-dd");
	if (db_annos_updated_==NO)
	{
		if (QMessageBox::question(this, "Report", "NGSD re-annotation not performed!\nDo you want to continue with annotations from " + mod_date + "?")==QMessageBox::No)
		{
			return;
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
	if (!missing.empty() && QMessageBox::question(this, "Report", "Gene inheritance information is missing for these genes:\n" + missing.join(", ")+"\nDo you want to continue?")==QMessageBox::No)
	{
		return;
	}

	//show report dialog
	ReportDialog dialog(filename_, this);
	dialog.addVariants(variants_, visible);
	dialog.setTargetRegionSelected(filter_widget_->targetRegion()!="");
	if (!dialog.exec()) return;

	//get export file name
	QString base_name = QFileInfo(filename_).baseName();
	QString file_rep = QFileDialog::getSaveFileName(this, "Export report file", last_report_path_ + "/" + base_name + targetFileName() + "_report_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (file_rep=="") return;
	last_report_path_ = QFileInfo(file_rep).absolutePath();

	//get BAM file name if necessary
	QString bam_file = "";
	if (dialog.detailsCoverage())
	{
		QMap<QString, QString> bams = getBamFiles();
		if (bams.count()==0) return;
		bam_file = bams.values().first();
	}

	//flag report variants in NGSD
	try
	{
		NGSD().setReportVariants(filename_, variants_, dialog.selectedIndices().toSet());
		NGSD().setReportOutcome(filename_, dialog.outcome());
	}
	catch (DatabaseException& e)
	{
		QMessageBox::warning(this, "Report variants in NGSD", "Flagging report variants in NGSD failed:\n" + e.message());
	}

	//show busy dialog
	busy_dialog_ = new BusyDialog("Report", this);
	busy_dialog_->init("Generating report", false);

	//start worker in new thread
	ReportWorker* worker = new ReportWorker(base_name, filter_widget_->appliedFilters(), variants_, dialog.selectedIndices(), preferred_transcripts_, dialog.outcome(), filter_widget_->targetRegion(), bam_file, dialog.minCoverage(), getLogFiles(), file_rep, dialog.calculateDepth());
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

void MainWindow::annotateVariantsComplete()
{
	if (variants_.count()==0) return;

	//show busy dialog
	busy_dialog_ = new BusyDialog("Database annotation", this);

	//start worker
	DBAnnotationWorker* worker = new DBAnnotationWorker(filename_, variants_, busy_dialog_);
	connect(worker, SIGNAL(finished(bool)), this, SLOT(databaseAnnotationFinished(bool)));
	worker->start();
}


void MainWindow::annotateVariantsROI()
{
	if (variants_.count()==0) return;

	if (filter_widget_->targetRegion()=="")
	{
		QMessageBox::warning(this, "Error", "ROI-based variant annotation is only possible when a target region is selected!");
		return;
	}

	//show busy dialog
	busy_dialog_ = new BusyDialog("Database annotation", this);

	//start worker
	DBAnnotationWorker* worker = new DBAnnotationWorker(filename_, variants_, busy_dialog_, filter_widget_->targetRegion());
	connect(worker, SIGNAL(finished(bool)), this, SLOT(databaseAnnotationFinished(bool)));
	worker->start();
}

QMap<QString, MainWindow::SampleInfo> MainWindow::getSampleHeader()
{
	QMap<QString, SampleInfo> output;

	foreach(QString line, variants_.comments())
	{
		line = line.trimmed();

		if (line.startsWith("##SAMPLE=<"))
		{
			auto parts = line.mid(10, line.length()-11).split(',');
			QString name;
			foreach(const QString& part, parts)
			{
				int sep_idx = part.indexOf('=');
				if (sep_idx==-1)
				{
					qDebug() << "Invalid sample header entry " << part << " in " << line;
					continue;
				}

				QString key = part.left(sep_idx);
				QString value = part.mid(sep_idx+1);
				if (key=="ID")
				{
					name = value;
					output[name].column_name = value;
				}
				else
				{
					output[name].properties[key] = value;
				}
			}
		}
	}

	//special handling of single-sample analysis
	for (int i=0; i<variants_.annotations().count(); ++i)
	{
		if (variants_.annotations()[i].name()=="genotype")
		{
			if (output.count()==0) //old single-sample analysis without '#SAMPLE header'. Old trio/somatic variant lists are no longer supported.
			{
				QString name = QFileInfo(filename_).baseName();
				output[name].properties["Status"] = "Affected";
			}

			if (output.count()==1)
			{
				output.first().column_name = "genotype";
			}
		}
		break;
	}

	return output;
}

QStringList MainWindow::genotypeColumns(MainWindow::AffectedState state)
{
	QStringList output;

	QMap<QString, SampleInfo> data = getSampleHeader();
	foreach(const SampleInfo& info, data)
	{
		if (state==ALL || (state==AFFECTED && info.isAffected()) || (state==CONTROL && !info.isAffected()))
		{
			output << info.column_name;
		}
	}

	return output;
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

	//clean
	worker->deleteLater();
}

void MainWindow::applyDefaultFiltersGermline()
{
	filter_widget_->applyDefaultFilters();
}

void MainWindow::applyDefaultFiltersTrio()
{
	filter_widget_->applyDefaultFiltersTrio();
}

void MainWindow::applyDefaultFiltersMultiSample()
{
	filter_widget_->applyDefaultFiltersMultiSample();
}

void MainWindow::applyDefaultFiltersSomatic()
{
	filter_widget_->applyDefaultFiltersSomatic();
}

void MainWindow::clearFilters()
{
	filter_widget_->reset(false, true);
}

void MainWindow::on_actionNGSD_triggered()
{
	if (filename_=="") return;

	try
	{
		QString url = NGSD().url(filename_);
		QDesktopServices::openUrl(QUrl(url));
	}
	catch (DatabaseException e)
	{
		GUIHelper::showMessage("NGSD error", "The processed sample database ID could not be determined!\nDoes the file name '"  + filename_ + "' start with the processed sample ID?\nError message: " + e.message());
		return;
	}
}

void MainWindow::on_actionSampleInformation_triggered()
{
	if (filename_=="") return;

	SampleInformationDialog dialog(this, filename_);
	dialog.exec();
}

void MainWindow::on_actionGenderXY_triggered()
{
	ExternalToolDialog dialog("SampleGender", "-method xy", this);
	dialog.exec();
}

void MainWindow::on_actionGenderHet_triggered()
{
	ExternalToolDialog dialog("SampleGender", "-method hetx", this);
	dialog.exec();
}

void MainWindow::on_actionGenderSRY_triggered()
{
	ExternalToolDialog dialog("SampleGender", "-method sry", this);
	dialog.exec();
}

void MainWindow::on_actionStatisticsBED_triggered()
{
	ExternalToolDialog dialog("BedInfo", "", this);
	dialog.exec();
}

void MainWindow::on_actionStatisticsFastA_triggered()
{
	ExternalToolDialog dialog("FastaInfo", "", this);
	dialog.exec();
}

void MainWindow::on_actionSampleCorrelationTSV_triggered()
{
	ExternalToolDialog dialog("SampleCorrelation", "", this);
	dialog.exec();
}

void MainWindow::on_actionSampleCorrelationBAM_triggered()
{
	ExternalToolDialog dialog("SampleCorrelation", "-bam", this);
	dialog.exec();
}

void MainWindow::on_actionSampleDiff_triggered()
{
	ExternalToolDialog dialog("SampleDiff", "", this);
	dialog.exec();
}

void MainWindow::on_actionTrio_triggered()
{
	TrioDialog dlg(this);
	if (dlg.exec()==QDialog::Accepted)
	{
		HttpHandler handler;
		QString reply = handler.getHttpReply(Settings::string("SampleStatus")+"restart.php?type=trio&high_priority&user="+Helper::userName()+"&f=" + dlg.father() + "&m=" + dlg.mother() + "&c=" + dlg.child());
		if (!reply.startsWith("Restart successful"))
		{
			QMessageBox::warning(this, "Trio analysis", "Queueing analysis failed:\n" + reply);
		}
		else
		{
			QMessageBox::information(this, "Trio analysis", "Queueing analysis successful!");
		}
	}
}

void MainWindow::on_actionMultiSample_triggered()
{
	MultiSampleDialog dlg(this);
	if (dlg.exec()==QDialog::Accepted)
	{
		HttpHandler handler;
		QString reply = handler.getHttpReply(Settings::string("SampleStatus")+"restart.php?type=multi&high_priority&user="+Helper::userName()+"&samples=" + dlg.samples().join(',')+"&status=" + dlg.status().join(','));
		if (!reply.startsWith("Restart successful"))
		{
			QMessageBox::warning(this, "Multi-sample analysis", "Queueing analysis failed:\n" + reply);
		}
		else
		{
			QMessageBox::information(this, "Multi-sample analysis", "Queueing analysis successful!");
		}
	}
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
	GUIHelper::showWidgetAsDialog(edit, "Gaps of gene '" + gene + "' from low-coverage BED file '" + report + "':", false);
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
	QMap<QString, QString> bams = getBamFiles();
	if (bams.empty()) return;
	QString bam_file = bams.values().first();

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

	//load original VCF
	QString orig_name = filename_;
	orig_name.replace(".GSvar", "_var_annotated.vcf.gz");
	if (!QFile::exists(orig_name))
	{
		GUIHelper::showMessage("VCF export error", "Could not find original VCF file '" + orig_name + "'!");
		return;
	}
	VariantList orig_vcf;
	orig_vcf.load(orig_name, VariantList::VCF_GZ, &roi);
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
		output.store(file_name, VariantList::VCF);
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
		output.store(file_name, VariantList::TSV);
	}
}

void MainWindow::on_actionShowTranscripts_triggered()
{
	QString text = "<pre>";
	for (auto it=preferred_transcripts_.cbegin(); it!=preferred_transcripts_.cend(); ++it)
	{
		text += it.key() + "\t" + it.value().join(", ") + "\n";
	}
	text += "</pre>";
	QTextEdit* edit = new QTextEdit(text);
	GUIHelper::showWidgetAsDialog(edit, "Preferred transcripts list", false);
}

void MainWindow::on_actionImportTranscripts_triggered()
{
	//check if gene list file is set
	QString filename = QFileDialog::getOpenFileName(this, "Import preferred transcript file", "" , "TSV files (*.tsv);;All files (*.*)");
	if (filename=="") return;

	//parse file
	QMap<QString, QStringList> preferred_transcripts;
	QStringList file = Helper::loadTextFile(filename);
	foreach(QString line, file)
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
		QString gene = parts[0].trimmed();
		NGSD db;
		int gene_id = db.geneToApprovedID(gene);
		if(gene_id==-1)
		{
			QMessageBox::warning(this, "Invalid preferred transcript line", "Gene name '" + gene + "' is not a HGNC-approved name!\n\nAborting!");
			return;
		}
		gene = db.geneSymbol(gene_id);

		//remove version number if present (NM_000543.3 => NM_000543.)
		QString transcript = parts[1].trimmed();
		if (transcript.contains("."))
		{
			transcript = transcript.left(transcript.lastIndexOf('.'));
		}

		preferred_transcripts[gene].append(transcript);
	}

	//store in INI file
	QMap<QString, QVariant> tmp;
	for(auto it=preferred_transcripts.cbegin(); it!=preferred_transcripts.cend(); ++it)
	{
		tmp.insert(it.key(), it.value());
	}
	Settings::setMap("preferred_transcripts", tmp);

	//update in-memory copy of preferred transcripts
	updatePreferredTranscripts();
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

	GeneInfoDialog dlg(symbol, this);
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

void MainWindow::openSubpanelDesignDialog(QStringList genes)
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
	//no selection
	if (ui_.vars->selectedRanges().count()!=1) return;
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
			if (split_quality && col==qual_index)
			{
				QStringList quality_values;
				for(int i=0; i<quality_keys.count(); ++i) quality_values.append("");
				QStringList entries = ui_.vars->item(row, col)->text().split(';');
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
				selected_text.append(ui_.vars->item(row, col)->text().replace('\n',' ').replace('\r',' '));
			}
		}
	}

	QApplication::clipboard()->setText(selected_text);
}

QString MainWindow::nobr()
{
	return "<p style='white-space:pre; margin:0; padding:0;'>";
}

void MainWindow::uploadtoLovd(int variant_index)
{
/*
	HttpHandler http_handler;
	qDebug() << "GET HTTPS";
	qDebug() << http_handler.getHttpReply("https://httpbin.org/get");

	qDebug() << "POST HTTPS";
	qDebug() << http_handler.getHttpReply("https://httpbin.org/post", "my_conent");

	qDebug() << "GET HTTP (local)";
	qDebug() << http_handler.getHttpReply("http://localhost/");

	qDebug() << "GET HTTP";
	qDebug() << http_handler.getHttpReply("http://httpbin.org/get");

	qDebug() << "POST HTTP";
	qDebug() << http_handler.getHttpReply("http://httpbin.org/post", "my_conent");
*/

	QString sample = QFileInfo(filename_).baseName();
	NGSD db;
	QString gender = db.sampleGender(sample);

	//select phenotype
	PhenotypeSelector* p_sel = new PhenotypeSelector();
	QSharedPointer<QDialog> dlg = GUIHelper::showWidgetAsDialog(p_sel, "Select patient phenotype", true);
	Phenotype pheno = p_sel->selectedPhenotype();
	if (dlg->result()==QDialog::Rejected || pheno.name().isEmpty()) return;

	//select gene
	QByteArray gene_anno = variants_[variant_index].annotations()[variants_.annotationIndexByName("gene")];
	QStringList genes = QString(gene_anno).split(',');
	QString gene = genes[0];
	if (genes.count()!=1)
	{
		bool ok = false;
		gene = (genes.count()==1 ? genes[0] : QInputDialog::getItem(this, "Select gene", "affected gene:", genes, 0, false, &ok));
		if (!ok) return;
	}

	//gene to approved
	gene = db.geneToApproved(gene).first;

	QByteArray upload_file = LovdUploadFile::create(sample, gender, gene, pheno, variants_, variants_[variant_index]);
	if (!upload_file.isEmpty())
	{
		HttpHandler http_handler;
		QString reply = http_handler.getHttpReply("http://databases.lovd.nl/shared/api/submissions", upload_file);
		qDebug() << "REPLY:" << reply;
	}
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
	QMap<QString, SampleInfo> sample_data = getSampleHeader();

	//resize
	ui_.vars->setRowCount(variants_.count());
	ui_.vars->setColumnCount(5 + variants_.annotations().count());

	//header
	ui_.vars->setHorizontalHeaderItem(0, new QTableWidgetItem("chr"));
	ui_.vars->setHorizontalHeaderItem(1, new QTableWidgetItem("start"));
	ui_.vars->setHorizontalHeaderItem(2, new QTableWidgetItem("end"));
	ui_.vars->setHorizontalHeaderItem(3, new QTableWidgetItem("ref"));
	ui_.vars->setHorizontalHeaderItem(4, new QTableWidgetItem("obs"));
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
		foreach(const SampleInfo& info, sample_data)
		{
			if (info.column_name==anno)
			{
				auto it = info.properties.cbegin();
				while(it != info.properties.cend())
				{
					add_desc += "\n - "+it.key() + ": " + it.value();

					if (info.isAffected())
					{
						header->setForeground(QBrush(Qt::darkRed));
					}

					++it;
				}
			}
		}

		QString header_desc = "";
		VariantAnnotationDescription vad = variants_.annotationDescriptionByName(anno, false, false);
		if(!vad.description().isNull())	header_desc = vad.description();

		header->setToolTip(header_desc + add_desc);
		ui_.vars->setHorizontalHeaderItem(i+5, header);
	}

	//content
	int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, false);
	int i_validated = variants_.annotationIndexByName("validated", true, false);
	int i_classification = variants_.annotationIndexByName("classification", true, false);
	int i_comment = variants_.annotationIndexByName("comment", true, false);
	int i_ihdb_hom = variants_.annotationIndexByName("ihdb_allsys_hom", true, false);
	int i_ihdb_het = variants_.annotationIndexByName("ihdb_allsys_het", true, false);
	int i_clinvar = variants_.annotationIndexByName("ClinVar", true, false);
	int i_hgmd = variants_.annotationIndexByName("HGMD", true, false);
	for (int i=0; i<variants_.count(); ++i)
	{
		const Variant& row = variants_[i];
		ui_.vars->setItem(i, 0, new QTableWidgetItem(QString(row.chr().str())));
		ui_.vars->setItem(i, 1, new QTableWidgetItem(QString::number(row.start())));
		ui_.vars->setItem(i, 2, new QTableWidgetItem(QString::number(row.end())));
		ui_.vars->setItem(i, 3, new QTableWidgetItem(row.ref(), 0));
		ui_.vars->setItem(i, 4, new QTableWidgetItem(row.obs(), 0));
		bool is_warning_line = false;
		bool is_notice_line = false;
		bool is_ok_line = false;
		for (int j=0; j<row.annotations().count(); ++j)
		{
			QString anno = row.annotations().at(j);
			QTableWidgetItem* item = new QTableWidgetItem(anno);

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
			if (j==i_validated && anno.contains("TP"))
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


			ui_.vars->setItem(i, 5+j, item);
		}

		//mark vertical header - warning (red), notice (orange)
		if (is_notice_line && !is_ok_line)
		{
			QTableWidgetItem* item = new QTableWidgetItem(QString::number(i+1));
			item->setForeground(QBrush(QColor(255, 135, 60)));
			QFont font;
			font.setWeight(QFont::Bold);
			item->setFont(font);
			ui_.vars->setVerticalHeaderItem(i, item);
		}
		else if (is_warning_line && !is_ok_line)
		{
			QTableWidgetItem* item = new QTableWidgetItem(QString::number(i+1));
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
	//get item
	QTableWidgetItem* item = ui_.vars->itemAt(pos);
	if (!item) return;

	//init
	bool ngsd_enabled = Settings::boolean("NGSD_enabled", true);
	const Variant& variant = variants_[item->row()];
	int i_gene = variants_.annotationIndexByName("gene", true, true);
	QStringList genes = QString(variant.annotations()[i_gene]).split(',', QString::SkipEmptyParts);

	//create contect menu
	QMenu menu(ui_.vars);
	QMenu* sub_menu = menu.addMenu(QIcon("://Icons/NGSD.png"), "Variant");
	sub_menu->setEnabled(ngsd_enabled);
	sub_menu->addAction("Open variant in NGSD");
	sub_menu->addAction("Search for position in NGSD");
	sub_menu->addSeparator();
	sub_menu->addAction("Set validation status");
	sub_menu->addAction("Set classification");
	sub_menu->addAction("Edit comment");
	if (!genes.isEmpty())
	{
		sub_menu = menu.addMenu(QIcon("://Icons/NGSD.png"), "Gene info");
		foreach(QString g, genes)
		{
			sub_menu->addAction(g);
		}
		sub_menu->setEnabled(ngsd_enabled);
	}

	//PrimerDesign
	QAction* action = menu.addAction(QIcon("://Icons/PrimerDesign.png"), "PrimerDesign");
	action->setEnabled(Settings::string("PrimerDesign")!="");

	//Alamut
	if (Settings::string("Alamut")!="")
	{
		sub_menu = menu.addMenu(QIcon("://Icons/Alamut.png"), "Alamut");

		//BAM
		if (getType()==GERMLINE_SINGLESAMPLE)
		{
			sub_menu->addAction("BAM");
		}

		//genomic location
		sub_menu->addAction(variant.chr().str() + ":" + QByteArray::number(variant.start()));

		//genes
		foreach(QString g, genes)
		{
			sub_menu->addAction(g);
		}
		sub_menu->addSeparator();

		//transcripts
		int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);
		QList<QByteArray> transcripts = variant.annotations()[i_co_sp].split(',');
		foreach(QByteArray transcript, transcripts)
		{
			QList<QByteArray> parts = transcript.split(':');
			if (parts.count()>5)
			{
				QString gene = parts[0].trimmed();
				QString trans_id = parts[1].trimmed();
				QString cdna_change = parts[5].trimmed();
				if  (trans_id!="" && cdna_change!="")
				{
					QAction* action = sub_menu->addAction(trans_id + ":" + cdna_change + " (" + gene + ")");

					//highlight preferred transcripts
					if (preferred_transcripts_.value(gene).contains(trans_id))
					{
						QFont font = action->font();
						font.setBold(true);
						action->setFont(font);
					}
				}
			}
		}
	}

	/* TODO
	action = menu.addAction(QIcon(":/Icons/LOVD.png"), "Publish in LOVD");
	action->setEnabled(ngsd_enabled);
	*/

	//Execute menu
	action = menu.exec(ui_.vars->viewport()->mapToGlobal(pos));
	if (!action) return;
	QMenu* parent_menu = qobject_cast<QMenu*>(action->parent());

	QByteArray text = action->text().toLatin1();
	if (text=="Open variant in NGSD")
	{
		try
		{
			QString url = NGSD().url(filename_, variant);
			QDesktopServices::openUrl(QUrl(url));
		}
		catch (DatabaseException& e)
		{
			GUIHelper::showMessage("NGSD error", "The variant database ID could not be determined!\nDoes the file name '"  + filename_ + "' start with the prcessed sample ID?\nError message: " + e.message());
			return;
		}
	}
	else if (text=="Search for position in NGSD")
	{
		QString url = NGSD().urlSearch(variant.chr().str() + ":" + QString::number(variant.start()) + "-" + QString::number(variant.end()));
		QDesktopServices::openUrl(QUrl(url));
	}
	else if (text=="Set validation status")
	{
		try
		{
			int i_quality = variants_.annotationIndexByName("quality", true, true);
			ValidationDialog dlg(this, filename_, variant, i_quality);

			if (dlg.exec()) //update DB
			{
				NGSD().setValidationStatus(filename_, variant, dlg.info());

				//update GUI
				QByteArray status = dlg.info().status.toLatin1();
				if (status=="true positive") status = "TP";
				if (status=="false positive") status = "FP";
				int i_validated = variants_.annotationIndexByName("validated", true, true);
				variants_[item->row()].annotations()[i_validated] = status;
				variantListChanged();
			}
		}
		catch (DatabaseException& e)
		{
			GUIHelper::showMessage("NGSD error", e.message());
			return;
		}
	}
	else if (text=="Set classification")
	{
		try
		{
			ClassificationDialog dlg(this, variant);

			if (dlg.exec())
			{
				//update DB
					NGSD().setClassification(variant, dlg.classification(), dlg.comment());

				//update GUI
				int i_class = variants_.annotationIndexByName("classification", true, true);
				variants_[item->row()].annotations()[i_class] = dlg.classification().replace("n/a", "").toLatin1();
				int i_class_comment = variants_.annotationIndexByName("classification_comment", true, true);
				variants_[item->row()].annotations()[i_class_comment] = dlg.comment().toLatin1();
				variantListChanged();
			}
		}
		catch (DatabaseException& e)
		{
			GUIHelper::showMessage("NGSD error", e.message());
			return;
		}
	}
	else if (text=="Edit comment")
	{
		try
		{
			bool ok = true;
			QByteArray text = QInputDialog::getMultiLineText(this, "Variant comment", "Text: ", NGSD().comment(filename_, variant), &ok).toUtf8();

			if (ok)
			{
				//update DB
				int row = item->row();
				NGSD().setComment(filename_, variants_[row], text);

				//get annotation text (from NGSD to get comments of other samples as well)
				VariantList tmp;
				tmp.append(variants_[row]);
				NGSD().annotate(tmp, filename_);
				text = tmp[0].annotations()[tmp.annotationIndexByName("comment", true, true)];

				//update datastructure (if comment column is present)
				int col_index = variants_.annotationIndexByName("comment", true, false);
				if (col_index!=-1)
				{
					variants_[row].annotations()[col_index] = text;
				}

				//update GUI (if column is present)
				int gui_index = guiColumnIndex("comment");
				if (gui_index!=-1)
				{
					ui_.vars->item(item->row(), gui_index)->setText(text);
					var_widget_->updateVariant(variants_, row);
				}
			}
		}
		catch (DatabaseException& e)
		{
			GUIHelper::showMessage("NGSD error", e.message());
			return;
		}
	}
	else if (text=="PrimerDesign")
	{
		try
		{
			QString url = Settings::string("PrimerDesign")+"/index.php?user="+Helper::userName()+"&sample="+NGSD::sampleName(filename_)+"&chr="+variant.chr().str()+"&start="+QString::number(variant.start())+"&end="+QString::number(variant.end())+"";
			QDesktopServices::openUrl(QUrl(url));
		}
		catch (Exception& e)
		{
			GUIHelper::showMessage("NGSD error", "Error while processing'"  + filename_ + "'!\nError message: " + e.message());
			return;
		}
	}
	else if (text=="Publish in LOVD")
	{
		try
		{
			uploadtoLovd(item->row());
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
	else if (parent_menu && parent_menu->title()=="Alamut")
	{
		QStringList parts = action->text().split(" ");
		if (parts.count()>=1)
		{
			QString value = parts[0];
			if (value=="BAM")
			{
				QMap<QString, QString> bams = getBamFiles();
				if (bams.empty()) return;
				value = "BAM<" + bams.values().first();
			}
			HttpHandler().getHttpReply(Settings::string("Alamut")+"/show?request="+value);
		}
	}
}

void MainWindow::updateVariantDetails()
{
	if (!var_widget_->isVisible()) return;

	//determine variant (first in first range)
	auto ranges = ui_.vars->selectedRanges();
	if (ranges.count()!=1 || ranges[0].rowCount()!=1)
	{
		var_last_ = -1;
		var_widget_->clear();
		return;
	}

	//display variant details
	int row = ranges[0].topRow();
	if (row!=var_last_)
	{
		var_widget_->updateVariant(variants_, row);
		var_last_ = row;
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
		THROW(Exception, "Could not not execute IGV command '" + command + "'.\nAnswer: " + answer);
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

QMap<QString, QString> MainWindow::getBamFiles()
{
    QMap<QString, QString> output;

	QString sample_folder = QFileInfo(filename_).path();
	QString project_folder = QFileInfo(sample_folder).path();

	QMap<QString, SampleInfo> data = getSampleHeader();
	foreach(QString sample, data.keys())
	{
		QString bam1 = sample_folder + "/" + sample + ".bam";
		QString bam2 = project_folder + "/Sample_" + sample + "/" + sample + ".bam";
		if (QFile::exists(bam1))
		{
			output[sample] = bam1;
		}
		else if (QFile::exists(bam2))
		{
			output[sample] = bam2;
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

MainWindow::VariantListType MainWindow::getType()
{
	foreach(QString line, variants_.comments())
	{
		line = line.trimmed();
		if (line.startsWith("##ANALYSISTYPE="))
		{
			QString type = line.mid(15);
			if (type=="GERMLINE_SINGLESAMPLE") return GERMLINE_SINGLESAMPLE;
			else if (type=="GERMLINE_TRIO") return GERMLINE_TRIO;
			else if (type=="GERMLINE_MULTISAMPLE") return GERMLINE_MULTISAMPLE;
			else if (type=="SOMATIC_SINGLESAMPLE") return SOMATIC_SINGLESAMPLE;
			else if (type=="SOMATIC_PAIR") return SOMATIC_PAIR;
			else THROW(FileParseException, "Invalid analysis type '" + type + "' in GSvar file!");
		}
	}

	return GERMLINE_SINGLESAMPLE; //fallback for old files without ANALYSISTYPE header
}

void MainWindow::filtersChanged()
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	try
	{
		//apply annotation filters
		QTime timer;
		timer.start();

		//main filters
		VariantFilter filter(variants_);
		if (filter_widget_->applyMaf())
		{
			double max_af = filter_widget_->mafPerc() / 100.0;
			filter.flagByAllelFrequency(max_af);
		}
		if (filter_widget_->applyImpact())
		{
			filter.flagByImpact(filter_widget_->impact());
		}
		if (filter_widget_->applyIhdb())
		{
			QStringList geno_cols = (filter_widget_->ihdbIgnoreGenotype() ? QStringList() : genotypeColumns(AFFECTED));
			filter.flagByIHDB(filter_widget_->ihdb(), geno_cols);
		}
		QStringList remove = filter_widget_->filterColumnsRemove();
		if (remove.count()>0)
		{
			filter.flagByFilterColumnMatching(remove);
		}

		//filter columns (keep)
		QStringList keep = filter_widget_->filterColumnsKeep();
		if (keep.count()>0)
        {
			for(int i=0; i<variants_.count(); ++i)
			{
				if (filter.flags()[i]) continue;

				foreach(const QByteArray& f, variants_[i].filters())
				{
					if (keep.contains(f)) filter.flags()[i] = true;
				}
			}
		}

		//remove variants with low classification
		if (filter_widget_->applyClassification())
		{
			filter.flagByClassification(filter_widget_->classification());
		}

		//prevent class>=X variants from beeing filtered out by any filter (except region/gene filters)
		if (filter_widget_->keepClassGreaterEqual()!=-1)
		{
			int i_class = variants_.annotationIndexByName("classification", true, true);
			int min_class = filter_widget_->keepClassGreaterEqual();
			for(int i=0; i<variants_.count(); ++i)
			{
				if (filter.flags()[i]) continue;

				bool ok = false;
				int classification_value = variants_[i].annotations()[i_class].toInt(&ok);
				if (!ok) continue;

				filter.flags()[i] = (classification_value>=min_class);
			}
		}

		//prevent class M variants from beeing filtered out by any filter (except region/gene filters)
		if (filter_widget_->keepClassM())
		{
			int i_class = variants_.annotationIndexByName("classification", true, true);
			for(int i=0; i<variants_.count(); ++i)
			{
				if (filter.flags()[i]) continue;

				filter.flags()[i] = (variants_[i].annotations()[i_class]=="M");
			}
        }

        //filter columns (filter)
		QStringList filter_cols = filter_widget_->filterColumnsFilter();
		if (filter_cols.count()>0)
        {
            for(int i=0; i<variants_.count(); ++i)
            {
				if (!filter.flags()[i]) continue;

                const QList<QByteArray>& filters = variants_[i].filters();
                if(filters.isEmpty())
                {
					filter.flags()[i] = false;
                    continue;
                }

                bool keep = false;
                foreach(const QByteArray& f, filters)
                {
					if (filter_cols.contains(f)) keep = true;
                }
				filter.flags()[i] = keep;
            }
        }

        Log::perf("Applying annotation filter took ", timer);
        timer.start();

		//roi changed
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
		}

		//roi filter
		if (roi!="")
		{
			filter.flagByRegions(last_roi_);
			Log::perf("Applying target region filter took ", timer);
			timer.start();
		}

		//gene filter
		GeneSet genes_filter = filter_widget_->genes();
		if (!genes_filter.isEmpty())
		{
			filter.flagByGenes(genes_filter);
			Log::perf("Applying gene filter took ", timer);
			timer.start();
		}

		//target region filter
		BedLine region = BedLine::fromString(filter_widget_->region());
		if (region.isValid())
		{
			filter.flagByRegion(region);
			Log::perf("Applying region filter took ", timer);
			timer.start();
		}

		//genotype filter (control)
		if (filter_widget_->applyGenotypeControl())
		{
			QString geno = filter_widget_->genotypeControl();
			bool invert = false;
			if (geno.startsWith("not "))
			{
				geno = geno.mid(4);
				invert = true;
			}
			filter.flagByGenotype(geno, invert, genotypeColumns(CONTROL));
			Log::perf("Applying genotype filter (control) took ", timer);
		}

		//genotype filter (affected)
		if (filter_widget_->applyGenotypeAffected())
		{
			QString geno = filter_widget_->genotypeAffected();
			if (geno == "compound-het")
			{
				filter.flagCompoundHeterozygous(genotypeColumns(AFFECTED));
			}
			else if (geno == "compound-het or hom")
			{
				filter.flagCompoundHeterozygous(genotypeColumns(AFFECTED), true);
			}
			else
			{
				filter.flagByGenotype(filter_widget_->genotypeAffected(), false, genotypeColumns(AFFECTED));
			}
			Log::perf("Applying genotype filter (affected) took ", timer);
		}

		//update GUI
		timer.start();
		ui_.vars->setUpdatesEnabled(false);
		for(int i=0; i<variants_.count(); ++i)
		{
			if (filter.flags()[i])
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
		QString status = QString::number(filter.countPassing()) + " of " + QString::number(variants_.count()) + " variants passed filters.";
		ui_.statusBar->showMessage(status);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog !");
	}

	QApplication::restoreOverrideCursor();
}

void MainWindow::resetAnnoationStatus()
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

void MainWindow::updatePreferredTranscripts()
{
	preferred_transcripts_.clear();

	QMap<QString, QVariant> tmp = Settings::map("preferred_transcripts");
	for(auto it=tmp.cbegin(); it!=tmp.cend(); ++it)
	{
		preferred_transcripts_[it.key()] = it.value().toStringList();
	}

	//update variant details widget
	var_widget_->setPreferredTranscripts(preferred_transcripts_);
}

void MainWindow::updateNGSDSupport()
{
	bool ngsd_enabled = Settings::boolean("NGSD_enabled", true);
	bool target_file_folder_set = Settings::string("target_file_folder_windows")!="" && Settings::string("target_file_folder_linux")!="";

	//toolbar
	ui_.actionReport->setEnabled(ngsd_enabled);
	ui_.actionNGSD->setEnabled(ngsd_enabled);
	ui_.actionDatabase->setEnabled(ngsd_enabled);
	ui_.actionTrio->setEnabled(ngsd_enabled);
	ui_.actionMultiSample->setEnabled(ngsd_enabled);
	ui_.actionSampleInformation->setEnabled(ngsd_enabled);
	ui_.actionGapsRecalculate->setEnabled(ngsd_enabled);
	ui_.actionGeneSelector->setEnabled(ngsd_enabled);

	//tools menu
	ui_.actionOpenNGSD->setEnabled(ngsd_enabled);
	ui_.actionGeneInfo->setEnabled(ngsd_enabled);
	ui_.actionGenesToRegions->setEnabled(ngsd_enabled);
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

bool MainWindow::SampleInfo::isAffected() const
{
	auto it = properties.cbegin();
	while(it != properties.cend())
	{
		if (it.key().toLower()=="status" && it.value().toLower()=="affected")
		{
			return true;
		}

		++it;
	}

	return false;
}
