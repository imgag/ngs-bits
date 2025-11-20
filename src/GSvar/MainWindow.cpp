#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include "Settings.h"
#include "Exceptions.h"
#include "ChromosomalIndex.h"
#include "Log.h"
#include "Helper.h"
#include "GUIHelper.h"
#include "GeneSet.h"
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QTime>
#include "ExternalToolDialog.h"
#include "ReportDialog.h"
#include <QInputDialog>
#include <QToolButton>
#include <GenLabDB.h>
#include <QToolTip>
#include <QImage>
#include <QBuffer>
#include "Background/ReportWorker.h"
#include "ScrollableTextDialog.h"
#include "AnalysisStatusWidget.h"
#include "HttpHandler.h"
#include "ValidationDialog.h"
#include "ClassificationDialog.h"
#include "BasicStatistics.h"
#include "ApprovedGenesDialog.h"
#include "GeneWidget.h"
#include "PhenoToGenesDialog.h"
#include "GenesToRegionsDialog.h"
#include "SubpanelDesignDialog.h"
#include "SubpanelArchiveDialog.h"
#include "GapDialog.h"
#include "EmailDialog.h"
#include "CnvWidget.h"
#include "CnvList.h"
#include "RohWidget.h"
#include "GeneSelectorDialog.h"
#include "NGSHelper.h"
#include "DiseaseInfoWidget.h"
#include "SmallVariantSearchWidget.h"
#include "TSVFileStream.h"
#include "OntologyTermCollection.h"
#include "SvWidget.h"
#include "VariantWidget.h"
#include "SomaticReportConfigurationWidget.h"
#include "Histogram.h"
#include "ProcessedSampleWidget.h"
#include "DBSelector.h"
#include "SequencingRunWidget.h"
#include "SimpleCrypt.h"
#include "ToolBase.h"
#include "BedpeFile.h"
#include "SampleSearchWidget.h"
#include "ProcessedSampleSelector.h"
#include "ReportVariantDialog.h"
#include "SomaticReportVariantDialog.h"
#include "GSvarHelper.h"
#include "SampleDiseaseInfoWidget.h"
#include "SomaticRnaReport.h"
#include "ProcessingSystemWidget.h"
#include "ProjectWidget.h"
#include "TsvTableWidget.h"
#include "DBTableAdministration.h"
#include "SequencingRunOverview.h"
#include "MidCheckWidget.h"
#include "CnvSearchWidget.h"
#include "VariantValidationWidget.h"
#include "SomaticReportDialog.h"
#include "GeneOmimInfoWidget.h"
#include "LoginManager.h"
#include "LoginDialog.h"
#include "IgvSessionManager.h"
#include "VariantConversionWidget.h"
#include "PasswordDialog.h"
#include "CircosPlotWidget.h"
#include "SomaticReportSettings.h"
#include "CytobandToRegionsDialog.h"
#include "RepeatLocusList.h"
#include "SomaticDataTransferWidget.h"
#include "PRSWidget.h"
#include "EvaluationSheetEditDialog.h"
#include "SvSearchWidget.h"
#include "PublishedVariantsWidget.h"
#include "PreferredTranscriptsWidget.h"
#include "TumorOnlyReportWorker.h"
#include "TumorOnlyReportDialog.h"
#include "VariantScores.h"
#include "CfDNAPanelDesignDialog.h"
#include "DiseaseCourseWidget.h"
#include "CfDNAPanelWidget.h"
#include "SomaticVariantInterpreterWidget.h"
#include "AlleleBalanceCalculator.h"
#include "ExpressionGeneWidget.h"
#include "GapClosingDialog.h"
#include "GermlineReportGenerator.h"
#include "SomaticReportHelper.h"
#include "Statistics.h"
#include "CohortAnalysisWidget.h"
#include "cfDNARemovedRegions.h"
#include "CfDNAPanelBatchImport.h"
#include "ClinvarUploadDialog.h"
#include "LiftOverWidget.h"
#include "BlatWidget.h"
#include "FusionWidget.h"
#include "CausalVariantEditDialog.h"
#include "VariantOpenDialog.h"
#include "ExpressionOverviewWidget.h"
#include "ExpressionExonWidget.h"
#include "SplicingWidget.h"
#include "VariantHgvsAnnotator.h"
#include "VirusDetectionWidget.h"
#include "SomaticcfDNAReport.h"
#include "ClientHelper.h"
#include "GHGAUploadDialog.h"
#include "BurdenTestWidget.h"
#include "IgvLogWidget.h"
#include "SettingsDialog.h"
#include "GlobalServiceProvider.h"
#include "ImportDialog.h"
#include "PathogenicWtDialog.h"
#include "Background/NGSDCacheInitializer.h"
#include "RepeatExpansionWidget.h"
#include "ReSearchWidget.h"
#include "CustomProxyService.h"
#include "GeneInterpretabilityDialog.h"
#include "HerediVarImportDialog.h"
#include "SampleCountWidget.h"
#include "MethylationWidget.h"
#include <QClipboard>
#include "VersatileTextStream.h"
#include "FileLocationProviderLocal.h"
#include "FileLocationProviderRemote.h"
#include <QMimeData>
#include "MaintenanceDialog.h"
#include <QStyleFactory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtCharts/QChartView>
#else
#include <QChartView>
QT_CHARTS_USE_NAMESPACE
#endif

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui_()
	, var_last_(-1)
	, notification_label_(new QLabel())
	, igv_history_label_(new ClickableLabel())
	, background_job_label_(new ClickableLabel())
	, filename_()
	, variants_changed_()
	, last_report_path_(QDir::homePath())
	, init_timer_(this, true)
	, server_version_()   
{
    // Automatic configuration will be triggered, if a template file is detected and no settings files are present.
    // A new settings.ini file is created with parameters based on the current application path value. If there is no
    // template and/or settings exist, the contructor will skip the automatic configuration step
    QString settings_template_file = QCoreApplication::applicationDirPath() + QDir::separator() + "cloud_settings_template.ini";
    if (QFile::exists(settings_template_file) && !QFile::exists(QCoreApplication::applicationDirPath() + QDir::separator() + "settings.ini"))
    {
        int res = QMessageBox::question(this, "Configuration check", "GSvar is not configured correctly.\n Do you want to start automatic configuration?");
        if (res==QMessageBox::Yes)
        {
            QSettings* settings_generated = new QSettings(QCoreApplication::applicationDirPath() + QDir::separator() + "settings.ini", QSettings::IniFormat);
            QSettings* settings_template = new QSettings(settings_template_file, QSettings::IniFormat);
            if (settings_template != nullptr)
            {
                Log::info("Generating a new settings file from a template");
                QStringList template_keys = settings_template->allKeys();
                for (int i = 0; i< template_keys.count(); i++)
                {
                    settings_generated->setValue(template_keys[i], GSvarHelper::appPathForTemplate(settings_template->value(template_keys[i]).toString()));
                }
            }
            settings_generated->sync();
        }
    }

    // Use a proxy server for all connections to the GSvar server
    if (Settings::boolean("use_proxy_for_gsvar_server", true))
    {
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(Settings::string("proxy_host"));
        proxy.setPort(Settings::integer("proxy_port"));
        proxy.setUser(Settings::string("proxy_user"));
        proxy.setPassword(Settings::string("proxy_password"));
        CustomProxyService::setProxy(proxy);
    }

	//set style
	setStyle(Settings::string("window_style", true));

    //setup GUI
	ui_.setupUi(this);
	setWindowTitle(appName());
	GUIHelper::styleSplitter(ui_.splitter);
	ui_.splitter->setStretchFactor(0, 10);
	ui_.splitter->setStretchFactor(1, 1);
	GUIHelper::styleSplitter(ui_.splitter_2);
	ui_.splitter_2->setStretchFactor(0, 10);
	ui_.splitter_2->setStretchFactor(1, 1);
	connect(ui_.tabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
	connect(ui_.tabs, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tabContextMenu(QPoint)));

	//gaps button
	gap_btn_ = new QToolButton();
	gap_btn_->setObjectName("gap_btn");
	gap_btn_->setIcon(QIcon(":/Icons/Gaps_lookup.png"));
	gap_btn_->setToolTip("Determine low-coverage regions (gaps) and request closing the gaps if necessary.");
	gap_btn_->setPopupMode(QToolButton::InstantPopup);
	gap_btn_->setMenu(new QMenu());
	gap_btn_->menu()->addAction("Gaps by target region filter", this, SLOT(calculateGapsByTargetRegionFilter()));
	gap_btn_->menu()->addAction("Gaps by gene(s)", this, SLOT(calculateGapsByGenes()));
	ui_.tools->insertWidget(ui_.actionGeneSelector, gap_btn_);

	// add rna menu
	rna_menu_btn_ = new QToolButton();
	rna_menu_btn_->setObjectName("rna_btn");
	rna_menu_btn_->setIcon(QIcon(":/Icons/RNA.png"));
	rna_menu_btn_->setToolTip("Open RNA menu entries");
	rna_menu_btn_->setPopupMode(QToolButton::InstantPopup);
	rna_menu_btn_->setMenu(new QMenu());
	rna_menu_btn_->menu()->addAction(ui_.actionExpressionData);
	rna_menu_btn_->menu()->addAction(ui_.actionExonExpressionData);
	rna_menu_btn_->menu()->addAction(ui_.actionShowSplicing);
	rna_menu_btn_->menu()->addAction(ui_.actionShowRnaFusions);
	rna_menu_btn_->menu()->addAction(ui_.actionShowProcessingSystemCoverage);

	ui_.actionExpressionData->setEnabled(false);
	ui_.actionExonExpressionData->setEnabled(false);
	ui_.actionShowSplicing->setEnabled(false);
	ui_.actionShowRnaFusions->setEnabled(false);

	ui_.tools->addWidget(rna_menu_btn_);


	// add cfdna menu
	cfdna_menu_btn_ = new QToolButton();
	cfdna_menu_btn_->setObjectName("cfdna_btn");
	cfdna_menu_btn_->setIcon(QIcon(":/Icons/cfDNA.png"));
	cfdna_menu_btn_->setToolTip("Open cfDNA menu entries");
	cfdna_menu_btn_->setPopupMode(QToolButton::InstantPopup);
	cfdna_menu_btn_->setMenu(new QMenu());
	cfdna_menu_btn_->menu()->addAction(ui_.actionDesignCfDNAPanel);
	cfdna_menu_btn_->menu()->addAction(ui_.actionShowCfDNAPanel);
	cfdna_menu_btn_->menu()->addAction(ui_.actionCfDNADiseaseCourse);
	cfdna_menu_btn_->menu()->addAction(ui_.actionCfDNAAddExcludedRegions);
	ui_.tools->addWidget(cfdna_menu_btn_);
	// deaktivate on default (only available in somatic)
	cfdna_menu_btn_->setVisible(false);
	cfdna_menu_btn_->setEnabled(false);
	ui_.actionVirusDetection->setEnabled(false);

	//debugging
	if (Settings::boolean("debug_mode_enabled", true))
	{
		QToolButton* debug_btn = new QToolButton();
		debug_btn->setObjectName("cfdna_btn");
		debug_btn->setIcon(QIcon(":/Icons/bug.png"));
		debug_btn->setPopupMode(QToolButton::InstantPopup);
		debug_btn->setMenu(new QMenu());
		debug_btn->menu()->addAction("user-specific function", this, SLOT(userSpecificDebugFunction()));
		debug_btn->menu()->addSeparator();
		debug_btn->menu()->addAction("variant: chr1:1212033-1212033 G>T", this, SLOT(openDebugTab()));
		debug_btn->menu()->addAction("variant: chr15:42411019-42411019 C>T", this, SLOT(openDebugTab()));
		debug_btn->menu()->addSeparator();
		debug_btn->menu()->addAction("gene: BRCA2", this, SLOT(openDebugTab()));
		debug_btn->menu()->addSeparator();
		debug_btn->menu()->addAction("processed sample: NA12878_58", this, SLOT(openDebugTab()));
		debug_btn->menu()->addAction("processed sample: NA12878x3_28", this, SLOT(openDebugTab()));
		debug_btn->menu()->addAction("processed sample: 23014LRa023L2_01", this, SLOT(openDebugTab()));
		debug_btn->menu()->addAction("processed sample: DNA2405534A1_01", this, SLOT(openDebugTab()));
		ui_.tools->addWidget(debug_btn);
	}
	ui_.actionEncrypt->setEnabled(Settings::boolean("debug_mode_enabled", true));

	//signals and slots
    connect(ui_.actionExit, SIGNAL(triggered()), this, SLOT(closeAndLogout()));

	connect(ui_.filters, SIGNAL(filtersChanged()), this, SLOT(refreshVariantTable()));
	connect(ui_.vars, SIGNAL(itemSelectionChanged()), this, SLOT(updateVariantDetails()));
	connect(ui_.vars, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(variantCellDoubleClicked(int, int)));
	connect(ui_.vars, SIGNAL(showMatchingCnvsAndSvs(BedLine)), this, SLOT(showMatchingCnvsAndSvs(BedLine)));
	connect(ui_.vars->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(variantHeaderDoubleClicked(int)));
	ui_.vars->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.vars->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(varHeaderContextMenu(QPoint)));
	ui_.vars->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.vars->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(columnContextMenu(QPoint)));
	connect(ui_.actionDesignSubpanel, SIGNAL(triggered()), this, SLOT(openSubpanelDesignDialog()));
	connect(ui_.filters, SIGNAL(phenotypeImportNGSDRequested()), this, SLOT(importPhenotypesFromNGSD()));
	connect(ui_.filters, SIGNAL(phenotypeSubPanelRequested()), this, SLOT(createSubPanelFromPhenotypeFilter()));

	//variants tool bar
	connect(ui_.vars_copy_btn, SIGNAL(clicked(bool)), ui_.vars, SLOT(copyToClipboard()));
	ui_.vars_export_btn->setMenu(new QMenu());
	ui_.vars_export_btn->menu()->addAction("Export GSvar (filtered)", this, SLOT(exportGSvar()));
	ui_.vars_export_btn->menu()->addAction("Export VCF (filtered)", this, SLOT(exportVCF()));
	if (Settings::string("HerediVar", true).trimmed()!="")
	{
		ui_.vars_export_btn->menu()->addAction("Export VCF for HerediCare", this, SLOT(exportHerediCareVCF()));
	}

	ui_.report_btn->setMenu(new QMenu());
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report_add_causal.png"), "Add/edit other causal variant", this, SLOT(editOtherCausalVariant()));
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report_exclude.png"), "Delete other causal variant", this, SLOT(deleteOtherCausalVariant()));
	ui_.report_btn->menu()->addSeparator();
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report.png"), "Generate report", this, SLOT(generateReport()));
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report.png"), "Generate evaluation sheet", this, SLOT(generateEvaluationSheet()));
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report_info.png"), "Show report configuration info", this, SLOT(showReportConfigInfo()));
	ui_.report_btn->menu()->addSeparator();
	ui_.report_btn->menu()->addAction(QIcon(":/Icons/Report_finalize.png"), "Finalize report configuration", this, SLOT(finalizeReportConfig()));
	ui_.report_btn->menu()->addSeparator();
	ui_.report_btn->menu()->addAction("Transfer somatic data to MTB", this, SLOT(transferSomaticData()) );
	connect(ui_.vars_folder_btn, SIGNAL(clicked(bool)), this, SLOT(openVariantListFolder()));
	connect(ui_.open_qc_files, SIGNAL(clicked(bool)), this, SLOT(openVariantListQcFiles()));
	ui_.vars_ranking->setMenu(new QMenu());
	ui_.vars_ranking->menu()->addAction("dominant model", this, SLOT(variantRanking()))->setObjectName("GSvar_v2_dominant");
	ui_.vars_ranking->menu()->addAction("recessive model", this, SLOT(variantRanking()))->setObjectName("GSvar_v2_recessive");
	ui_.vars_af_hist->setMenu(new QMenu());
	ui_.vars_af_hist->menu()->addAction("Show AF histogram (all small variants)", this, SLOT(showAfHistogram_all()));
	ui_.vars_af_hist->menu()->addAction("Show AF histogram (small variants after filter)", this, SLOT(showAfHistogram_filtered()));
	ui_.vars_af_hist->menu()->addSeparator();
	ui_.vars_af_hist->menu()->addAction("Show CN histogram (in given region)", this, SLOT(showCnHistogram()));
	ui_.vars_af_hist->menu()->addAction("Show BAF histogram (in given region)", this, SLOT(showBafHistogram()));
	ui_.vars_resize_btn->setMenu(new QMenu());
	ui_.vars_resize_btn->menu()->addAction("Open column settings", this, SLOT(openColumnSettings()));
	ui_.vars_resize_btn->menu()->addAction("Apply column width settings", ui_.vars, SLOT(adaptColumnWidths()));
	ui_.vars_resize_btn->menu()->addAction("Show all columns", ui_.vars, SLOT(showAllColumns()));

	connect(ui_.ps_details, SIGNAL(clicked(bool)), this, SLOT(openProcessedSampleTabsCurrentAnalysis()));

	//if at home, use Patientenserver
	QString gsvar_report_folder = Settings::path("gsvar_report_folder", true);
	if (gsvar_report_folder!="" && QDir(gsvar_report_folder).exists())
	{
		last_report_path_ = gsvar_report_folder;
	}

	//toolbar: add notification icon
	notification_label_->hide();
	notification_label_->setScaledContents(true);
	notification_label_->setMaximumSize(16,16);
	notification_label_->setPixmap(QPixmap(":/Icons/email.png"));
	ui_.statusBar->addPermanentWidget(notification_label_);

	//toolbar: add IGV history icon
	igv_history_label_->setScaledContents(true);
    igv_history_label_->setMaximumSize(16,16);
    igv_history_label_->setPixmap(QPixmap(":/Icons/IGV.png"));
    igv_history_label_->setToolTip("Show the history of IGV commands");
    ui_.statusBar->addPermanentWidget(igv_history_label_);
	connect(igv_history_label_, SIGNAL(clicked(QPoint)), this, SLOT(displayIgvHistoryTable()));

	//toolbar: add background job
	bg_job_dialog_ = new BackgroundJobDialog(this);
	bg_job_dialog_->hide();
	background_job_label_->setScaledContents(true);
	background_job_label_->setMaximumSize(16,16);
	background_job_label_->setPixmap(QPixmap(":/Icons/multithreading.png"));
	background_job_label_->setToolTip("Show the history of background jobs");
	ui_.statusBar->addPermanentWidget(background_job_label_);
	connect(background_job_label_, SIGNAL(clicked(QPoint)), this, SLOT(showBackgroundJobDialog()));

	// Setting a value for the current working directory. On Linux it is defined in the TMPDIR environment
	// variable or /tmp if TMPDIR is not set. On Windows it is saved in the TEMP or TMP environment variable.
	// e.g. c:\Users\USER_NAME\AppData\Local\Temp
	// It is needed to enable saving *.bai files while accessing remote *.bam files. htsLib tries to save the
	// index file locally, if it deals with a remote *.bam file. The index file is always saved at the current
	// working directory, and it seems there is no way to change it. On some systems users may not have write
	// priveleges for the working directory and this is precisely why we came up with this workaround:
	QDir::setCurrent(QDir::tempPath());

	//enable timers needed in client-server mode
	if (ClientHelper::isClientServerMode())
	{
		// renew existing session, if it is about to expire
		// a new token will be requested slightly in advance
        QTimer *login_timer = new QTimer(this);
        connect(login_timer, SIGNAL(timeout()), this, SLOT(updateSecureToken()));
        login_timer->start(20 * 60 * 1000); // every 20 minutes

		//check if the server is running
		QTimer *server_ping_timer = new QTimer(this);
		connect(server_ping_timer, SIGNAL(timeout()), this, SLOT(checkServerAvailability()));
		server_ping_timer->start(10 * 60 * 1000); // every 10 minutes


		//check if there are new notifications for the users
		if (Settings::boolean("display_user_notifications", true))
		{
			QTimer *user_notification_timer = new QTimer(this);
			connect(user_notification_timer, SIGNAL(timeout()), this, SLOT(checkUserNotifications()));
			user_notification_timer->start(12 * 60 * 1000); // every 12 minutes
		}

		displayed_maintenance_message_id_ = "";
	}

	connect(ui_.vars, SIGNAL(publishToClinvarTriggered(int, int)), this, SLOT(uploadToClinvar(int, int)));
	connect(ui_.vars, SIGNAL(alamutTriggered(QAction*)), this, SLOT(openAlamut(QAction*)));

	// Environment variable containing the file path to the list of certificate authorities
	// (needed for HTTPS to work correctly, especially for htslib and BamReader)
	QString curl_ca_bundle = Settings::string("curl_ca_bundle", true);
	if ((Helper::isWindows()) && (!curl_ca_bundle.isEmpty()))
	{
		if (!qputenv("CURL_CA_BUNDLE", curl_ca_bundle.toUtf8()))
		{
			Log::error("Could not set CURL_CA_BUNDLE variable, access to BAM/CRAM files over HTTPS may not be possible");
		}
	}

	if (Settings::boolean("use_proxy_for_gsvar_server", true))
	{
		QString proxy_params = "http://" + Settings::string("proxy_user") + ":" + Settings::string("proxy_password") + "@" + Settings::string("proxy_host") + ":" + QString::number(Settings::integer("proxy_port"));
		if (!qputenv("HTTPS_PROXY", proxy_params.toUtf8()))
		{
			Log::error("Could not set HTTPS_PROXY variable, access to BAM/CRAM files over HTTPS may not be possible");
		}
		if (!qputenv("HTTP_PROXY", proxy_params.toUtf8()))
		{
			Log::error("Could not set HTTP_PROXY variable, access to BAM/CRAM files over HTTP may not be possible");
		}
	}
}

QString MainWindow::appName() const
{
	QString name = QCoreApplication::applicationName();

	GenomeBuild build = GSvarHelper::build();
	if (build!=GenomeBuild::HG38) name += " - " + buildToString(build);

	return name;
}

bool MainWindow::isServerRunning()
{
    int status_code = -1;
    ServerInfo server_info = ClientHelper::getServerInfo(status_code);

    if (server_info.isEmpty())
	{
		QMessageBox::warning(this, "Server not available", "GSvar is configured for the client-server mode, but the server is not available. The application will be closed");
		return false;
	}

    if (status_code!=200)
    {
        QMessageBox::warning(this, "Server availability problem", "Server replied with " + QString::number(status_code) + " code. The application will be closed");
        return false;
    }

    if (!server_version_.isEmpty() && (server_version_ != server_info.version))
    {
        QMessageBox::information(this, "Server version changed", "Server version has changed from " + server_version_ + " to " + server_info.version + ". No action is required");
    }
    server_version_ = server_info.version;

	if (ClientHelper::serverApiVersion() != server_info.api_version)
	{
		QMessageBox::warning(this, "Version mismatch", "GSvar uses API " + ClientHelper::serverApiVersion() + ", while the server uses API " + server_info.api_version + ". No stable work can be guaranteed. The application will be closed");
		return false;
	}

    return true;
}

void MainWindow::lazyLoadIGVfiles(QString current_file)
{
    IgvSessionManager::get(0).removeCache();
    IgvSessionManager::get(0).startCachingForRegularIGV(variants_.type(), current_file);
}

void MainWindow::checkServerAvailability()
{
	if (!isServerRunning())
	{
		close();
	}
}

void MainWindow::checkUserNotifications()
{
	UserNotification user_notification = ClientHelper::getUserNotification();

	if (user_notification.id.isEmpty() || user_notification.message.isEmpty()) return;
	if ((!displayed_maintenance_message_id_.isEmpty()) && (displayed_maintenance_message_id_ == user_notification.id)) return;

	displayed_maintenance_message_id_ = user_notification.id;
	QMessageBox::information(this, "Important information", user_notification.message);
}

void MainWindow::checkClientUpdates()
{
	if (!ClientHelper::isClientServerMode()) return;

	ClientInfo client_info = ClientHelper::getClientInfo();
	if (client_info.isEmpty())
	{
		Log::warn("Could not retrieve updates information from the server");
		return;
	}

	int commit_pos = QCoreApplication::applicationVersion().lastIndexOf("-");
	if (commit_pos==-1) return;

	QString short_version = QCoreApplication::applicationVersion().left(commit_pos);
	if (!ClientInfo(short_version, "").isOlderThan(client_info)) return;

	Log::info("Client version from the server: " + client_info.version);
	QString message = client_info.message.isEmpty() ? "Please restart the application" : client_info.message;
	QMessageBox::warning(this, "GSvar client notification", message);
}

QString MainWindow::getCurrentFileName()
{
    return filename_;
}

AnalysisType MainWindow::getCurrentAnalysisType()
{
    return variants_.type();
}

void MainWindow::userSpecificDebugFunction()
{
    QElapsedTimer timer;
	timer.start();

	QString user = Helper::userName();
	if (user=="ahsturm1")
	{
		QTextStream out(stdout);
		QElapsedTimer timer;
		QByteArray line = Helper::randomString(1024*1024-10).toLatin1() + "\n"; // 1 line == 1 MB
		foreach(QByteArray filename, QByteArrayList() << "C:\\Marc\\test.txt" << "E:\\Marc\\test.txt")
		{
			foreach(int lines, QList<int>() << 100 << 1000 << 10000)
			{
				out << filename << " - " << QString::number(lines) << " MB\n";

				//remove output file
				if (QFile::exists(filename))
				{
					QFile::remove(filename);
				}

				//write test
				timer.start();
				QFile file(filename);
				file.open(QFile::ReadWrite);
				for(int i=0; i<lines; ++i)
				{
					file.write(line);
				}
				file.close();
				out << "  write: " << Helper::elapsedTime(timer, true) << "\n";
				out.flush();

				//read test
				timer.start();
				char c = 'x';
				file.setFileName(filename);
				file.open(QFile::ReadOnly);
				while(!file.atEnd())
				{
					const int buf_size = 1024*1024;
					char buf[buf_size];
					qint64 chars_read = file.readLine(buf, buf_size);
					if (chars_read!=-1)
					{
						c = buf[0];
					}
				}
				out << "  read: " << Helper::elapsedTime(timer, true) << " (char=" << c << ")\n";
				out.flush();
			}
		}
	}
	else if (user=="ahschul1")
	{
		qDebug() << NGSD().secondaryAnalyses("21073LRa154_01", "trio");
//		qDebug() << NGSD().secondaryAnalyses("21073LRa033_01", "trio");
//		qDebug() << NGSD().secondaryAnalyses("21073LRa036_01", "trio");

	}
	else if (user=="ahott1a1")
	{

	}

	qDebug() << "Elapsed time debug function:" << Helper::elapsedTime(timer, true);
}

void MainWindow::openDebugTab()
{
	QAction* action = qobject_cast<QAction*>(sender());
	QString text = action->text();
	if (text.startsWith("variant:"))
	{
		openVariantTab(Variant::fromString(text.mid(8).trimmed()));
	}
	else if (text.startsWith("gene:"))
	{
		openGeneTab(text.mid(5).trimmed());
	}
	else if (text.startsWith("processed sample:"))
	{
		openProcessedSampleTab(text.mid(17).trimmed());
	}
	else
	{
		THROW(ProgrammingException, "Unprocessed debug action: " + text);
	}
}



void MainWindow::on_actionConvertVcfToGSvar_triggered()
{
	VariantConversionWidget* widget = new VariantConversionWidget();
	widget->setMode(VariantConversionWidget::VCF_TO_GSVAR);
	auto dlg = GUIHelper::createDialog(widget, "Variant conversion (VCF > GSvar)");
	addModelessDialog(dlg);
}

void MainWindow::on_actionConvertHgvsToGSvar_triggered()
{
	VariantConversionWidget* widget = new VariantConversionWidget();
	widget->setMode(VariantConversionWidget::HGVSC_TO_GSVAR);
	auto dlg = GUIHelper::createDialog(widget, "Variant conversion (HGVS.c > GSvar)");
	addModelessDialog(dlg);
}

void MainWindow::on_actionConvertGSvarToVcf_triggered()
{
	VariantConversionWidget* widget = new VariantConversionWidget();
	widget->setMode(VariantConversionWidget::GSVAR_TO_VCF);
	auto dlg = GUIHelper::createDialog(widget, "Variant conversion (GSvar > VCF)");
	addModelessDialog(dlg);
}

void MainWindow::on_actionCytobandsToRegions_triggered()
{
	CytobandToRegionsDialog dlg(this);

	dlg.exec();
}

void MainWindow::on_actionRegionToGenes_triggered()
{
	QString title = "Region > Genes";

	try
	{
		//get region string
		QString region_text = QInputDialog::getText(this, title, "genomic region");
		if (region_text=="") return;

		QApplication::setOverrideCursor(Qt::BusyCursor);

		//convert to region
		Chromosome chr;
		int start, end;
		NGSHelper::parseRegion(region_text, chr, start, end);

		//convert region to gene set
		NGSD db;
		GeneSet genes = db.genesOverlapping(chr, start, end);

		QApplication::restoreOverrideCursor();

		//show results
		ScrollableTextDialog dlg(this, title);
		dlg.setReadOnly(true);
		dlg.setWordWrapMode(QTextOption::NoWrap);
		dlg.appendLine("#GENE\tOMIM_GENE\tOMIM_PHENOTYPES");
        for (const QByteArray& gene : genes)
		{
			QList<OmimInfo> omim_genes = db.omimInfo(gene);
			foreach (const OmimInfo& omim_gene, omim_genes)
			{
				dlg.appendLine(gene + "\t" + omim_gene.gene_symbol + "\t" + omim_gene.phenotypes.toString());
			}
		}
		dlg.exec();
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, title, "Error:\n" + e.message());
		return;
	}
}

