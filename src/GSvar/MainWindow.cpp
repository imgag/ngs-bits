#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "Settings.h"
#include "Exceptions.h"
#include "ChromosomalIndex.h"
#include "Log.h"
#include "Helper.h"
#include "GUIHelper.h"
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
#include "ReportWorker.h"
#include "DBAnnotationWorker.h"
#include "SampleInformationDialog.h"
#include "ScrollableTextDialog.h"
#include "GPD.h"
#include "TrioDialog.h"
#include "HttpHandler.h"
#include "ValidationDialog.h"
#include "BasicStatistics.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, filter_widget_(new FilterDockWidget(this))
	, busy_dialog_(this)
	, filename_()
	, db_annos_updated_(false)
	, last_report_path_(QDir::homePath())
{
	//set up GUI
	ui_.setupUi(this);
	setWindowTitle(QCoreApplication::applicationName());
	addDockWidget(Qt::RightDockWidgetArea, filter_widget_);
	filter_widget_->raise();

	//signals and slots
	connect(ui_.actionClose, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui_.vars, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(varsContextMenu(QPoint)));
	connect(filter_widget_, SIGNAL(filtersChanged()), this, SLOT(filtersChanged()));
	connect(&filewatcher_, SIGNAL(fileChanged()), this, SLOT(handleInputFileChange()));

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

void MainWindow::delayedInizialization()
{
	if (!isVisible()) return;
	if (!delayed_init_timer_.isActive()) return;
	delayed_init_timer_.stop();

	//initialize LOG file
	if (QFile(Log::fileName()).exists() && !QFileInfo(Log::fileName()).isWritable())
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
		QMessageBox::warning(this, "GSvar ini file empty", "The ini file '" + Settings::fileName() + "' is empty.\nPlease inform your administrator!");
		close();
		return;
	}
	updateRecentFilesMenu();
	updateIGVMenu();

	//load command line argument
	if (QApplication::arguments().count()>=2)
	{
		loadFile(QApplication::arguments().at(1));
	}
}

