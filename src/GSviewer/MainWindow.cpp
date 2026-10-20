#include "MainWindow.h"
#include "Settings.h"
#include "TranscriptCache.h"
#include "SharedData.h"
#include "AboutDialog.h"
#include "ParameterEditor.h"
#include "htslib/hts.h"
#include "GenomeVisualizationWidget.h"
#include "FileLoader.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStandardPaths>
#include <QApplication>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, init_timer_(this, true)
{
	ui_.setupUi(this);

	//sginals and slots
	connect(ui_.actionLoadFile, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::openFileDialog);
	connect(ui_.actionLoadURL, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::openUrlDialog);
	connect(ui_.actionReloadTracks, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::reloadTracks);
	connect(ui_.actionClearSession, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::clearSession);
	connect(ui_.actionStore_session, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::saveSession);
	connect(ui_.actionLoadSession, &QAction::triggered, ui_.gvw, &GenomeVisualizationWidget::loadSession);
	connect(ui_.actionOpen_documentation, &QAction::triggered, this, &MainWindow::openDocumentation);
	connect(ui_.actionSettings, &QAction::triggered, this, &MainWindow::showSettings);
	connect(ui_.actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
	connect(ui_.actionExit, &QAction::triggered, this, &MainWindow::close);
	connect(ui_.actionAddNRegions, &QAction::triggered, this, &MainWindow::addNRegions);

	//Set environment variable containing SSL certificates - needed for HTTPS to work for BamReader/htslib
	QByteArray curl_ca_bundle = Settings::string("curl_ca_bundle", true).trimmed().toUtf8();
	if (!curl_ca_bundle.isEmpty())
	{
		if(!QFile::exists(curl_ca_bundle)) THROW(FileAccessException, "CURL_CA_BUNDLE file does not exist: "+curl_ca_bundle);
		if (!qputenv("CURL_CA_BUNDLE", curl_ca_bundle))
		{
			qDebug() << "Could not set CURL_CA_BUNDLE variable, access to BAM/CRAM files over HTTPS may not be possible";
		}
	}
}

void MainWindow::delayedInitialization()
{
	//load transcripts from GFF
	try
	{
		QElapsedTimer timer;
		timer.start();
		{
			QString gff_file = Settings::string("ensembl_gff");
			QProgressDialog progress_dialog("loading gene/transcript Gff file: "+gff_file, QString(), 0, 0, this);
			progress_dialog.setWindowModality(Qt::ApplicationModal);
			progress_dialog.setCancelButton(nullptr);
			progress_dialog.setMinimumDuration(0);
			progress_dialog.show();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

			SharedData::setTranscripts(TranscriptCache::load(gff_file));
		}
		qDebug() << "Parsing transcripts took: " << Helper::elapsedTime(timer);
    }
	catch (Exception e)
	{
		QTextStream(stderr) << "Error loading transcripts: "+e.message();
        exit(-1);
	}
}

void MainWindow::addNRegions()
{
	try
	{
		//show progress dialog
		QProgressDialog progress_dialog("Finding N regions in the reference genome...", "Cancel", 0, 27, this);
		progress_dialog.setWindowModality(Qt::ApplicationModal);
		progress_dialog.setCancelButton(nullptr);
		progress_dialog.setMinimumDuration(0);
		progress_dialog.show();

		//determine N regions
		QSharedPointer<BedFile> regions(new BedFile());
		int chr_num = 0;
		for (const Chromosome& chr : SharedData::genome().chromosomes())
		{
			++chr_num;
			progress_dialog.setValue(chr_num);

			const int length = SharedData::genome().lengthOf(chr);
			int n_start = 0;
			for (int start = 1; start <= length; start += 1024 * 1024)
			{
				const Sequence sequence = SharedData::genome().seq(chr, start, std::min(1024 * 1024, length - start + 1), false);
				for (int i = 0; i < sequence.size(); ++i)
				{
					if (sequence[i] == 'N' || sequence[i] == 'n')
					{
						if (n_start == 0) n_start = start + i;
					}
					else if (n_start > 0)
					{
						regions->append(BedLine(chr, n_start, start + i - 1));
						n_start = 0;
					}
				}
			}
			if (n_start > 0) regions->append(BedLine(chr, n_start, length));
		}
		regions->sort();

		//store BED
		const QString folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/generated_tracks";
		if (!QDir().mkpath(folder)) THROW(FileAccessException, "Could not create directory: " + folder);
		const QByteArray reference = QFileInfo(Settings::string("reference_genome")).absoluteFilePath().toUtf8();
		const QString file_path = folder + "/N_regions.bed";
		regions->store(file_path);

		ui_.gvw->addBedTrack(regions, file_path, "N regions");
	}
	catch (const Exception& e)
	{
		QMessageBox::warning(this, "Add N regions", e.message());
	}
}

void MainWindow::showSettings()
{
	QDialog dialog(this);
	dialog.setWindowTitle("Global settings");
	dialog.setMinimumSize(300, 330);
	QVBoxLayout* layout = new QVBoxLayout(&dialog);
	layout->setContentsMargins(QMargins(3,3,3,3));
	layout->setSpacing(3);
	ParameterEditor* editor = new ParameterEditor(SharedData::parameters(), &dialog);
	editor->disableAutoApply();
	layout->addWidget(editor);
	dialog.exec();
}

void MainWindow::openDocumentation()
{
	QDesktopServices::openUrl(QUrl("https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md"));
}

void MainWindow::showAboutDialog()
{
	AboutDialog dlg(this);
	dlg.setIcon(QPixmap(":/Icons/Icon.png"));
	dlg.setDescription("A free viewer for sequencing data.<br>Check the <a href='https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md'>GitHub page</a> for details.");
	dlg.addLibVersionLine("htslib version: " + QString(hts_version()));
	dlg.exec();
}