void MainWindow::on_actionSearchSNVs_triggered()
{
	SmallVariantSearchWidget* widget = new SmallVariantSearchWidget();
	auto dlg = GUIHelper::createDialog(widget, "Small variants search");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSearchCNVs_triggered()
{
	CnvSearchWidget* widget = new CnvSearchWidget();
	auto dlg = GUIHelper::createDialog(widget, "CNV search");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSearchSVs_triggered()
{
	SvSearchWidget* widget = new SvSearchWidget();
	auto dlg = GUIHelper::createDialog(widget, "SV search");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSearchREs_triggered()
{
	ReSearchWidget* widget = new ReSearchWidget();
	auto dlg = GUIHelper::createDialog(widget, "RE search");
	addModelessDialog(dlg);
}
void MainWindow::on_actionUploadVariantToClinVar_triggered()
{
	QSharedPointer<QDialog> dlg = QSharedPointer<QDialog>(new ClinvarUploadDialog(this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowPublishedVariants_triggered()
{
	PublishedVariantsWidget* widget = new PublishedVariantsWidget();

	auto dlg = GUIHelper::createDialog(widget, "Published variants");
	dlg->exec();
}

void MainWindow::on_actionAlleleBalance_triggered()
{
	AlleleBalanceCalculator* widget = new AlleleBalanceCalculator();
	auto dlg = GUIHelper::createDialog(widget, "Allele balance of heterzygous variants");
	dlg->exec();
}

void MainWindow::on_actionLiftOver_triggered()
{
	LiftOverWidget* widget = new LiftOverWidget(this);
	auto dlg = GUIHelper::createDialog(widget, "Lift-over genome coordinates");
	addModelessDialog(dlg);
}

void MainWindow::on_actionGetGenomicSequence_triggered()
{
	QString title = "Get genomic sequence";
	try
	{
		//get region
		QString region_text = QInputDialog::getText(this, title, "genomic region:");
		if (region_text=="") return;

		Chromosome chr;
		int start, end;
		NGSHelper::parseRegion(region_text, chr, start, end);

		//get sequence
		QString genome_file = Settings::string("reference_genome", false);
		FastaFileIndex genome_idx(genome_file);
		int length = end-start+1;
		Sequence sequence = genome_idx.seq(chr, start, length, true);

		//copy to clipboard
		QApplication::clipboard()->setText(sequence);

		//show message
		if (sequence.length()>100)
		{
			sequence.resize(100);
			sequence += "...";
		}
		QMessageBox::information(this, title, "Extracted reference sequence of region " + chr.strNormalized(true) + ":" + QString::number(start) + "-" + QString::number(end) + " (length " + QString::number(length) + "):\n" + sequence + "\n\nThe sequence was copied to the clipboard.");
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, title, "Error getting reference sequence:\n" + e.message());
	}
}

void MainWindow::on_actionBlatSearch_triggered()
{
	BlatWidget* widget = new BlatWidget(this);

	auto dlg = GUIHelper::createDialog(widget, "BLAT search");
	addModelessDialog(dlg);
}


void MainWindow::on_actionClose_triggered()
{
	loadFile();
}

void MainWindow::on_actionCloseMetaDataTabs_triggered()
{
	for (int t=ui_.tabs->count()-1; t>0; --t)
	{
		closeTab(t);
	}
}

void MainWindow::on_actionImportVariants_triggered()
{
	ImportDialog dlg(this, ImportDialog::VARIANTS);
	dlg.exec();
}

void MainWindow::on_actionIgvClear_triggered()
{
    IgvSessionManager::get(0).clear();
}

void MainWindow::on_actionIgvDocumentation_triggered()
{
	QDesktopServices::openUrl(QUrl("https://igv.org/doc/desktop/#UserGuide/reference_genome/"));
}

void MainWindow::on_actionDeleteIgvFolder_triggered()
{
	QString title = "Delete IGV folder";
	QString path = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0) + QDir::separator() + "igv" + QDir::separator();
	QString path_canonical = QFileInfo(path).canonicalFilePath();
	if(path_canonical.isEmpty())
	{
	   QMessageBox::information(this, title, "IGV folder does not exist:\n" + path);
	   return;
	}

	int res = QMessageBox::question(this, title, "Do you want to delete the IGV folder?\nLocation: "+path_canonical+"\n\nYou will loose all settings (proxy, alignment options, ...)!");
	if (res==QMessageBox::Yes)
	{
		QDir(path_canonical).removeRecursively();
	}
}

void MainWindow::on_actionSV_triggered()
{
	if(filename_ == "") return;

	if (!svs_.isValid())
	{
		QMessageBox::information(this, "SV file missing", "No structural variant file is present in the analysis folder!");
		return;
	}

	//create list of genes with heterozygous variant hits
	GeneSet het_hit_genes;
	int i_genes = variants_.annotationIndexByName("gene", true, false);
	QList<int> i_genotypes = variants_.getSampleHeader().sampleColumns(true);
	i_genotypes.removeAll(-1);

	if (i_genes!=-1 && i_genotypes.count()>0)
	{
		//check that a filter was applied (otherwise this can take forever)
		int passing_vars = filter_result_.countPassing();
		if (passing_vars>3000)
		{
			int res = QMessageBox::question(this, "Continue?", "There are " + QString::number(passing_vars) + " small variants that pass the filters.\nGenerating the list of candidate genes for compound-heterozygous hits may take very long for this amount of variants.\nDo you want to continue?", QMessageBox::Yes, QMessageBox::No);
			if(res==QMessageBox::No) return;
		}
		for (int i=0; i<variants_.count(); ++i)
		{
			if (!filter_result_.passing(i)) continue;

			bool all_genos_het = true;
			foreach(int i_genotype, i_genotypes)
			{
				if (variants_[i].annotations()[i_genotype]!="het")
				{
					all_genos_het = false;
				}
			}
			if (!all_genos_het) continue;
			het_hit_genes.insert(GeneSet::createFromText(variants_[i].annotations()[i_genes], ','));
		}
	}
	else if (variants_.type()!=SOMATIC_PAIR && variants_.type() != SOMATIC_SINGLESAMPLE)
	{
		QMessageBox::information(this, "Invalid variant list", "Column for genes or genotypes not found in variant list. Cannot apply compound-heterozygous filter based on variants!");
	}

	try
	{
		//open SV widget
		SvWidget* sv_widget;
		if(svs_.isSomatic())
		{
			sv_widget = new SvWidget(this, svs_, somatic_report_settings_.report_config, het_hit_genes);
			connect(sv_widget, SIGNAL(updateSomaticReportConfiguration()), this, SLOT(storeSomaticReportConfig()));
		}
		else //germline
		{
			//determine processed sample ID (needed for report config)
			QString ps_id = "";
			QSharedPointer<ReportConfiguration> report_config = nullptr;
			if (germlineReportSupported())
			{
				ps_id = NGSD().processedSampleId(germlineReportSample(), false);
				report_config = report_settings_.report_config;
			}

			//open SV widget
			sv_widget = new SvWidget(this, svs_, ps_id, report_config, het_hit_genes);
		}

		auto dlg = GUIHelper::createDialog(sv_widget, "Structural variants of " + variants_.analysisName());
		addModelessDialog(dlg);
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

	if (!cnvs_.isValid())
	{
		QMessageBox::information(this, "CNV file missing", "No copy-number file is present in the analysis folder!");
		return;
	}

	AnalysisType type = variants_.type();

	//create list of genes with heterozygous variant hits
	GeneSet het_hit_genes;
	int i_genes = variants_.annotationIndexByName("gene", true, false);
	QList<int> i_genotypes = variants_.getSampleHeader().sampleColumns(true);
	i_genotypes.removeAll(-1);
	if (i_genes!=-1 && i_genotypes.count()>0)
	{
		//check that a filter was applied (otherwise this can take forever)
		int passing_vars = filter_result_.countPassing();
		if (passing_vars>3000)
		{
			int res = QMessageBox::question(this, "Continue?", "There are " + QString::number(passing_vars) + " small variants that pass the filters.\nGenerating the list of candidate genes for compound-heterozygous hits may take very long for this amount of variants.\nPlease set a filter for the variant list, e.g. the recessive filter, and retry!\nDo you want to continue?", QMessageBox::Yes, QMessageBox::No);
			if(res==QMessageBox::No) return;
		}
		for (int i=0; i<variants_.count(); ++i)
		{
			if (!filter_result_.passing(i)) continue;

			bool all_genos_het = true;
			foreach(int i_genotype, i_genotypes)
			{
				if (variants_[i].annotations()[i_genotype]!="het")
				{
					all_genos_het = false;
				}
			}
			if (!all_genos_het) continue;
			het_hit_genes.insert(GeneSet::createFromText(variants_[i].annotations()[i_genes], ','));
		}
	}
	else if (type!=SOMATIC_PAIR && type!=SOMATIC_SINGLESAMPLE)
	{
		QMessageBox::information(this, "Invalid variant list", "Column for genes or genotypes not found in variant list. Cannot apply compound-heterozygous filter based on variants!");
	}

	//determine processed sample ID (needed for report config)
	QString ps_id = "";
	if (germlineReportSupported())
	{
		ps_id = NGSD().processedSampleId(germlineReportSample(), false);
	}

	QSharedPointer<ReportConfiguration> rc_germline;
	QSharedPointer<SomaticReportConfiguration> rc_somatic;
	if(cnvs_.type() == CnvListType::CLINCNV_TUMOR_NORMAL_PAIR || cnvs_.type() == CnvListType::CLINCNV_TUMOR_ONLY)
	{
		rc_somatic = somatic_report_settings_.report_config;
	}
	else
	{
		rc_germline = report_settings_.report_config;
	}
	CnvWidget* cnv_widget = new CnvWidget(this, cnvs_, ps_id, rc_germline, rc_somatic, het_hit_genes);
	connect(cnv_widget, SIGNAL(storeSomaticReportConfiguration()), this, SLOT(storeSomaticReportConfig()));

	auto dlg = GUIHelper::createDialog(cnv_widget, "Copy number variants of " + variants_.analysisName());
	addModelessDialog(dlg);

	//mosaic CNVs
	if (type==GERMLINE_SINGLESAMPLE)
	{
		FileLocation mosaic_file = GlobalServiceProvider::fileLocationProvider().getAnalysisMosaicCnvFile();
		if (mosaic_file.exists)
		{
			QStringList mosaic_data = Helper::loadTextFile(mosaic_file.filename, false, '#', true);
			if (!mosaic_data.isEmpty())
			{
				ScrollableTextDialog dlg(this, "Possible mosaic CNV(s) detected!");
				dlg.appendLine("#CHR\tSTART\tEND\tCOPY NUMBER");

				foreach (const QString& line, mosaic_data)
				{
					if(line.trimmed().isEmpty() || line.startsWith("#")) continue;

					QStringList parts = line.split("\t");
					if(parts.length()<4)
					{
						Log::warn("Mosaic CNV file '" + mosaic_file.filename + "' has line with less than 4 elements: " + line);
					}
					else
					{
						dlg.appendLine(parts.mid(0, 4).join("\t"));
					}
				}
				dlg.exec();
			}
		}
	}
}

void MainWindow::on_actionROH_triggered()
{
	if (filename_=="") return;

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE) return;

	//trio special handling: show UPD file is not empty
	if (type==GERMLINE_TRIO)
	{
		//UPDs
		FileLocation upd_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisUpdFile();
		if (!upd_loc.exists)
		{
			QMessageBox::warning(this, "UPD detection", "The UPD file is missing!\n" + upd_loc.filename);
		}
		else
		{
			QStringList upd_data = Helper::loadTextFile(upd_loc.filename, false, QChar::Null, true);
			if (upd_data.count()>1)
			{
				ScrollableTextDialog dlg(this, "UPD(s) detected!");
				QStringList headers = upd_data[0].split("\t");
				for (int r=1; r<upd_data.count(); ++r)
				{
					QStringList parts = upd_data[r].split("\t");
					QString line = parts[0] + ":" + parts[1] + "-" + parts[2];
					for(int c=3 ; c<parts.count(); ++c)
					{
						line += " " + headers[c] + "=" + parts[c];
					}
					dlg.appendLine(line);
				}
				dlg.exec();
			}
		}
	}

	//check report sample ROH file exists
	QStringList roh_files = GlobalServiceProvider::fileLocationProvider().getRohFiles(false).filterById(germlineReportSample()).asStringList();
	if (roh_files.isEmpty())
	{
		QMessageBox::warning(this, "Runs of homozygosity", "Could not find a ROH file for sample " + germlineReportSample() + ". Aborting!");
		return;
	}

	RohWidget* list = new RohWidget(this, roh_files[0]);
	auto dlg = GUIHelper::createDialog(list, "Runs of homozygosity of " + variants_.analysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionGeneSelector_triggered()
{
	if (filename_=="") return;
	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE) return;

	QString ps_name = germlineReportSample();

	//show dialog
	GeneSelectorDialog dlg(ps_name, this);
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

void MainWindow::on_actionCircos_triggered()
{
	if (filename_=="") return;

	//load plot file
	FileLocationList plot_files = GlobalServiceProvider::fileLocationProvider().getCircosPlotFiles(true);
	if (plot_files.isEmpty()) return; //this should not happen because the button is not enabled then...
	if (!plot_files[0].exists)
	{
		QMessageBox::warning(this, "Circos plot file access", "Circos plot image file does not exist or the URL has expired");
		return;
	}
	//show plot
	CircosPlotWidget* widget = new CircosPlotWidget(plot_files[0].filename);
	auto dlg = GUIHelper::createDialog(widget, "Circos Plot of " + variants_.analysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionExpressionData_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;

	QString title = "Expression data";

	NGSD db;
	QString sample_id = db.sampleId(filename_, false);
	if (sample_id=="")
	{
		QMessageBox::warning(this, title, "Error: Sample not found in NGSD!");
		return;
	}

	//get count files of all RNA processed samples corresponding to the current sample
	QStringList rna_ps_ids;
	foreach (int rna_sample, db.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
	{
		rna_ps_ids << db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample));
	}

	QStringList rna_count_files;
	foreach (QString rna_ps_id, rna_ps_ids)
	{
		FileLocation file_location = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::EXPRESSION);
		if (file_location.exists) rna_count_files << file_location.filename;
	}
	rna_count_files.removeDuplicates();

	if (rna_count_files.isEmpty())
	{
		QMessageBox::warning(this, title, "Error: No RNA count files of corresponding RNA samples found!");
		return;
	}

	//select file to open
	QString count_file;
	if (rna_count_files.size()==1)
	{
		count_file = rna_count_files.at(0);
	}
	else
	{
		bool ok;
		count_file = getFileSelectionItem(title, "Multiple RNA count files found.\nPlease select a file:", rna_count_files, &ok);
		if (!ok) return;
	}

	int rna_sys_id = db.processingSystemIdFromProcessedSample(count_file);
	QString rna_ps_id = db.processedSampleId(count_file);
	QString tissue = db.getSampleData(db.sampleId(count_file)).tissue;
	QString project = db.getProcessedSampleData(rna_ps_id).project_name;

	GeneSet variant_target_region;
	if(ui_.filters->phenotypes().count() > 0)
	{
		for (const Phenotype& phenotype : ui_.filters->phenotypes())
		{
			variant_target_region << db.phenotypeToGenes(db.phenotypeIdByAccession(phenotype.accession()), false);
		}
	}

	if(ui_.filters->targetRegion().isValid())
	{
		if (variant_target_region.isEmpty())
		{
			variant_target_region = ui_.filters->targetRegion().genes;
		}
		else
		{
			variant_target_region = ui_.filters->targetRegion().genes.intersect(variant_target_region);
		}
	}

	RnaCohortDeterminationStategy cohort_type;
	if (germlineReportSupported())
	{
		cohort_type = RNA_COHORT_GERMLINE;
	}
	else
	{
		cohort_type = RNA_COHORT_SOMATIC;
	}

	ExpressionGeneWidget* widget = new ExpressionGeneWidget(count_file, rna_sys_id, tissue, ui_.filters->genes().toString(", "), variant_target_region, project, rna_ps_id,
															cohort_type, this);
	auto dlg = GUIHelper::createDialog(widget, "Gene expression of " + db.processedSampleName(rna_ps_id) + " (DNA: " + variants_.analysisName() + ")");
	addModelessDialog(dlg);
}

void MainWindow::on_actionExonExpressionData_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;

	QString title = "Exon expression data";

	NGSD db;
	QString sample_id = db.sampleId(filename_, false);
	if (sample_id=="")
	{
		QMessageBox::warning(this, title, "Error: Sample not found in NGSD!");
		return;
	}

	//get count files of all RNA processed samples corresponding to the current sample
	QStringList rna_ps_ids;
	foreach (int rna_sample, db.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
	{
		rna_ps_ids << db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample));
	}

	QStringList rna_count_files;
	foreach (QString rna_ps_id, rna_ps_ids)
	{
		FileLocation file_location = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::EXPRESSION_EXON);
		if (file_location.exists) rna_count_files << file_location.filename;
	}
	rna_count_files.removeDuplicates();

	if (rna_count_files.isEmpty())
	{
		QMessageBox::warning(this, title, "Error: No RNA count files of corresponding RNA samples found!");
		return;
	}

	//select file to open
	QString count_file;
	if (rna_count_files.size()==1)
	{
		count_file = rna_count_files.at(0);
	}
	else
	{
		bool ok;
		count_file = getFileSelectionItem(title,"Multiple RNA count files found.\nPlease select a file:", rna_count_files, &ok);
		if (!ok) return;
	}

	int rna_sys_id = db.processingSystemIdFromProcessedSample(count_file);
	QString rna_ps_id = db.processedSampleId(count_file);
	QString tissue = db.getSampleData(db.sampleId(count_file)).tissue;
	QString project = db.getProcessedSampleData(rna_ps_id).project_name;

	GeneSet variant_target_region;
	if(ui_.filters->phenotypes().count() > 0)
	{
        for (const Phenotype& phenotype : ui_.filters->phenotypes())
		{
			variant_target_region << db.phenotypeToGenes(db.phenotypeIdByAccession(phenotype.accession()), false);
		}
	}

	if(ui_.filters->targetRegion().isValid())
	{
		if (variant_target_region.isEmpty())
		{
			variant_target_region = ui_.filters->targetRegion().genes;
		}
		else
		{
			variant_target_region = ui_.filters->targetRegion().genes.intersect(variant_target_region);
		}
	}

	RnaCohortDeterminationStategy cohort_type = RNA_COHORT_GERMLINE;
	if (somaticReportSupported()) cohort_type = RNA_COHORT_SOMATIC;

	ExpressionExonWidget* widget = new ExpressionExonWidget(count_file, rna_sys_id, tissue, ui_.filters->genes().toStringList().join(", "), variant_target_region, project, rna_ps_id, cohort_type, this);
	auto dlg = GUIHelper::createDialog(widget, "Exon expression of " + db.processedSampleName(rna_ps_id) + " (DNA: " + variants_.analysisName() + ")");
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowSplicing_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;

	NGSD db;

	//get all available files
	QStringList splicing_files;
	foreach (int rna_sample_id, db.relatedSamples(db.sampleId(variants_.mainSampleName()).toInt(), "same sample", "RNA"))
	{
		// check for required files
		foreach (const QString& rna_ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample_id)))
		{
			// search for fusion file
			FileLocation splicing_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::SPLICING_ANN);
			if (splicing_file.exists) splicing_files << splicing_file.filename;
		}
	}

	if (splicing_files.isEmpty())
	{
		QMessageBox::warning(this, "Splicing files missing", "Error: No RNA splicing files of corresponding RNA samples found!");
		return;
	}

	//select file to open
	QString splicing_filepath;
	if (splicing_files.size()==1)
	{
		splicing_filepath = splicing_files.at(0);
	}
	else
	{
		bool ok;
		splicing_filepath = getFileSelectionItem("Multiple files found", "Multiple RNA splicing files found.\nPlease select a file:", splicing_files, &ok);
		if (!ok) return;
	}

	SplicingWidget* splicing_widget = new SplicingWidget(splicing_filepath, this);

	auto dlg = GUIHelper::createDialog(splicing_widget, "Splicing Alterations of " + variants_.analysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowRnaFusions_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;

	NGSD db;

	//get all available files
	QStringList arriba_fusion_files;
	foreach (int rna_sample_id, db.relatedSamples(db.sampleId(variants_.mainSampleName()).toInt(), "same sample", "RNA"))
	{
		// check for required files
		foreach (const QString& rna_ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample_id)))
		{
			// search for fusion file
			FileLocation fusion_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::FUSIONS);
			if (fusion_file.exists) arriba_fusion_files << fusion_file.filename;
		}
	}

	if (arriba_fusion_files.isEmpty())
	{
		QMessageBox::warning(this, "Fusion files missing", "Error: No RNA fusion files of corresponding RNA samples found!");
		return;
	}

	//select file to open
	QString fusion_filepath;
	if (arriba_fusion_files.size()==1)
	{
		fusion_filepath = arriba_fusion_files.at(0);
	}
	else
	{
		bool ok;
		fusion_filepath = getFileSelectionItem("Multiple files found", "Multiple RNA fusion files found.\nPlease select a file:", arriba_fusion_files, &ok);
		if (!ok) return;
	}

	FusionWidget* fusion_widget = new FusionWidget(fusion_filepath, this);

	auto dlg = GUIHelper::createDialog(fusion_widget, "Fusions of " + variants_.analysisName() + " (arriba)");
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowProcessingSystemCoverage_triggered()
{
	//set filter widget
	FilterWidget* variant_filter_widget = nullptr;
	if(filename_ != "") variant_filter_widget = ui_.filters;

	auto expression_level_widget = new ExpressionOverviewWidget(variant_filter_widget, this);

	auto dlg = GUIHelper::createDialog(expression_level_widget, "Expression of processing systems");
	addModelessDialog(dlg);
}

void MainWindow::on_actionMethylation_triggered()
{
	if (filename_=="") return;
	if (variants_.type()!=GERMLINE_SINGLESAMPLE) return;
	FileLocation methylation_file = GlobalServiceProvider::fileLocationProvider().getMethylationFile();
	if (!methylation_file.exists)
	{
		//this should not happen because the button is not enabled then...
		QMessageBox::warning(this, "Methylation file access", "Methylation file does not exist or the URL has expired");
		return;
	}

	MethylationWidget* widget = new MethylationWidget(methylation_file.filename, this);
	auto dlg = GUIHelper::createDialog(widget, "Methylation of " + variants_.analysisName());

	addModelessDialog(dlg, true);
}

void MainWindow::on_actionRE_triggered()
{
	if (filename_=="") return;
	if (variants_.type()!=GERMLINE_SINGLESAMPLE) return;

	//show dialog
	QString sys_name = "";
	if (LoginManager::active())
	{
		NGSD db;
		QString ps_id = db.processedSampleId(germline_report_ps_);
		sys_name = db.getProcessedSampleData(ps_id).processing_system;
	}
	RepeatExpansionWidget* widget = new RepeatExpansionWidget(this, res_, report_settings_.report_config, sys_name);
	auto dlg = GUIHelper::createDialog(widget, "Repeat expansions of " + variants_.analysisName());

	addModelessDialog(dlg, true);
}

void MainWindow::on_actionPRS_triggered()
{
	if (filename_=="") return;

	// determine PRS file name
	FileLocationList prs_files = GlobalServiceProvider::fileLocationProvider().getPrsFiles(false);
	if (prs_files.isEmpty()) return; //this should not happen because the button is not enabled then...

	//show dialog
	PRSWidget* widget = new PRSWidget(prs_files[0].filename);
	auto dlg = GUIHelper::createDialog(widget, "Polygenic Risk Scores of " + variants_.analysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionPathogenicWT_triggered()
{
	if (filename_=="") return;
	if (!germlineReportSupported(false)) return;

	//determine BAM file
	QString ps = germlineReportSample();
	FileLocationList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(ps);
	if (bams.isEmpty())
	{
		QMessageBox::warning(this, "Pathogenic WT variants", "Could not find a BAM file for sample " + ps + ". Aborting!");
		return;
	}

	//check if long-read
	bool is_longread = (LoginManager::active() ? NGSD().isLongRead(filename_) : true);

	//show dialog
	PathogenicWtDialog dlg(this, bams[0].filename, is_longread);
	dlg.exec();
}

void MainWindow::on_actionDesignCfDNAPanel_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;
	if (!(somaticReportSupported()||tumoronlyReportSupported())) return;

	// Workaround to manual add panels for non patient-specific processing systems
	DBTable cfdna_processing_systems = NGSD().createTable("processing_system", "SELECT id, name_short FROM processing_system WHERE type='cfDNA (patient-specific)' OR type='cfDNA'");

	QSharedPointer<CfDNAPanelDesignDialog> dialog(new CfDNAPanelDesignDialog(variants_, filter_result_, somatic_report_settings_.report_config, variants_.mainSampleName(), cfdna_processing_systems, this));
	dialog->setWindowFlags(Qt::Window);

	addModelessDialog(dialog);
}

void MainWindow::on_actionShowCfDNAPanel_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;
	if (!(somaticReportSupported()||tumoronlyReportSupported())) return;

	NGSD db;
// get cfDNA panels:
	QList<CfdnaPanelInfo> cfdna_panels = db.cfdnaPanelInfo(db.processedSampleId(variants_.mainSampleName()));
	CfdnaPanelInfo selected_panel;
	if (cfdna_panels.size() < 1)	{
		// show message
		GUIHelper::showMessage("No cfDNA panel found!", "No cfDNA panel was found for the given tumor sample!");
		return;
	}
	else if (cfdna_panels.size() > 1)
	{
		QStringList cfdna_panel_description;
		foreach (const CfdnaPanelInfo& panel, cfdna_panels)
		{
			cfdna_panel_description.append("cfDNA panel for " + db.getProcessingSystemData(panel.processing_system_id).name  + " (" + panel.created_date.toString("dd.MM.yyyy") + " by "
										   + db.userName(panel.created_by) + ")");
		}

		QComboBox* cfdna_panel_selector = new QComboBox(this);
		cfdna_panel_selector->addItems(cfdna_panel_description);

		// create dlg
		auto dlg = GUIHelper::createDialog(cfdna_panel_selector, "Select cfDNA panel", "", true);
		int btn = dlg->exec();
		if (btn!=1)
		{
			return;
		}
		selected_panel = cfdna_panels.at(cfdna_panel_selector->currentIndex());
	}
	else
	{
		selected_panel = cfdna_panels.at(0);
	}

	//show dialog
	CfDNAPanelWidget* widget = new CfDNAPanelWidget(selected_panel);
	auto dlg = GUIHelper::createDialog(widget, "cfDNA panel for tumor " + variants_.analysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionCfDNADiseaseCourse_triggered()
{
	if (filename_=="") return;
	if (!somaticReportSupported()) return;

	DiseaseCourseWidget* widget = new DiseaseCourseWidget(variants_.mainSampleName());
	auto dlg = GUIHelper::createDialog(widget, "Personalized cfDNA variants");
	addModelessDialog(dlg);
}

void MainWindow::on_actionCfDNAAddExcludedRegions_triggered()
{
	if (filename_=="") return;
	if (!LoginManager::active()) return;
	if (!somaticReportSupported()) return;

	QSharedPointer<cfDNARemovedRegions> dialog(new cfDNARemovedRegions(variants_.mainSampleName(), this));
	dialog->setWindowFlags(Qt::Window);

	addModelessDialog(dialog);
}

void MainWindow::on_actionGeneOmimInfo_triggered()
{
	GeneOmimInfoWidget* widget = new GeneOmimInfoWidget(this);
	auto dlg = GUIHelper::createDialog(widget, "OMIM information for genes");
	dlg->exec();
}

void MainWindow::openVariantListFolder()
{
	if (filename_=="") return;

	try
	{
		QString sample_folder;

		if (GlobalServiceProvider::fileLocationProvider().isLocal())
		{
			sample_folder = QFileInfo(filename_).absolutePath();
		}
		else //client-server (allow opening folders if GSvar project paths are configured)
		{
			NGSD db;
			if (variants_.type()==AnalysisType::GERMLINE_SINGLESAMPLE)
			{
				QString ps = variants_.mainSampleName();
				QString ps_id = db.processedSampleId(ps);
				QString project_type = db.getProcessedSampleData(ps_id).project_type;
				QString project_folder = db.projectFolder(project_type).trimmed();
				if (!project_folder.isEmpty())
				{
					sample_folder = db.processedSamplePath(ps_id, PathType::SAMPLE_FOLDER);
					if (!QDir(sample_folder).exists()) THROW(Exception, "Sample folder does not exist: " + sample_folder);
				}
			}
			else
			{
				THROW(Exception, "In client-server mode, opening analysis folders is only supported for germline single sample!");
			}
		}

		QDesktopServices::openUrl(sample_folder);
	}
	catch(Exception& e)
	{
		QMessageBox::information(this, "Open analysis folder", "Could not open analysis folder:\n" + e.message());
	}
}

void MainWindow::openVariantListQcFiles()
{
	if (filename_=="") return;

	const FileLocationProvider& flp = GlobalServiceProvider::fileLocationProvider();

	foreach(const FileLocation& file, flp.getQcFiles())
	{
		if (flp.isLocal())
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(file.filename));
		}
		else
		{
			//create a local copy of the qcML file
			VersatileFile in_file(file.filename);
			in_file.open();
			QByteArray file_contents = in_file.readAll();

			QString tmp_filename = GSvarHelper::localQcFolder() + file.fileName();
			QSharedPointer<QFile> tmp_file = Helper::openFileForWriting(tmp_filename);
			tmp_file->write(file_contents);
			tmp_file->close();

			QDesktopServices::openUrl(QUrl::fromLocalFile(tmp_filename));
		}
	}
}

