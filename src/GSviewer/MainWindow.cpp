#include "MainWindow.h"
#include "Settings.h"
#include "TranscriptCache.h"
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
#include "ColorNames.h"
#include "htslib/hts.h"
#include "GenomeVisualizationWidget.h"
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
#include <QTableWidget>
#include <QHeaderView>

MainWindow::MainWindow(QString bind, QWidget *parent)
    : QMainWindow(parent),
    ui_(),
    init_timer_(this, true),
    api(bind, this)
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
	connect(ui_.actionShowNamedColors, &QAction::triggered, this, &MainWindow::showNamedColors);

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

    // API

    if (!bind.isEmpty())
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

			SharedData::setTranscripts(TranscriptCache::load(gff_file));
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

void MainWindow::showNamedColors()
{
	QDialog dialog(this);
	dialog.setWindowTitle("Named colors");
	dialog.resize(380, 600);
	auto* layout = new QVBoxLayout(&dialog);
	const QStringList names = ColorNames::names();
	auto* table = new QTableWidget(names.size(), 2, &dialog);
	table->setHorizontalHeaderLabels({"Name", "Color"});
	table->verticalHeader()->hide();
	table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	for (int row = 0; row < names.size(); ++row)
	{
		const QColor color = ColorNames::color(names[row]);
		table->setItem(row, 0, new QTableWidgetItem(names[row]));
		auto* swatch = new QTableWidgetItem(color.name());
		swatch->setBackground(color);
		swatch->setForeground(color.lightness() < 128 ? Qt::white : Qt::black);
		swatch->setTextAlignment(Qt::AlignCenter);
		table->setItem(row, 1, swatch);
	}
	layout->addWidget(table);
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	layout->addWidget(buttons);
	dialog.exec();
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

void MainWindow::executeApiCommand(ApiCommand cmd) {
    try {
        if (cmd.verb == "new")
            ui_.gvw->clearSession();

        else if (cmd.verb == "echo")
            return api.dispatchResponse(cmd, cmd.arguments.join("; "));

        else if (cmd.verb == "goto")
            handleGoto(cmd.arguments);

        else if (cmd.verb == "load")
            handleLoad(cmd.arguments);

        else if (cmd.verb == "genome")
            handleGenome(cmd.arguments);

        else if (cmd.verb == "setSleepInterval")
            handleSleep(cmd.arguments);

        else if (cmd.verb == "viewaspairs")
            handleViewAsPairs(cmd.arguments);

        else if (cmd.verb == "colorBy")
            handleColorBy(cmd.arguments);

        else if (cmd.verb == "group")
            handleGroupBy(cmd.arguments);

        else if (cmd.verb == "snapshot")
            handleScreenshot(cmd.arguments);

        else {
            QString msg = QString("UNKOWN COMMAND: %1").arg(cmd.verb);  // Absichtlicher Tippfehler. Das kommmt aus IGV so
            qDebug() << "Unrecognised verb" << cmd.verb;

            api.dispatchFailure(cmd, msg);
            return;
        }
    } catch (Exception e) {
        QString msg = QString("ERR: %1").arg(e.message());
        qDebug() << "An error occurred" << cmd.verb;

        api.dispatchFailure(cmd, msg);

        err("An invalid command was received", e.message());
        // THROW(Exception, "An invalid command was received: " + e.message());

        return;
    }

    api.dispatchResponse(cmd);
}

void MainWindow::err(QString header, QString text) {
    auto *box = new QMessageBox(QMessageBox::Warning, header, text, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();   // window-modal, returns immediately
}

void MainWindow::handleGoto(QStringList args)
{
    for (QString &locus : args)
    {
        BedLine region = BedLine::fromString(locus);
        if (region.isValid())
        {
            SharedData::setRegion(region.chr(), region.start(), region.end());
            return;
        } else {
            THROW(Exception, "The region specified is not valid and can therefore not be used as a destination. If you meant to jump to a particular coordinate, be sure to include the chromosome and start/end positions in the format (chrN:start-end), or just the chromonose name: (chrN).");
        }
    }
}

void MainWindow::handleLoad(QStringList args)
{
    for (auto arg : args) {
        if (arg.trimmed().isEmpty())
        {
            THROW(Exception, "The URL is empty or invalid");
        } // ui_.gvw->openFileDialog();
        else {
            QUrl url = api.parseUrl(arg);

            if (url.isLocalFile()) ui_.gvw->loadFile(url.toLocalFile());
            else ui_.gvw->loadFile(url.toString());
        }
    }
}

void MainWindow::handleGenome(QStringList args) {
    // This code used to work, but has since stopped, because GSvar passes a different file format than what is expected. Resultantly, disable the code and return an error if this function is used.

    // if (args.length() != 1) THROW(Exception, "Expected exactly 1 argument. Got " + QString::number(args.length()));

    // QString file = args[0];

    // if (file.isEmpty()) THROW(Exception, "Expected a path or URL. Got nothing");

    // RefGenomeService::instance().setReferenceGenome(api.parseUrl(file).toString());
    // ui_.gvw->clearSession();

    THROW(Exception, "This command will not be implemented due to differences in Scope between IGV and GSViewer.");

    return;
}

void MainWindow::handleSleep(QStringList args) {
    return; // TODO: Consider whether implementation is truly necessary
}

void MainWindow::handleViewAsPairs(QStringList args) {
    for (auto track : ui_.gvw->bam_tracks()) {
        track->parameters().setBool("view as pairs", true);
    }
}

static const QHash<QString, QString> RENAME_COLORBY = {
    { "UNEXPECTED_PAIR", "insert size" }
};

void MainWindow::handleColorBy(QStringList args) {
    if (args.length() != 1)
        THROW(Exception, "Invalid number of arguments received. Expected exactly 1, got " + QString::number(args.length()));

    auto rename = RENAME_COLORBY.contains(args[0]) ? RENAME_COLORBY.value(args[0]) : args[0];

    for (auto track : ui_.gvw->bam_tracks()) {
        track->parameters().setString("color by", rename.toUtf8());
    }
}

static const QHash<QString, QString> RENAME_GROUPBY = {
    {  }
};

void MainWindow::handleGroupBy(QStringList args) {
    if (args.length() < 1 || args.length() > 2)
        THROW(Exception, "Expected exactly 1-2 arguments. Got " + QString::number(args.length()));

    auto rename = RENAME_GROUPBY.contains(args[0]) ? RENAME_GROUPBY.value(args[0]) : args[0];

    for (auto track : ui_.gvw->bam_tracks())
        track->parameters().setString("group by", rename.toUtf8());
}

void MainWindow::handleScreenshot(QStringList args) {
    THROW(Exception, "This command is not implemented yet. We're awaiting architectural changes before this is possible.");
}

void MainWindow::showAboutDialog()
{
	AboutDialog dlg(this);
	dlg.setIcon(QPixmap(":/Icons/Icon.png"));
	dlg.setDescription("A free viewer for sequencing data.<br>Check the <a href='https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md'>GitHub page</a> for details.");
	dlg.addLibVersionLine("htslib version: " + QString(hts_version()));
    dlg.exec();
}
