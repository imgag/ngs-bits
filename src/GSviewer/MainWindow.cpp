#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include "AboutDialog.h"
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
	connect(ui_.actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
	connect(ui_.actionExit, &QAction::triggered, this, &MainWindow::close);
	connect(ui_.actionAddNRegions, &QAction::triggered, this, &MainWindow::addNRegions);

	//Set environment variable containing SSL certificates - needed for HTTPS to work for BamReader/htslib
	QString curl_ca_bundle = Settings::string("curl_ca_bundle", true);
	if (!curl_ca_bundle.isEmpty())
	{
		if (!qputenv("CURL_CA_BUNDLE", curl_ca_bundle.toUtf8()))
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
			QProgressDialog loading_dialog("loading gene/transcript Gff file: "+gff_file, QString(), 0, 0, this);
			loading_dialog.setWindowModality(Qt::ApplicationModal);
			loading_dialog.setCancelButton(nullptr);
			loading_dialog.setMinimumDuration(0);
			loading_dialog.show();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

			GffSettings settings;
			settings.print_to_stdout = false;
			GffData data = GffData::load(gff_file, settings);
			SharedData::setTranscripts(data.transcripts);
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
		QProgressDialog progress("Finding N regions in the reference genome...", "Cancel", 0, 1, this);
		progress.setWindowModality(Qt::ApplicationModal);
		progress.setMinimumDuration(0);
		progress.setAutoClose(false);
		progress.setAutoReset(false);
		auto regions = QSharedPointer<BedFile>::create(FileLoader::referenceNRegions(SharedData::genome(), [&](int completed, int total)
		{
			progress.setMaximum(total);
			progress.setValue(completed);
			QApplication::processEvents();
			return !progress.wasCanceled();
		}));
		if (progress.wasCanceled()) return;
		const QString folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/generated_tracks";
		if (!QDir().mkpath(folder)) THROW(FileAccessException, "Could not create directory: " + folder);
		const QByteArray reference = QFileInfo(Settings::string("reference_genome")).absoluteFilePath().toUtf8();
		const QString hash = QString::fromLatin1(QCryptographicHash::hash(reference, QCryptographicHash::Sha256).toHex().left(16));
		const QString file_path = folder + "/N_regions_" + hash + ".bed";
		regions->store(file_path);
		ui_.gvw->addBedTrack(regions, file_path, "N regions");
	}
	catch (const Exception& e)
	{
		QMessageBox::warning(this, "Add N regions", e.message());
	}
}

void MainWindow::showAboutDialog()
{
	AboutDialog dlg(this);
	dlg.setIcon(QPixmap(":/Icons/Icon.png"));
	dlg.setDescription("A free viewer for sequencing data.<br>Check the <a href='https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md'>GitHub page</a> for details.");
	dlg.addLibVersionLine("htslib version: " + QString(hts_version()));
	dlg.exec();
}