void MainWindow::on_actionReanalyze_triggered()
{
	if (filename_=="") return;

	AnalysisType type = variants_.type();
	SampleHeaderInfo header_info = variants_.getSampleHeader();
	QList<AnalysisJobSample> samples;
	if (type==GERMLINE_SINGLESAMPLE  || type==CFDNA || type==SOMATIC_SINGLESAMPLE)
	{
		samples << AnalysisJobSample {header_info[0].name, ""};
	}
	else if (type==GERMLINE_MULTISAMPLE)
	{
		foreach(const SampleInfo& info, header_info)
		{
			samples << AnalysisJobSample {info.name, info.isAffected() ? "affected" : "control"};
		}
	}
	else if (type==GERMLINE_TRIO)
	{
		foreach(const SampleInfo& info, header_info)
		{
			if(info.isAffected())
			{
				samples << AnalysisJobSample {info.name, "child"};
			}
			else
			{
				samples << AnalysisJobSample {info.name, info.gender()=="male" ? "father" : "mother"};
			}
		}
	}
	else if (type==SOMATIC_PAIR)
	{
		foreach(const SampleInfo& info, header_info)
		{
			samples << AnalysisJobSample {info.name, info.isTumor() ? "tumor" : "normal"};
		}
	}

	GSvarHelper::queueSampleAnalysis(type, samples, this);
}

void MainWindow::delayedInitialization()
{
	//initialize LOG file
	Log::setFileEnabled(true);
	Log::appInfo();

	//check that INI file is configured
	QStringList keys_missing;
	foreach(QString key, QStringList() << "build" << "reference_genome" << "igv_app" << "igv_genome" << "threads")
	{
	   if (!Settings::contains(key)) keys_missing << key;
	}
	if (!keys_missing.isEmpty())
	{
		QMessageBox::warning(this, "GSvar setup error", "The GSvar INI file is not set up correctly.\nThe following keys are missing or contain no value: " + keys_missing.join(", ") + "\nPlease inform your administrator!");
		close();
		return;
	}

	//close the app if the server is not available
	if (ClientHelper::isClientServerMode())
	{
		if (!isServerRunning())
		{
			close();
			return;
		}
	}

    if (NGSD::isAvailable())
    {
        //user login for database
        LoginDialog dlg(this);
        dlg.exec();

        if (LoginManager::active())
        {
            try
            {
                ui_.filters->loadTargetRegions();
            }
            catch(Exception& e)
            {
                Log::warn("Target region data for filter widget could not be loaded from NGSD: " + e.message());
            }
        }

        //start initialization of NGSD gene/transcript cache
        NGSDCacheInitializer* ngsd_initializer = new NGSDCacheInitializer();
        startJob(ngsd_initializer, false);
    }

	//create default IGV session (variants)
	IGVSession* igv_default = IgvSessionManager::create(this, "Default IGV", Settings::path("igv_app").trimmed(), Settings::string("igv_host"), Settings::path("igv_genome"));
	connect(igv_default, SIGNAL(started()), this, SLOT(changeIgvIconToActive()));
	connect(igv_default, SIGNAL(finished()), this, SLOT(changeIgvIconToNormal()));

	//create IGV session for virus detection
	QString virus_genome;
	if (!ClientHelper::isClientServerMode())
	{
		virus_genome = Settings::string("igv_virus_genome", true);
		if (virus_genome.isEmpty()) QMessageBox::information(this, "Virus genome not set", "Virus genome path is missing from the settings!");
	}
	else
	{
	   virus_genome = ClientHelper::serverApiUrl() + "genome/somatic_viral.fa";
	}
	IGVSession* igv_virus = IgvSessionManager::create(this, "Virus IGV", Settings::path("igv_app").trimmed(), Settings::string("igv_host"), virus_genome);
	connect(igv_virus, SIGNAL(started()), this, SLOT(changeIgvIconToActive()));
	connect(igv_virus, SIGNAL(finished()), this, SLOT(changeIgvIconToNormal()));

	//init GUI
	updateRecentSampleMenu();
	updateIGVMenu();
	updateNGSDSupport();
	registerCustomContextMenuActions();

	//parse arguments
	for (int i=1; i<QApplication::arguments().count(); ++i)
	{
		QString arg = QApplication::arguments().at(i);

		if (arg.startsWith("filter:")) //filter (by name)
		{
			int sep_pos = arg.indexOf(':');
			QString filter_name = arg.mid(sep_pos+1).trimmed();

			if (!ui_.filters->setFilter(filter_name))
			{
				qDebug() << "Filter name " << arg << " not found. Ignoring it!";
			}
		}
		else if (arg.startsWith("roi:")) //target region (by name)
		{
			int sep_pos = arg.indexOf(':');
			QString roi_name = arg.mid(sep_pos+1).trimmed();

			if (!ui_.filters->setTargetRegionByDisplayName(roi_name))
			{
				qDebug() << "Target region name " << roi_name << " not found. Ignoring it!";
			}
		}
		else if (i==1) //first argument: sample to open
		{
			if (QFile::exists(arg)) //file path
			{
				loadFile(arg);
			}
			else if (LoginManager::active()) //processed sample name (via NGSD)
			{
				NGSD db;
				if (db.processedSampleId(arg, false)!="")
				{
					openProcessedSampleFromNGSD(arg, false);
				}
				else if (db.sampleId(arg, false)!="")
				{
					openSampleFromNGSD(arg);
				}
            }
		}
		else
		{
			qDebug() << "Unprocessed argument: " << arg;
		}
	}
}

void MainWindow::variantCellDoubleClicked(int row, int /*col*/)
{
    const Variant& v = variants_[ui_.vars->rowToVariantIndex(row)];
    IgvSessionManager::get(0).gotoInIGV(v.chr().str() + ":" + QString::number(v.start()) + "-" + QString::number(v.end()), true);
}

void MainWindow::variantHeaderDoubleClicked(int row)
{
	if (!LoginManager::active()) return;

	int var_index = ui_.vars->rowToVariantIndex(row);
	editVariantReportConfiguration(var_index);
}

void MainWindow::openCustomIgvTrack()
{
	QAction* action = qobject_cast<QAction*>(sender());
	if (action==nullptr) return;

    IgvSessionManager::get(0).loadFileInIGV(action->toolTip(), false);
}

void MainWindow::editVariantValidation(int index)
{
	Variant& variant = variants_[index];

	try
	{
		QString ps = selectProcessedSample();
		if (ps.isEmpty()) return;

		NGSD db;

		//get variant ID - add if missing
		QString variant_id = db.variantId(variant, false);
		if (variant_id=="")
		{
			variant_id = db.addVariant(variant, variants_);
		}

		//get sample ID
		QString sample_id = db.sampleId(ps);

		//get variant validation ID - add if missing
		QVariant val_id = db.getValue("SELECT id FROM variant_validation WHERE variant_id='" + variant_id + "' AND sample_id='" + sample_id + "'", true);
		bool added_validation_entry = false;
		if (!val_id.isValid())
		{
			//get genotype
			QByteArray genotype = "het";
			int i_genotype = variants_.getSampleHeader().infoByID(ps).column_index;
			if (i_genotype!=-1) //genotype column only available in germline, but not for somatic analysis.
			{
				genotype = variant.annotations()[i_genotype];
			}

			//insert
			SqlQuery query = db.getQuery();
			query.exec("INSERT INTO variant_validation (user_id, sample_id, variant_type, variant_id, genotype, status) VALUES ('" + LoginManager::userIdAsString() + "','" + sample_id + "','SNV_INDEL','" + variant_id + "','" + genotype + "','n/a')");
			val_id = query.lastInsertId();

			added_validation_entry = true;
		}

		ValidationDialog dlg(this, val_id.toInt());

		if (dlg.exec())
		{
			//update DB
			dlg.store();

			//update variant table
			QByteArray status = dlg.status().toUtf8();
			if (status=="true positive") status = "TP";
			if (status=="false positive") status = "FP";
			int i_validation = variants_.annotationIndexByName("validation", true, true);
			variant.annotations()[i_validation] = status;

			//update details widget and filtering
			ui_.variant_details->updateVariant(variants_, index);
			refreshVariantTable(true, true);

			//mark variant list as changed
			markVariantListChanged(variant, "validation", status);
		}
		else if (added_validation_entry)
		{
			// remove created but empty validation if ValidationDialog is aborted
			SqlQuery query = db.getQuery();
			query.exec("DELETE FROM variant_validation WHERE id=" + val_id.toString());
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
	}
}

void MainWindow::editVariantComment(int index)
{
	Variant& variant = variants_[index];

	try
	{
		//add variant if missing
		NGSD db;
		if (db.variantId(variant, false)=="")
		{
			db.addVariant(variant, variants_);
		}

		bool ok = true;
		QByteArray text = QInputDialog::getMultiLineText(this, "Variant comment", "Text: ", db.comment(variant), &ok).toUtf8();

		if (ok)
		{
			//update DB
			db.setComment(variant, text);

			//update datastructure (if comment column is present)
			int col_index = variants_.annotationIndexByName("comment", true, false);
			if (col_index!=-1)
			{
				variant.annotations()[col_index] = text;
				refreshVariantTable();

				//mark variant list as changed
				markVariantListChanged(variant, "comment", text);
			}
		}
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
	}
}

void MainWindow::showAfHistogram_all()
{
	showAfHistogram(false);
}

void MainWindow::showAfHistogram_filtered()
{
	showAfHistogram(true);
}

void MainWindow::showCnHistogram()
{
	if (filename_=="") return;

	QString title = "Copy-number histogram";

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE)
	{
		QMessageBox::information(this, title, "This functionality is only available for germline single sample and germline trio analysis.");
		return;
	}

	//check report sample SEG file exists
	QStringList seg_files = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(false).filterById(germlineReportSample()).asStringList();
	if (seg_files.isEmpty())
	{
		QMessageBox::warning(this, title, "Could not find a SEG file for sample " + germlineReportSample() + ". Aborting!");
		return;
	}

	try
	{
		//get region
		Chromosome chr;
		int start, end;
		QString region_text = QInputDialog::getText(this, title, "genomic region");
		if (region_text=="") return;

		NGSHelper::parseRegion(region_text, chr, start, end, true);

		//determine CN values
		QVector<double> cn_values;
		VersatileTextStream stream(seg_files[0]);
		while (!stream.atEnd())
		{
			QString line = stream.readLine();
			QStringList parts = line.split("\t");
			if (parts.count()<6) continue;

			//check if range overlaps input interval
			Chromosome chr2(parts[1]);
			if (chr!=chr2) continue;

			int start2 = Helper::toInt(parts[2], "Start coordinate");
			int end2 = Helper::toInt(parts[3], "End coordinate");
			if (!BasicStatistics::rangeOverlaps(start, end, start2, end2)) continue;

			//skip invalid copy-numbers
			QString cn_str = parts[5];
			if (cn_str.toLower()=="nan") continue;
			double cn = Helper::toDouble(cn_str, "Copy-number");
			if (cn<0) continue;

			cn_values << cn;
		}

		//create histogram
		std::sort(cn_values.begin(), cn_values.end());
		double median = BasicStatistics::median(cn_values,false);
		double max = ceil(median*2+0.0001);
		Histogram hist(0.0, max, max/40);
		foreach(double cn, cn_values)
		{
			hist.inc(cn, true);
		}

		//show chart
		QChartView* view = GUIHelper::histogramChart(hist, "Copy-number");
		auto dlg = GUIHelper::createDialog(view, QString("Copy-number in region ") + region_text);
		dlg->exec();
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, title, "Error:\n" + e.message());
		return;
	}
}

void MainWindow::showBafHistogram()
{
	if (filename_=="") return;

	QString title = "BAF histogram";

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE)
	{
		QMessageBox::information(this, title, "This functionality is only available for germline single sample and germline trio analysis.");
		return;
	}

	//check report sample SEG file exists
	QStringList baf_files = GlobalServiceProvider::fileLocationProvider().getBafFiles(false).filterById(germlineReportSample()).asStringList();
	if (baf_files.isEmpty())
	{
		QMessageBox::warning(this, title, "Could not find a BAF file for sample " + germlineReportSample() + ". Aborting!");
		return;
	}

	try
	{
		//get region
		Chromosome chr;
		int start, end;
		QString region_text = QInputDialog::getText(this, title, "genomic region");
		if (region_text=="") return;

		NGSHelper::parseRegion(region_text, chr, start, end, true);

		//determine CN values
		Histogram hist(0.0, 1.0, 0.025);
		VersatileFile file(baf_files[0], false);
		file.open(QFile::ReadOnly | QIODevice::Text);
		while (!file.atEnd())
        {
			QString line = file.readLine();
			QStringList parts = line.split("\t");
			if (parts.count()<5) continue;

			//check if range overlaps input interval
			Chromosome chr2(parts[0]);
			if (chr!=chr2) continue;

			int start2 = Helper::toInt(parts[1], "Start coordinate");
			int end2 = Helper::toInt(parts[2], "End coordinate");
			if (!BasicStatistics::rangeOverlaps(start, end, start2, end2)) continue;

			double baf =  Helper::toDouble(parts[4], "BAF");
			hist.inc(baf, true);
		}

		//show chart
		QChartView* view = GUIHelper::histogramChart(hist, "BAF");
		auto dlg = GUIHelper::createDialog(view, QString("BAF in region ") + region_text);
		dlg->exec();
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, title, "Error:\n" + e.message());
		return;
	}
}

void MainWindow::showAfHistogram(bool filtered)
{
	if (filename_=="") return;

	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO)
	{
		QMessageBox::information(this, "Allele frequency histogram", "This functionality is only available for germline single sample and germline trio analysis.");
		return;
	}

	//create histogram
	Histogram hist(0.0, 1.0, 0.05);
	int col_quality = variants_.annotationIndexByName("quality");
	for (int i=0; i<variants_.count(); ++i)
	{
		if (filtered && !filter_result_.passing(i)) continue;

		QByteArrayList parts = variants_[i].annotations()[col_quality].split(';');
		foreach(const QByteArray& part, parts)
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

	//show chart
	QChartView* view = GUIHelper::histogramChart(hist, "Allele frequency");
	auto dlg = GUIHelper::createDialog(view, QString("Allele frequency histogram ") + (filtered ? "(after filter)" : "(all variants)"));
	dlg->exec();
}

void MainWindow::on_actionEncrypt_triggered()
{
	//get input
	QString input = QInputDialog::getText(this, "Text for encryption", "text");
	if (input.isEmpty()) return;

	//decrypt
	QStringList out_lines;
	out_lines << ("Input text: " + input);
	try
	{
		qulonglong crypt_key = ToolBase::encryptionKey("encryption helper");
		out_lines << ("Encrypted text: " + SimpleCrypt(crypt_key).encryptToString(input));
	}
	catch(Exception& e)
	{
		out_lines << ("Error: " + e.message());
	}

	//show output
	QTextEdit* edit = new QTextEdit(this);
	edit->setText(out_lines.join("\n"));
	auto dlg = GUIHelper::createDialog(edit, "Encryption output");
	dlg->exec();
}

void MainWindow::on_actionSettings_triggered()
{
	openSettingsDialog();
}

void MainWindow::openSettingsDialog(QString page_name, QString section)
{
	SettingsDialog dlg(this);
	dlg.setWindowFlags(Qt::Window);
	dlg.gotoPage(page_name, section);
	if (dlg.exec()==QDialog::Accepted)
	{
		dlg.storeSettings();
		setStyle(Settings::string("window_style", true));
	}
}

void MainWindow::on_actionSampleSearch_triggered()
{
	TabType type = TabType::SAMPLE_SEARCH;
	QString name = "Sample search";
	if (focusTab(type, name)) return;

	SampleSearchWidget* widget = new SampleSearchWidget(this);
	openTab(QIcon(":/Icons/NGSD_sample_search.png"), name, type, widget);
}

void MainWindow::on_actionRunOverview_triggered()
{
	TabType type = TabType::RUN;
	QString name = "Sequencing run overview";
	if (focusTab(type, name)) return;

	SequencingRunOverview* widget = new SequencingRunOverview(this);
	openTab(QIcon(":/Icons/NGSD_run_overview.png"), name, type, widget);
}

void MainWindow::openColumnSettings()
{
	openSettingsDialog("columns", variantTypeToString(VariantType::SNVS_INDELS));
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

	connect(dlg.data(), SIGNAL(finished(int)), this, SLOT(deleteClosedModelessDialogs()));

	modeless_dialogs_.append(dlg);
}

void MainWindow::deleteClosedModelessDialogs()
{
	//Clean up when we add another dialog. Like that, only one dialog can be closed and not destroyed at the same time.
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
	if (filename_=="") return;

	try
	{
		QString ps_name = germlineReportSupported() ? germlineReportSample() : variants_.mainSampleName();
		NGSD db;
		QString sample_id = db.sampleId(ps_name);
		PhenotypeList phenotypes = db.getSampleData(sample_id).phenotypes;

		ui_.filters->setPhenotypes(phenotypes);
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, "Error loading phenotype data from NGSD");
	}
}

void MainWindow::createSubPanelFromPhenotypeFilter()
{
	//convert phenotypes to genes
	QApplication::setOverrideCursor(Qt::BusyCursor);
	NGSD db;
	GeneSet genes;
    for (const Phenotype& pheno : ui_.filters->phenotypes())
	{
		genes << db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);
	}
	QApplication::restoreOverrideCursor();

	//open dialog
	openSubpanelDesignDialog(genes);
}

void MainWindow::on_actionOpen_triggered()
{
	//get file to open
	QString path = Settings::path("path_variantlists", true);
	QString filename = QFileDialog::getOpenFileName(this, "Open variant list", path, "GSvar files (*.GSvar);;All files (*.*)");
	if (filename=="") return;

	//update data
	loadFile(filename);
}

void MainWindow::on_actionOpenByName_triggered()
{
	ProcessedSampleSelector dlg(this, false);
	dlg.showSearchMulti(true);
	if (!dlg.exec()) return;

	QString ps_name = dlg.processedSampleName();
	if (ps_name.isEmpty()) return;
	openProcessedSampleFromNGSD(ps_name, dlg.searchMulti());
}

void MainWindow::openProcessedSampleFromNGSD(QString processed_sample_name, bool search_multi)
{
	checkClientUpdates();
	try
	{
		NGSD db;
		QString processed_sample_id = db.processedSampleId(processed_sample_name);

		//check user can access
		if (!db.userCanAccess(LoginManager::userId(), processed_sample_id.toInt()))
		{
			INFO(AccessDeniedException, "You do not have permissions to open sample '" + processed_sample_name + "'!");
		}

		//processed sample exists > add to recent samples menu
		addToRecentSamples(processed_sample_name);

		//germline single sample analysis
		QStringList analyses;
		FileLocation file_location = GlobalServiceProvider::database().processedSamplePath(processed_sample_id, PathType::GSVAR);
		if (file_location.exists) analyses << file_location.filename;

		//somatic tumor sample > ask user if he wants to open the tumor-normal pair
		//check for tumor-normal analyses
		QString normal_sample = db.normalSample(processed_sample_id);
		if (normal_sample!="")
		{
			analyses << GlobalServiceProvider::database().secondaryAnalyses(processed_sample_name + "-" + normal_sample, "somatic");
		}
		//check for tumor-only analyses
		QStringList tumor_only_analyses = GlobalServiceProvider::database().secondaryAnalyses(processed_sample_name, "somatic");
		if (!tumor_only_analyses.isEmpty())
		{
			analyses << tumor_only_analyses;
		}

		//check for germline trio/multi analyses
		if (search_multi)
		{
			analyses << GlobalServiceProvider::database().secondaryAnalyses(processed_sample_name, "trio");
			analyses << GlobalServiceProvider::database().secondaryAnalyses(processed_sample_name, "multi sample");
		}

		//determine analysis to load
		QString file;
		if (analyses.count()==0)
		{
			INFO(ArgumentException, "The GSvar file does not exist:\n" + file_location.filename);
		}
		else if (analyses.count()==1)
		{
			file = analyses[0];
		}
		else //several analyses > let the user decide
		{
			//create list of anaylsis names
			QList<MultiSampleAnalysisInfo> analysis_info_list = GlobalServiceProvider::database().getMultiSampleAnalysisInfo(analyses);
			QStringList names;
			foreach(MultiSampleAnalysisInfo info, analysis_info_list)
			{
				names.append(info.analysis_name);
			}

			//show selection dialog (analysis name instead of file name)
			bool ok = false;
			QString name = QInputDialog::getItem(this, "Several analyses of the sample present", "select analysis:", names, 0, false, &ok);
			if (!ok) return;

			int index = names.indexOf(name);
			foreach(QString ps_id, analysis_info_list[index].ps_sample_id_list)
			{
				if (!db.userCanAccess(LoginManager::userId(), ps_id.toInt()))
				{
					INFO(AccessDeniedException, "You do not have permissions to open all the samples from the selected multi-sample analysis!");
				}
			}
			file = analysis_info_list[index].analysis_file;
		}

        loadFile(file);
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Error opening processed sample by name");
	}
}

void MainWindow::openSampleFromNGSD(QString sample_name)
{
	try
	{
		NGSD db;
		QStringList processed_samples = db.getValues("SELECT CONCAT(s.name,'_',LPAD(ps.process_id,2,'0')) FROM processed_sample ps, sample s WHERE ps.sample_id=s.id AND ps.quality!='bad' AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples) AND s.name=:0", sample_name);
		if (processed_samples.isEmpty())
		{
			THROW(ArgumentException, "No high-quality processed sample found for sample name '" + sample_name + "'");
		}

		if (processed_samples.count()==1)
		{
			openProcessedSampleFromNGSD(processed_samples[0], false);
		}
		else
		{
			bool ok = false;
			QString ps = QInputDialog::getItem(this, "Several processed samples found for sample '" + sample_name + "'", "select processed sample:", processed_samples, 0, false, &ok);
			if (!ok) return;

			openProcessedSampleFromNGSD(ps, false);
        }
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Error opening sample from NGSD", e.message());
	}
}

void MainWindow::checkMendelianErrorRate()
{
	QString output = "";
	try
	{
		SampleHeaderInfo infos = variants_.getSampleHeader();
		int i_c = infos.infoByStatus(true).column_index;
		int i_f = infos.infoByStatus(false, "male").column_index;
		int i_m = infos.infoByStatus(false, "female").column_index;

		int i_qual = variants_.annotationIndexByName("quality");

		int errors = 0;
		int used = 0;
		for (int i=0; i<variants_.count(); ++i)
		{
			const Variant& v = variants_[i];
			if (!v.chr().isAutosome()) continue;

			//remove no genotyping
			QString geno_c = v.annotations()[i_c];
			QString geno_f = v.annotations()[i_f];
			QString geno_m = v.annotations()[i_m];
			if (geno_c=="n/a" || geno_f=="n/a" || geno_m=="n/a") continue;

			//remove filter entry
			if (!v.filters().isEmpty()) continue;

			//remove low depth
			bool low_depth = false;
			QByteArrayList entries = v.annotations()[i_qual].split(';');
			foreach(const QByteArray& entry, entries)
			{
				if (!entry.startsWith("DP=")) continue;
				foreach(const QByteArray& value, entry.mid(3).split(','))
				{
					if (value.toInt()<20) low_depth = true;
				}
			}
			if (low_depth) continue;

			++used;

			if ((geno_c=="wt" && (geno_f=="hom" || geno_m=="hom")) ||
				(geno_c=="hom" && (geno_f=="wt" || geno_m=="wt")) ||
				(geno_c!="hom" && (geno_f=="hom" && geno_m=="hom")) ||
				(geno_c!="wt" && (geno_f=="wt" && geno_m=="wt")))
			{
				++errors;
				//qDebug() << v.toString() << geno_c << geno_f << geno_m << entries.join(" ");
			}
		}

		double percentage = 100.0 * errors / used;

		qDebug() << used << errors << percentage;
		if (percentage>10)
		{
			output = "Mendelian error rate too high:\n" + QString::number(errors) + "/" + QString::number(used) + " ~ " + QString::number(percentage, 'f', 2) + "%";
		}
	}
	catch (Exception& e)
	{
		output = "Mendelian error rate calulation failed:\n" + e.message();
	}

	if (!output.isEmpty())
	{
		QMessageBox::warning(this, "Medelian error rate", output);
	}
}

void MainWindow::openProcessedSampleTab(QString ps_name)
{
	TabType type = TabType::SAMPLE;
	if (focusTab(type, ps_name)) return;

	try
	{
		QString ps_id = NGSD().processedSampleId(ps_name);

		ProcessedSampleWidget* widget = new ProcessedSampleWidget(this, ps_id);
		connect(widget, SIGNAL(clearMainTableSomReport(QString)), this, SLOT(clearSomaticReportSettings(QString)));
		connect(widget, SIGNAL(addModelessDialog(QSharedPointer<QDialog>, bool)), this, SLOT(addModelessDialog(QSharedPointer<QDialog>, bool)));
		int index = openTab(QIcon(":/Icons/NGSD_sample.png"), ps_name, type, widget);
		ui_.tabs->tabBar()->setTabData(index, ps_id);
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "Open processed sample tab");
	}
}

void MainWindow::openRunTab(QString run_name)
{
	TabType type = TabType::RUN;
	if (focusTab(type, run_name)) return;

	QString run_id;
	try
	{
		run_id = NGSD().getValue("SELECT id FROM sequencing_run WHERE name=:0", true, run_name).toString();
	}
	catch (DatabaseException e)
	{
		GUIHelper::showMessage("NGSD error", "The run database ID could not be determined for '"  + run_name + "'!\nError message: " + e.message());
		return;
	}

	SequencingRunWidget* widget = new SequencingRunWidget(this, QStringList() << run_id);
	connect(widget, SIGNAL(addModelessDialog(QSharedPointer<QDialog>, bool)), this, SLOT(addModelessDialog(QSharedPointer<QDialog>, bool)));
	int index = openTab(QIcon(":/Icons/NGSD_run.png"), run_name, type, widget);
	ui_.tabs->tabBar()->setTabData(index, run_id);
}

void MainWindow::openRunBatchTab(const QStringList& run_names)
{
	TabType type = TabType::RUN;
	if (focusTab(type, run_names.join(", "))) return;

	QStringList run_ids;
	foreach (const QString& run_name, run_names)
	{
		try
		{
			run_ids << NGSD().getValue("SELECT id FROM sequencing_run WHERE name=:0", true, run_name).toString();
		}
		catch (DatabaseException e)
		{
			GUIHelper::showMessage("NGSD error", "The run database ID could not be determined for '"  + run_name + "'!\nError message: " + e.message());
			return;
		}

	}

	SequencingRunWidget* widget = new SequencingRunWidget(this, run_ids);
	connect(widget, SIGNAL(addModelessDialog(QSharedPointer<QDialog>, bool)), this, SLOT(addModelessDialog(QSharedPointer<QDialog>, bool)));
	int index = openTab(QIcon(":/Icons/NGSD_run.png"), run_names.join(", "), type, widget);
	if (Settings::boolean("debug_mode_enabled"))
	{
		ui_.tabs->setTabToolTip(index, "NGSD ID: " + run_ids.join(", "));
	}
}

void MainWindow::openGeneTab(QString symbol)
{
	TabType type = TabType::GENE;
	if (focusTab(type, symbol)) return;

	QPair<QString, QString> approved = NGSD().geneToApprovedWithMessage(symbol);
	if (approved.second.startsWith("ERROR:"))
	{
		GUIHelper::showMessage("NGSD error", "Gene name '" + symbol + "' is not a HGNC-approved name!\nError message:\n" + approved.second);
		return;
	}

	GeneWidget* widget = new GeneWidget(this, symbol.toUtf8());
	int index = openTab(QIcon(":/Icons/NGSD_gene.png"), symbol, type, widget);
	ui_.tabs->tabBar()->setTabData(index, QString::number(NGSD().geneId(symbol.toUtf8())));
}

