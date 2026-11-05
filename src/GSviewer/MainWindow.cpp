#include "MainWindow.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>
#include "AboutDialog.h"
#include "ColorNames.h"
#include "GenomeVisualizationWidget.h"
#include "BamAlignmentTrack.h"
#include "SharedData.h"
#include "ParameterEditor.h"
#include "Settings.h"
#include <TrackGroupManager.h>
#include "TrackWidget.h"
#include "TranscriptCache.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui_()
	, init_timer_(this, true)
	, api_(this)
{
	ui_.setupUi(this);

	// signals and slots
	connect(ui_.actionLoadFile, &QAction::triggered, ui_.gvw_l, &GenomeVisualizationWidget::openFileDialog);
	connect(ui_.actionLoadURL, &QAction::triggered, ui_.gvw_l, &GenomeVisualizationWidget::openUrlDialog);
	connect(ui_.actionReloadTracks, &QAction::triggered, ui_.gvw_l, &GenomeVisualizationWidget::reloadTracks);
	connect(ui_.actionReloadTracks, &QAction::triggered, ui_.gvw_r, &GenomeVisualizationWidget::reloadTracks);
	connect(ui_.actionClearSession, &QAction::triggered, ui_.gvw_l, &GenomeVisualizationWidget::clearSession);
	connect(ui_.actionClearSession, &QAction::triggered, this, &MainWindow::hideSecondarySplit);
	connect(ui_.actionStore_session, &QAction::triggered, ui_.gvw_l, &GenomeVisualizationWidget::saveSession);
	connect(ui_.actionLoadSession, &QAction::triggered, ui_.gvw_l, &GenomeVisualizationWidget::loadSession);
	connect(ui_.actionOpen_documentation, &QAction::triggered, this, &MainWindow::openDocumentation);
	connect(ui_.actionSettings, &QAction::triggered, this, &MainWindow::showSettings);
	connect(ui_.actionAbout, &QAction::triggered, this, &MainWindow::showAboutDialog);
	connect(ui_.actionExit, &QAction::triggered, this, &MainWindow::close);
	connect(ui_.actionAddNRegions, &QAction::triggered, this, &MainWindow::addNRegions);
	connect(&api_, &CommandServer::commandReceived, this, &MainWindow::executeApiCommand);

	ui_.gvw_r->hide();

	//Set environment variable containing SSL certificates - needed for HTTPS to work for BamReader/htslib
	QByteArray curl_ca_bundle = Settings::string("curl_ca_bundle", true).trimmed().toUtf8();
	if (!curl_ca_bundle.isEmpty())
	{
		if (!QFile::exists(curl_ca_bundle))
		{
			THROW(FileAccessException, "CURL_CA_BUNDLE file does not exist: " + curl_ca_bundle);
		}
		if (!qputenv("CURL_CA_BUNDLE", curl_ca_bundle))
		{
			qDebug() << "Could not set CURL_CA_BUNDLE variable, access to BAM/CRAM files over HTTPS may not be possible";
		}
	}
}

void MainWindow::openPort(int port)
{
	if (port<1 || port>65535) THROW(ArgumentException, "Invalid port provided: " + QString::number(port));

	api_.open(port);
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
			QProgressDialog progress_dialog("loading gene/transcript Gff file: " + gff_file, QString(), 0, 0, this);
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
		QTextStream(stderr) << "Error loading transcripts: " + e.message();
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
						if (n_start == 0)
						{
							n_start = start + i;
						}
					}
					else if (n_start > 0)
					{
						regions->append(BedLine(chr, n_start, start + i - 1));
						n_start = 0;
					}
				}
			}
			if (n_start > 0)
			{
				regions->append(BedLine(chr, n_start, length));
			}
		}
		regions->sort();

		//store BED
		const QString folder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/generated_tracks";
		if (!QDir().mkpath(folder))
		{
			THROW(FileAccessException, "Could not create directory: " + folder);
		}
		const QByteArray reference = QFileInfo(Settings::string("reference_genome")).absoluteFilePath().toUtf8();
		const QString file_path = folder + "/N_regions.bed";
		regions->store(file_path);

		ui_.gvw_l->addBedTrack(regions, file_path, "N regions");
		ui_.gvw_r->addBedTrack(regions, file_path, "N regions");
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
	layout->setContentsMargins(QMargins(3, 3, 3, 3));
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

void MainWindow::executeApiCommand(ApiCommand cmd)
{
	try
	{
		if (cmd.verb == "new")
		{
			ui_.gvw_l->clearSession();

			ui_.gvw_r->clearSession();
			ui_.gvw_r->hide();
		}
		else if (cmd.verb == "echo")
		{
			return api_.dispatchResponse(cmd, cmd.arguments.join("; "));
		}
		else if (cmd.verb == "goto")
		{
			handleGoto(cmd.arguments);
		}
		else if (cmd.verb == "load")
		{
			handleLoad(cmd.arguments);
		}
		else if (cmd.verb == "viewaspairs")
		{
			handleViewAsPairs();
		}
		else if (cmd.verb == "colorBy")
		{
			handleColorBy(cmd.arguments);
		}
		else if (cmd.verb == "group")
		{
			handleGroupBy(cmd.arguments);
		}
		else if (cmd.verb == "snapshot")
		{
			handleScreenshot(cmd.arguments);
		}
		else if (cmd.verb == "collapse" || cmd.verb == "genome" || cmd.verb == "setSleepInterval")
		{
			//nothing to do
		}
		else
		{
			api_.dispatchFailure(cmd, "Unknown command: " + cmd.verb);
			return;
		}
	}
	catch (Exception e)
	{
		QString msg = QString("ERR: %1").arg(e.message());
		qDebug() << "An error occurred while processing '" << cmd.verb << "' command: " << e.message();
		api_.dispatchFailure(cmd, msg);
		return;
	}

	api_.dispatchResponse(cmd);
}