void MainWindow::handleInputFileChange()
{
	QMessageBox::information(this, "GSvar file changed", "The input file changed.\nIt is reloaded now!");
	loadFile(filename_, false);
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

void MainWindow::on_actionChangeLog_triggered()
{
	ScrollableTextDialog dlg(this);
	dlg.setWindowTitle("ChangeLog");
	dlg.setText(Helper::loadTextFile("://Resources/ChangeLog.html").join("\n"));
	dlg.exec();
}

void MainWindow::loadFile(QString filename, bool show_sample_info_dialog)
{
	//reset GUI and data structures
	setWindowTitle(QCoreApplication::applicationName());
	filter_widget_->reset();
	filename_ = "";
	filewatcher_.clearFile();
	db_annos_updated_ = false;
	ui_.vars->setRowCount(0);
	ui_.vars->setColumnCount(0);

	//update recent files (before try block to remove non-existing files from the recent files menu)
	addToRecentFiles(filename);

	//load data
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
	try
	{
		variants_.load(filename);

		//update data structures
		Settings::setPath("path_variantlists", filename);
		filename_ = filename;
		filewatcher_.setFile(filename);

		//update GUI
		setWindowTitle(QCoreApplication::applicationName() + " - " + filename);
		ui_.statusBar->showMessage("Loaded variant list with " + QString::number(variants_.count()) + " variant.");

		variantListChanged();
		QApplication::restoreOverrideCursor();

		//show sample info dialog
		if (show_sample_info_dialog)
		{
			SampleInformationDialog dialog(this, filename_);
			dialog.exec();
		}
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Error", "Loading the file '" + filename + "' or displaying the contained variants failed!\nError message:\n" + e.message());
	}
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(this, "About " + QCoreApplication::applicationName(), QCoreApplication::applicationName()+ " " + QCoreApplication::applicationVersion()+ "\n\nGenome Sequencing Variant Viewer");
}

void MainWindow::on_actionResize_triggered()
{
	ui_.vars->resizeColumnsToContents();

	//limit column width to 400 pixel (approximate width of coding_splicing column with one entry)
	for (int i=0; i<ui_.vars->columnCount(); ++i)
	{
		if (ui_.vars->columnWidth(i)>400)
		{
			ui_.vars->setColumnWidth(i, 400);
		}
	}
}

void MainWindow::on_actionReport_triggered()
{
	if (variants_.count()==0) return;

	//check if DB annotations are up-to-date
	if (!db_annos_updated_)
	{
		GUIHelper::showMessage("Error", "Cannot generate report with outdated DB annotations!\nPlease re-annotate NGSD columns first!");
		return;
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

	//show report dialog
	ReportDialog dialog(filename_, this);
	dialog.addVariants(variants_, visible);
	dialog.setTargetRegionSelected(filter_widget_->targetRegion()!="");
	if (!dialog.exec()) return;

	//get export file name
	QString base_name = QFileInfo(filename_).baseName();
	QString target_short = "";
	if (filter_widget_->targetRegion()!="")
	{
		target_short = "_" + QFileInfo(filter_widget_->targetRegion()).fileName();
		target_short.remove(".bed");
		target_short.remove(QRegExp("_[0-9_]{4}_[0-9_]{2}_[0-9_]{2}"));
	}
	QString file_rep = QFileDialog::getSaveFileName(this, "Export report file", last_report_path_ + "/" + base_name + target_short + "_report_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (file_rep=="") return;
	last_report_path_ = QFileInfo(file_rep).absolutePath();

	//get BAM file name if necessary
	QString bam_file = "";
	if (dialog.detailsCoverage())
	{
		if (isTrio())
		{
			QStringList bam_files = getBamFilesTrio();
			if (bam_files.count()==0) return;
			bam_file = bam_files[0];
		}
		else
		{
			bam_file = getBamFile();
			if (bam_file=="") return;
		}
	}

	//flag report variants in NGSD
	try
	{
		NGSD ngsd;
		for(int i=0; i<dialog.selectedIndices().count(); ++i)
		{
			ngsd.setReport(filename_, variants_[dialog.selectedIndices()[i].first], true);
		}
		ngsd.setReportOutcome(filename_, dialog.outcome());
	}
	catch (DatabaseException& e)
	{
		QMessageBox::warning(this, "Report variants in NGSD", "Flagging report variants in NGSD failed:\n" + e.message());
	}

	//show busy dialog
	busy_dialog_.setText("Report", "Generating report.");
	busy_dialog_.show();

	//start worker in new thread
	ReportWorker* worker = new ReportWorker(base_name, NGSD().getExternalSampleName(filename_), filter_widget_->appliedFilters(), variants_, dialog.selectedIndices(), dialog.outcome(), filter_widget_->targetRegion(), bam_file, dialog.detailsVariants(), getLogFiles(), file_rep);
	connect(worker, SIGNAL(finished(bool)), this, SLOT(reportGenerationFinished(bool)));
	worker->start();
}

void MainWindow::reportGenerationFinished(bool success)
{
	busy_dialog_.hide();

	//show result info box
	ReportWorker* worker = qobject_cast<ReportWorker*>(sender());
	if (success)
	{
		QMessageBox::information(this, "Report", "Report generated successfully!");
	}
	else
	{
		QMessageBox::warning(this, "Error", "Report generation failed:\n" + worker->errorMessage());
	}

	//clean
	worker->deleteLater();
}

void MainWindow::on_actionDatabase_triggered()
{
	if (variants_.count()==0) return;

	//show busy dialog
	busy_dialog_.setText("Database annotation", "Updating annotations from NGSD/GPD.");
	busy_dialog_.show();

	//start worker
	QString genome = Settings::string("reference_genome");
	DBAnnotationWorker* worker = new DBAnnotationWorker(filename_, genome, variants_);
	connect(worker, SIGNAL(finished(bool)), this, SLOT(databaseAnnotationFinished(bool)));
	worker->start();
}

void MainWindow::databaseAnnotationFinished(bool success)
{
	busy_dialog_.hide();

	//show result info box
	DBAnnotationWorker* worker = qobject_cast<DBAnnotationWorker*>(sender());
	if (success)
	{
		db_annos_updated_ = true;
		variantListChanged();
	}
	else
	{
		QMessageBox::warning(this, "Error", "Database annotation failed:\n" + worker->errorMessage());
	}

	//clean
	worker->deleteLater();
}

void MainWindow::on_actionFilters_triggered()
{
	filter_widget_->applyDefaultFilters();
	on_actionResize_triggered();
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

void MainWindow::on_actionGeneListBED_triggered()
{
	ExternalToolDialog dialog("BedGeneOverlap", "", this);
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
		QString reply = handler.getHttpReply(Settings::string("SampleStatus")+"restart_trio.php?f=" + dlg.father() + "&m=" + dlg.mother() + "&c=" + dlg.child() + "&high_priority");
		if (!reply.startsWith("Restart successful"))
		{
			QMessageBox::warning(this, "Trio analysis", "Queueing trio analysis failed:\n" + reply);
		}
		else
		{
			QMessageBox::information(this, "Trio analysis", "Queueing trio analysis successful!");
		}
	}
}

void MainWindow::on_actionGaps_triggered()
{
	if (filename_=="") return;

	//get gene name from user
	QString gene = QInputDialog::getText(this, "Display gaps", "Gene:");
	if (gene=="") return;

	//locate report(s)
	QDir dir = QFileInfo(filename_).absoluteDir();
	dir.setNameFilters(QStringList("*_lowcov.bed"));
	QStringList reports = dir.entryList();

	//abort if no report is found
	if (reports.count()==0)
	{
		GUIHelper::showMessage("Error", "Could not detect low-coverage BED file in folder '" + dir.absolutePath() + "'.");
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
	QStringList lines = Helper::loadTextFile(dir.absolutePath() + "/" + report, true);
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
	GUIHelper::showWidgetAsDialog(edit, "Gaps of gene '" + gene + "'' from low-coverage BED file '" + report + "':", false);
	edit->deleteLater();
}

void MainWindow::on_actionExportVCF_triggered()
{
	//load original VCF
	QString orig_name = filename_;
	orig_name.replace(".GSvar", "_var_annotated.vcf");
	if (!QFile::exists(orig_name))
	{
		GUIHelper::showMessage("VCF export error", "Could not find original VCF file '" + orig_name + "'!");
		return;
	}
	VariantList orig_vcf;
	orig_vcf.load(orig_name, VariantList::VCF);
	ChromosomalIndex<VariantList> orig_idx(orig_vcf);

	//create new VCF
	VariantList output;
	output.copyMetaData(orig_vcf);
	for(int i=0; i<variants_.count(); ++i)
	{
		if (!ui_.vars->isRowHidden(i))
		{
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
			if (hit_count!=1) THROW(ProgrammingException, "Found " + QString::number(hit_count) + " matching variants for " + v.toString() + " in VCF file. Exactly one expected!");
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
	if (split_quality)
	{
		if (ui_.vars->columnCount()<7 || ui_.vars->horizontalHeaderItem(6)->text()!="quality")
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
			if (split_quality && col==6)
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
			if (split_quality && col==6)
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

void MainWindow::variantListChanged()
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	QTime timer;
	timer.start();

	//resize
	ui_.vars->setRowCount(variants_.count());
	ui_.vars->setColumnCount(5 + variants_.annotations().count());

	//header
	link_indices_.clear();
	ui_.vars->setHorizontalHeaderItem(0, new QTableWidgetItem("chr"));
	ui_.vars->setHorizontalHeaderItem(1, new QTableWidgetItem("start"));
	ui_.vars->setHorizontalHeaderItem(2, new QTableWidgetItem("end"));
	ui_.vars->setHorizontalHeaderItem(3, new QTableWidgetItem("ref"));
	ui_.vars->setHorizontalHeaderItem(4, new QTableWidgetItem("obs"));
	for (int i=0; i<variants_.annotations().count(); ++i)
	{
		QString anno = variants_.annotations()[i].name();
		QTableWidgetItem* header = new QTableWidgetItem(anno);
		header->setToolTip(variants_.annotations()[i].description());
		ui_.vars->setHorizontalHeaderItem(i+5, header);
		//link columns
		QStringList link_cols;
		link_cols << "dbSNP" << "OMIM" << "ClinVar" << "HGMD" << "COSMIC" << "GPD_gene" << "GPD_var";
		foreach(const QString& link_col, link_cols)
		{
			if (anno.contains(link_col))
			{
			   header->setIcon(QIcon("://Icons/Link.png"));
			   link_indices_.insert(i+5);
			}
		}
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

			//tooltip
			QString tooltip;
			tooltip = row.chr().str() + ":" + QString::number(row.start()) + "-" + QString::number(row.end()) + " " + row.ref() + ">" + row.obs();
			//tooltip for link columns
			if (link_indices_.contains(j+5))
			{
				QStringList entries = anno.split("];");
				QString ids = "";
				QString add_info = "";
				foreach(const QString& entry, entries)
				{
					if (entry.trimmed()=="") continue;
					if (ids!="")
					{
						ids += ", ";
						add_info += "\n";
					}
					ids += entry.mid(0, entry.indexOf("[")-1).trimmed();
					add_info += entry.trimmed();
					if (entry.contains("[")) add_info += "]";
				}
				item->setText(ids);
				tooltip += "\n" + add_info;
			}

			//tooltip for fields with large content
			if (j==i_co_sp)
			{
				tooltip += "\n" + item->text().replace(",", "\n");
			}
			if (j==i_comment)
			{
				tooltip += "\n" + item->text();
			}
			item->setToolTip(tooltip);

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
			else if (j==i_clinvar && (anno.contains("[pathogenic]") || anno.contains("[probable-pathogenic]")))
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

	QApplication::restoreOverrideCursor();

	Log::perf("Painting took ", timer);

	//re-filter in case some relevant columns changed
	filtersChanged();
}

void MainWindow::varsContextMenu(QPoint pos)
{
	//get item
	QTableWidgetItem* item = ui_.vars->itemAt(pos);
	if (!item) return;

	//create contect menu
	QMenu menu(ui_.vars);
	if (item->text()!="" && link_indices_.contains(item->column()))
	{
		menu.addAction(QIcon("://Icons/Link.png"), "Open annotation website");
	}
	QMenu* sub_menu = menu.addMenu(QIcon("://Icons/IGV.png"), "IGV");
	sub_menu->addAction("Open BAM and jump to position");
	sub_menu->addAction("Jump to position");
	sub_menu = menu.addMenu(QIcon("://Icons/NGSD.png"), "NGSD");
	sub_menu->addAction("Open variant in NGSD");
	sub_menu->addAction("Search for position in NGSD");
	sub_menu->addSeparator();
	sub_menu->addAction("Set validation status");
	QMenu* sub_sub_menu = sub_menu->addMenu("Set classification");
	QStringList vus_enum = NGSD().getEnum("variant", "vus");
	foreach(QString entry, vus_enum)
	{
		sub_sub_menu->addAction(entry);
	}
	sub_menu->addAction("Edit comment");
	menu.addAction(QIcon("://Icons/PrimerDesign.png"), "PrimerDesign");

	//Execute menu
	QAction* action = menu.exec(ui_.vars->viewport()->mapToGlobal(pos));
	if (!action) return;

	QByteArray text = action->text().toLatin1();
	QString text_menu = qobject_cast<QMenu*>(action->parent())->title();

	if (text=="Open annotation website")
	{
		QString header = ui_.vars->horizontalHeaderItem(item->column())->text();

		//extract IDs
		QStringList ids = item->text().split(", ", QString::SkipEmptyParts);
		QStringList details = item->toolTip().split("\n", QString::SkipEmptyParts);
		details = details.mid(1);
		for(int i=0; i<ids.count(); ++i)
		{
			QString id = ids[i];

			//determine URL
			QString url;
			if(header=="dbSNP")
			{
				url = "http://www.ncbi.nlm.nih.gov/projects/SNP/snp_ref.cgi?rs=";
			}
			else if(header=="OMIM")
			{
				url = "http://omim.org/entry/";
			}
			else if(header=="ClinVar")
			{
				url = "http://www.ncbi.nlm.nih.gov/clinvar/";
			}
			else if(header=="HGMD")
			{
				QString gene = details[i].split("GENE=").value(1);
				gene = gene.left(gene.length()-1);
				url = "http://www.hgmd.cf.ac.uk/ac/gene.php?gene="+gene+"&accession=";
			}
			else if(header=="COSMIC")
			{
				if (id.startsWith("COSM"))
				{
					url = "http://cancer.sanger.ac.uk/cosmic/mutation/overview?id=";
				}
				else
				{
					url = "http://cancer.sanger.ac.uk/cosmic/ncv/overview?id=";
				}
				id = id.mid(4);
			}
			else if(header=="GPD_gene")
			{
				url = Settings::string("GPD")+"/genes/view/";
			}
			else if(header=="GPD_var")
			{
				url = Settings::string("GPD")+"/variants/view/";
			}
			else THROW(ProgrammingException, "Unknown link header " + header + "!");

			QDesktopServices::openUrl(QUrl(url + id));
		}
	}
	else if (text=="Open BAM and jump to position")
	{
		//determine BAM file(s) to load
		QStringList bam_files;
		if (isTrio())
		{
			bam_files = getBamFilesTrio();
			if (bam_files.count()==0) return;
		}
		else
		{
			QString bam_file = getBamFile();
			bam_files.append(bam_file);
			if (bam_file=="") return;
		}

		//send commands to IGV
		QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
		try
		{
			executeIGVCommand("new");
			executeIGVCommand("genome hg19");

			//load IGV tracks as requested
			if (ui_.actionIgvSample->isChecked())
			{
				executeIGVCommand("load " + QString(filename_).replace(".GSvar", "_var_annotated.vcf"));
			}
			if (ui_.actionIgvLowcov->isChecked())
			{
				QString system = NGSD().getProcessingSystem(filename_, NGSD::SHORT);
				executeIGVCommand("load " + QString(filename_).replace(".GSvar", "_" + system + "_lowcov.bed"));
			}
			QList<QAction*> igv_actions = ui_.menuIGV->findChildren<QAction*>();
			foreach(QAction* action, igv_actions)
			{
				if (action->isChecked())
				{
					executeIGVCommand("load " + action->toolTip());
				}
			}
			executeIGVCommand("collapse");

			//load BAM files
			foreach(QString bam_file, bam_files)
			{
				executeIGVCommand("load " + bam_file);
			}
			//load reference file
			QString ref = filter_widget_->referenceSample();
			if (ref!="")
			{
				if (!QFile::exists(ref))
				{
					QMessageBox::warning(this, "IGV problem", "Reference file '" + ref + "' does not exist!");
				}
				else
				{
					executeIGVCommand("load " + ref);
				}
			}
			//load target region file(s)
			QString roi = filter_widget_->targetRegion();
			if (roi!="")
			{
				if (!QFile::exists(roi))
				{
					QMessageBox::warning(this, "IGV problem", "Target region file '" + roi + "' does not exist!");
				}
				else
				{
					executeIGVCommand("load " + roi);
				}
				QString amplicons = roi.left(roi.length()-4) + "_amplicons.bed";
				if (QFile::exists(amplicons))
				{
					executeIGVCommand("load " + amplicons);
				}
			}
			//goto location
			QString loc = ui_.vars->item(item->row(), 0)->text() + ":" + ui_.vars->item(item->row(), 1)->text() + "-" + ui_.vars->item(item->row(), 2)->text();
			executeIGVCommand("goto " + loc);
		}
		catch(Exception& e)
		{
			QMessageBox::warning(this, "Could not connect to IGV", e.message());
		}
		QApplication::restoreOverrideCursor();
	}
	else if (text=="Jump to position")
	{
		//send commands to IGV
		QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
		try
		{
			QString loc = ui_.vars->item(item->row(), 0)->text() + ":" + ui_.vars->item(item->row(), 1)->text() + "-" + ui_.vars->item(item->row(), 2)->text();
			executeIGVCommand("goto " + loc);
		}
		catch(Exception& e)
		{
			QMessageBox::warning(this, "Could not connect to IGV", e.message());
		}
		QApplication::restoreOverrideCursor();
	}
	else if (text=="Open variant in NGSD")
	{
		try
		{
			QString url = NGSD().url(filename_, variants_[item->row()]);
			QDesktopServices::openUrl(QUrl(url));
		}
		catch (DatabaseException e)
		{
			GUIHelper::showMessage("NGSD error", "The variant database ID could not be determined!\nDoes the file name '"  + filename_ + "' start with the prcessed sample ID?\nError message: " + e.message());
			return;
		}
	}
	else if (text=="Search for position in NGSD")
	{
		const Variant& v = variants_[item->row()];
		QString url = NGSD().urlSearch(v.chr().str() + ":" + QString::number(v.start()) + "-" + QString::number(v.end()));
		QDesktopServices::openUrl(QUrl(url));
	}
	else if (text=="Set validation status")
	{
		ValidationDialog dlg(this, filename_, variants_[item->row()], variants_.annotationIndexByName("quality", true, true));

		if (dlg.exec())
		{
			//update DB
			try
			{
				NGSD().setValidationStatus(filename_, variants_[item->row()], dlg.status(), dlg.comment());
			}
			catch (DatabaseException e)
			{
				GUIHelper::showMessage("NGSD error", e.message());
				return;
			}

			//update GUI
			QByteArray status = dlg.status().toLatin1();
			if (status=="true positive") status = "TP";
			if (status=="false positive") status = "FP";
			variants_[item->row()].annotations()[variants_.annotationIndexByName("validated", true, true)] = status;
			variantListChanged();
		}
	}
	else if (text_menu=="Set classification")
	{
		//check if multiselect
		QModelIndexList selection = ui_.vars->selectionModel()->selectedRows();
		bool multi_set = false;
		if (selection.count()>1)
		{
			if (QMessageBox::question(this, "Set VUS for multiple variants?", QString::number(selection.count()) + " variant rows are selected. Set VUS for them all?")==QMessageBox::Yes)
			{
				multi_set = true;
			}
		}

		//update DB
		try
		{
			if (multi_set)
			{
				for (int i=0; i<selection.count(); ++i)
				{
					NGSD().setClassification(variants_[selection.at(i).row()], text);
				}
			}
			else
			{
				NGSD().setClassification(variants_[item->row()], text);
			}
		}
		catch (DatabaseException e)
		{
			GUIHelper::showMessage("NGSD error", e.message());
			return;
		}

		//update GUI
		variants_[item->row()].annotations()[variants_.annotationIndexByName("classification", true, true)] = text;
		variantListChanged();
	}
	else if (text=="Edit comment")
	{
		try
		{
			bool ok = true;
			QByteArray text = QInputDialog::getText(this, "Variant comment", "Text: ", QLineEdit::Normal, NGSD().comment(filename_, variants_[item->row()]), &ok).toLatin1();

			if (ok)
			{
				//update DB
				NGSD().setComment(filename_, variants_[item->row()], text);

				//update GUI
				int col_index = variants_.annotationIndexByName("comment", true, false);
				if (col_index!=-1)
				{
					variants_[item->row()].annotations()[col_index] = text;
					variantListChanged();
				}
			}
		}
		catch (DatabaseException e)
		{
			GUIHelper::showMessage("NGSD error", e.message());
			return;
		}
	}
	else if (text=="PrimerDesign")
	{
		try
		{
			const Variant& v = variants_[item->row()];
			QString url = Settings::string("PrimerDesign")+"/index.php?user="+Helper::userName()+"&sample="+NGSD::sampleName(filename_)+"&chr="+v.chr().str()+"&start="+QString::number(v.start())+"&end="+QString::number(v.end())+"";
			QDesktopServices::openUrl(QUrl(url));
		}
		catch (DatabaseException e)
		{
			GUIHelper::showMessage("NGSD error", "The variant database ID could not be determined!\nDoes the file name '"  + filename_ + "' start with the prcessed sample ID?\nError message: " + e.message());
			return;
		}
	}
}

void MainWindow::executeIGVCommand(QString command)
{
	//connect
	QAbstractSocket socket(QAbstractSocket::UnknownSocketType, this);
	socket.connectToHost("127.0.0.1", 60151);
	if (!socket.waitForConnected(1000))
	{
		THROW(Exception, "Could not connect to IGV.\nPlease start IGV and enable the remote control port and proxy:\nView => Preferences => Advanced => Enable port => 60151\nView => Preferences => Proxy");
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

QString MainWindow::getBamFile()
{
	QDir data_dir(QFileInfo(filename_).path());
	QStringList bam_files = data_dir.entryList(QStringList("*.bam"),  QDir::Files);
	if (bam_files.count()!=1)
	{
		QMessageBox::warning(this, "Could not locate sample BAM file.", "Exactly one BAM file must be present in the sample folder:\n" + data_dir.path() + "\n\nFound the following files:\n" + bam_files.join("\n"));
		return "";
	}
	return data_dir.path() + QDir::separator() + bam_files.at(0);
}

QStringList MainWindow::getBamFilesTrio()
{
	QStringList output;

	//get folder name
	QFileInfo trio_folder(QFileInfo(filename_).path());

	//split folder name
	QStringList folder_parts = trio_folder.baseName().split("_");
	if (folder_parts.count()<7)
	{
		QMessageBox::warning(this, "Malformatted trio folder", "Trio folder does not consist of three processed sample names!\nIt should look like this:\nTrio_GS140527_02_GS140528_02_GS140531_02");
		return QStringList();
	}

	//locate BAM files in sample folders
	for (int i=1; i<=5; i+=2)
	{
		QString sample_name = folder_parts[i] + "_" + folder_parts[i+1];
		QString bam_file = trio_folder.path() + "\\Sample_" + sample_name + "\\" + sample_name + ".bam";
		if (!QFile::exists(bam_file))
		{
			QMessageBox::warning(this, "Missing BAM file!", "Could not find BAM file at: " + bam_file);
			return QStringList();
		}
		output.append(bam_file);
	}

	return output;
}

bool MainWindow::isTrio()
{
	return (variants_.annotationIndexByName("trio", true, false)!=-1);
}

void MainWindow::filtersChanged()
{
	QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

	try
	{
		//apply annotation filters
		QTime timer;
		timer.start();

		QBitArray pass(variants_.count(), true);

		//MAF filter
		if (filter_widget_->applyMaf())
		{
			int i_1000g = variants_.annotationIndexByName("1000g", true, true);
			int i_exac = variants_.annotationIndexByName("ExAC", true, true);
			int i_esp6500 = variants_.annotationIndexByName("ESP6500EA", true, true);

			double max_maf = filter_widget_->mafPerc() / 100.0;
			for(int i=0; i<variants_.count(); ++i)
			{
				pass[i] = pass[i] && variants_[i].annotations()[i_1000g].toDouble()<=max_maf;
				pass[i] = pass[i] && variants_[i].annotations()[i_exac].toDouble()<=max_maf;
				pass[i] = pass[i] && variants_[i].annotations()[i_esp6500].toDouble()<=max_maf;
			}
		}

		//impact filter
		if (filter_widget_->applyImpact())
		{
			int i_co_sp = variants_.annotationIndexByName("coding_and_splicing", true, true);

			//determine impacts list
			QStringList impacts = filter_widget_->impact();
			for(int i=0; i<impacts.count(); ++i)
			{
				impacts[i] = ":" + impacts[i] + ":";
			}

			//filter
			for(int i=0; i<variants_.count(); ++i)
			{
				if (!pass[i]) continue;

				bool pass_impact = false;
				foreach(const QString& impact, impacts)
				{
					if (variants_[i].annotations()[i_co_sp].contains(impact.toLatin1()))
					{
						pass_impact = true;
						break;
					}
				}
				pass[i] = pass_impact;
			}
		}

		//ihdb filter
		if (filter_widget_->applyIhdb())
		{
			int i_ihdb_hom = variants_.annotationIndexByName("ihdb_allsys_hom", true, true);
			int i_ihdb_het = variants_.annotationIndexByName("ihdb_allsys_het", true, true);
			int i_geno = variants_.annotationIndexByName("genotype", true, true);

			int max_ihdb = filter_widget_->ihdb();
			for(int i=0; i<variants_.count(); ++i)
			{
				if (!pass[i]) continue;

				const QString& genotype = variants_[i].annotations()[i_geno];
				if (genotype=="hom")
				{
					pass[i] = variants_[i].annotations()[i_ihdb_hom].toInt()<=max_ihdb;
				}
				else if (genotype=="het")
				{
					pass[i] = variants_[i].annotations()[i_ihdb_hom].toInt()<=max_ihdb && variants_[i].annotations()[i_ihdb_het].toInt()<=max_ihdb;
				}
				else THROW(ProgrammingException, "Unknown genotype '" + genotype + "'!");
			}
		}

		//VUS filter
		if (filter_widget_->applyVus())
		{
			int i_class = variants_.annotationIndexByName("classification", true, true);

			int min_vus = filter_widget_->vus();
			for(int i=0; i<variants_.count(); ++i)
			{
				if (!pass[i]) continue;

				const QString& classification = variants_[i].annotations()[i_class];
				if (classification=="M") continue; //M always passes

				bool ok = false;
				int classification_value = classification.toInt(&ok);

				pass[i] = !ok || classification_value>=min_vus;
			}
		}

		//genotype filter
		if (filter_widget_->applyGenotype())
		{
			int i_geno = variants_.annotationIndexByName("genotype", true, true);
			QString geno = filter_widget_->genotype();
			for(int i=0; i<variants_.count(); ++i)
			{
				if (!pass[i]) continue;
				pass[i] = (variants_[i].annotations()[i_geno] == geno);
			}
		}

		//trio filter
		if (filter_widget_->applyTrio())
		{
			int i_trio = variants_.annotationIndexByName("trio", true, true);
			for(int i=0; i<variants_.count(); ++i)
			{
				pass[i] = pass[i] && !variants_[i].annotations()[i_trio].isEmpty();
			}
		}

		//prevent important variants from beeing filtered out
		if (filter_widget_->keepImportant())
		{
			for(int i=0; i<variants_.count(); ++i)
			{
				//warning (red)
				if (!pass[i] && ui_.vars->verticalHeaderItem(i)!=0 && ui_.vars->verticalHeaderItem(i)->foreground().color()==Qt::red)
				{
					pass[i] = true;
				}
				//notice (orange)
				if (!pass[i] && ui_.vars->verticalHeaderItem(i)!=0 && ui_.vars->verticalHeaderItem(i)->foreground().color()==QColor(255, 135, 60))
				{
					pass[i] = true;
				}
			}
		}

		//quality filter
		if (filter_widget_->applyQuality())
		{
			int i_qual = variants_.annotationIndexByName("quality", true, true);

			for(int i=0; i<variants_.count(); ++i)
			{
				if (!pass[i]) continue;

				QList<QByteArray> qual_parts = variants_[i].annotations()[i_qual].split(';');
				foreach(const QByteArray& part, qual_parts)
				{
					QList<QByteArray> key_value = part.split('=');
					if (key_value.count()!=2 || !BasicStatistics::isValidFloat(key_value[1]))
					{
						THROW(ArgumentException, "Cannot parse quality column part '" + part + "'");
					}
					if ((key_value[0]=="QUAL" && key_value[1].toDouble()<100)
						||
						(key_value[0]=="MQM" && key_value[1].toDouble()<50)
						||
						(key_value[0]=="AF" && key_value[1].toDouble()<0.25)
						||
						(key_value[0]=="DP" && key_value[1].toDouble()<20)
						)
					{
						pass[i] = false;
					}
				}
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
			ChromosomalIndex<BedFile> bed_index(last_roi_);
			for(int i=0; i<variants_.count(); ++i)
			{
				if (pass[i])
				{
					const Variant& variant = variants_[i];
					int matches = bed_index.matchingIndices(variant.chr(), variant.start(), variant.end()).count();
					pass[i] = (matches>0);
				}
			}
			Log::perf("Applying target region filter took ", timer);
		}

		//gene filter
		QByteArray gene = filter_widget_->gene().toUpper().toLatin1();
		if (gene!="")
		{
			int i_gene = variants_.annotationIndexByName("gene", true, true);
			for(int i=0; i<variants_.count(); ++i)
			{
				if (pass[i])
				{
					pass[i] = variants_[i].annotations()[i_gene].toUpper().contains(gene);
				}
			}
			Log::perf("Applying gene filter took ", timer);
		}

		//update GUI
		timer.start();
		ui_.vars->setUpdatesEnabled(false);
		for(int i=0; i<variants_.count(); ++i)
		{
			if (pass[i])
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
		QString status = QString::number(pass.count(true)) + " of " + QString::number(pass.count()) + " variants passed filters.";
		ui_.statusBar->showMessage(status);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog !");
	}

	QApplication::restoreOverrideCursor();
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
		ui_.menuIGV->addAction("No entries in INI file!");
	}
	else
	{
		foreach(QString entry, entries)
		{
			QStringList parts = entry.split("\t");
			if(parts.count()!=3) THROW(ArgumentException, "Could not split 'igv_menu' entry from INI file into three parts: '" + entry + "'.");
			QAction* action = ui_.menuIGV->addAction(parts[0]);
			action->setCheckable(true);
			action->setChecked(parts[1]=="1");
			action->setToolTip(parts[2]);
		}
	}
}

void MainWindow::openRecentFile()
{
	QAction* action = qobject_cast<QAction*>(sender());
	loadFile(action->text());
}