void MainWindow::openVariantTab(Variant variant)
{
	try
	{
		//check variant is in NGSD
		NGSD db;
		QString v_id = db.variantId(variant);

		TabType type = TabType::VARIANT;
        QString name = variant.toString(QChar(), -1, true);
		if (focusTab(type, name)) return;

		//open tab
		VariantWidget* widget = new VariantWidget(variant, this);
		int index = openTab(QIcon(":/Icons/NGSD_variant.png"), name, type, widget);
		ui_.tabs->tabBar()->setTabData(index, v_id);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Open variant tab", e.message());
	}
}

void MainWindow::openProcessingSystemTab(QString system_name)
{
	NGSD db;
	int sys_id = db.processingSystemId(system_name, false);
	if (sys_id==-1)
	{
		GUIHelper::showMessage("NGSD error", "Processing system name '" + system_name + "' not found in NGSD!");
		return;
	}

	TabType type = TabType::SYSTEM;
	QString name = db.getProcessingSystemData(sys_id).name;
	if (focusTab(type, name)) return;

	ProcessingSystemWidget* widget = new ProcessingSystemWidget(this, sys_id);
	int index = openTab(QIcon(":/Icons/NGSD_processing_system.png"), name, type, widget);
	ui_.tabs->tabBar()->setTabData(index, QString::number(sys_id));
}

void MainWindow::openProjectTab(QString name)
{
	TabType type = TabType::PROJECT;
	if (focusTab(type, name)) return;

	ProjectWidget* widget = new ProjectWidget(this, name);
	int index = openTab(QIcon(":/Icons/NGSD_project.png"), name, type, widget);
	ui_.tabs->tabBar()->setTabData(index, NGSD().getValue("SELECT id FROM project WHERE name=:0", true, name).toString());
}

int MainWindow::openTab(QIcon icon, QString name, TabType type, QWidget* widget)
{
	QScrollArea* scroll_area = new QScrollArea(this);
	scroll_area->setFrameStyle(QFrame::NoFrame);
	scroll_area->setWidgetResizable(true);
	scroll_area->setWidget(widget);
	scroll_area->setProperty("TabType", (int)type);
	//fix color problems
	QPalette pal;
	pal.setColor(QPalette::Window,QColor(0,0,0,0));
	scroll_area->setPalette(pal);
	scroll_area->setBackgroundRole(QPalette::Window);
	scroll_area->widget()->setPalette(pal);
	scroll_area->widget()->setBackgroundRole(QPalette::Window);
	//show tab
	int index = ui_.tabs->addTab(scroll_area, icon, name);
	ui_.tabs->setCurrentIndex(index);

	return index;
}

void MainWindow::closeTab(int index)
{
	//main variant list
	if (index==0)
	{
		if (filename_!="")
		{
			int res = QMessageBox::question(this, "Close file?", "Do you want to close the current sample?", QMessageBox::Yes, QMessageBox::No);
			if (res==QMessageBox::Yes)
			{
				loadFile();
			}
		}
		return;
	}

	//make sure we do not close tabs while they are loading/upading
	QWidget* widget = ui_.tabs->widget(index);
	TabBaseClass* tab_widget = widget->findChild<TabBaseClass*>();
	if (tab_widget!=nullptr)
	{
		if (tab_widget->isBusy())
		{
			QMessageBox::information(this, ui_.tabs->tabText(index), "You cannot close a tab while it is busy, e.g. initializing, updating, searching.\nPlease retry when the tab is no longer busy!");
			return;
		}
	}

	//remove tab and delete tab widget
	ui_.tabs->removeTab(index);
	widget->deleteLater();
}

bool MainWindow::focusTab(TabType type, QString name)
{
	for (int t=0; t<ui_.tabs->count(); ++t)
	{
		if (ui_.tabs->widget(t)->property("TabType").toInt()==(int)type && ui_.tabs->tabText(t)==name)
		{
			ui_.tabs->setCurrentIndex(t);
			return true;
		}
	}

	return false;
}

void MainWindow::tabContextMenu(QPoint pos)
{
	int index = ui_.tabs->tabBar()->tabAt(pos);
	QString data = ui_.tabs->tabBar()->tabData(index).toString();

	// create menu
	QMenu menu(ui_.tabs);
	QAction* a_copy_text = menu.addAction("Copy text");
	a_copy_text->setEnabled(index!=-1);
	QAction* a_copy_id = menu.addAction("Copy NGSD id");
	a_copy_id->setEnabled(index!=-1 && !data.isEmpty());

	// execute menu
	QAction* action = menu.exec(ui_.tabs->mapToGlobal(pos));
	if (action == nullptr) return;

	// execute
	if (action==a_copy_text)
	{
		QApplication::clipboard()->setText(ui_.tabs->tabText(index));
	}
	else if (action==a_copy_id)
	{
		QApplication::clipboard()->setText(data);
	}
	else
	{
		THROW(ProgrammingException, "Invalid menu action in context menu selected!")
	}
}

void MainWindow::on_actionChangeLog_triggered()
{
	QDesktopServices::openUrl(QUrl("https://github.com/imgag/ngs-bits/tree/master/doc/GSvar/changelog.md"));
}

void MainWindow::loadFile(QString filename, bool show_only_error_issues)
{
	//store variant list in case it changed
	if (!variants_changed_.isEmpty())
	{
		int result = QMessageBox::question(this, "Store GSvar file?", "The GSvar file was changed by you.\nDo you want to store the changes to file?", QMessageBox::Yes, QMessageBox::No);
		if (result==QMessageBox::Yes)
		{
			storeCurrentVariantList();
		}
	}

    QElapsedTimer timer;
	timer.start();

	//mark IGV as not initialized
	IgvSessionManager::get(0).setInitialized(false);

	//reset GUI and data structures
	setWindowTitle(appName());
	filename_ = "";
	variants_.clear();
	GlobalServiceProvider::clearFileLocationProvider();
	variants_changed_.clear();
	cnvs_.clear();
	svs_.clear();
	res_.clear();
	ui_.vars->clearContents();
	report_settings_ = ReportSettings();
	connect(report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeReportConfig()));
	germline_report_ps_ = "";
	somatic_report_settings_ = SomaticReportSettings();
	ui_.tabs->setCurrentIndex(0);
	ui_.filters->reset(true);
	Log::perf("Clearing variant table took ", timer);

	if (filename=="") return;

	//load data
	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		//load variants
		timer.restart();
		variants_.load(filename);
		Log::perf("Loading small variant list took ", timer);
		QString mode_title = "";
		if (Helper::isHttpUrl(filename))
		{
			GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProviderRemote>(new FileLocationProviderRemote(filename)));
		}
		else
		{
			GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProviderLocal>(new FileLocationProviderLocal(filename, variants_.getSampleHeader(), variants_.type())));
			mode_title = " (local mode)";
		}
        lazyLoadIGVfiles(filename);

		//load CNVs
		timer.restart();
		FileLocation cnv_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisCnvFile();
		if (cnv_loc.exists)
		{
			try
			{
				cnvs_.load(cnv_loc.filename);
				int cnv_count_initial = cnvs_.count();
				double min_ll = 0.0;
				while (cnvs_.count()>50000)
				{
					min_ll += 1.0;
					FilterResult result(cnvs_.count());
					FilterCnvLoglikelihood filter;
					filter.setDouble("min_ll", min_ll);
					filter.apply(cnvs_, result);
					result.removeFlagged(cnvs_);
				}
				if (min_ll>0)
				{
					QMessageBox::information(this, "CNV pre-filtering applied", "The CNV calls file contains too many CNVs: " + QString::number(cnv_count_initial) +".\nOnly CNVs with log-likelyhood >= " + QString::number(min_ll) +" are displayed: " + QString::number(cnvs_.count()) +".");
				}
			}
			catch(Exception& e)
			{
				QMessageBox::warning(this, "Error loading CNVs", e.message());
				cnvs_.clear();
			}
		}
		else
		{
			cnvs_.clear();
		}
		Log::perf("Loading CNV list took ", timer);

		//load SVs
		timer.restart();
		FileLocation sv_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisSvFile();
		if (sv_loc.exists)
		{
			try
			{
				svs_.load(sv_loc.filename);
			}
			catch(Exception& e)
			{
				QMessageBox::warning(this, "Error loading SVs", e.message());
				svs_.clear();
			}
		}
		else
		{
			svs_.clear();
		}
		Log::perf("Loading SV list took ", timer);


		//load REs
		FileLocationList re_locs = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionFiles(false);
		if (variants_.type()==GERMLINE_SINGLESAMPLE && re_locs.count()>0 && re_locs[0].exists)
		{
			timer.restart();
			try
			{
				res_.load(re_locs[0].filename);
			}
			catch(Exception& e)
			{
				QMessageBox::warning(this, "Error loading REs", e.message());
				res_.clear();
			}
			Log::perf("Loading RE list took ", timer);
		}
		else
		{
			res_.clear();
		}

		//determine valid filter entries from filter column (and add new filters low_mappability/mosaic to make outdated GSvar files work as well)
		QStringList valid_filter_entries = variants_.filters().keys();
		if (!valid_filter_entries.contains("low_mappability")) valid_filter_entries << "low_mappability";
		if (!valid_filter_entries.contains("mosaic")) valid_filter_entries << "mosaic";
		ui_.filters->setValidFilterEntries(valid_filter_entries);

		//update data structures
		Settings::setPath("path_variantlists", filename);
		filename_ = filename;

		//update GUI
		setWindowTitle(appName() + " - " + variants_.analysisName() + mode_title);
		ui_.statusBar->showMessage("Loaded variant list with " + QString::number(variants_.count()) + " variants.");

		refreshVariantTable(false);

		QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Error", "Loading the file '" + filename + "' or displaying the contained variants failed!\nError message:\n" + e.message());
		loadFile();
		return;
	}

	//check analysis for issues (outdated, missing columns, wrong genome build, bad quality, ...)
	QList<QPair<Log::LogLevel, QString>> issues;
	try
	{
		ui_.variant_details->setLabelTooltips(variants_);
	}
	catch(Exception& e)
	{
		issues << qMakePair(Log::LOG_INFO, e.message());
	}
	checkVariantList(issues);

	//check variant list in NGSD
	checkProcessedSamplesInNGSD(issues);

	//show issues
	if (showAnalysisIssues(issues, show_only_error_issues)==QDialog::Rejected)
	{
		loadFile();
		return;
	}

	//load report config
	if (germlineReportSupported())
	{
		loadReportConfig();
	}
	else if(LoginManager::active() && somaticReportSupported())
	{
		loadSomaticReportConfig();
	}

	//check mendelian error rate for trios
	AnalysisType type = variants_.type();
	if (type==GERMLINE_TRIO)
	{
		checkMendelianErrorRate();
	}

	//notify for variant validation
	checkPendingVariantValidations();

	//activate Circos plot menu item if plot is available
	if (type==GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getCircosPlotFiles(false).isEmpty())
	{
		ui_.actionCircos->setEnabled(true);
	}
	else
	{
		ui_.actionCircos->setEnabled(false);
	}

	//activate repeat expansion menu item if RE calls are available
	if (type==GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getRepeatExpansionFiles(false).isEmpty())
	{
		ui_.actionRE->setEnabled(true);
	}
	else
	{
		ui_.actionRE->setEnabled(false);
	}

	//activate PRS menu item if PRS are available
	if (type==GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getPrsFiles(false).isEmpty())
	{
		ui_.actionPRS->setEnabled(true);
	}
	else
	{
		ui_.actionPRS->setEnabled(false);
	}

	//activate virus table
	ui_.actionVirusDetection->setEnabled(false);
	if (type==SOMATIC_PAIR && NGSD::isAvailable())
	{
		QString ps_tumor = variants_.mainSampleName();
		NGSD db;
		QString ps_tumor_id = db.processedSampleId(ps_tumor, false);
		if (GlobalServiceProvider::database().processedSamplePath(ps_tumor_id, PathType::VIRAL).exists)
		{
			ui_.actionVirusDetection->setEnabled(true);
		}
	}

	//activate cfDNA menu entries and get all available cfDNA samples
	cf_dna_available = false;
	ui_.actionDesignCfDNAPanel->setVisible(false);
	ui_.actionCfDNADiseaseCourse->setVisible(false);
	ui_.actionDesignCfDNAPanel->setEnabled(false);
	ui_.actionCfDNADiseaseCourse->setEnabled(false);
	cfdna_menu_btn_->setVisible(false);
	cfdna_menu_btn_->setEnabled(false);
	if (somaticReportSupported() || tumoronlyReportSupported())
	{
		ui_.actionDesignCfDNAPanel->setVisible(true);
		ui_.actionCfDNADiseaseCourse->setVisible(true);
		cfdna_menu_btn_->setVisible(true);

		if (LoginManager::active())
		{
			ui_.actionDesignCfDNAPanel->setEnabled(true);
			cfdna_menu_btn_->setEnabled(true);
			NGSD db;

			// get all same samples
			int sample_id = db.sampleId(variants_.mainSampleName()).toInt();
			QSet<int> same_sample_ids = db.relatedSamples(sample_id, "same sample");
			same_sample_ids << sample_id; // add current sample id

			// get all related cfDNA
			QSet<int> cf_dna_sample_ids;
			foreach (int cur_sample_id, same_sample_ids)
			{
				cf_dna_sample_ids.unite(db.relatedSamples(cur_sample_id, "tumor-cfDNA"));
			}

			if (cf_dna_sample_ids.size() > 0)
			{
				ui_.actionCfDNADiseaseCourse->setEnabled(true);
				cf_dna_available = true;
			}
		}
	}

	//activate RNA menu
	ui_.actionExpressionData->setEnabled(false);
	ui_.actionExonExpressionData->setEnabled(false);
	ui_.actionShowSplicing->setEnabled(false);
	ui_.actionShowRnaFusions->setEnabled(false);
	if (LoginManager::active())
	{
		NGSD db;

		QString sample_id = db.sampleId(filename_, false);
		if (sample_id!="")
		{
			foreach (int rna_sample_id, db.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
			{
				// check for required files
				foreach (const QString& rna_ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample_id)))
				{
					if (!db.userCanAccess(LoginManager::userId(), rna_ps_id.toInt())) continue;

					// search for gene count file
					FileLocation rna_count_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::EXPRESSION);
					if (rna_count_file.exists) ui_.actionExpressionData->setEnabled(true);

					// search for gene count file
					rna_count_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::EXPRESSION_EXON);
					if (rna_count_file.exists) ui_.actionExonExpressionData->setEnabled(true);

					// search for splicing predictions tsv
					FileLocation splicing_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::SPLICING_ANN);
					if (splicing_file.exists) ui_.actionShowSplicing->setEnabled(true);


					// search for arriba fusion file
					FileLocation arriba_fusion_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::FUSIONS);
					if (arriba_fusion_file.exists) ui_.actionShowRnaFusions->setEnabled(true);
				}
			}
		}
	}

	//activate Methylation menu
	if (type==GERMLINE_SINGLESAMPLE)
	{
		FileLocation met_loc = GlobalServiceProvider::fileLocationProvider().getMethylationFile();
		ui_.actionMethylation->setEnabled(met_loc.exists);

	}
}

void MainWindow::checkVariantList(QList<QPair<Log::LogLevel, QString>>& issues)
{
	//check genome builds match
	if (variants_.build()!=GSvarHelper::build())
	{
		issues << qMakePair(Log::LOG_ERROR, "Genome build of GSvar file (" + buildToString(variants_.build(), true) + ") not matching genome build of the GSvar application (" + buildToString(GSvarHelper::build(), true) + ")! Re-do the analysis for " + buildToString(GSvarHelper::build(), true) +"!");
	}

	//check creation date
	QDate create_date = variants_.getCreationDate();
	if (create_date.isValid())
	{
		if (create_date < QDate::currentDate().addDays(-42))
		{
			issues << qMakePair(Log::LOG_INFO, "Variant annotations are older than six weeks (" + create_date.toString(Qt::ISODate) + ").");
		}
		QDate gsvar_file_outdated_before = QDate::fromString(Settings::string("gsvar_file_outdated_before", true), "yyyy-MM-dd");
		if (gsvar_file_outdated_before.isValid() && create_date<gsvar_file_outdated_before)
		{
			issues << qMakePair(Log::LOG_WARNING, "Variant annotations are outdated! They are older than " + gsvar_file_outdated_before.toString(Qt::ISODate) + ". Please re-annotate variants!");
		}
	}

	//check sample header
	try
	{
		variants_.getSampleHeader();
	}
	catch(Exception e)
	{
		issues << qMakePair(Log::LOG_WARNING,  e.message());
	}

	//create list of required columns
	QStringList cols;
	cols << "filter";
	AnalysisType type = variants_.type();
	if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
	{
		cols << "classification";
		cols << "NGSD_hom";
		cols << "NGSD_het";
		cols << "comment";
		cols << "gene_info";
	}
	if (type==SOMATIC_SINGLESAMPLE || type==SOMATIC_PAIR || type==CFDNA)
	{
		cols << "somatic_classification";
		cols << "somatic_classification_comment";
		cols << "NGSD_som_vicc_interpretation";
		cols << "NGSD_som_vicc_comment";
	}

	//check columns
	foreach(const QString& col, cols)
	{
		if (variants_.annotationIndexByName(col, true, false)==-1)
		{
			issues << qMakePair(Log::LOG_WARNING, "Column '" + col + "' missing. Please re-annotate variants!");
		}
	}

	//check data was loaded completely
	if (germlineReportSupported())
	{
		NGSD db;
		int sys_id = db.processingSystemIdFromProcessedSample(germlineReportSample());
		ProcessingSystemData sys_data = db.getProcessingSystemData(sys_id);
		if (sys_data.type=="WES" || sys_data.type=="WGS" || sys_data.type=="lrGS")
		{
			QSet<Chromosome> chromosomes;
			for(int i=0; i<variants_.count(); ++i)
			{
				chromosomes << variants_[i].chr();
			}
			if (chromosomes.size()<23)
			{
				issues << qMakePair(Log::LOG_WARNING, "Variants detected on " + QString::number(chromosomes.size()) + " chromosomes only! Expected variants on at least 23 chromosomes for WES/WGS data! Please re-do variant calling of small variants!");
			}
		}
	}

	//check sv annotation
	if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
	{
		if (svs_.isValid())
		{
			//check for NGSD count annotation
			if(!svs_.annotationHeaders().contains("NGSD_HOM") || !svs_.annotationHeaders().contains("NGSD_HET") || !svs_.annotationHeaders().contains("NGSD_AF"))
			{
				issues << qMakePair(Log::LOG_WARNING, QString("Annotation of structural variants is outdated! Please re-annotate structural variants!"));
			}
		}
	}

	//check GenLab Labornummer is present (for diagnostic samples only)
	if (GenLabDB::isAvailable() && NGSD::isAvailable())
	{
		NGSD db;
		foreach(const SampleInfo& info, variants_.getSampleHeader())
		{
			QString ps_name = info.name;
			QString ps_id = db.processedSampleId(ps_name, false);
			if (ps_id=="") continue; //not in NGSD

			QString project_type = db.getValue("SELECT p.type FROM processed_sample ps, project p WHERE p.id=ps.project_id AND ps.id=" + ps_id).toString();
			if (project_type=="diagnostics")
			{
				GenLabDB genlab;
				QString genlab_patient_id = genlab.patientIdentifier(ps_name);
				if (genlab_patient_id.isEmpty())
				{
					issues << qMakePair(Log::LOG_WARNING, QString("GenLab Labornummer probably not set correctly for dianostic sample '" + ps_name + "'. Please correct the Labornummer in GenLab!"));
				}
			}
		}
	}
}

void MainWindow::checkProcessedSamplesInNGSD(QList<QPair<Log::LogLevel, QString>>& issues)
{
	if (!LoginManager::active()) return;

	NGSD db;

	foreach(const SampleInfo& info, variants_.getSampleHeader())
	{
		QString ps = info.name;
		QString ps_id = db.processedSampleId(ps, false);
		if (ps_id=="") continue;

		//check quality
		QString quality = db.getValue("SELECT quality FROM processed_sample WHERE id=" + ps_id).toString();
		if (quality=="bad")
		{
			issues << qMakePair(Log::LOG_WARNING, "Quality of processed sample '" + ps + "' is 'bad'!");
		}
		else if (quality=="n/a")
		{
			issues << qMakePair(Log::LOG_WARNING, "Quality of processed sample '" + ps + "' is not set!");
		}

		//check sequencing run is marked as analyzed
		QString run_status = db.getValue("SELECT r.status FROM sequencing_run r, processed_sample ps WHERE r.id=ps.sequencing_run_id AND ps.id=" + ps_id).toString();
		if (run_status!="analysis_finished")
		{
			issues << qMakePair(Log::LOG_WARNING, "Sequencing run of the sample '" + ps + "' does not have status 'analysis_finished'!");
		}

		//check KASP result
		try
		{
			double error_prob = db.kaspData(ps_id).random_error_prob;
			if (error_prob>0.03)
			{
				issues << qMakePair(Log::LOG_WARNING, "KASP swap probability of processed sample '" + ps + "' is larger than 3%!");
			}
		}
		catch (DatabaseException /*e*/)
		{
			//nothing to do (KASP not done or invalid)
		}

		//check variants are imported
		AnalysisType type = variants_.type();
		if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
		{

			QString sys_type = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(ps)).type;
			if (sys_type=="WGS" || sys_type=="WES" || sys_type=="lrGS")
			{
				ImportStatusGermline import_status = db.importStatus(ps_id);
				if (import_status.small_variants==0)
				{
					issues << qMakePair(Log::LOG_WARNING, "No germline variants imported into NGSD for processed sample '" + ps + "'!");
				}
			}
		}

		//check if sample is scheduled for resequencing
		if (db.getValue("SELECT scheduled_for_resequencing FROM processed_sample WHERE id=" + ps_id).toBool())
		{
			issues << qMakePair(Log::LOG_WARNING, "The processed sample " + ps + " is scheduled for resequencing!");
		}
	}
}

int MainWindow::showAnalysisIssues(QList<QPair<Log::LogLevel, QString>>& issues, bool show_only_error_issues)
{
	if (issues.empty()) return QDialog::Accepted;

	//generate text
	bool has_error = false;
	QStringList lines;
	foreach(auto issue, issues)
	{
		if (issue.first==Log::LOG_ERROR)
		{
			has_error = true;
			lines << "<font color=red>Error:</font>";
		}
		else if (issue.first==Log::LOG_WARNING)
		{
			lines << "<font color=orange>Warning:</font>";
		}
		else
		{
			lines << "Notice:";
		}
		lines << issue.second;
		lines << "";
	}

	if (show_only_error_issues && !has_error) return QDialog::Accepted;

	//show dialog
	QLabel* label = new QLabel(lines.join("<br>"));
	label->setMargin(6);
	auto dlg = GUIHelper::createDialog(label, "GSvar analysis issues", "The following issues were encountered when loading the analysis:", true);
	return dlg->exec();
}

void MainWindow::on_actionAbout_triggered()
{
	QString about_text = appName()+ " " + QCoreApplication::applicationVersion();

	about_text += "\n\nA free decision support system for germline and somatic variants.";

	//general infos
	about_text += "\n";
	about_text += "\nGenome build: " + GSvarHelper::buildAsString();
	about_text += "\nArchitecture: " + QSysInfo::buildCpuArchitecture();
	about_text += "\nhtslib version: " + QString(hts_version());

	//client-server infos
	about_text += "\n";
	if (ClientHelper::isClientServerMode())
	{
		about_text += "\nServer information:";
        int status_code = -1;
        ServerInfo server_info = ClientHelper::getServerInfo(status_code);
        if (status_code!=200)
        {
            about_text += "\nServer returned " + QString::number(status_code);
        }
        else
        {
			about_text += "\n  version: " + server_info.version;
			about_text += "\n  start time: " + server_info.server_start_time.toString("yyyy-MM-dd hh:mm:ss");
			about_text += "\n  API URL: " + server_info.server_url;
			about_text += "\n  API version: " + server_info.api_version;
			about_text += "\n  htslib version: " + server_info.htslib_version;
		}
    }
	else
	{
		about_text += "\nMode: stand-alone (no server)";
	}

	QMessageBox::about(this, "About " + appName(), about_text);
}

void MainWindow::loadReportConfig()
{
	//check if applicable
	if (!germlineReportSupported()) return;

	//check if report config exists
	NGSD db;
	QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
	int rc_id = db.reportConfigId(processed_sample_id);
	if (rc_id==-1) return;

	//load
	report_settings_.report_config = db.reportConfig(rc_id, variants_, cnvs_, svs_, res_);
	connect(report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeReportConfig()));

	//updateGUI
	refreshVariantTable();
}

void MainWindow::loadSomaticReportConfig()
{
	if(filename_ == "") return;

	NGSD db;

	//Determine processed sample ids
	QString ps_tumor = variants_.mainSampleName();
	QString ps_tumor_id = db.processedSampleId(ps_tumor, false);
	if(ps_tumor_id == "") return;
	QString ps_normal = normalSampleName();
	if(ps_normal.isEmpty()) return;
	QString ps_normal_id = db.processedSampleId(ps_normal, false);
	if(ps_normal_id == "") return;

	somatic_report_settings_.tumor_ps = ps_tumor;
	somatic_report_settings_.normal_ps = ps_normal;
	somatic_report_settings_.msi_file = GlobalServiceProvider::fileLocationProvider().getSomaticMsiFile().filename;
	somatic_report_settings_.viral_file = GlobalServiceProvider::database().processedSamplePath(ps_tumor_id, PathType::VIRAL).filename;

	somatic_report_settings_.sbs_signature = GlobalServiceProvider::fileLocationProvider().getSignatureSbsFile().filename;
	somatic_report_settings_.id_signature = GlobalServiceProvider::fileLocationProvider().getSignatureIdFile().filename;
	somatic_report_settings_.dbs_signature = GlobalServiceProvider::fileLocationProvider().getSignatureDbsFile().filename;
	somatic_report_settings_.cnv_signature = GlobalServiceProvider::fileLocationProvider().getSignatureCnvFile().filename;

	try //load normal sample
	{
		somatic_control_tissue_variants_.load(GlobalServiceProvider::database().processedSamplePath(db.processedSampleId(ps_normal), PathType::GSVAR).filename);
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "Could not load germline GSvar file", "Could not load germline GSvar file. No germline variants will be parsed for somatic report generation. Message: " + e.message());
	}



	//Continue loading report (only if existing in NGSD)
	if(db.somaticReportConfigId(ps_tumor_id, ps_normal_id) == -1) return;


	QStringList messages;
	somatic_report_settings_.report_config = db.somaticReportConfig(ps_tumor_id, ps_normal_id, variants_, cnvs_, svs_, somatic_control_tissue_variants_, messages);


	if(!messages.isEmpty())
	{
		QMessageBox::warning(this, "Somatic report configuration", "The following problems were encountered while loading the som. report configuration:\n" + messages.join("\n"));
	}

	//Preselect target region bed file in NGSD
	if(somatic_report_settings_.report_config->targetRegionName()!="")
	{
		ui_.filters->setTargetRegionByDisplayName(somatic_report_settings_.report_config->targetRegionName());
	}

	//Preselect filter from NGSD som. rep. conf.
	if (somatic_report_settings_.report_config->filterName() != "") ui_.filters->setFilter( somatic_report_settings_.report_config->filterName());
	if(somatic_report_settings_.report_config->filters().count() != 0) ui_.filters->setFilterCascade(somatic_report_settings_.report_config->filters());


	somatic_report_settings_.target_region_filter = ui_.filters->targetRegion();

	refreshVariantTable();
}

void MainWindow::storeSomaticReportConfig()
{
	if(filename_ == "") return;
	if(!LoginManager::active()) return;
	if(variants_.type() != SOMATIC_PAIR) return;

	NGSD db;
	QString ps_tumor_id = db.processedSampleId(variants_.mainSampleName(), false);
	QString ps_normal_id = db.processedSampleId(normalSampleName(), false);

	if(ps_tumor_id=="" || ps_normal_id == "")
	{
		QMessageBox::warning(this, "Storing somatic report configuration", "Samples were not found in the NGSD!");
		return;
	}

	int conf_id = db.somaticReportConfigId(ps_tumor_id, ps_normal_id);

	if (conf_id!=-1)
	{
		SomaticReportConfigurationData conf_creation = db.somaticReportConfigData(conf_id);
		if (conf_creation.last_edit_by!="" && conf_creation.last_edit_by!=LoginManager::userName())
		if (QMessageBox::question(this, "Storing report configuration", conf_creation.history() + "\n\nDo you want to update/override it?")==QMessageBox::No)
		{
			return;
		}
	}

	//store
	try
	{
		db.setSomaticReportConfig(ps_tumor_id, ps_normal_id, somatic_report_settings_.report_config, variants_, cnvs_, svs_, somatic_control_tissue_variants_, Helper::userName());
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Storing somatic report configuration", "Error: Could not store the somatic report configuration.\nPlease resolve this error or report it to the administrator:\n\n" + e.message());
	}
}