void MainWindow::handleGoto(QStringList args)
{
	if (args.length() < 1 || args.length() > 2) THROW(Exception, "Goto command expectes 1 or 2 arguments. Got " + QString::number(args.length()));

	BedLine region = BedLine::fromString(args[0]);
	if (!region.isValid()) THROW(Exception, "Invalid chromosomal region provided: " + args[0]);
	ui_.gvw_l->setRegion(region.chr(), region.start(), region.end());

	if (args.length() != 2)
	{
		ui_.gvw_r->hide();
	}
	else
	{
		ui_.gvw_r->show();
		region = BedLine::fromString(args[1]);
		if (!region.isValid()) THROW(Exception, "Invalid chromosomal region provided: " + args[1]);
		ui_.gvw_r->setRegion(region.chr(), region.start(), region.end());
	}
}

void MainWindow::handleLoad(QStringList args)
{
	for (auto &arg : args)
	{
		if (arg.trimmed().isEmpty())
		{
			THROW(Exception, "The URL is empty or invalid");
		}
		else
		{
			QUrl url = api_.parseUrl(arg);

			if (url.isLocalFile())
			{
				ui_.gvw_l->loadFile(url.toLocalFile());
				if (!ui_.gvw_r->isHidden()) ui_.gvw_r->loadFile(url.toLocalFile());
			}
			else
			{
				ui_.gvw_l->loadFile(url.toString());
				if (!ui_.gvw_r->isHidden()) ui_.gvw_r->loadFile(url.toString());
			}
		}
	}
}

void MainWindow::handleViewAsPairs()
{
	for (auto track : ui_.gvw_l->bamTracks())
	{
		track->parameters().setBool("view as pairs", true);
	}

	if (!ui_.gvw_r->isHidden())
	{
		for (auto track : ui_.gvw_r->bamTracks())
		{
			track->parameters().setBool("view as pairs", true);
		}
	}
}

static const QHash<QString, QString> RENAME_COLORBY = {{"UNEXPECTED_PAIR", "insert size"}};

void MainWindow::handleColorBy(QStringList args)
{
	if (args.length()!=1) THROW(Exception, "ColorBy command expects 1 argument. Got " + QString::number(args.length()));

	auto rename = RENAME_COLORBY.contains(args[0]) ? RENAME_COLORBY.value(args[0]) : args[0];

	for (auto track : ui_.gvw_l->bamTracks())
	{
		track->parameters().setString("color by", rename.toUtf8());
	}

	if (!ui_.gvw_r->isHidden())
	{
		for (auto track : ui_.gvw_r->bamTracks())
		{
			track->parameters().setString("color by", rename.toUtf8());
		}
	}
}

void MainWindow::handleGroupBy(QStringList args)
{
	if (args.length()!=1) THROW(Exception, "GroupBy command expects 1 argument. Got " + QString::number(args.length()));

	for (auto track : ui_.gvw_l->bamTracks())
	{
		track->parameters().setString("group by", args[0].toUtf8());
	}

	if (!ui_.gvw_r->isHidden())
	{
		for (auto track : ui_.gvw_r->bamTracks())
		{
			track->parameters().setString("group by", args[0].toUtf8());
		}
	}
}

void MainWindow::handleScreenshot(QStringList args)
{
	if (args.length()!=1) THROW(Exception, "Snapshot command expects 1 argument. Got " + QString::number(args.length()));

	// TODO: refactor to capture both panels if open.

	auto track = findChild<TrackGroupManager*>();
	track->ensurePolished();
	if (track->layout()) track->layout()->activate();
	if (track->size().isEmpty()) track->resize(track->sizeHint());

	QImage img(track->size(), QImage::Format_ARGB32_Premultiplied);
	img.setDevicePixelRatio(1.0);
	img.fill(Qt::transparent);
	track->render(&img, QPoint(), QRegion(), QWidget::DrawWindowBackground | QWidget::DrawChildren);
	img.save(args[0]);
}

void MainWindow::hideSecondarySplit()
{
	ui_.gvw_r->hide();
}

void MainWindow::showSecondarySplit()
{
	ui_.gvw_r->synchroniseFrom(ui_.gvw_l);
	ui_.gvw_r->show();
}

void MainWindow::toggleSecondarySplit()
{
	ui_.gvw_r->setHidden(!ui_.gvw_r->isHidden());
}

void MainWindow::showAboutDialog()
{
	AboutDialog dlg(this);
	dlg.setIcon(QPixmap(":/Icons/Icon.png"));
	dlg.setDescription("A free viewer for sequencing data.<br>It it similar to IGV, but specifically designed for visusalizing human data.<br>Check the <a href='https://github.com/imgag/ngs-bits/blob/master/doc/GSviewer/index.md'>GitHub page</a> for details.");
	dlg.addLibVersionLine("htslib version: " + QString(hts_version()));

	//API status
	int port = api_.port();
	dlg.setAddInfo(port==-1 ? "API enabled: no" : "API enabled: yes<br>API port: " + QString::number(port));

	dlg.exec();
}
