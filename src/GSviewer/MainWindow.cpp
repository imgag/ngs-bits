#include "MainWindow.h"
#include "Settings.h"
#include "GffData.h"
#include "SharedData.h"
#include "GenomeVisualizationWidget.h"
#include "AboutDialog.h"
#include "htslib/hts.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStyleFactory>
#include <RefGenomeService.h>
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

    // signals and slots
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
	QString curl_ca_bundle = Settings::string("curl_ca_bundle", true);
	if (!curl_ca_bundle.isEmpty())
	{
		if (!qputenv("CURL_CA_BUNDLE", curl_ca_bundle.toUtf8()))
		{
			qDebug() << "Could not set CURL_CA_BUNDLE variable, access to BAM/CRAM files over HTTPS may not be possible";
		}
	}

    // API
    connect(&api, &CommandServer::commandReceived, this, &MainWindow::executeApiCommand);
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

			GffSettings settings;
			settings.print_to_stdout = false;
			GffData data = GffData::load(gff_file, settings);
			SharedData::setTranscripts(data.transcripts);
		}
        qDebug() << "Parsing transcripts took: " << Helper::elapsedTime(timer);

        bool enableApi = Settings::boolean("enable_remote_application_control", false);

        if (enableApi)
            ui_.actionEnable_Remote_Application_Control->setChecked(true);

        SharedData::setRegion("chr17", 43091889, 43093530);
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

void MainWindow::on_actionEnable_Remote_Application_Control_changed()
{
    bool shouldEnable = ui_.actionEnable_Remote_Application_Control->isChecked();

    if (shouldEnable)
        api.bind();

    else
        api.terminate();
}

void MainWindow::executeApiCommand(GSVCommand cmd) {
    try {
        if (cmd.verb == "new")
            ui_.gvw->clearSession();

        else if (cmd.verb == "echo")
            return api.dispatchResponse(cmd, cmd.arguments);

        else if (cmd.verb == "goto")
            handleGoto(cmd.arguments);

        else if (cmd.verb == "load")
            handleLoad(cmd.arguments);

        else if (cmd.verb == "genome")
            handleGenome(cmd.arguments);

        else {
            QString msg = QString("UNKOWN COMMAND: %1").arg(cmd.verb);  // Absichtlicher Tippfehler. Das kommmt aus IGV so
            qDebug() << "Executing" << cmd.verb;

            api.dispatchFailure(cmd, msg);
            return;
        }
    } catch (Exception e) {
        err("An invalid command was received", e.message());

        QString msg = QString("ERR: %1").arg(e.message());
        qDebug() << "Executing" << cmd.verb;

        api.dispatchFailure(cmd, msg);
        return;
    }

    qDebug() << "Dispatching default response";
    api.dispatchResponse(cmd);
}

void MainWindow::err(QString header, QString text) {
    auto *box = new QMessageBox(QMessageBox::Warning, header, text, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();   // window-modal, returns immediately
}

void MainWindow::handleGoto(QString args)
{
    const QRegularExpression re("[,;]");

    auto tracks = args.split(re);

    for (QString &locus : tracks)
    {
        BedLine region = BedLine::fromString(locus);
        if (region.isValid())
        {
            SharedData::setRegion(region.chr(), region.start(), region.end());
            return;
        }
        else
        {
            err("Invalid region", "The region specified is not valid and can therefore not be used as a destination. If you meant to jump to a particular coordinate, be sure to include the chromosome and start/end positions in the format (chrN:start-end), or just the chromonose name: (chrN).");
        }
    }
}

void MainWindow::handleLoad(QString args)
{
    if (args.trimmed().isEmpty())
        ui_.gvw->openFileDialog();

    else
    {
        const QRegularExpression re("[,;]");

        for (QString &file : args.split(re))
            ui_.gvw->loadFile(file);
    }
}

void MainWindow::handleGenome(QString args) {
    QString file = args;

    if (args.trimmed().isEmpty())
    {
        QString open_folder = Settings::path("load_store_file_folder", true);
        file = QFileDialog::getOpenFileName(this, "Open file(s)", open_folder, "Genome files(*.fa, *.fa.gz);;All files(*.*)");
    }

    ui_.gvw->clearSession();
    RefGenomeService::instance().setReferenceGenome(file);
}
void MainWindow::showAboutDialog()
{
	AboutDialog dlg(this);
	dlg.setIcon(QPixmap(":/Icons/Icon.png"));
	dlg.setDescription("A free viewer for sequencing data.<br>Check the <a href='https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md'>GitHub page</a> for details.");
	dlg.addLibVersionLine("htslib version: " + QString(hts_version()));
    dlg.exec();
}