void MainWindow::storeReportConfig()
{
	//check if applicable
	if (!germlineReportSupported()) return;

	//check sample
	NGSD db;
	QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
	if (processed_sample_id=="")
	{
		QMessageBox::warning(this, "Storing report configuration", "Sample was not found in the NGSD!");
		return;
	}

	//check if config exists and not edited by other user
	int conf_id = db.reportConfigId(processed_sample_id);
	if (conf_id!=-1)
	{
		QSharedPointer<ReportConfiguration> report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, res_);
		if (report_config->lastUpdatedBy()!="" && report_config->lastUpdatedBy()!=LoginManager::userName())
		{
			if (QMessageBox::question(this, "Storing report configuration", report_config->history() + "\n\nDo you want to override it?")==QMessageBox::No)
			{
				return;
			}
		}
	}

	//store
	try
	{
		report_settings_.report_config.data()->blockSignals(true); //block signals - otherwise the variantsChanged signal is emitted and storeReportConfig is called again, which leads to hanging of the application because of database locks
		db.setReportConfig(processed_sample_id, report_settings_.report_config, variants_, cnvs_, svs_, res_);
		report_settings_.report_config.data()->blockSignals(false);
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Storing report configuration", e.message());
	}
}

void MainWindow::generateEvaluationSheet()
{
	//check if applicable
	if (!germlineReportSupported()) return;

	QString base_name = germlineReportSample();

	//make sure free-text phenotype infos are available
	NGSD db;
	QString sample_id = db.sampleId(base_name);
	QList<SampleDiseaseInfo> disease_infos = db.getSampleDiseaseInfo(sample_id, "clinical phenotype (free text)");
	if (disease_infos.isEmpty() && QMessageBox::question(this, "Evaluation sheet", "No clinical phenotype (free text) is set for the sample!\nIt will be shown on the evaluation sheet!\n\nDo you want to set it?")==QMessageBox::Yes)
	{
		SampleDiseaseInfoWidget* widget = new SampleDiseaseInfoWidget(base_name, this);
		widget->setDiseaseInfo(db.getSampleDiseaseInfo(sample_id));
		auto dlg = GUIHelper::createDialog(widget, "Sample disease details", "", true);
		if (dlg->exec() != QDialog::Accepted) return;

		db.setSampleDiseaseInfo(sample_id, widget->diseaseInfo());
	}

	//try to get VariantListInfo from the NGSD
	QString ps_id = db.processedSampleId(base_name);
	EvaluationSheetData evaluation_sheet_data = db.evaluationSheetData(ps_id, false);
	evaluation_sheet_data.build = GSvarHelper::build();
	if (evaluation_sheet_data.ps_id == "") //No db entry found > init
	{
		evaluation_sheet_data.ps_id = db.processedSampleId(base_name);
		evaluation_sheet_data.dna_rna = db.getSampleData(sample_id).name_external;
		// make sure reviewer 1 contains name not user id
		evaluation_sheet_data.reviewer1 = db.userName(db.userId(report_settings_.report_config->createdBy()));
		evaluation_sheet_data.review_date1 = report_settings_.report_config->createdAt().date();
		evaluation_sheet_data.reviewer2 = LoginManager::userName();
		evaluation_sheet_data.review_date2 = QDate::currentDate();
	}

	//Show VariantSheetEditDialog
	EvaluationSheetEditDialog* edit_dialog = new EvaluationSheetEditDialog(this);
	edit_dialog->importEvaluationSheetData(evaluation_sheet_data);
	if (edit_dialog->exec() != QDialog::Accepted) return;

	//Store updated info in the NGSD
	db.storeEvaluationSheetData(evaluation_sheet_data, true);

	//get filename
	QString folder = Settings::path("gsvar_variantsheet_folder");
	QString filename = QFileDialog::getSaveFileName(this, "Store evaluation sheet",  folder + "/" + base_name + "_variant_sheet_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (filename.isEmpty()) return;

	//write sheet
	PrsTable prs_table; //not needed
	GermlineReportGeneratorData generator_data(GSvarHelper::build(), base_name, variants_, cnvs_, svs_, res_, prs_table, report_settings_, ui_.filters->filters(), GSvarHelper::preferredTranscripts(), GlobalServiceProvider::statistics());
	GermlineReportGenerator generator(generator_data);
	generator.writeEvaluationSheet(filename, evaluation_sheet_data);

	if (QMessageBox::question(this, "Evaluation sheet", "Evaluation sheet generated successfully!\nDo you want to open it in your browser?")==QMessageBox::Yes)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
	}
}

void MainWindow::transferSomaticData()
{
	try
	{
		if(variants_.type()!=AnalysisType::SOMATIC_PAIR)
		{
			INFO(Exception, "Error: only possible for tumor-normal pair!");
		}

		SomaticDataTransferWidget data_transfer(somatic_report_settings_.tumor_ps, somatic_report_settings_.normal_ps, this);
		data_transfer.exec();
	}
	catch(Exception e)
	{
		GUIHelper::showException(this, e, "Transfer somatic data to MTB");
	}
}

void MainWindow::showReportConfigInfo()
{
	//check if applicable
	if (!germlineReportSupported() && !somaticReportSupported()) return;

	QString ps = germlineReportSupported() ? germlineReportSample() : variants_.mainSampleName();
	QString title = "Report configuration information of " + ps;

	//check sample exists
	NGSD db;
	QString processed_sample_id = db.processedSampleId(ps, false);
	if (processed_sample_id=="")
	{
		QMessageBox::warning(this, title, "Sample was not found in the NGSD!");
		return;
	}

	//check config exists
	if(germlineReportSupported())
	{
		int conf_id = db.reportConfigId(processed_sample_id);
		if (conf_id==-1)
		{
			QMessageBox::warning(this, title , "No germline report configuration found in the NGSD!");
			return;
		}


		QSharedPointer<ReportConfiguration> report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, res_);

		QMessageBox::information(this, title, report_config->history() + "\n\n" + report_config->variantSummary());
	}
	else if(somaticReportSupported())
	{
		QString ps_normal_id = db.processedSampleId(normalSampleName(), false);
		int conf_id = db.somaticReportConfigId(processed_sample_id, ps_normal_id);
		if(conf_id==-1)
		{
			QMessageBox::warning(this, title, "No somatic report configuration found in the NGSD!");
			return;
		}
		QMessageBox::information(this, title, db.somaticReportConfigData(conf_id).history());
	}
}

void MainWindow::editOtherCausalVariant()
{
	QString title = "Add/edit other causal variant";
	try
	{
		//check if applicable
		if (!germlineReportSupported()) INFO(ArgumentException, "This feature is only available for germline!");

		QString ps = germlineReportSample();

		//check sample exists in NGSD
		NGSD db;
		QString processed_sample_id = db.processedSampleId(ps, false);
		if (processed_sample_id=="") INFO(ArgumentException, "Sample was not found in the NGSD!");

		// get report config
		OtherCausalVariant causal_variant = report_settings_.report_config->otherCausalVariant();
		QStringList variant_types = db.getEnum("report_configuration_other_causal_variant", "type");
		QStringList inheritance_modes = db.getEnum("report_configuration_other_causal_variant", "inheritance");

		//open edit dialog
		CausalVariantEditDialog dlg(causal_variant, variant_types, inheritance_modes, this);
		dlg.setWindowTitle(title + " of " + ps);

		if (dlg.exec()!=QDialog::Accepted) return;

		//store updated causal variant in NGSD
		if (dlg.causalVariant().isValid())
		{
			report_settings_.report_config->setOtherCausalVariant(dlg.causalVariant());
			report_settings_.report_config->variantsChanged();
		}
	}
	catch(Exception e)
	{
		GUIHelper::showException(this, e, title);
	}
}

void MainWindow::deleteOtherCausalVariant()
{
	QString title = "Delete other causal variant";
	try
	{
		//check if applicable
		if (!germlineReportSupported()) INFO(ArgumentException, "This feature is only available for germline!");

		QString ps = germlineReportSample();

		//check sample exists
		NGSD db;
		QString processed_sample_id = db.processedSampleId(ps, false);
		if (processed_sample_id=="") INFO(ArgumentException, "Sample was not found in the NGSD!");

		OtherCausalVariant causal_variant = report_settings_.report_config->otherCausalVariant();
		if(!causal_variant.isValid()) return;

		//show dialog to confirm by user
		QString message_text = "Are you sure you want to delete the following causal variant?\n" + causal_variant.type + " at " + causal_variant.coordinates + " (gene: " + causal_variant.gene + ", comment: " + causal_variant.comment.replace("\n", " ") + ")";
		QMessageBox::StandardButton response = QMessageBox::question(this, title + " of " + ps, message_text, QMessageBox::Yes|QMessageBox::No);
		if(response != QMessageBox::Yes) return;

		//replace other causal variant with empty struct
		report_settings_.report_config->setOtherCausalVariant(OtherCausalVariant());
	}
	catch(Exception e)
	{
		GUIHelper::showException(this, e, title);
	}
}

void MainWindow::finalizeReportConfig()
{
	QString title = "Finalize report configuration";
	try
	{
		//check if applicable
		if(!germlineReportSupported()) INFO(ArgumentException, "This feature is only available for germline!");

		//check sample exists
		NGSD db;
		QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
		if (processed_sample_id=="") INFO(ArgumentException, "Sample was not found in the NGSD!");

		//check config exists
		int conf_id = db.reportConfigId(processed_sample_id);
		if (conf_id==-1) INFO(ArgumentException, "No report configuration for this sample found in the NGSD!");

		//make sure the user knows what he does
		int button = QMessageBox::question(this, title, "Do you really want to finalize the report configuration?\nIt cannot be modified or deleted when finalized!");
		if (button!=QMessageBox::Yes) return;

		//finalize
		db.finalizeReportConfig(conf_id, LoginManager::userId());

		//update report settings data structure
		report_settings_.report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, res_);
		connect(report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeReportConfig()));
	}
	catch(Exception e)
	{
		GUIHelper::showException(this, e, title);
	}
}

void MainWindow::generateReport()
{
	if (filename_=="") return;

	QString error;

	AnalysisType type = variants_.type();
	QString type_str = analysisTypeToString(type);
	if (type==AnalysisType::SOMATIC_PAIR)
	{
		if (somaticReportSupported())
		{
			generateReportSomaticRTF();
		}
		else error = "Analysis type " + type_str + ", but somatic report not supported!";
	}
	else if (type==AnalysisType::SOMATIC_SINGLESAMPLE)
	{
		if (tumoronlyReportSupported())
		{
			generateReportTumorOnly();
		}
		else error = "Analysis type " + type_str + ", but tumor-only report not supported!";
	}
	else if (type==AnalysisType::GERMLINE_SINGLESAMPLE || type==AnalysisType::GERMLINE_TRIO || type==AnalysisType::GERMLINE_MULTISAMPLE)
	{
		QString error_reason;
		if (germlineReportSupported(true, &error_reason))
		{
			generateReportGermline();
		}
		else error = "Analysis type " + type_str + ", but germline report not supported: " + error_reason;
	}
	else error = "Report not supported for analysis type '" + type_str + "'!";

	if (!error.isEmpty()) QMessageBox::information(this, "Report error", error);
}


void MainWindow::generateReportTumorOnly()
{
	try
	{
		TumorOnlyReportWorker::checkAnnotation(variants_);
	}
	catch(FileParseException e)
	{
		QMessageBox::warning(this, "Invalid tumor only file" + filename_, "Could not find all neccessary annotations in tumor-only variant list. Aborting creation of report. " + e.message());
		return;
	}
	QString ps = variants_.mainSampleName();

	NGSD db;

	//get report settings
	TumorOnlyReportWorkerConfig config;
	config.threads = Settings::integer("threads");
	int sys_id = db.processingSystemIdFromProcessedSample(ps);

	config.sys = db.getProcessingSystemData(sys_id);
	config.ps_data = db.getProcessedSampleData(db.processedSampleId(ps));
	config.roi = ui_.filters->targetRegion();

	config.low_coverage_file = GlobalServiceProvider::fileLocationProvider().getSomaticLowCoverageFile().filename;
	config.bam_file = GlobalServiceProvider::fileLocationProvider().getBamFiles(true).at(0).filename;
	config.filter_result = filter_result_;
	config.preferred_transcripts = GSvarHelper::preferredTranscripts();
	config.build = GSvarHelper::build();

	TumorOnlyReportDialog dlg(variants_, config, this);
	if(!dlg.exec()) return;

	//get RTF file name
	QString destination_path = last_report_path_ + "/" + ps + "_DNA_tumor_only_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	QString file_rep = QFileDialog::getSaveFileName(this, "Store report file", destination_path, "RTF files (*.rtf);;All files(*.*)");
	if (file_rep=="") return;

	//Generate RTF
	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		TumorOnlyReportWorker worker(variants_, config);

		QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
		worker.writeRtf(temp_filename);
		Helper::moveFile(temp_filename, file_rep);

		if(!ui_.filters->targetRegion().isValid()) //if no ROI filter was set, use panel target information instead
		{
			TargetRegionInfo roi_info;
			roi_info.name = config.sys.name;
			roi_info.regions = GlobalServiceProvider::database().processingSystemRegions(sys_id, false);
			roi_info.genes = GlobalServiceProvider::database().processingSystemGenes(sys_id, false);
			config.roi = roi_info;
		}

		QString gsvar_xml_folder = Settings::path("gsvar_xml_folder", true);
		if (gsvar_xml_folder!="")
		{
			QString xml_file = gsvar_xml_folder + "/" + ps + "_tumor_only.xml" ;
			QByteArray temp_xml = Helper::tempFileName(".xml").toUtf8();
			worker.writeXML(temp_xml);
			Helper::moveFile(temp_xml,xml_file);
		}
	}
	catch(Exception e)
	{
		QMessageBox::warning(this, "Could not create tumor only report", "Could not write tumor-only report. Error message: " + e.message());
	}

	QApplication::restoreOverrideCursor();

	//open report
	if (QMessageBox::question(this, "DNA tumor-only report", "report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(file_rep) );
	}
}


//transforms png data into list of tuples (png data in hex format, width, height)
QList<RtfPicture> pngsFromFiles(QStringList files)
{
	QList<RtfPicture> pic_list;
	foreach(const QString& path, files)
	{
		QImage pic;
		if (path.startsWith("http", Qt::CaseInsensitive))
		{
			QByteArray response = HttpHandler(true).get(path);
			if (!response.isEmpty()) pic.loadFromData(response);
		}
		else
		{
			pic = QImage(path);
		}
		if(pic.isNull()) continue;

		QByteArray png_data = "";
		QBuffer buffer(&png_data);
		buffer.open(QIODevice::WriteOnly);
		if (!pic.save(&buffer, "PNG")) continue;
		buffer.close();

		pic_list << RtfPicture(png_data.toHex(), pic.width(), pic.height());
	}

	return pic_list;
}

void MainWindow::generateReportSomaticRTF()
{
	if(!LoginManager::active()) return;

	NGSD db;
	QString ps_tumor = variants_.mainSampleName();
	QString ps_tumor_id = db.processedSampleId(ps_tumor);
	QString ps_normal = normalSampleName();
	QString ps_normal_id = db.processedSampleId(ps_normal);

	//Set data in somatic report settings
	somatic_report_settings_.report_config->setTargetRegionName(ui_.filters->targetRegion().name);

	somatic_report_settings_.report_config->setFilterName((ui_.filters->filterName() != "[none]" ? ui_.filters->filterName() : "") ); //filter name -> goes to NGSD som. rep. conf.
	somatic_report_settings_.report_config->setFilters(ui_.filters->filters()); //filter cascase -> goes to report helper

	somatic_report_settings_.tumor_ps = ps_tumor;
	somatic_report_settings_.normal_ps = ps_normal;

	somatic_report_settings_.preferred_transcripts = GSvarHelper::preferredTranscripts();
	somatic_report_settings_.report_config->setEvaluationDate(QDate::currentDate());

	//load obo terms for filtering coding/splicing variants
	if (somatic_report_settings_.obo_terms_coding_splicing.size() == 0)
	{
		OntologyTermCollection obo_terms("://Resources/so-xp_3_1_0.obo",true);
		QList<QByteArray> ids;
		ids << obo_terms.childIDs("SO:0001580",true); //coding variants
		ids << obo_terms.childIDs("SO:0001568",true); //splicing variants
		foreach(const QByteArray& id, ids)
		{
			somatic_report_settings_.obo_terms_coding_splicing.add(obo_terms.getByID(id));
		}
	}

	somatic_report_settings_.target_region_filter = ui_.filters->targetRegion();
	if(!ui_.filters->targetRegion().isValid()) //use processing system data in case no filter is set
	{
		ProcessingSystemData sys_data = db.getProcessingSystemData(db.processingSystemIdFromProcessedSample(ps_tumor));

		TargetRegionInfo generic_target;
		if (sys_data.type == "WGS")
		{
			generic_target.regions = GlobalServiceProvider::database().processingSystemRegions(db.processingSystemIdFromProcessedSample(ps_tumor), false);
			generic_target.genes = db.approvedGeneNames();
			generic_target.name = sys_data.name;
			somatic_report_settings_.target_region_filter = generic_target;

		} else {
			generic_target.regions = GlobalServiceProvider::database().processingSystemRegions(db.processingSystemIdFromProcessedSample(ps_tumor), false);
			generic_target.genes = db.genesToApproved(GlobalServiceProvider::database().processingSystemGenes(db.processingSystemIdFromProcessedSample(ps_tumor), false), true);
			generic_target.name = sys_data.name;
			somatic_report_settings_.target_region_filter = generic_target;
		}
	}

	if (db.getValues("SELECT value FROM processed_sample_qc AS psqc LEFT JOIN qc_terms as qc ON psqc.qc_terms_id = qc.id WHERE psqc.processed_sample_id=" + ps_tumor_id + " AND (qc.qcml_id ='QC:2000062' OR qc.qcml_id ='QC:2000063' OR qc.qcml_id ='QC:2000064') ").size() < 3)
	{
		QMessageBox::warning(this, "No HRD score found", "Warning:\nNo hrd score values found in the imported QC of tumor sample. HRD score set to 0.");
		somatic_report_settings_.report_config->setCnvLohCount(0);
		somatic_report_settings_.report_config->setCnvTaiCount(0);
		somatic_report_settings_.report_config->setCnvLstCount(0);
	}
	else
	{
		QString query = "SELECT value FROM processed_sample_qc AS psqc LEFT JOIN qc_terms as qc ON psqc.qc_terms_id = qc.id WHERE psqc.processed_sample_id=" + ps_tumor_id + " AND qc.qcml_id = :1";
		somatic_report_settings_.report_config->setCnvLohCount( db.getValue(query, false, "QC:2000062").toInt() );
		somatic_report_settings_.report_config->setCnvTaiCount( db.getValue(query, false, "QC:2000063").toInt() );
		somatic_report_settings_.report_config->setCnvLstCount( db.getValue(query, false, "QC:2000064").toInt() );
	}


	//Preselect report settings if not already exists to most common values
	if(db.somaticReportConfigId(ps_tumor_id, ps_normal_id) == -1)
	{
		somatic_report_settings_.report_config->setIncludeTumContentByMaxSNV(true);
		somatic_report_settings_.report_config->setIncludeTumContentByClonality(true);
		somatic_report_settings_.report_config->setIncludeTumContentByHistological(true);
		somatic_report_settings_.report_config->setMsiStatus(true);
		somatic_report_settings_.report_config->setCnvBurden(true);
	}

	//Parse genome ploidy from ClinCNV file
	FileLocation cnvFile = GlobalServiceProvider::fileLocationProvider().getAnalysisCnvFile();
	if (cnvFile.exists)
	{
		QStringList cnv_data = Helper::loadTextFile(cnvFile.filename, true, QChar::Null, true);

		for (const QString& line: cnv_data)
		{
			if (line.startsWith("##ploidy:"))
			{
				QStringList parts = line.split(':');
				somatic_report_settings_.report_config->setPloidy(parts[1].toDouble());
				break;
			}

			if (! line.startsWith("##"))
			{
				break;
			}
		}
	}

	//Get ICD10 diagnoses from NGSD
	QStringList tmp_icd10;
	QStringList tmp_phenotype;
	QStringList tmp_rna_ref_tissue;
	foreach(const auto& entry, db.getSampleDiseaseInfo(db.sampleId(ps_tumor)))
	{
		if(entry.type == "ICD10 code") tmp_icd10.append(entry.disease_info);
		if(entry.type == "clinical phenotype (free text)") tmp_phenotype.append(entry.disease_info);
		if(entry.type == "RNA reference tissue") tmp_rna_ref_tissue.append(entry.disease_info);
	}
	somatic_report_settings_.icd10 = tmp_icd10.join(", ");
	somatic_report_settings_.phenotype = tmp_phenotype.join(", ");

	SomaticReportDialog dlg(filename_, somatic_report_settings_, cnvs_, somatic_control_tissue_variants_, this); //widget for settings


	//Fill in RNA processed sample ids into somatic report dialog
	QSet<int> rna_ids =   db.relatedSamples(db.sampleId(ps_tumor).toInt(), "same sample", "RNA");
	if(!rna_ids.isEmpty())
	{
		dlg.enableChoiceRnaReportType(true);

		QStringList rna_names;
		foreach(int rna_id, rna_ids)
		{
			foreach(const auto& rna_ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id=" + QString::number(rna_id)) )
			{
				rna_names << db.processedSampleName(rna_ps_id);
			}
		}
		dlg.setRNAids(rna_names);
	}

	// get all same samples
	int sample_id = db.sampleId(variants_.mainSampleName()).toInt();
	QSet<int> same_sample_ids = db.relatedSamples(sample_id, "same sample");
	same_sample_ids << sample_id; // add current sample id

	// get all related cfDNA
	QSet<int> cf_dna_sample_ids;
	foreach (int cur_sample_id, same_sample_ids)
	{
		cf_dna_sample_ids.unite(db.relatedSamples(cur_sample_id, "tumor-cfDNA"));
	}

	if (cf_dna_sample_ids.size() > 0)
	{
		dlg.enableChoicecfDnaReportType(true);
	}


	if(!dlg.exec())
	{
		return;
	}

	dlg.writeBackSettings();


	//store somatic report config in NGSD
	if(!dlg.skipNGSD())
	{
		db.setSomaticReportConfig(ps_tumor_id, ps_normal_id, somatic_report_settings_.report_config, variants_, cnvs_, svs_, somatic_control_tissue_variants_, Helper::userName());
	}

	QString destination_path; //path to rtf file
	if(dlg.getReportType() == SomaticReportDialog::report_type::DNA)
	{
		destination_path = last_report_path_ + "/" + ps_tumor + "_DNA_report_somatic_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	}
	else if (dlg.getReportType() == SomaticReportDialog::report_type::RNA)
	{
		destination_path = last_report_path_ + "/" + dlg.getRNAid() + "-" + ps_tumor + "_RNA_report_somatic_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	}
	else
	{
		destination_path = last_report_path_ + "/" + ps_tumor + "_cfDNA_report_somatic_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	}

	//get RTF file name
	QString file_rep = QFileDialog::getSaveFileName(this, "Store report file", destination_path, "RTF files (*.rtf);;All files(*.*)");
	if (file_rep=="") return;

	QApplication::setOverrideCursor(Qt::BusyCursor);

	if(dlg.getReportType() == SomaticReportDialog::report_type::DNA)
	{
		//generate somatic DNA report
		try
		{

			if(!SomaticReportHelper::checkGermlineSNVFile(somatic_control_tissue_variants_))
			{
				QApplication::restoreOverrideCursor();
				QMessageBox::warning(this, "Somatic report", "DNA report cannot be created because germline GSVar-file is invalid. Please check control tissue variant file.");
				return;
			}

			SomaticReportHelper report(GSvarHelper::build(), variants_, cnvs_, svs_, somatic_control_tissue_variants_, somatic_report_settings_);

			//Store XML file with the same somatic report configuration settings
            QElapsedTimer timer;

			try
			{
				timer.start();
				QString tmp_xml = Helper::tempFileName(".xml");
				report.storeXML(tmp_xml);
				Helper::moveFile(tmp_xml, Settings::path("gsvar_xml_folder") + "\\" + somatic_report_settings_.tumor_ps + "-" + somatic_report_settings_.normal_ps + ".xml");

				Log::perf("Generating somatic report XML took ", timer);
			}
			catch(Exception e)
			{
				QMessageBox::warning(this, "creation of XML file failed", e.message());
			}

			//Generate RTF
			timer.start();
			QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
			report.storeRtf(temp_filename);
			Helper::moveFile(temp_filename, file_rep);
			Log::perf("Generating somatic report RTF took ", timer);

			//Generate files for QBIC upload
			timer.start();
			QString path = ps_tumor + "-" + ps_normal;
			if (GlobalServiceProvider::fileLocationProvider().isLocal()) path = Settings::string("qbic_data_path") + "/" + path;
			report.storeQbicData(path);
			Log::perf("Generating somatic report QBIC data took ", timer);

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
		if (QMessageBox::question(this, "DNA report", "DNA report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(file_rep) );
		}

		//reminder of MTB upload
		QStringList studies = db.studies(ps_tumor_id);
		if (studies.contains("MTB") || studies.contains("Modellvorhaben_2024"))
		{
			if (QMessageBox::question(this, "DNA report", "This sample is part of the study 'MTB' or the study 'Modellvorhaben_2024'.\nDo you want to upload the data to MTB now?")==QMessageBox::Yes)
			{
				transferSomaticData();
			}
		}
	}
	else if (dlg.getReportType() == SomaticReportDialog::report_type::RNA)//RNA report
	{
		//Generate RTF
		try
		{
			QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();

			SomaticRnaReportData rna_report_data = somatic_report_settings_;
			rna_report_data.rna_ps_name = dlg.getRNAid();
			rna_report_data.rna_fusion_file = GlobalServiceProvider::database().processedSamplePath(db.processedSampleId(dlg.getRNAid()), PathType::FUSIONS).filename;
			rna_report_data.rna_expression_file = GlobalServiceProvider::database().processedSamplePath(db.processedSampleId(dlg.getRNAid()), PathType::EXPRESSION).filename;
			rna_report_data.rna_bam_file = GlobalServiceProvider::database().processedSamplePath(db.processedSampleId(dlg.getRNAid()), PathType::BAM).filename;
			rna_report_data.ref_genome_fasta_file = Settings::string("reference_genome");

			try
			{
				QString filename = GlobalServiceProvider::database().processedSamplePath(db.processedSampleId(dlg.getRNAid()), PathType::EXPRESSION_CORR).filename;
				VersatileFile file(filename, false);
				file.open(QFile::ReadOnly | QIODevice::Text);
				rna_report_data.expression_correlation = Helper::toDouble(file.readAll());
			}
			catch(Exception)
			{
				rna_report_data.expression_correlation = std::numeric_limits<double>::quiet_NaN();
			}

			try
			{
				TSVFileStream expr_file(rna_report_data.rna_expression_file);
				for (QByteArray comment: expr_file.comments()) {
					if (comment.contains("cohort_size"))
					{
						bool ok;
						int size = comment.split(':')[1].toInt(&ok);
						if(ok)
						{
							rna_report_data.cohort_size = size;
						}
					}
				}
			}
			catch (Exception)
			{
			}

			if (rna_report_data.cohort_size == 0)
			{
				try
				{
					TSVFileStream cohort_file( GlobalServiceProvider::database().processedSamplePath( db.processedSampleId(dlg.getRNAid()), PathType::EXPRESSION_COHORT ).filename );
					rna_report_data.cohort_size = cohort_file.header().count()-1;
				}
				catch(Exception)
				{
				}
			}

			rna_report_data.rna_qcml_data = db.getQCData(db.processedSampleId(dlg.getRNAid()));

			//Add data from fusion pics
			try
			{
				rna_report_data.fusion_pics = pngsFromFiles(GlobalServiceProvider::database().getRnaFusionPics(dlg.getRNAid()));
			}
			catch(Exception) //Nothing to do here
			{
			}
			//Add data from expression plots
			try
			{
				rna_report_data.expression_plots = pngsFromFiles(GlobalServiceProvider::database().getRnaExpressionPlots(dlg.getRNAid()));
			}
			catch(Exception)
			{
			}

			//Look in tumor sample for HPA reference tissue
			foreach(const auto& entry, db.getSampleDiseaseInfo(db.sampleId(dlg.getRNAid())) )
			{
				if(entry.type == "RNA reference tissue") tmp_rna_ref_tissue.append(entry.disease_info);
			}
			tmp_rna_ref_tissue.removeDuplicates();
			rna_report_data.rna_hpa_ref_tissue = tmp_rna_ref_tissue.join(", ");

			SomaticRnaReport rna_report(variants_, cnvs_, rna_report_data);

			rna_report.writeRtf(temp_filename);
			Helper::moveFile(temp_filename, file_rep);
			QApplication::restoreOverrideCursor();
		}
		catch(Exception& error)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error while creating somatic RNA report.", error.message());
			return;
		}
		catch(...)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error while creating somatic RNA report.", "No error message!");
			return;
		}

		if (QMessageBox::question(this, "RNA report", "RNA report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(file_rep));
		}
	}
	else if (dlg.getReportType() == SomaticReportDialog::report_type::cfDNA)
	{
		try
		{
			QStringList errors;
			CfdnaDiseaseCourseTable table = GSvarHelper::cfdnaTable(ps_tumor, errors, true);
			SomaticcfDNAReportData data(somatic_report_settings_, table);
			SomaticcfDnaReport report(data);

			QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
			report.writeRtf(temp_filename);
			Helper::moveFile(temp_filename, file_rep);
			QApplication::restoreOverrideCursor();

			if (QMessageBox::question(this, "cfDNA report", "cfDNA report generated successfully!\nDo you want to open the report in your default RTF viewer?")==QMessageBox::Yes)
			{
				QDesktopServices::openUrl(QUrl::fromLocalFile(file_rep));
			}
		}
		catch (Exception& error)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error while gathering data for somatic cfDNA report.", error.message());
			return;
		}
	}
	else
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, "Unknown somatic report type!", "Unknown somatic report type! This should not happen please inform the bioinformatic team.");
		return;
	}
}

void MainWindow::generateReportGermline()
{
	//check that sample is in NGSD
	NGSD db;
	QString ps_name = germlineReportSample();
	QString sample_id = db.sampleId(ps_name, false);
	QString processed_sample_id = db.processedSampleId(ps_name, false);
	if (sample_id.isEmpty() || processed_sample_id.isEmpty())
	{
		GUIHelper::showMessage("Error", "Sample not found in the NGSD.\nCannot generate a report for samples that are not in the NGSD!");
		return;
	}

	//check if there are unclosed gaps
	QStringList unclosed_gap_ids = db.getValues("SELECT id FROM gaps WHERE processed_sample_id='" + processed_sample_id + "' AND (status='to close' OR status='in progress')");
	if (unclosed_gap_ids.count()>0 && QMessageBox::question(this, "Not all gaps closed", "There are gaps for this sample, which still have to be closed!\nDo you want to continue?")==QMessageBox::No)
	{
		return;
	}

	//show report dialog
	ReportDialog dialog(ps_name, report_settings_, variants_, cnvs_, svs_, res_, ui_.filters->targetRegion(), this);
	if (!dialog.exec()) return;

	//set report type
	report_settings_.report_type = dialog.type();

	//get export file name
	QString trio_suffix = (variants_.type() == GERMLINE_TRIO ? "trio_" : "");
	QString type_suffix = dialog.type();
	if (type_suffix!="all") type_suffix = type_suffix.replace(" ", "_") + "s";
	QString roi_name = ui_.filters->targetRegion().name;
	if (roi_name!="") //remove date and prefix with '_'
	{
        roi_name.remove(QRegularExpression("_[0-9]{4}_[0-9]{2}_[0-9]{2}"));
		roi_name = "_" + roi_name;
	}
	QString file_rep = QFileDialog::getSaveFileName(this, "Export report file", last_report_path_ + "/" + ps_name + roi_name + "_report_" + trio_suffix + type_suffix + "_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (file_rep=="") return;
	last_report_path_ = QFileInfo(file_rep).absolutePath();

	//prepare report generation data
	PrsTable prs_table;
	FileLocationList prs_files = GlobalServiceProvider::fileLocationProvider().getPrsFiles(false).filterById(ps_name);
	if (prs_files.count()==1) prs_table.load(prs_files[0].filename);

	GermlineReportGeneratorData data(GSvarHelper::build(), ps_name, variants_, cnvs_, svs_, res_, prs_table, report_settings_, ui_.filters->filters(), GSvarHelper::preferredTranscripts(), GlobalServiceProvider::statistics());
	data.processing_system_roi = GlobalServiceProvider::database().processingSystemRegions(db.processingSystemIdFromProcessedSample(ps_name), false);
	data.ps_bam = GlobalServiceProvider::database().processedSamplePath(processed_sample_id, PathType::BAM).filename;
	data.ps_lowcov = GlobalServiceProvider::database().processedSamplePath(processed_sample_id, PathType::LOWCOV_BED).filename;
	if (ui_.filters->targetRegion().isValid())
	{
		data.roi = ui_.filters->targetRegion();
		data.roi.genes = db.genesToApproved(data.roi.genes, true);
	}

	//start worker in background
	ReportWorker* worker = new ReportWorker(data, file_rep);
	startJob(worker, true);
}

void MainWindow::openProcessedSampleTabsCurrentAnalysis()
{
	if (filename_=="") return;

	SampleHeaderInfo infos = variants_.getSampleHeader();
	foreach(const SampleInfo& info, infos)
	{
		openProcessedSampleTab(info.name);
	}
}

void MainWindow::on_actionOpenProcessedSampleTabByName_triggered()
{
	ProcessedSampleSelector dlg(this, false);
	if (!dlg.exec()) return;

	QString ps_name = dlg.processedSampleName();
	if (ps_name.isEmpty()) return;

	openProcessedSampleTab(ps_name);
}

void MainWindow::on_actionOpenSequencingRunTabByName_triggered()
{
	//create
	DBSelector* selector = new DBSelector(this);
	NGSD db;
	selector->fill(db.createTable("sequencing_run", "SELECT id, name FROM sequencing_run"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select sequencing run", "run:", true);
	if (dlg->exec()==QDialog::Rejected) return ;

	//handle invalid name
	if (selector->getId()=="") return;

	openRunTab(selector->text());
}

QString MainWindow::selectGene()
{
	//create
	DBSelector* selector = new DBSelector(GUIHelper::mainWindow());
	NGSD db;
	selector->fill(db.createTable("gene", "SELECT id, symbol FROM gene"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select gene", "symbol (or transcript name):", true);
	if (dlg->exec()==QDialog::Rejected) return "";

	//handle invalid gene name > check if it is a transcript name
	if (selector->getId()=="")
	{
		int gene_id = db.geneIdOfTranscript(selector->text().toUtf8(), false, GSvarHelper::build());
		if (gene_id!=-1)
		{
			return db.geneSymbol(gene_id);
		}
	}

	return selector->text();
}

QString MainWindow::selectProcessedSample()
{
	//determine processed sample names
	QStringList ps_list;
	foreach(const SampleInfo& info, variants_.getSampleHeader())
	{
		ps_list << info.name.trimmed();
	}

	//no samples => error
	if (ps_list.isEmpty())
	{
		THROW(ProgrammingException, "selectProcessedSample() cannot be used if there is no variant list loaded!");
	}

	//one sample => auto-select
	if (ps_list.count()==1)
	{
	   return ps_list[0];
	}

	//several affected => let user select
	bool ok = false;
	QString selected = QInputDialog::getItem(this, "Select processed sample", "processed sample:", ps_list, 0, false, &ok);
	if (ok) return selected;

	return "";
}

const TargetRegionInfo& MainWindow::targetRegion()
{
	return ui_.filters->targetRegion();
}

const VariantList& MainWindow::getSmallVariantList()
{
	return variants_;
}

const CnvList& MainWindow::getCnvList()
{
	return cnvs_;
}

const BedpeFile& MainWindow::getSvList()
{
	return svs_;
}

const RepeatLocusList& MainWindow::getReList()
{
	return res_;
}

FilterWidget* MainWindow::filterWidget()
{
	return ui_.filters;
}

void MainWindow::on_actionOpenGeneTabByName_triggered()
{
	QString symbol = selectGene();
	if (symbol=="") return;

	openGeneTab(symbol);
}


void MainWindow::on_actionOpenVariantTab_triggered()
{
	try
	{
		VariantOpenDialog dlg(this);
		if (dlg.exec()!=QDialog::Accepted) return;

		Variant v = dlg.variant();

		//check if variant is in NGSD
		NGSD db;
		QString v_id = db.variantId(v, false);
		if (v_id.isEmpty())
		{
			int res = QMessageBox::question(this, "Add variant to NGSD?", "Variant '" + v.toString() + "' not found in NGSD.\nDo you want to add it?");
			if (res!=QMessageBox::Yes) return;

			db.addVariant(v);
		}

		openVariantTab(v);
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Error opening variant", e.message());
	}
}

void MainWindow::on_actionOpenProcessingSystemTab_triggered()
{
	//create
	DBSelector* selector = new DBSelector(this);
	NGSD db;
	selector->fill(db.createTable("processing_system", "SELECT id, CONCAT(name_manufacturer, ' (', name_short, ')') FROM processing_system"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select processing system", "name:", true);
	if (dlg->exec()==QDialog::Rejected) return;

	//handle invalid name
	if (selector->getId()=="") return;

	openProcessingSystemTab(db.getValue("SELECT name_short FROM processing_system WHERE id=" + selector->getId()).toString());
}

void MainWindow::on_actionOpenProjectTab_triggered()
{
	//create
	DBSelector* selector = new DBSelector(this);
	NGSD db;
	selector->fill(db.createTable("project", "SELECT id, name FROM project"));

	//show
	auto dlg = GUIHelper::createDialog(selector, "Select project", "project:", true);
	if (dlg->exec()==QDialog::Rejected) return ;

	//handle invalid name
	if (selector->getId()=="") return;

	openProjectTab(selector->text());
}

void MainWindow::on_actionImportHerediVar_triggered()
{
	HerediVarImportDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionStatistics_triggered()
{
	try
	{
		LoginManager::checkRoleIn(QStringList{"admin", "user"});
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Permissions error", e.message());
		return;
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	NGSD db;
	TsvFile table;
	bool human_only = true;

	//table header
	table.addHeader("month");
	QStringList sys_types = QStringList() << "WGS" << "WES" << "Panel" << "RNA" << "lrGS";
	QStringList pro_types = QStringList() << "diagnostic" << "research";
	foreach(QString pro, pro_types)
	{
		foreach(QString sys, sys_types)
		{
			table.addHeader(sys + " " + pro);
		}
	}

	//table rows
	QSet<QString> comments;
	QDate start = QDate::currentDate();
	start = start.addDays(1-start.day());
	QDate end = start.addMonths(1);
	while(start.year()>=2015)
	{
		QVector<int> counts(table.columnCount(), 0);

		//select runs of current month
		SqlQuery q_run_ids = db.getQuery();
		q_run_ids.exec("SELECT id FROM sequencing_run WHERE end_date >='" + start.toString(Qt::ISODate) + "' AND end_date < '" + end.toString(Qt::ISODate) + "' AND status!='analysis_not_possible' AND status!='run_aborted' AND status!='n/a' AND quality!='bad'");
		while(q_run_ids.next())
		{
			//select samples
			SqlQuery q_sample_data = db.getQuery();
			q_sample_data.exec("SELECT sys.type, p.type FROM sample s, processed_sample ps, processing_system sys, project p WHERE " + QString(human_only ? " s.species_id=(SELECT id FROM species WHERE name='human') AND " : "") + " ps.processing_system_id=sys.id AND ps.sample_id=s.id AND ps.project_id=p.id AND ps.sequencing_run_id=" + q_run_ids.value(0).toString() + " AND ps.id NOT IN (SELECT processed_sample_id FROM merged_processed_samples)");

			//count
			while(q_sample_data.next())
			{
				QString sys_type = q_sample_data.value(0).toString();
				if (sys_type.contains("Panel")) sys_type = "Panel";
				if (!sys_types.contains(sys_type))
				{
					comments << "##Skipped processing system type '" + sys_type + "'";
					continue;
				}
				QString pro_type = q_sample_data.value(1).toString();
				if (!pro_types.contains(pro_type))
				{
					comments << "##Skipped project type '" + pro_type + "'";
					continue;
				}

				int index = table.columnIndex(sys_type + " " + pro_type, false);
				++counts[index];
			}
		}

		//create row
		QStringList row;
		for(int i=0; i<counts.count(); ++i)
		{
			if (i==0)
			{
				row << start.toString("yyyy/MM");
			}
			else
			{
				row << QString::number(counts[i]);
			}
		}
		table.addRow(row);

		//next month
		start = start.addMonths(-1);
		end = end.addMonths(-1);
	}

	//comments
	foreach(QString comment, comments)
	{
		table.addComment(comment);
	}

	QApplication::restoreOverrideCursor();

	//show dialog
	TsvTableWidget* widget = new TsvTableWidget(table, this);
	widget->setMinimumWidth(850);
	auto dlg = GUIHelper::createDialog(widget, "Statistics", "Sequencing statistics grouped by month (human)");
	dlg->exec();
}

void MainWindow::on_actionDevice_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("device");
	auto dlg = GUIHelper::createDialog(widget, "Device administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionGenome_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("genome");
	auto dlg = GUIHelper::createDialog(widget, "Genome administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionMID_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("mid");
	auto dlg = GUIHelper::createDialog(widget, "MID administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionProcessingSystem_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("processing_system");
	auto dlg = GUIHelper::createDialog(widget, "Processing system administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionProject_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("project");
	auto dlg = GUIHelper::createDialog(widget, "Project administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionRepeatExpansion_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("repeat_expansion");
	auto dlg = GUIHelper::createDialog(widget, "Repeat expansion administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionReportPolymorphisms_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("report_polymorphisms");
	auto dlg = GUIHelper::createDialog(widget, "Report polymorphisms administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSample_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("sample");
	auto dlg = GUIHelper::createDialog(widget, "Sample administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSampleGroup_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("sample_group");
	auto dlg = GUIHelper::createDialog(widget, "Sample group administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSender_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("sender");
	auto dlg = GUIHelper::createDialog(widget, "Sender administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSpecies_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("species");
	auto dlg = GUIHelper::createDialog(widget, "Species administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionUsers_triggered()
{
	try
	{
		LoginManager::checkRoleIn(QStringList{"admin"});
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Permissions error", e.message());
		return;
	}

	//show user table
	DBTableAdministration* widget = new DBTableAdministration("user");
	auto dlg = GUIHelper::createDialog(widget, "User administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionExportTestData_triggered()
{
	NGSD db;
	QMap<QString, QSet<int>> sql_history;
    QElapsedTimer timer;
	QStringList base_tables = {
		"user",
		"device",
		"disease_term",
		"disease_gene",
		"gene",
		"gene_alias",
		"gene_transcript",
		"gene_exon",
		"gene_pseudogene_relation",
		"geneinfo_germline",
		"genome",
		"hpo_term",
		"hpo_parent",
		"hpo_genes",
		"mid",
		"omim_gene",
		"omim_phenotype",
		"omim_preferred_phenotype",
		"preferred_transcripts",
		"processing_system",
		"project",
		"qc_terms",
        "repeat_expansion",
		"sender",
		"sequencing_run",
		"somatic_pathway",
		"somatic_pathway_gene",
		"somatic_gene_role",
		"runqc_read",
		"runqc_lane",
        "species"
	};

	try
	{
		LoginManager::checkRoleIn(QStringList{"admin", "user"});

		//get samples from user
		bool ok = false;
		QString ps_text = QInputDialog::getMultiLineText(this, "Test data export", "List the processed samples (one per line):", "", &ok);
		if (!ok) return;

		//check processed sample list
		QStringList ps_list;
		foreach(const QString& ps, ps_text.split("\n"))
		{
			if (ps.trimmed().isEmpty()) continue;

			QString ps_id = db.processedSampleId(ps);
			QString project_type = db.getProcessedSampleData(ps_id).project_type;
			if (project_type!="test")
			{
				THROW(ArgumentException, "Processes sample '" + ps + "' has project type '" +project_type + "', but only samples from type 'test' can be exported!");
			}
			ps_list << ps;
		}

		//get and open output file
		QString file_name = QFileDialog::getSaveFileName(this, "Export database tables", QDir::homePath()+QDir::separator()+"db_data_"+QDateTime::currentDateTime().toString("dd_MM_yyyy")+".sql", "SQL (*.sql);;All files (*.*)");
		if (file_name.isEmpty()) return;

		QSharedPointer<QFile> file = Helper::openFileForWriting(file_name, false);
        QTextStream output_stream(file.data());
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        output_stream.setEncoding(QStringConverter::Utf8);
        #else
        output_stream.setCodec("UTF-8");
        #endif

		QApplication::setOverrideCursor(Qt::BusyCursor);

		timer.start();
		for (int i = 0; i < base_tables.count(); i++)
		{
			ui_.statusBar->showMessage("Exporting table \"" + base_tables[i] + "\"");
			QApplication::processEvents();
			db.exportTable(base_tables[i], output_stream);
		}
		Log::perf("Exporting base tables took ", timer);

		timer.start();
		foreach(const QString& ps, ps_list)
		{
			ui_.statusBar->showMessage("Exporting data of " + ps);
			QApplication::processEvents();

			QString s_id = db.sampleId(ps);
			QString ps_id = db.processedSampleId(ps);
			db.exportTable("sample", output_stream, "id='"+s_id+"'", &sql_history);
			db.exportTable("sample_disease_info", output_stream, "sample_id='"+s_id+"'", &sql_history);
			db.exportTable("processed_sample", output_stream, "id='"+ps_id+"'", &sql_history);
			db.exportTable("processed_sample_qc", output_stream, "processed_sample_id='"+ps_id+"'", &sql_history);
            db.exportTable("repeat_expansion_genotype", output_stream, "processed_sample_id='"+ps_id+"'", &sql_history);

			QStringList variant_id_list = db.getValues("SELECT variant_id FROM detected_variant WHERE processed_sample_id='"+ps_id+"'");
			db.exportTable("variant", output_stream, "id IN ("+variant_id_list.join(", ")+")", &sql_history);
			db.exportTable("detected_variant", output_stream, "processed_sample_id='"+ps_id+"'", &sql_history);

			QString ps_cnv_id = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", true, ps_id).toString();
			if (!ps_cnv_id.isEmpty())
			{
				db.exportTable("cnv_callset", output_stream, "id="+ps_cnv_id, &sql_history);
				db.exportTable("cnv", output_stream, "cnv_callset_id="+ps_cnv_id, &sql_history);
			}

			QString sv_callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id='"+ps_id+"'", true).toString();
			if (!sv_callset_id.isEmpty())
			{
				db.exportTable("sv_callset", output_stream, "id="+sv_callset_id, &sql_history);
				db.exportTable("sv_deletion", output_stream, "sv_callset_id="+sv_callset_id, &sql_history);
				db.exportTable("sv_duplication", output_stream, "sv_callset_id="+sv_callset_id, &sql_history);
				db.exportTable("sv_insertion", output_stream, "sv_callset_id="+sv_callset_id, &sql_history);
				db.exportTable("sv_inversion", output_stream, "sv_callset_id="+sv_callset_id, &sql_history);
				db.exportTable("sv_translocation", output_stream, "sv_callset_id="+sv_callset_id, &sql_history);
			}
		}
		Log::perf("Exporting processed sample data took ", timer);

		QApplication::restoreOverrideCursor();

		QMessageBox::information(this, "Test data export", "Exported test data to " + file_name);
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "NGSD export error");
	}
}

void MainWindow::on_actionExportSampleData_triggered()
{	
	NGSD db;
	QElapsedTimer timer;

	try
	{
		LoginManager::checkRoleIn(QStringList{"admin", "user"});

		//get samples from user
		ProcessedSampleSelector dlg(this, false);
		dlg.showSearchMulti(true);
		if (!dlg.exec()) return;

		QString ps_name = dlg.processedSampleName();
		if (ps_name.isEmpty()) return;

		QString ps_id = db.processedSampleId(ps_name.trimmed());

		//get and open output file
		QString file_name = QFileDialog::getSaveFileName(this, "Export sample data", QDir::homePath()+QDir::separator()+ps_name+"_data_"+QDateTime::currentDateTime().toString("dd_MM_yyyy")+".sql", "SQL (*.sql);;All files (*.*)");
		if (file_name.isEmpty()) return;

		QSharedPointer<QFile> file = Helper::openFileForWriting(file_name, false);
		QTextStream output_stream(file.data());
		#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
		output_stream.setEncoding(QStringConverter::Utf8);
		#else
		output_stream.setCodec("UTF-8");
		#endif

		QApplication::setOverrideCursor(Qt::BusyCursor);
		timer.start();

		QList<QString> sample_db_data; // store all SQL statements here

		db.exportSampleData(ps_id, sample_db_data);

		for (QString single_query: sample_db_data)
		{
			output_stream << single_query.replace("\n", "\\n") << ";\n";
		}

		Log::perf("Exporting processed sample data took ", timer);

		QApplication::restoreOverrideCursor();
		QMessageBox::information(this, "Sample data export", "Exported sample data to " + file_name);
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "NGSD export error");
	}



}

void MainWindow::on_actionImportSequencingRuns_triggered()
{
	ImportDialog dlg(this, ImportDialog::RUNS);
	dlg.exec();
}

void MainWindow::on_actionImportTestData_triggered()
{
	NGSD db;

	try
	{
		//check role
		LoginManager::checkRoleIn(QStringList{"admin", "user"});

		//prevent overriding the production database
		if (db.isProductionDb())
		{
			THROW(DatabaseException, "Cannot import test data into a production database (see db_info table)!");
		}

		//get input file
		QString file_name = QFileDialog::getOpenFileName(this, "Import SQL data", QDir::homePath(), "SQL (*.sql);;All files (*.*)");
		if (file_name.isEmpty()) return;

		//import
		QApplication::setOverrideCursor(Qt::BusyCursor);
		db.removeInitData();

		QString query;
		QSharedPointer<QFile> file = Helper::openFileForReading(file_name, false);
		while(!file->atEnd())
		{
			QString line = file->readLine().trimmed();
			if (line.isEmpty()) continue;

			//comments > show in status bar
			if (line.startsWith("--"))
			{
				line = line.replace("--", "").trimmed();
				if (!line.isEmpty())
				{
					ui_.statusBar->showMessage("Importing \"" + line + "\"");
					QApplication::processEvents();
				}
				continue;
			}

			//add line to query
			query.append(' ');
			query.append(line);

			//execute if query finished
			if (query.endsWith(';'))
			{
				db.getQuery().exec(query);
				query.clear();
			}
		}

		QApplication::restoreOverrideCursor();

		QMessageBox::information(this, "Test data import", "Import is complete");
	}
	catch (Exception& e)
	{
		GUIHelper::showException(this, e, "NGSD import error");
	}
}

void MainWindow::on_actionImportMids_triggered()
{
	ImportDialog dlg(this, ImportDialog::MIDS);
	dlg.exec();
}

void MainWindow::on_actionImportStudy_triggered()
{
	ImportDialog dlg(this, ImportDialog::STUDY_SAMPLE);
	dlg.exec();
}
void MainWindow::on_actionImportSamples_triggered()
{
	ImportDialog dlg(this, ImportDialog::SAMPLES);
	dlg.exec();
}

void MainWindow::on_actionImportProcessedSamples_triggered()
{
	ImportDialog dlg(this, ImportDialog::PROCESSED_SAMPLES);
	dlg.exec();
}

void MainWindow::on_actionImportSampleRelations_triggered()
{
	ImportDialog dlg(this, ImportDialog::SAMPLE_RELATIONS);
	dlg.exec();
}

void MainWindow::on_actionImportSampleHpoTerms_triggered()
{
	ImportDialog dlg(this, ImportDialog::SAMPLE_HPOS);
	dlg.exec();
}

void MainWindow::on_actionImportCfDNAPanels_triggered()
{
	CfDNAPanelBatchImport* dlg = new CfDNAPanelBatchImport(this);
	dlg->exec();
}

void MainWindow::on_actionMidClashDetection_triggered()
{
	MidCheckWidget* widget = new MidCheckWidget();
	auto dlg = GUIHelper::createDialog(widget, "MID clash detection");
	dlg->exec();
}

void MainWindow::on_actionVariantValidation_triggered()
{
	VariantValidationWidget* widget = new VariantValidationWidget();
	auto dlg = GUIHelper::createDialog(widget, "Variant validation");
	dlg->exec();
}

void MainWindow::on_actionChangePassword_triggered()
{
	PasswordDialog dlg(this);
	if(dlg.exec()==QDialog::Accepted)
	{
		NGSD db;
		db.setPassword(LoginManager::userId(), dlg.password());
	}
}

void MainWindow::on_actionStudy_triggered()
{
	DBTableAdministration* widget = new DBTableAdministration("study");
	auto dlg = GUIHelper::createDialog(widget, "Study administration");
	addModelessDialog(dlg);
}

void MainWindow::on_actionSampleCounts_triggered()
{
	if (!LoginManager::active()) return;

	SampleCountWidget* widget = new SampleCountWidget();
	auto dlg = GUIHelper::createDialog(widget, "Sample counts");
	addModelessDialog(dlg);
}

void MainWindow::on_actionGaps_triggered()
{
	GapClosingDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionPrepareGhgaUpload_triggered()
{
	GHGAUploadDialog dlg(this);
	dlg.exec();
}

void MainWindow::on_actionMaintenance_triggered()
{
	try
	{
		LoginManager::checkRoleIn(QStringList{"admin"});
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Permissions error", e.message());
		return;
	}


	MaintenanceDialog* dlg = new MaintenanceDialog(this);
	dlg->exec();
}

void MainWindow::on_actionNotifyUsers_triggered()
{
	try
	{
		LoginManager::checkRoleIn(QStringList{"admin"});
	}
	catch (Exception& e)
	{
		QMessageBox::warning(this, "Permissions error", e.message());
		return;
	}

	NGSD db;
	QStringList to, body;
	to = db.getValues("SELECT email from user WHERE user_role<>'special' AND active='1'");

	QStringList excluded_emails = {"restricted_user@med.uni-tuebingen.de"};
	foreach (QString email, excluded_emails)
	{
		int email_pos = to.indexOf(email);
		if (email_pos>-1) to.removeAt(email_pos);
	}

	QString subject = "GSvar update";
	body << "Dear all,";
	body << "";
	body << "";
	body << "Best regards,";
	body << LoginManager::userName();

	EmailDialog dlg(this, to, subject, body);
	dlg.exec();
}


void MainWindow::on_actionCohortAnalysis_triggered()
{
	CohortAnalysisWidget* widget = new CohortAnalysisWidget(this);
	auto dlg = GUIHelper::createDialog(widget, "Cohort analysis");
	addModelessDialog(dlg);
}



void MainWindow::on_actionGenderXY_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("Determine gender", "xy", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionGenderHet_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("Determine gender", "hetx", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionGenderSRY_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("Determine gender", "sry", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionStatisticsBED_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("BED file information", "", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionSampleSimilarityGSvar_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("Sample similarity", "gsvar", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionSampleSimilarityVCF_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("Sample similarity", "vcf", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionSampleSimilarityBAM_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("Sample similarity", "bam", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionSampleAncestry_triggered()
{
	QSharedPointer<ExternalToolDialog> dlg(new ExternalToolDialog("Sample ancestry", "", this));
	addModelessDialog(dlg);
}

void MainWindow::on_actionAnalysisStatus_triggered()
{
	TabType type = TabType::ANALYSIS_STATUS;
	if (focusTab(type, "Analysis status")) return;

	//open new
	AnalysisStatusWidget* widget = new AnalysisStatusWidget(this);
	connect(widget, SIGNAL(loadFile(QString)), this, SLOT(loadFile(QString)));
	openTab(QIcon(":/Icons/Server.png"), "Analysis status", type, widget);
}

void MainWindow::calculateGapsByTargetRegionFilter()
{
	if (filename_=="" || !LoginManager::active()) return;

	QString title = "Gaps by target region filter";

	if (!ui_.filters->targetRegion().isValid())
	{
		QMessageBox::warning(this, title, "No target region filter set!");
		return;
	}

	showGapsClosingDialog(title, ui_.filters->targetRegion().regions, ui_.filters->targetRegion().genes);
}

void MainWindow::calculateGapsByGenes()
{
	if (filename_=="" || !LoginManager::active()) return;

	QString title = "Gaps by gene(s)";

	QString text = QInputDialog::getMultiLineText(this, title, "genes (one per line):");
	if (text=="") return;

	//convert to regions
	QApplication::setOverrideCursor(Qt::BusyCursor);
	NGSD db;
	BedFile regions;
	GeneSet genes = GeneSet::createFromStringList(text.split("\n"));
	foreach(const QByteArray& gene, genes)
	{
		regions.add(db.geneToRegions(gene, Transcript::ENSEMBL, "gene", true, false));
	}
	regions.extend(5000);
	regions.merge();
	QApplication::restoreOverrideCursor();

	showGapsClosingDialog(title, regions, genes);
}

void MainWindow::showGapsClosingDialog(QString title, const BedFile& regions, const GeneSet& genes)
{
	//only available for certain types
	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE && type!=SOMATIC_SINGLESAMPLE)
	{
		QMessageBox::warning(this, title, "Only available for germline single/multi/trio analysis or somatic tumor-only!");
		return;
	}

	//check for BAM file
	QString ps = (type==SOMATIC_SINGLESAMPLE) ? variants_.getSampleHeader()[0].name : germlineReportSample();
	QStringList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(ps).asStringList();
	if (bams.empty())
	{
		QMessageBox::warning(this, title, "No BAM/CRAM file found for sample " + ps + "!");
		return;
	}

	//show dialog
	QStringList low_covs = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(false).filterById(ps).asStringList();
	if (low_covs.isEmpty()) low_covs << ""; //add empty string in case there is no low-coverage file > this case is handled inside the dialog
	GapDialog dlg(this, ps, bams[0], low_covs[0], regions, genes);
	dlg.exec();
}

void MainWindow::exportVCF()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//generate GSvar with variants passing filter only
		VariantList selected_variants;
		selected_variants.copyMetaData(variants_);
		for(int i=0; i<variants_.count(); ++i)
		{
			if (!filter_result_.passing(i)) continue;
			selected_variants.append(variants_[i]);
		}

		//convert to VCF
		QString ref_genome = Settings::string("reference_genome", false);
		VcfFile vcf_file = VcfFile::fromGSvar(selected_variants, ref_genome);

		//store
		QString folder = Settings::path("gsvar_variant_export_folder", true);
		QString file_name = folder + QDir::separator() + QFileInfo(filename_).baseName() + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".vcf";

		file_name = QFileDialog::getSaveFileName(this, "Export VCF", file_name, "VCF (*.vcf);;All files (*.*)");
		if (file_name!="")
		{
			vcf_file.store(file_name);
			QApplication::restoreOverrideCursor();
			QMessageBox::information(this, "VCF export", "Exported VCF file with " + QString::number(vcf_file.count()) + " variants.");
		}
		else
		{
			QApplication::restoreOverrideCursor();
		}
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, "VCF export error", e.message());
	}
}

void MainWindow::exportHerediCareVCF()
{
	//init
	QString title = "HerediCare VCF export";
	GeneSet genes;
	genes << "ABRAXAS1" << "APC" << "ATM" << "BARD1" << "BRCA1" << "BRCA2" << "BRIP1" << "CDH1" << "CDKN2A" << "CHEK2" << "EPCAM" << "FANCC" << "FANCM" << "HOXB13" << "MEN1" << "MLH1" << "MRE11" << "MSH2" << "MSH6" << "MUTYH" << "NBN" << "NF1" << "NTHL1" << "PALB2" << "PMS2" << "POLD1" << "POLE" << "PTEN" << "RAD50" << "RAD51C" << "RAD51D" << "SMARCA4" << "STK11" << "TP53";
	FastaFileIndex ref_genome(Settings::string("reference_genome", false));

	//check it is a germine analysis
	if (variants_.type()!=GERMLINE_SINGLESAMPLE)
	{
		QMessageBox::information(this, title, "This functionality is only available for germline single sample analysis.");
		return;
	}

	try
	{
		//get HerediCare ID from user
		QString id = QInputDialog::getText(this, title, "HerediCare ID (used in VCF header):");
		if (id.isEmpty()) return;

		//get export file name from user
		QString file_name = Settings::path("gsvar_variant_export_folder", true) + QDir::separator() + id + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".vcf";
		file_name = QFileDialog::getSaveFileName(this, title, file_name, "VCF (*.vcf);;All files (*.*)");
		if (file_name.isEmpty()) return;

		//convert gene list to exon regions
		NGSD db;
		BedFile roi = db.genesToRegions(genes, Transcript::ENSEMBL, "exon");
		roi.extend(20);
		roi.merge();
		ChromosomalIndex<BedFile> roi_idx(roi);

		//create VCF file and add header data
		VcfFile vcf;
		vcf.vcfHeader().addInfoLine(InfoFormatLine{"CLASS", "1", "String", "ACMG classification"});
		vcf.vcfHeader().addFormatLine(InfoFormatLine{"GT", "1", "String", "Genotype in the sample."});
		vcf.vcfHeader().addFormatLine(InfoFormatLine{"DP", "1", "Integer", "Depth at the variant location."});
		vcf.vcfHeader().addFormatLine(InfoFormatLine{"AF", "1", "Float", "Allele frequency in the sample."});
		vcf.setSampleNames(QByteArrayList() << id.toUtf8());

		//add variants in ROI to VCF
		int i_qual = variants_.annotationIndexByName("quality");
		int i_geno = variants_.getSampleHeader()[0].column_index;
		int c_classified = 0;
		for(int i=0; i<variants_.count(); ++i)
		{
			const Variant& v = variants_[i];

			//check ROI
			if (roi_idx.matchingIndex(v.chr(), v.start(), v.end())==-1) continue;

			VcfLine v2 = v.toVCF(ref_genome, i_geno);

			//add quality, DP and AF
			QByteArray qual;
			QByteArray dp;
			QByteArray af;
			foreach(QByteArray entry, v.annotations()[i_qual].split(';'))
			{
				entry = entry.trimmed();
				if (entry.startsWith("QUAL=")) qual = entry.mid(5);
				if (entry.startsWith("DP=")) dp = entry.mid(3);
				if (entry.startsWith("AF=")) af = entry.mid(3);
			}
			v2.setQual(Helper::toDouble(qual, "QUAL"));
			v2.addFormatKeys(QByteArrayList() << "DP" << "AF");
			v2.setFormatValues(0, QByteArrayList() << v2.formatValueFromSample("GT") << dp << af);

			//add ACMG class
			QByteArray c = db.getClassification(v).classification.toUtf8();
			if (!c.isEmpty())
			{
				v2.setInfo(QByteArrayList() << "CLASS", QByteArrayList() << c);
				++c_classified;
			}

			vcf.append(v2);
		}

		vcf.store(file_name);
		QMessageBox::information(this, title, "Exported " + QString::number(vcf.count()) + " variants in exons/splice region of the " + QString::number(genes.count()) + " genes:\n" + genes.join(", ") + "\n\n" + QString::number(c_classified) + " of the variants have a classification.");
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, title, e.message());
	}
}

void MainWindow::exportGSvar()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		//create new GSvar file with passing variants
		VariantList output;
		output.copyMetaData(variants_);
		for(int i=0; i<variants_.count(); ++i)
		{
			if (filter_result_.passing(i))
			{
				output.append(variants_[i]);
			}
		}

		//store
		QString folder = Settings::path("gsvar_variant_export_folder", true);
		QString file_name = folder + QDir::separator() + QFileInfo(filename_).baseName() + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".GSvar";

		file_name = QFileDialog::getSaveFileName(this, "Export GSvar", file_name, "VCF (*.GSvar);;All files (*.*)");
		if (file_name!="")
		{
			output.store(file_name);
			QApplication::restoreOverrideCursor();
			QMessageBox::information(this, "GSvar export", "Exported GSvar file with " + QString::number(output.count()) + " variants.");
		}
		else
		{
			QApplication::restoreOverrideCursor();
		}

	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, "GSvar export error", e.message());
	}
}

void MainWindow::on_actionPreferredTranscripts_triggered()
{
	PreferredTranscriptsWidget* widget = new PreferredTranscriptsWidget();
	auto dlg = GUIHelper::createDialog(widget, "Preferred transcripts");
	dlg->exec();

	//re-load preferred transcripts from NGSD
	GSvarHelper::preferredTranscripts(true);
}

void MainWindow::on_actionEditSomaticGeneRoles_triggered()
{
	DBTableAdministration* table = new DBTableAdministration("somatic_gene_role");
	auto dlg = GUIHelper::createDialog(table, "Somatic Gene Roles");
	addModelessDialog(dlg);
}

void MainWindow::on_actionEditSomaticPathways_triggered()
{
	DBTableAdministration* table = new DBTableAdministration("somatic_pathway");
	auto dlg = GUIHelper::createDialog(table, "Somatic pathways");
	addModelessDialog(dlg);
}

void MainWindow::on_actionEditSomaticPathwayGeneAssociations_triggered()
{
	DBTableAdministration* table = new DBTableAdministration("somatic_pathway_gene");
	auto dlg = GUIHelper::createDialog(table, "Somatic pathways-gene associations");
	addModelessDialog(dlg);
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

void MainWindow::on_actionGeneInterpretability_triggered()
{
	QString title = "Gene interpretability";
	try
	{
		QStringList interpretability_regions = Settings::stringList("interpretability_regions");
		if (interpretability_regions.isEmpty()) INFO(Exception, "GSvar settings entry 'interpretability_regions' is empty!");

		QList<GeneInterpretabilityRegion> regions;
		foreach(QString entry, interpretability_regions)
		{
			QStringList parts = entry.split('\t');
			if (parts.count()!=2) THROW(Exception, "GSvar settings entry 'interpretability_regions' has invalid entry: " + entry);

			if (!QFile::exists(parts[1])) THROW(Exception, "GSvar settings entry 'interpretability_regions' has entry with non-existing BED file: " + parts[1]);
			regions << GeneInterpretabilityRegion{parts[0], parts[1]};
		}

		GeneInterpretabilityDialog dlg(this, regions);
		dlg.setWindowTitle(title);
		dlg.exec();
	}
	catch(Exception& e)
	{
		GUIHelper::showException(this, e, title);
	}
}

void MainWindow::openSubpanelDesignDialog(const GeneSet& genes)
{
	SubpanelDesignDialog dlg(this);
	dlg.setGenes(genes);

	dlg.exec();

	if (dlg.lastCreatedSubPanel()!="")
	{
		//update target region list
		ui_.filters->loadTargetRegions();

		//optionally use sub-panel as target regions
		if (QMessageBox::question(this, "Use sub-panel?", "Do you want to set the sub-panel as target region?")==QMessageBox::Yes)
		{
			ui_.filters->setTargetRegionByDisplayName(dlg.lastCreatedSubPanel());
		}
	}
}

void MainWindow::on_actionManageSubpanels_triggered()
{
	SubpanelArchiveDialog dlg(this);
	dlg.exec();
	if (dlg.changedSubpanels())
	{
		ui_.filters->loadTargetRegions();
	}
}

QString MainWindow::nobr()
{
	return "<p style='white-space:pre; margin:0; padding:0;'>";
}

void MainWindow::uploadToClinvar(int variant_index1, int variant_index2)
{
	if (!LoginManager::active()) return;

	try
	{
		if(variant_index1 < 0)
		{
			THROW(ArgumentException, "A valid variant index for the first variant has to be provided!");
		}
		//abort if API key is missing
		if(Settings::string("clinvar_api_key", true).trimmed().isEmpty())
		{
			THROW(ProgrammingException, "ClinVar API key is needed, but not found in settings.\nPlease inform the bioinformatics team");
		}

		NGSD db;

		//(1) prepare data as far as we can
		ClinvarUploadData data;
		data.processed_sample = germlineReportSample();
		data.variant_type1 = VariantType::SNVS_INDELS;
		if(variant_index2 < 0)
		{
			//Single variant submission
			data.submission_type = ClinvarSubmissionType::SingleVariant;
			data.variant_type2 = VariantType::INVALID;
		}
		else
		{
			//CompHet variant submission
			data.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
			data.variant_type2 = VariantType::SNVS_INDELS;
		}

		QString sample_id = db.sampleId(data.processed_sample);
		SampleData sample_data = db.getSampleData(sample_id);


		//get disease info
		data.disease_info = db.getSampleDiseaseInfo(sample_id, "OMIM disease/phenotype identifier");
		data.disease_info.append(db.getSampleDiseaseInfo(sample_id, "Orpha number"));
		if (data.disease_info.length() < 1)
		{
			INFO(InformationMissingException, "The sample has to have at least one OMIM or Orphanet disease identifier to publish a variant in ClinVar.");
		}

		// get affected status
		data.affected_status = sample_data.disease_status;

		//get phenotype(s)
		data.phenos = sample_data.phenotypes;

		//get variant info
		data.snv1 = variants_[variant_index1];
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) data.snv2 = variants_[variant_index2];

		// get report info
		if (!report_settings_.report_config.data()->exists(VariantType::SNVS_INDELS, variant_index1))
		{
			INFO(InformationMissingException, "The variant 1 has to be in the report configuration to be published!");
		}
		data.report_variant_config1 = report_settings_.report_config.data()->get(VariantType::SNVS_INDELS, variant_index1);
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			if (!report_settings_.report_config.data()->exists(VariantType::SNVS_INDELS, variant_index2))
			{
				INFO(InformationMissingException, "The variant 2 has to be in the report configuration to be published!");
			}
			data.report_variant_config2 = report_settings_.report_config.data()->get(VariantType::SNVS_INDELS, variant_index2);
		}



		//update classification
		data.report_variant_config1.classification = db.getClassification(data.snv1).classification;
		if (data.report_variant_config1.classification.trimmed().isEmpty() || (data.report_variant_config1.classification.trimmed() == "n/a"))
		{
			INFO(InformationMissingException, "The variant 1 has to be classified to be published!");
		}
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			data.report_variant_config2.classification = db.getClassification(data.snv2).classification;
			if (data.report_variant_config2.classification.trimmed().isEmpty() || (data.report_variant_config2.classification.trimmed() == "n/a"))
			{
				INFO(InformationMissingException, "The variant 2 has to be classified to be published!");
			}
		}

		//genes
		int gene_idx = variants_.annotationIndexByName("gene");
		data.genes = GeneSet::createFromText(data.snv1.annotations()[gene_idx], ',');
		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) data.genes <<  GeneSet::createFromText(data.snv2.annotations()[gene_idx], ',');

		//determine NGSD ids of variant and report variant for variant 1
		QString var_id = db.variantId(data.snv1, false);
		if (var_id == "")
		{
			INFO(InformationMissingException, "The variant 1 has to be in NGSD and part of a report config to be published!");
		}
		data.variant_id1 = Helper::toInt(var_id);
		//extract report variant id
		int rc_id = db.reportConfigId(db.processedSampleId(data.processed_sample));
		if (rc_id == -1 )
		{
			THROW(DatabaseException, "Could not determine report config id for sample " + data.processed_sample + "!");
		}

		data.report_variant_config_id1 = db.getValue("SELECT id FROM report_configuration_variant WHERE report_configuration_id=" + QString::number(rc_id) + " AND variant_id="
													 + QString::number(data.variant_id1), false).toInt();

		if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
		{
			//determine NGSD ids of variant and report variant for variant 2
			var_id = db.variantId(data.snv2, false);
			if (var_id == "")
			{
				INFO(InformationMissingException, "The variant 2 has to be in NGSD and part of a report config to be published!");
			}
			data.variant_id2 = Helper::toInt(var_id);

			//extract report variant id
			data.report_variant_config_id2 = db.getValue("SELECT id FROM report_configuration_variant WHERE report_configuration_id=" + QString::number(rc_id) + " AND variant_id="
														 + QString::number(data.variant_id2), false).toInt();
		}


		// (2) show dialog
		ClinvarUploadDialog dlg(this);
		dlg.setData(data);
		dlg.exec();
	}
	catch(Exception& e)
	{
        GUIHelper::showException(this, e, "ClinVar submission error");
    }
}

void MainWindow::updateSecureToken()
{
    if (ClientHelper::isClientServerMode())
    {
        LoginManager::renewLogin();
        for(int i = 0; i < IgvSessionManager::count(); i++)
        {
            if (IgvSessionManager::get(i).isIgvRunning())
            {
                IgvSessionManager::get(i).execute(QStringList() << "SetAccessToken " + LoginManager::userToken() + " *" + Settings::string("server_host") + "*", false);
            }
        }
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

void MainWindow::closeEvent(QCloseEvent* event)
{
	//unload the data
	loadFile();

    if (ClientHelper::isClientServerMode()) performLogout();
	//here one could cancel closing the window by calling event->ignore()

	event->accept();
}

void MainWindow::refreshVariantTable(bool keep_widths, bool keep_heights)
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

    QElapsedTimer timer;
	timer.start();

	//apply filters
	applyFilters(false);
	int passing_variants = filter_result_.countPassing();
	QString status = QString::number(passing_variants) + " of " + QString::number(variants_.count()) + " variants passed filters.";
	int max_variants = 10000;
	if (passing_variants>max_variants)
	{
		status += " Displaying " + QString::number(max_variants) + " variants only!";
	}
	ui_.statusBar->showMessage(status);

	Log::perf("Applying all filters took ", timer);
	timer.start();

	//force update of variant details widget
	var_last_ = -1;

	//update variant table
	QList<int> col_widths = ui_.vars->columnWidths();
	AnalysisType type = variants_.type();
	if (type==SOMATIC_SINGLESAMPLE || type==SOMATIC_PAIR || type==CFDNA)
	{
		ui_.vars->update(variants_, filter_result_, somatic_report_settings_, max_variants);
	}
	else if (type==GERMLINE_SINGLESAMPLE || type==GERMLINE_TRIO || type==GERMLINE_MULTISAMPLE)
	{
		ui_.vars->update(variants_, filter_result_, report_settings_, max_variants);
	}
	else
	{
		THROW(ProgrammingException, "Unsupported analysis type in refreshVariantTable!");
	}

	//height
	if (!keep_heights)
	{
		ui_.vars->adaptRowHeights();
	}

	//widths
	if (keep_widths)
	{
		ui_.vars->setColumnWidths(col_widths);
	}
	else
	{
		ui_.vars->adaptColumnWidths();
	}
	QApplication::restoreOverrideCursor();

	Log::perf("Updating variant table took ", timer);
}


void MainWindow::varHeaderContextMenu(QPoint pos)
{
	if (!LoginManager::active()) return;

	//get variant index
	int index = ui_.vars->selectedVariantIndex();
	if(index==-1) return; //several variants selected

	//set up menu
	QMenu menu(ui_.vars->verticalHeader());
	QAction* a_edit = menu.addAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	QAction* a_delete =menu.addAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");

	if(germlineReportSupported())
	{
		a_delete->setEnabled(!report_settings_.report_config->isFinalized() && report_settings_.report_config->exists(VariantType::SNVS_INDELS, index));
	}
	else if(somaticReportSupported())
	{
		 a_delete->setEnabled(somatic_report_settings_.report_config->exists(VariantType::SNVS_INDELS, index));
	}
	else
	{
		a_delete->setEnabled(false);
	}

	//exec menu
	pos = ui_.vars->verticalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;

	//actions
	if (action==a_edit)
	{
		editVariantReportConfiguration(index);
	}
	else if (action==a_delete)
	{
		if(germlineReportSupported())
		{
			report_settings_.report_config->remove(VariantType::SNVS_INDELS, index);
		}
		else
		{
			somatic_report_settings_.report_config->remove(VariantType::SNVS_INDELS, index);
		}
		updateReportConfigHeaderIcon(index);
	}
}

void MainWindow::columnContextMenu(QPoint pos)
{
	int col_index = ui_.vars->indexAt(pos).column();
	if (col_index==-1) return;

	QString col = ui_.vars->horizontalHeaderItem(col_index)->text();
	bool col_is_annotation = variants_.annotationIndexByName(col, true, false)!=-1;

	//set up menu
	QMenu menu(ui_.vars->horizontalHeader());

	QAction* a_filter = menu.addAction(QIcon(":/Icons/Filter.png"), "Add/edit column filter");
	a_filter->setEnabled(col_is_annotation);

	//exec menu
	pos = ui_.vars->horizontalHeader()->viewport()->mapToGlobal(pos);
	QAction* action = menu.exec(pos);
	if (action==nullptr) return;

	//actions
	if (action==a_filter)
	{
		ui_.filters->editColumnFilter(col);
	}
}

void MainWindow::registerCustomContextMenuActions()
{
	bool  ngsd_user_logged_in = LoginManager::active();

	QList<QAction*> actions;
	context_menu_actions_.separator = new QAction("---");

	//NGSD report configuration
	context_menu_actions_.a_report_edit = new QAction(QIcon(":/Icons/Report.png"), "Add/edit report configuration");
	context_menu_actions_.a_report_edit->setEnabled(ngsd_user_logged_in);
	actions << context_menu_actions_.a_report_edit;

	context_menu_actions_.a_report_del = new QAction(QIcon(":/Icons/Remove.png"), "Delete report configuration");
	context_menu_actions_.a_report_del->setEnabled(ngsd_user_logged_in);
	actions << context_menu_actions_.a_report_del;
	actions << context_menu_actions_.separator;

	//NGSD variant options
	context_menu_actions_.a_var_class = new QAction("Edit classification");
	context_menu_actions_.a_var_class->setEnabled(ngsd_user_logged_in);
	actions << context_menu_actions_.a_var_class;

	context_menu_actions_.a_var_class_somatic = new QAction("Edit classification  (somatic)");
	context_menu_actions_.a_var_class_somatic->setEnabled(ngsd_user_logged_in);
	actions << context_menu_actions_.a_var_class_somatic;
	context_menu_actions_.a_var_interpretation_somatic = new QAction("Edit VICC interpretation (somatic)");
	context_menu_actions_.a_var_interpretation_somatic->setEnabled(ngsd_user_logged_in);
	actions << context_menu_actions_.a_var_interpretation_somatic;

	context_menu_actions_.a_var_comment = new QAction("Edit comment");
	context_menu_actions_.a_var_comment->setEnabled(ngsd_user_logged_in);
	actions << context_menu_actions_.a_var_comment;
	context_menu_actions_.a_var_val = new QAction("Perform variant validation");
	context_menu_actions_.a_var_val->setEnabled(ngsd_user_logged_in);
	actions << context_menu_actions_.a_var_val;
	actions << context_menu_actions_.separator;

	ui_.vars->addCustomContextMenuActions(actions);
	connect(ui_.vars, SIGNAL(customActionTriggered(QAction*,int)), this, SLOT(execContextMenuAction(QAction*,int)));
}


void MainWindow::execContextMenuAction(QAction* action, int index)
{
	//perform actions
	if (action == context_menu_actions_.a_report_edit)
	{
		editVariantReportConfiguration(index);
	}
	else if (action == context_menu_actions_.a_report_del)
	{
		if ((!report_settings_.report_config->isFinalized() && report_settings_.report_config->exists(VariantType::SNVS_INDELS, index)) || somatic_report_settings_.report_config->exists(VariantType::SNVS_INDELS, index))
		{
			if(germlineReportSupported())
			{
				report_settings_.report_config->remove(VariantType::SNVS_INDELS, index);
			}
			else if(somaticReportSupported())
			{
				somatic_report_settings_.report_config->remove(VariantType::SNVS_INDELS, index);
				storeSomaticReportConfig();
			}

			updateReportConfigHeaderIcon(index);
		}
		else
		{
			QMessageBox::information(this, "Report configuration error", "This variant is not part of the report configuration. It can not be deleted from the report!");
		}
	}
	else if (action == context_menu_actions_.a_var_class)
	{
		editVariantClassification(variants_, index);
	}
	else if (action == context_menu_actions_.a_var_class_somatic)
	{
		editVariantClassification(variants_, index, true);
	}
	else if (action == context_menu_actions_.a_var_interpretation_somatic)
	{
		editSomaticVariantInterpretation(variants_, index);
	}
	else if (action == context_menu_actions_.a_var_comment)
	{
		editVariantComment(index);
	}
	else if (action == context_menu_actions_.a_var_val)
	{
		editVariantValidation(index);
	}
}

void MainWindow::openAlamut(QAction* action)
{
	//documentation of the alamut API:
	// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.14/accessing.html
	// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.11/Alamut-HTTP.html
	// - http://www.interactive-biosoftware.com/doc/alamut-visual/2.14/programmatic-access.html
	QStringList parts = action->text().split(" ");
	if (parts.count()>=1)
	{
		QString value = parts[0];
		if (value=="BAM")
		{
			QStringList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(germlineReportSample()).asStringList();
			if (bams.empty()) return;
			value = "BAM<" + bams[0];
		}

		try
		{
			QString host = Settings::string("alamut_host");
			QString institution = Settings::string("alamut_institution");
			QString apikey = Settings::string("alamut_apikey");
			HttpHandler(true).get(host+"/search?institution="+institution+"&apikey="+apikey+"&request="+value);
		}
		catch (Exception& e)
		{
			GUIHelper::showException(this, e, "Communication with Alamut failed!");
		}
	}
}

void MainWindow::showMatchingCnvsAndSvs(BedLine v_reg)
{
	try
	{
		//determine overlapping genes
		NGSD db;
		GeneSet genes = db.genesOverlapping(v_reg.chr(), v_reg.start(), v_reg.end());
		if (genes.isEmpty()) THROW(Exception, "Could not find a gene overlapping the variant region (gene transcripts are not padded by 5000 bases)!");

		//determine overlapping genes region
		BedFile regions = db.genesToRegions(genes, Transcript::ENSEMBL, "gene", true);
		regions.overlapping(v_reg); //sometimes genes have several loci due to duplicate gene names > exclude those
		regions.merge();

		//check target region
		if (regions.count()==0) THROW(Exception, "Could not determine a target region overlapping variant from genes: " + genes.join(", "));
		if (regions.count()>1) THROW(Exception, "Several target regions overlapping variant from genes: " + genes.join(", "));

		//create table
		TsvFile table;
		table.addHeader("type");
		table.addHeader("variant");
		table.addHeader("genotype");
		table.addHeader("details");

		//select CNVs
		{
			const Chromosome& chr = regions[0].chr();
			int start = regions[0].start();
			int end = regions[0].end();
			const QByteArrayList& headers = cnvs_.annotationHeaders();
			for (int i=0; i<cnvs_.count(); ++i)
			{
				const CopyNumberVariant& v = cnvs_[i];
				if (v.overlapsWith(chr, start, end))
				{
					int cn = v.copyNumber(headers);
					QStringList row;
					row << (cn<=1 ? "CNV - DEL" : "CNV - DUP");
					row << v.toString();
					row << "cn="+QString::number(cn);
					row << "size="+QString::number(v.size()/1000.0, 'f', 3) + "kb regions="+QString::number(v.regions());
					table.addRow(row);
				}
			}
		}

		//select SVs
		{
			QList<QByteArray> headers = svs_.annotationHeaders();
			for (int i=0; i<svs_.count(); ++i)
			{
				const BedpeLine& v = svs_[i];
				if (v.intersectsWith(regions))
				{
					QStringList row;
					row << ("SV - " + StructuralVariantTypeToString(v.type()));
					row << v.toString(false);
					row << "genotype="+v.genotypeHumanReadable(headers);
					row << "size="+QString::number(v.size()/1000.0, 'f', 3)+"kb";
					table.addRow(row);
				}
			}
		}

		//show table
		TsvTableWidget* widget = new TsvTableWidget(table, this);
		connect(widget, SIGNAL(rowDoubleClicked(int)), this, SLOT(jumpToCnvOrSvPosition(int)));
		QSharedPointer<QDialog> dlg = GUIHelper::createDialog(widget, "CNVs and SVs matching " + v_reg.toString(true));
		dlg->exec();
	}
	catch(Exception& e)
	{
        GUIHelper::showException(this, e, "Showing matching CNVs and SVs failed!");
    }
}

void MainWindow::closeAndLogout()
{
    if (ClientHelper::isClientServerMode()) performLogout();
	close();
}

void MainWindow::displayIgvHistoryTable()
{
	//check if already present > bring to front
	QList<IgvLogWidget*> dialogs = findChildren<IgvLogWidget*>();
	if (!dialogs.isEmpty())
	{
		QDialog* dlg = qobject_cast<QDialog*>(dialogs.at(0)->parent());
		dlg->raise();
		return;
	}

	//open new dialog
    IgvLogWidget* widget = new IgvLogWidget(this);
	auto dlg = GUIHelper::createDialog(widget, "IGV command history");
	addModelessDialog(dlg);
}

void MainWindow::changeIgvIconToActive()
{
    igv_history_label_->setPixmap(QPixmap(":/Icons/IGV_active.png"));
    igv_history_label_->setToolTip("IGV is currently processing a command");
}

void MainWindow::changeIgvIconToNormal()
{
	igv_history_label_->setPixmap(QPixmap(":/Icons/IGV.png"));
	igv_history_label_->setToolTip("IGV is idle at the moment");
}

void MainWindow::showBackgroundJobDialog()
{
	bg_job_dialog_->show();
}

int MainWindow::startJob(BackgroundWorkerBase* worker, bool show_busy_dialog)
{
    return bg_job_dialog_->start(worker, show_busy_dialog);
}

QString MainWindow::getJobStatus(int id)
{
    return bg_job_dialog_->getJobStatus(id);
}

QString MainWindow::getJobMessages(int id)
{
	return bg_job_dialog_->getJobMessages(id);
}

void MainWindow::jumpToCnvOrSvPosition(int row)
{
	TsvTableWidget* tsv_table = qobject_cast<TsvTableWidget*>(sender());
	QString pos = tsv_table->getText(row, 1);

	IgvSessionManager::get(0).gotoInIGV(pos, true);
}

void MainWindow::setStyle(QString name)
{
	QStyle* style = QStyleFactory::create(name);
	if (style==nullptr)
	{
		Log::info("Invalid style name '" + name + "' selected!");
		return;
	}

	QApplication::setStyle(style);
}

void MainWindow::on_actionVirusDetection_triggered()
{
	//get virus file
	QString ps_tumor = variants_.mainSampleName();
	NGSD db;
	QString ps_tumor_id = db.processedSampleId(ps_tumor, false);
	FileLocation virus_file = GlobalServiceProvider::database().processedSamplePath(ps_tumor_id, PathType::VIRAL);

	//show widget
	VirusDetectionWidget* widget = new VirusDetectionWidget(virus_file.filename);
	auto dlg = GUIHelper::createDialog(widget, "Virus detection");
	addModelessDialog(dlg);
}

void MainWindow::on_actionBurdenTest_triggered()
{
	BurdenTestWidget* widget = new BurdenTestWidget(this);

	auto dlg = GUIHelper::createDialog(widget, "Gene-based burden test");
	addModelessDialog(dlg);
}

void MainWindow::on_actionOpenLogFile_triggered()
{
	QDesktopServices::openUrl("file:///"+ Log::fileName());
}

void MainWindow::on_actionClearLogFile_triggered()
{
	QString title = "Delete GSvar log file";
	QString filename = Log::fileName();

	int res = QMessageBox::question(this, title, "Do you want to delete the GSvar log file?\nLocation: "+filename);
	if (res==QMessageBox::Yes)
	{
		QFile::remove(filename);
	}
}

void MainWindow::on_actionOpenGSvarDataFolder_triggered()
{
	QDesktopServices::openUrl("file:///"+ QFileInfo(Log::fileName()).absolutePath());
}

void MainWindow::editVariantClassification(VariantList& variants, int index, bool is_somatic)
{
	try
	{
		Variant& variant = variants[index];

		//execute dialog
		ClassificationDialog dlg(this, variant, is_somatic);
		if (dlg.exec()!=QDialog::Accepted) return;

		//update NGSD
		NGSD db;

		ClassificationInfo class_info = dlg.classificationInfo();
		if(is_somatic)
		{
			db.setSomaticClassification(variant, class_info);

			//update variant list classification
			int i_som_class = variants.annotationIndexByName("somatic_classification");
			QString new_class = class_info.classification.replace("n/a", "");
			variant.annotations()[i_som_class] = new_class.toUtf8();

			markVariantListChanged(variant, "somatic_classification", new_class);

			//update variant list classification comment
			int i_som_class_comment = variants.annotationIndexByName("somatic_classification_comment");
			variant.annotations()[i_som_class_comment] = class_info.comments.toUtf8();

			markVariantListChanged(variant, "somatic_classification_comment", class_info.comments);

		}
		else //germline variants
		{
			db.setClassification(variant, variants_, class_info);

			//update variant list classification
			int i_class = variants.annotationIndexByName("classification");
			QString new_class = class_info.classification.replace("n/a", "");
			variant.annotations()[i_class] = new_class.toUtf8();

			markVariantListChanged(variant, "classification", new_class);

			//update variant list classification comment
			int i_class_comment = variants.annotationIndexByName("classification_comment");
			variant.annotations()[i_class_comment] = class_info.comments.toUtf8();

			markVariantListChanged(variant, "classification_comment", class_info.comments);

			//check if already uploaded to ClinVar
			QString var_id = db.variantId(variant);
			QString sample_id = db.sampleId(germlineReportSample(), false);
			if (!sample_id.isEmpty())
			{
				QString clinvar_class = db.getValue("SELECT class FROM  variant_publication WHERE variant_table='variant' AND db='ClinVar' AND sample_id='" + sample_id + "' AND variant_id='" + var_id + "' ORDER BY id DESC LIMIT 1").toString();
				if(!clinvar_class.isEmpty() && clinvar_class!=new_class)
				{
					//update on ClinVar
					int return_value = QMessageBox::information(this, "Clinvar upload required!", "Variant already uploaded to ClinVar. You should also update the classification there!", QMessageBox::Ok, QMessageBox::NoButton);
					if(return_value == QMessageBox::Ok)	uploadToClinvar(index);
				}
			}
		}

		//update details widget and filtering
		ui_.variant_details->updateVariant(variants, index);
		refreshVariantTable(true, true);
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::editSomaticVariantInterpretation(const VariantList &vl, int index)
{
	SomaticVariantInterpreterWidget* interpreter = new SomaticVariantInterpreterWidget(index, vl, this);
	auto dlg = GUIHelper::createDialog(interpreter, "Somatic Variant Interpretation");
	connect(interpreter, SIGNAL(stored(int, QString, QString)), this, SLOT(updateSomaticVariantInterpretationAnno(int, QString, QString)) );
	connect(interpreter, SIGNAL(closeDialog() ), dlg.data(), SLOT(close()) );

	dlg->exec();
}

void MainWindow::updateSomaticVariantInterpretationAnno(int index, QString vicc_interpretation, QString vicc_comment)
{
	int i_vicc = variants_.annotationIndexByName("NGSD_som_vicc_interpretation");
	variants_[index].annotations()[i_vicc] = vicc_interpretation.toUtf8();

	markVariantListChanged(variants_[index], "NGSD_som_vicc_interpretation", vicc_interpretation);

	int i_vicc_comment = variants_.annotationIndexByName("NGSD_som_vicc_comment");
	variants_[index].annotations()[i_vicc_comment] = vicc_comment.toUtf8();

	markVariantListChanged(variants_[index], "NGSD_som_vicc_comment", vicc_comment);

	//update details widget and filtering
	ui_.variant_details->updateVariant(variants_, index);
	refreshVariantTable();
}

void MainWindow::on_actionAnnotateSomaticVariantInterpretation_triggered()
{
	if (filename_.isEmpty()) return;
	if (!LoginManager::active()) return;
	AnalysisType type = variants_.type();
	if (type!=SOMATIC_SINGLESAMPLE && type!=SOMATIC_PAIR) return;

	int i_vicc = variants_.annotationIndexByName("NGSD_som_vicc_interpretation");
	int i_vicc_comment = variants_.annotationIndexByName("NGSD_som_vicc_comment");

	NGSD db;
	for(int i=0; i<variants_.count(); ++i)
	{
		//skip variants without VICC infos in NGSD
		SomaticViccData vicc_data = db.getSomaticViccData(variants_[i], false);
		if (vicc_data.created_by.isEmpty()) continue;

		//update score
		QByteArray vicc_score = SomaticVariantInterpreter::viccScoreAsString(vicc_data).toUtf8();
		if (vicc_score!=variants_[i].annotations()[i_vicc])
		{
			variants_[i].annotations()[i_vicc] = vicc_score;
			markVariantListChanged(variants_[i], "NGSD_som_vicc_interpretation", vicc_score);
		}

		//update comment
		QByteArray vicc_comment = vicc_data.comment.toUtf8();
		if (variants_[i].annotations()[i_vicc_comment]!=vicc_comment)
		{
			variants_[i].annotations()[i_vicc_comment]= vicc_comment;
			markVariantListChanged(variants_[i], "NGSD_som_vicc_comment", vicc_comment);
		}
	}

	//update details widget and filtering
	refreshVariantTable();
}

bool MainWindow::germlineReportSupported(bool require_ngsd, QString* error)
{
	if (error!=nullptr) error->clear();

	//no file loaded
	if (filename_.isEmpty())
	{
		if (error!=nullptr) error->operator=("No file loaded!");
		return false;
	}

	//user has to be logged in
	if (require_ngsd && !LoginManager::active())
	{
		if (error!=nullptr) error->operator=("No user logged in!");
		return false;
	}

	//single, trio or multi only
	AnalysisType type = variants_.type();
	if (type!=GERMLINE_SINGLESAMPLE && type!=GERMLINE_TRIO && type!=GERMLINE_MULTISAMPLE)
	{
		if (error!=nullptr) error->operator=("Invalid analysis type!");
		return false;
	}

	//multi-sample only with at least one affected
	if (type==GERMLINE_MULTISAMPLE && variants_.getSampleHeader().sampleColumns(true).count()<1)
	{
		if (error!=nullptr) error->operator=("No affected sample found in multi-sample analysis!");
		return false;
	}

	//affected samples are in NGSD
	if (require_ngsd)
	{
		NGSD db;
		foreach(const SampleInfo& info, variants_.getSampleHeader())
		{
			if(info.isAffected())
			{
				QString ps = info.name.trimmed();
				if (db.processedSampleId(ps, false)=="")
				{
					if (error!=nullptr) error->operator=("Processed sample '" + ps + " not found in NGSD!");
					return false;
				}
			}
		}
	}

	return true;
}

QString MainWindow::germlineReportSample()
{
	if (!germlineReportSupported(false))
	{
		THROW(ProgrammingException, "germlineReportSample() cannot be used if germline report is not supported!");
	}

	//set sample for report
	while (germline_report_ps_.isEmpty())
	{
		//determine affected sample names
		QStringList affected_ps;
		foreach(const SampleInfo& info, variants_.getSampleHeader())
		{
			if(info.isAffected())
			{
				affected_ps << info.name.trimmed();
			}
		}

		if (affected_ps.isEmpty()) //no affected => error
		{
			THROW(ProgrammingException, "germlineReportSample() cannot be used if there is no affected sample!");
		}
		else if (affected_ps.count()==1) //one affected => auto-select
		{
			germline_report_ps_ = affected_ps[0];
		}
		else //several affected => let user select
		{
			bool ok = false;
			QString selected = QInputDialog::getItem(this, "Report sample", "processed sample used for report:", affected_ps, 0, false, &ok);
			if (ok)
			{
				germline_report_ps_ = selected;
			}
		}
	}

	return germline_report_ps_;
}

bool MainWindow::somaticReportSupported()
{
	return variants_.type()==SOMATIC_PAIR;
}

bool MainWindow::tumoronlyReportSupported()
{
	return variants_.type()==SOMATIC_SINGLESAMPLE;
}

void MainWindow::updateVariantDetails()
{
	int var_current = ui_.vars->selectedVariantIndex();
	if (var_current==-1) //no several variant => clear
	{
		ui_.variant_details->clear();
	}
	else if (var_current!=var_last_) //update variant details (if changed)
	{
		ui_.variant_details->updateVariant(variants_, var_current);
	}

	var_last_ = var_current;
}

void MainWindow::editVariantReportConfiguration(int index)
{
	if (!germlineReportSupported() && !somaticReportSupported())
	{
		QMessageBox::information(this, "Report configuration error", "Report configuration not supported for this type of analysis!");
		return;
	}

	NGSD db;

	if(germlineReportSupported()) //germline report configuration
	{
		//init/get config
		ReportVariantConfiguration var_config;
		bool report_settings_exist = report_settings_.report_config->exists(VariantType::SNVS_INDELS, index);
		if (report_settings_exist)
		{
			var_config = report_settings_.report_config->get(VariantType::SNVS_INDELS, index);
		}
		else
		{
			var_config.variant_index = index;
		}

		//get inheritance mode by gene
		const Variant& variant = variants_[index];
		QList<KeyValuePair> inheritance_by_gene;
		int i_genes = variants_.annotationIndexByName("gene", true, false);

		if (i_genes!=-1)
		{
			GeneSet genes = GeneSet::createFromText(variant.annotations()[i_genes], ',');
            for (const QByteArray& gene : genes)
			{
				GeneInfo gene_info = db.geneInfo(gene);
				inheritance_by_gene << KeyValuePair{gene, gene_info.inheritance};
			}
		}

		//exec dialog
        ReportVariantDialog dlg(variant.toString(QChar()), inheritance_by_gene, var_config, this);
		dlg.setEnabled(!report_settings_.report_config->isFinalized());
		if (dlg.exec()!=QDialog::Accepted) return;


		//update config, GUI and NGSD
		report_settings_.report_config->set(var_config);
		updateReportConfigHeaderIcon(index);

		//force classification of causal variants
		if(var_config.causal)
		{
			const Variant& variant = variants_[index];
			ClassificationInfo classification_info = db.getClassification(variant);
			if (classification_info.classification=="" || classification_info.classification=="n/a")
			{
				QMessageBox::warning(this, "Variant classification required!", "Causal variants should have a classification!", QMessageBox::Ok, QMessageBox::NoButton);
				editVariantClassification(variants_, index);
			}

			//enforce ClinVar upload of class 4/5 variants
			classification_info = db.getClassification(variant);
			if (classification_info.classification=="4" || classification_info.classification=="5")
			{
				QList<int> publication_ids = db.getValuesInt("SELECT id FROM variant_publication WHERE variant_id='" + db.variantId(variants_[index]) + "'");
				if (publication_ids.isEmpty())
				{
					QMessageBox::information(this, "Clinvar upload required!", "Class 4 or 5 variants should be uploaded to ClinVar!", QMessageBox::Ok, QMessageBox::NoButton);
					uploadToClinvar(index);
				}
			}
		}
	}
	else if(somaticReportSupported()) //somatic report variant configuration
	{
		SomaticReportVariantConfiguration var_config;
		bool settings_exists = somatic_report_settings_.report_config->exists(VariantType::SNVS_INDELS, index);
		if(settings_exists)
		{
			var_config = somatic_report_settings_.report_config->get(VariantType::SNVS_INDELS, index);
		}
		else
		{
			var_config.variant_index = index;
		}

		SomaticReportVariantDialog* dlg = new SomaticReportVariantDialog(variants_[index].toString(), var_config, this);

		if(dlg->exec() != QDialog::Accepted) return;
		somatic_report_settings_.report_config->addSomaticVariantConfiguration(var_config);

		storeSomaticReportConfig();
		updateReportConfigHeaderIcon(index);
	}
}

void MainWindow::updateReportConfigHeaderIcon(int index)
{
	if(germlineReportSupported())
	{
		//report config-based filter is on => update whole variant list
		if (ui_.filters->reportConfigurationFilter()!=ReportConfigFilter::NONE)
		{
			refreshVariantTable();
		}
		else //no filter => refresh icon only
		{
			ui_.vars->updateVariantHeaderIcon(report_settings_, index);
		}
	}
	else if(somaticReportSupported())
	{
		if(!ui_.filters->targetRegion().isValid() || ui_.filters->filters().count() > 0)
		{
			refreshVariantTable();
		}
		else
		{
			ui_.vars->updateVariantHeaderIcon(somatic_report_settings_, index);
		}
	}
}

void MainWindow::markVariantListChanged(const Variant& variant, QString column, QString text)
{
	variants_changed_ << VariantListChange{variant, column, text};
}

void MainWindow::storeCurrentVariantList()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);

	if (GlobalServiceProvider::fileLocationProvider().isLocal())
	{
		try
		{
			//store to temporary file
			QString tmp = filename_ + ".tmp";
			variants_.store(tmp);

			//copy temp
			QFile::remove(filename_);
			QFile::rename(tmp, filename_);

			variants_changed_.clear();
		}
		catch(Exception& e)
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Error storing GSvar file", "The GSvar file could not be stored:\n" + e.message());
		}
	}
	else
	{
		QJsonDocument json_doc = QJsonDocument();
		QJsonArray json_array;
		QJsonObject json_object;

		foreach(const VariantListChange& variant_changed, variants_changed_)
		{
			try
			{
				json_object.insert("variant", variant_changed.variant.toString());
				json_object.insert("column", variant_changed.column);
				json_object.insert("text", variant_changed.text);
				json_array.append(json_object);
			}
			catch (Exception& e)
			{
				QMessageBox::warning(this, "Could not process the changes to be sent to the server:", e.message());
			}
		}

		json_doc.setArray(json_array);

		QString ps_url_id;
		QList<QString> filename_parts = filename_.split("/");
		if (filename_parts.size()>3)
		{
			ps_url_id = filename_parts[filename_parts.size()-2];
		}

		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Accept", "application/json");
			add_headers.insert("Content-Type", "application/json");
            add_headers.insert("Content-Length", QByteArray::number(json_doc.toJson().size()));

			QString reply = HttpHandler(true).put(
						ClientHelper::serverApiUrl() + "project_file?ps_url_id=" + ps_url_id + "&token=" + LoginManager::userToken(),
						json_doc.toJson(),
						add_headers
					);
		}
		catch (Exception& e)
		{
			QMessageBox::warning(this, "Could not reach the server:", e.message());
		}
	}

	QApplication::restoreOverrideCursor();
}

void MainWindow::checkPendingVariantValidations()
{
	if (!LoginManager::active()) return;

	NGSD db;
	QStringList vv_pending = db.getValues("SELECT id FROM variant_validation WHERE status='for reporting' AND user_id='" + LoginManager::userIdAsString() + "'");
	if (vv_pending.isEmpty()) return;

	showNotification("Variant validation: " + QString::number(vv_pending.count()) + " pending variants 'for reporting'!");
}

void MainWindow::showNotification(QString text)
{
	text = text.trimmed();

	//update tooltip
    QStringList tooltips = notification_label_->toolTip().split("\n", QT_SKIP_EMPTY_PARTS);
	if (!tooltips.contains(text)) tooltips.prepend(text);
	notification_label_->setToolTip(tooltips.join("<br>"));

	//show popup
	notification_label_->show();
	QPoint pos = ui_.statusBar->mapToGlobal(notification_label_->pos()) + QPoint(8,8);
	QToolTip::showText(pos, text);
}

void MainWindow::variantRanking()
{
	if (filename_.isEmpty()) return;
	if (!LoginManager::active()) return;

	//determine title
	QString algorithm = sender()->objectName();
	QString title = "Ranking variants with algorithm '" + algorithm + "'";

	//init
	NGSD db;
	QString ps_name = germlineReportSample();

	PhenotypeList phenotypes = ui_.filters->phenotypes();
	if (phenotypes.isEmpty())
	{
		QString sample_id = db.sampleId(ps_name);
		phenotypes = db.getSampleData(sample_id).phenotypes;
		if (phenotypes.isEmpty())
		{
			QMessageBox::warning(this, title, "Phenotype data missing. Please set a phenotype filter!");
			return;
		}
		else
		{
			int button = QMessageBox::information(this, title, "No phenotype filter set.\nDo you want to use phenotype information of the sample from NGSD?", QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
			if (button==QMessageBox::No) return;
		}
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		//create phenotype list
		QHash<Phenotype, BedFile> phenotype_rois;		
		for (const Phenotype& pheno : phenotypes)
		{
			//pheno > genes
			GeneSet genes = db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);

			//genes > roi
			BedFile roi;
            for (const QByteArray& gene : genes)
			{
				roi.add(GlobalServiceProvider::geneToRegions(gene, db));
			}
			roi.merge();

			phenotype_rois[pheno] = roi;
		}

		//score
		VariantScores::Parameters parameters;
		VariantScores::Result result = VariantScores::score(algorithm, variants_, phenotype_rois, parameters);

		//update variant list
		VariantScores::annotate(variants_, result, true);
		ui_.filters->reset(true);
		ui_.filters->setFilter("GSvar score/rank");

		QApplication::restoreOverrideCursor();

		//show warnings
		if (result.warnings.count()>0)
		{
			QMessageBox::warning(this, title, "Please note the following warnings:\n" + result.warnings.join("\n"));
		}
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(this, title, "An error occurred:\n" + e.message());
	}
}

void MainWindow::clearSomaticReportSettings(QString ps_id_in_other_widget)
{
	if(!LoginManager::active()) return;
	QString this_ps_id = NGSD().processedSampleId(variants_.mainSampleName(),false);

	if(this_ps_id == "") return;

	if(this_ps_id != ps_id_in_other_widget) return; //skip if ps id of file is different than in other widget
	somatic_report_settings_ = SomaticReportSettings();
	refreshVariantTable();
}

void MainWindow::applyFilters(bool debug_time)
{
	try
	{
		//apply main filter
        QElapsedTimer timer;
		timer.start();

		const FilterCascade& filter_cascade = ui_.filters->filters();

		filter_result_ = filter_cascade.apply(variants_, false, debug_time);

		ui_.filters->markFailedFilters();

		if (debug_time)
		{
			Log::perf("Applying annotation filters took ", timer);
			timer.start();
		}

		//roi filter
		if (ui_.filters->targetRegion().isValid())
		{
			FilterRegions::apply(variants_, ui_.filters->targetRegion().regions, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying target region filter took ", timer);
				timer.start();
			}
		}

		//gene filter
		GeneSet genes_filter = ui_.filters->genes();
		if (!genes_filter.isEmpty())
		{
			FilterGenes filter;
			filter.setStringList("genes", genes_filter.toStringList());
			filter.apply(variants_, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying gene filter took ", timer);
				timer.start();
			}
		}

		//text filter
		QByteArray text = ui_.filters->text();
		if (!text.isEmpty())
		{
			FilterAnnotationText filter;
			filter.setString("term", text);
			filter.setString("action", "FILTER");
			filter.apply(variants_, filter_result_);

			if (debug_time)
			{

				Log::perf("Applying text filter took ", timer);
				timer.start();
			}
		}

		//region filter
		QString region_text = ui_.filters->region();
		BedLine region = BedLine::fromString(region_text);
		if (!region.isValid()) //check if valid chr
		{
			Chromosome chr(region_text);
			if (chr.isNonSpecial())
			{
				region.setChr(chr);
				region.setStart(1);
				region.setEnd(999999999);
			}
		}
		if (region.isValid()) //valid region (chr,start, end or only chr)
		{
			BedFile tmp;
			tmp.append(region);
			FilterRegions::apply(variants_, tmp, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying region filter took ", timer);
				timer.start();
			}
		}
		//phenotype selection changed => update ROI
		const PhenotypeList& phenos = ui_.filters->phenotypes();

		//update phenotypes for variant context menu search
		ui_.vars->updateActivePhenotypes(phenos);

		const PhenotypeSettings& pheno_settings = ui_.filters->phenotypeSettings();
		if (phenos!=last_phenos_ || pheno_settings!=last_pheno_settings_)
		{
			last_phenos_ = phenos;
			last_pheno_settings_ = pheno_settings;

			//convert phenotypes to genes
			NGSD db;
			GeneSet pheno_genes;
			int i = 0;
            for (const Phenotype& pheno : phenos)
			{
				GeneSet genes = db.phenotypeToGenesbySourceAndEvidence(db.phenotypeIdByAccession(pheno.accession()), pheno_settings.sources, pheno_settings.evidence_levels, true, false);

				if (pheno_settings.mode==PhenotypeCombimnationMode::MERGE || (pheno_settings.mode==PhenotypeCombimnationMode::INTERSECT && i==0))
				{
					pheno_genes << genes;
				}
				else
				{
					pheno_genes = pheno_genes.intersect(genes);
				}
				++i;
			}

			//convert genes to ROI (using a cache to speed up repeating queries)
			phenotype_roi_.clear();

            for (const QByteArray& gene : pheno_genes)
			{
				phenotype_roi_.add(GlobalServiceProvider::geneToRegions(gene, db));
			}
			phenotype_roi_.merge();

			if (debug_time)
			{
				Log::perf("Updating phenotype filter took ", timer);
				timer.start();
			}
		}

		//phenotype filter
		if (!last_phenos_.isEmpty())
		{
			FilterRegions::apply(variants_, phenotype_roi_, filter_result_);

			if (debug_time)
			{
				Log::perf("Applying phenotype filter took ", timer);
				timer.start();
			}
		}

		//report configuration filter (show only variants with report configuration)
		ReportConfigFilter rc_filter = ui_.filters->reportConfigurationFilter();
		if (germlineReportSupported() && rc_filter!=ReportConfigFilter::NONE)
		{
            QSet<int> report_variant_indices = LIST_TO_SET(report_settings_.report_config->variantIndices(VariantType::SNVS_INDELS, false));
			for(int i=0; i<variants_.count(); ++i)
			{
				if (!filter_result_.flags()[i]) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{
					filter_result_.flags()[i] = report_variant_indices.contains(i);
				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{
					filter_result_.flags()[i] = !report_variant_indices.contains(i);
				}
			}
		}
		else if( somaticReportSupported() && rc_filter != ReportConfigFilter::NONE) //somatic report configuration filter (show only variants with report configuration)
		{
			QSet<int> report_variant_indices = LIST_TO_SET(somatic_report_settings_.report_config->variantIndices(VariantType::SNVS_INDELS, false));
			for(int i=0; i<variants_.count(); ++i)
			{
				if ( !filter_result_.flags()[i] ) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{
					filter_result_.flags()[i] = report_variant_indices.contains(i);
				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{
					filter_result_.flags()[i] = !report_variant_indices.contains(i);
				}
			}
		}

		//keep somatic variants that are marked with "include" in report settings (overrides possible filtering for that variant)
		if( somaticReportSupported() && rc_filter != ReportConfigFilter::NO_RC)
		{
			foreach(int index, somatic_report_settings_.report_config->variantIndices(VariantType::SNVS_INDELS, false))
			{
				filter_result_.flags()[index] = filter_result_.flags()[index] || somatic_report_settings_.report_config->variantConfig(index, VariantType::SNVS_INDELS).showInReport();
			}
		}
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nPlease re-run the annotation steps for the analysis!");

		filter_result_ = FilterResult(variants_.count(), false);
	}
}

void MainWindow::addToRecentSamples(QString ps)
{
	//update settings
	QStringList recent_samples = Settings::stringList("recent_samples", true);
	recent_samples.removeAll(ps);
	recent_samples.prepend(ps);
	recent_samples = recent_samples.mid(0, 20);

	Settings::setStringList("recent_samples", recent_samples);

	//update GUI
	updateRecentSampleMenu();
}


void MainWindow::updateRecentSampleMenu()
{
	QStringList recent_samples = Settings::stringList("recent_samples", true);

	QMenu* menu = new QMenu();
	foreach(const QString& sample, recent_samples)
	{
		menu->addAction(sample, this, SLOT(openRecentSample()));
	}
	ui_.actionRecent->setMenu(menu);
}

void MainWindow::updateIGVMenu()
{
	QStringList entries = Settings::stringList("igv_menu");
	if (entries.count()==0)
	{
		ui_.menuOpenCustomTrack->addAction("No custom entries in INI file!");
	}
	else
	{
		foreach(QString entry, entries)
		{
			QStringList parts = entry.trimmed().split("\t");
			if(parts.count()!=3) continue;

			QString filename = parts[2];

			//add to menu "open custom track"
			QAction* action = ui_.menuOpenCustomTrack->addAction(parts[0], this, SLOT(openCustomIgvTrack()));
			action->setToolTip(filename); //file path is taken from tooltip when opening track
			if (!QFile::exists(filename))
			{
				action->setEnabled(false);
				action->setText(action->text() + " (missing)");
			}
		}
	}
}

void MainWindow::updateNGSDSupport()
{
	//init
	bool ngsd_user_logged_in = LoginManager::active();

	//toolbar
	ui_.actionOpenProcessedSampleTabByName->setEnabled(ngsd_user_logged_in);
	ui_.actionOpenSequencingRunTabByName->setEnabled(ngsd_user_logged_in);
	ui_.actionOpenGeneTabByName->setEnabled(ngsd_user_logged_in);
	ui_.actionOpenVariantTab->setEnabled(ngsd_user_logged_in);
	ui_.actionOpenProjectTab->setEnabled(ngsd_user_logged_in);
	ui_.actionOpenProcessingSystemTab->setEnabled(ngsd_user_logged_in);
	ui_.report_btn->setEnabled(ngsd_user_logged_in);
	ui_.actionAnalysisStatus->setEnabled(ngsd_user_logged_in);
	ui_.actionReanalyze->setEnabled(ngsd_user_logged_in);
	ui_.actionGeneSelector->setEnabled(ngsd_user_logged_in);
	ui_.actionSampleSearch->setEnabled(ngsd_user_logged_in);
	ui_.actionRunOverview->setEnabled(ngsd_user_logged_in);
	ui_.actionConvertHgvsToGSvar->setEnabled(ngsd_user_logged_in);
	ui_.actionRegionToGenes->setEnabled(ngsd_user_logged_in);
	gap_btn_->setEnabled(ngsd_user_logged_in);
	ui_.actionAnnotateSomaticVariantInterpretation->setEnabled(ngsd_user_logged_in);
	ui_.actionImportHerediVar->setEnabled(ngsd_user_logged_in && Settings::string("HerediVar", true).trimmed()!="");

	//NGSD menu
	ui_.menuNGSD->setEnabled(ngsd_user_logged_in);
	ui_.actionDesignSubpanel->setEnabled(ngsd_user_logged_in);

	//other actions
	ui_.actionOpenByName->setEnabled(ngsd_user_logged_in);
	ui_.ps_details->setEnabled(ngsd_user_logged_in);
	ui_.vars_ranking->setEnabled(ngsd_user_logged_in);

	ui_.filters->updateNGSDSupport();

	//disable certain actions/buttons for restricted users
	if (ngsd_user_logged_in)
	{
		NGSD db;
		if (db.userRoleIn(LoginManager::userLogin(), QStringList{"user_restricted"}))
		{
			auto actions = ui_.menuAdmin->actions();
			foreach(QAction* action, actions)
			{
				if (action!=ui_.actionChangePassword)
				{
					action->setEnabled(false);
				}
			}
		}
	}
}

void MainWindow::openRecentSample()
{
	QAction* action = qobject_cast<QAction*>(sender());
	openProcessedSampleFromNGSD(action->text());
}

QString MainWindow::normalSampleName()
{
	if(variants_.type() != AnalysisType::SOMATIC_PAIR) return "";

	foreach(const SampleInfo& info, variants_.getSampleHeader())
	{
		if (!info.isTumor()) return info.name;
	}

	return "";
}

QString MainWindow::getFileSelectionItem(QString window_title, QString label_text, QStringList file_list, bool *ok)
{
	QStringList rna_count_files_displayed_names = file_list;
	if (ClientHelper::isClientServerMode())
	{
		rna_count_files_displayed_names.clear();
		foreach (QString full_name, file_list)
		{
			rna_count_files_displayed_names << QUrl(full_name).fileName();
		}
	}

	QString selected_file_name = QInputDialog::getItem(this, window_title, label_text, rna_count_files_displayed_names, 0, false, ok);
	if (ClientHelper::isClientServerMode())
	{
		int selection_index = rna_count_files_displayed_names.indexOf(selected_file_name);
		if (selection_index>-1) return file_list[selection_index];
		QMessageBox::warning(this, "File URL not found", "Could not find the URL for the selected file!");
		return "";
	}

    return selected_file_name;
}

void MainWindow::performLogout()
{
    if (!LoginManager::active()) return;

    LoginManager::logout();
}



