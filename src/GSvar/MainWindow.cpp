#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include "Settings.h"
#include "Exceptions.h"
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
#include "ScrollableTextDialog.h"
#include "AnalysisStatusWidget.h"
#include "HttpHandler.h"
#include "ValidationDialog.h"
#include "ClassificationDialog.h"
#include "ApprovedGenesDialog.h"
#include "GeneWidget.h"
#include "PhenoToGenesDialog.h"
#include "GenesToRegionsDialog.h"
#include "SubpanelArchiveDialog.h"
#include "GapDialog.h"
#include "EmailDialog.h"
#include "CnvWidget.h"
#include "RohWidget.h"
#include "GeneSelectorDialog.h"
#include "NGSHelper.h"
#include "SmallVariantSearchWidget.h"
#include "SvWidget.h"
#include "VariantWidget.h"
#include "Histogram.h"
#include "ProcessedSampleWidget.h"
#include "DBSelector.h"
#include "SequencingRunWidget.h"
#include "SimpleCrypt.h"
#include "ToolBase.h"
#include "SampleSearchWidget.h"
#include "ProcessedSampleSelector.h"
#include "ReportVariantDialog.h"
#include "SomaticReportVariantDialog.h"
#include "GSvarHelper.h"
#include "SampleDiseaseInfoWidget.h"
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
#include "CytobandToRegionsDialog.h"
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
#include "VirusDetectionWidget.h"
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
#include <QMimeData>
#include "MaintenanceDialog.h"
#include <QStyleFactory>
#include <QLibraryInfo>
#include <QtCharts/QChartView>
#include "FilterCascade.h"
#include "FilterWidgetHelper.h"

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
    , data_controller_(AnalysisDataController::instance())
    , ui_()
    , var_last_(-1)
	, notification_label_(new QLabel())
	, igv_history_label_(new ClickableLabel())
	, background_job_label_(new ClickableLabel())
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
	if (Helper::runningInQtCreator())
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
	ui_.actionEncrypt->setEnabled(Helper::runningInQtCreator());

	//signals and slots
    connect(&data_controller_, SIGNAL(thrownError(QString, QString)), this, SLOT(showError(QString,QString)));
    connect(&data_controller_, SIGNAL(thrownWarning(QString, QString)), this, SLOT(showWarning(QString,QString)));
    connect(&data_controller_, SIGNAL(thrownInfo(QString, QString)), this, SLOT(showInfo(QString,QString)));

    connect(&data_controller_, SIGNAL(smallVariantsFilterResultChanged()), this, SLOT(refreshVariantTable()));
	connect(&data_controller_, SIGNAL(smallVariantsChanged()), this, SLOT(refreshVariantTable()));
	connect(&data_controller_, SIGNAL(smallVariantsChanged()), this, SLOT(refreshVariantTable()));
	connect(&data_controller_, SIGNAL(chooseGermlineReportSample(QStringList)), this, SLOT(chooseGermlineReportSample(QStringList)));


    connect(ui_.actionExit, SIGNAL(triggered()), this, SLOT(closeAndLogout()));

	connect(ui_.vars, SIGNAL(itemSelectionChanged()), this, SLOT(updateVariantDetails()));
	connect(ui_.vars, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(variantCellDoubleClicked(int, int)));
	connect(ui_.vars->verticalHeader(), SIGNAL(sectionDoubleClicked(int)), this, SLOT(variantHeaderDoubleClicked(int)));
	ui_.vars->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.vars->verticalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(varHeaderContextMenu(QPoint)));
	ui_.vars->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui_.vars->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(columnContextMenu(QPoint)));
    connect(ui_.actionDesignSubpanel, SIGNAL(triggered()), this, SLOT(on_actionDesignSubpanel_triggered()));

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

void MainWindow::userSpecificDebugFunction()
{
    QElapsedTimer timer;
	timer.start();

	QString user = Helper::userName();
	if (user=="ahsturm1")
	{
		//show imported somatic variant statistics for DNA2510181A1_01
		NGSD db;
		QString ps_id = "159881";
		SqlQuery query = db.getQuery();

		query.exec("SELECT * FROM somatic_snv_callset WHERE processed_sample_id_tumor="+ps_id);
		while (query.next())
		{
			QString t_id = query.value("processed_sample_id_tumor").toString();
			QString n_id = query.value("processed_sample_id_normal").isNull() ? "" : query.value("processed_sample_id_normal").toString();
			qDebug() << "somatic_snv_callset" << "id="+query.value("id").toString() << "T="+t_id << "N="+n_id << "caller="+ query.value("caller").toString()+" "+query.value("caller_version").toString() << "date="+query.value("call_date").toString();

			QString add = (n_id=="") ? " IS NULL" : "="+n_id;
			int count = db.getValue("SELECT count(*) FROM detected_somatic_variant WHERE processed_sample_id_tumor="+t_id+" AND processed_sample_id_normal"+add).toInt();
			qDebug() << "  variants: " << count;
		}

		query.exec("SELECT * FROM somatic_cnv_callset WHERE ps_tumor_id="+ps_id);
		while (query.next())
		{
			QString id = query.value("id").toString();
			qDebug() << "somatic_cnv_callset" << "id="+id << "T="+query.value("ps_tumor_id").toString() << "N="+query.value("ps_normal_id").toString() << "caller="+ query.value("caller").toString()+" "+query.value("caller_version").toString() << "date="+query.value("call_date").toString();

			int count = db.getValue("SELECT count(*) FROM somatic_cnv WHERE somatic_cnv_callset_id="+id).toInt();
			qDebug() << "  variants: " << count;
		}

		query.exec("SELECT * FROM somatic_sv_callset WHERE ps_tumor_id="+ps_id);
		while (query.next())
		{
			QString id = query.value("id").toString();
			qDebug() << "somatic_sv_callset" << "id="+id << "T="+query.value("ps_tumor_id").toString() << "N="+query.value("ps_normal_id").toString() << "caller="+ query.value("caller").toString()+" "+query.value("caller_version").toString() << "date="+query.value("call_date").toString();

			int count = db.getValue("SELECT count(*) FROM somatic_sv_deletion WHERE somatic_sv_callset_id="+id).toInt();
			count += db.getValue("SELECT count(*) FROM somatic_sv_duplication WHERE somatic_sv_callset_id="+id).toInt();
			count += db.getValue("SELECT count(*) FROM somatic_sv_insertion WHERE somatic_sv_callset_id="+id).toInt();
			count += db.getValue("SELECT count(*) FROM somatic_sv_inversion WHERE somatic_sv_callset_id="+id).toInt();
			count += db.getValue("SELECT count(*) FROM somatic_sv_translocation WHERE somatic_sv_callset_id="+id).toInt();
			qDebug() << "  variants: " << count;
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
		for (const QByteArray& gene: std::as_const(genes))
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

bool MainWindow::checkHetHitGenes()
{
	//list is generated and used to filter variants
    int i_genes = data_controller_.getSmallVariantList().annotationIndexByName("gene", true, false);
    QList<int> i_genotypes = data_controller_.getSmallVariantList().getSampleHeader().sampleColumns(true);
    i_genotypes.removeAll(-1);
    if (i_genes!=-1 && i_genotypes.count()>0)
    {
        //check that a filter was applied (otherwise this can take forever)
        int passing_vars = data_controller_.getSmallVariantsFilterResult().countPassing();
        if (passing_vars>3000)
        {
            int res = QMessageBox::question(this, "Continue?", "There are " + QString::number(passing_vars) + " small variants that pass the filters.\nGenerating the list of candidate genes for compound-heterozygous hits may take very long for this amount of variants.\nPlease set a filter for the variant list, e.g. the recessive filter, and retry!\nDo you want to continue?", QMessageBox::Yes, QMessageBox::No);
            if(res==QMessageBox::No) return false;
        }
    }
    else if (data_controller_.getAnalysisType()!=AnalysisType::SOMATIC_PAIR && data_controller_.getAnalysisType()!=AnalysisType::SOMATIC_SINGLESAMPLE)
    {
        QMessageBox::information(this, "Invalid variant list", "Column for genes or genotypes not found in variant list. Cannot apply compound-heterozygous filter based on variants!");
		return false;
    }

    return true;
}

void MainWindow::on_actionSV_triggered()
{
    if (!data_controller_.existSvs())
	{
		QMessageBox::information(this, "SV file missing", "No structural variant file is present in the analysis folder!");
		return;
	}

	try
	{
		if (! checkHetHitGenes()) return;

		//open SV widget
		SvWidget* sv_widget = new SvWidget(this);
        auto dlg = GUIHelper::createDialog(sv_widget, "Structural variants of " + data_controller_.getAnalysisName());
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
    if (!data_controller_.existCnvs())
	{
		QMessageBox::information(this, "CNV file missing", "No copy-number file is present in the analysis folder!");
		return;
	}

	if (! checkHetHitGenes()) return;

	CnvWidget* cnv_widget = new CnvWidget(this);

    auto dlg = GUIHelper::createDialog(cnv_widget, "Copy number variants of " + data_controller_.getAnalysisName());
	addModelessDialog(dlg);

	//mosaic CNVs
    if (data_controller_.getAnalysisType() == AnalysisType::GERMLINE_SINGLESAMPLE && data_controller_.existMosaicCnvs())
	{

        QStringList mosaic_data = data_controller_.getMosaicCnvs();
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
                    Log::warn("Mosaic CNV file has line with less than 4 elements: " + line);
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

void MainWindow::on_actionROH_triggered()
{

	//trio special handling: show UPD file is not empty
    if (data_controller_.getAnalysisType() == AnalysisType::GERMLINE_TRIO)
	{
		//UPDs
        if (!data_controller_.existUpds())
		{
            QMessageBox::warning(this, "UPD detection", "The UPD file is missing!\n");
		}
		else
		{
            QStringList upd_data = data_controller_.getUpdFile();
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
    if (data_controller_.existRohs())
	{
        QMessageBox::warning(this, "Runs of homozygosity", "Could not find a ROH file for sample " + data_controller_.germlineReportSample() + ". Aborting!");
		return;
	}

    RohWidget* list = new RohWidget(this, data_controller_.getRohFile().filename);
    auto dlg = GUIHelper::createDialog(list, "Runs of homozygosity of " + data_controller_.getAnalysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionGeneSelector_triggered()
{
    QString ps_name = data_controller_.germlineReportSample();

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
            FilterWidgetHelper::openSubpanelDesignDialog(dlg.genesForVariants(), ui_.filters->targetRegionBox(), data_controller_.getSmallVariantsFilterState());
		}
	}
}

void MainWindow::on_actionCircos_triggered()
{
	//load plot file
    FileLocation plot_file = data_controller_.getCircosFile();

	//show plot
    CircosPlotWidget* widget = new CircosPlotWidget(plot_file.filename);
    auto dlg = GUIHelper::createDialog(widget, "Circos Plot of " + data_controller_.getAnalysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionExpressionData_triggered()
{
	QString title = "Expression data";
    QString count_file = data_controller_.getRnaExpressionGenes().filename;

    NGSD db;
	int rna_sys_id = db.processingSystemIdFromProcessedSample(count_file);
	QString rna_ps_id = db.processedSampleId(count_file);
	QString tissue = db.getSampleData(db.sampleId(count_file)).tissue;
	QString project = db.getProcessedSampleData(rna_ps_id).project_name;

    GeneSet variant_target_region = data_controller_.getSmallVariantsFilterState().getRelevantGenes();
	RnaCohortDeterminationStategy cohort_type;
    if (data_controller_.germlineReportSupported())
	{
		cohort_type = RNA_COHORT_GERMLINE;
	}
	else
	{
		cohort_type = RNA_COHORT_SOMATIC;
	}

    ExpressionGeneWidget* widget = new ExpressionGeneWidget(count_file, rna_sys_id, tissue, data_controller_.getSmallVariantsFilterState().getGenes().toString(", "), variant_target_region, project, rna_ps_id,
															cohort_type, this);
    auto dlg = GUIHelper::createDialog(widget, "Gene expression of " + db.processedSampleName(rna_ps_id) + " (DNA: " + data_controller_.getAnalysisName() + ")");
	addModelessDialog(dlg);
}

void MainWindow::on_actionExonExpressionData_triggered()
{
	QString title = "Exon expression data";

	NGSD db;
    QString count_file = data_controller_.getRnaExpressionExons().filename;

	int rna_sys_id = db.processingSystemIdFromProcessedSample(count_file);
	QString rna_ps_id = db.processedSampleId(count_file);
	QString tissue = db.getSampleData(db.sampleId(count_file)).tissue;
	QString project = db.getProcessedSampleData(rna_ps_id).project_name;

    GeneSet variant_target_region = data_controller_.getSmallVariantsFilterState().getRelevantGenes();

	RnaCohortDeterminationStategy cohort_type = RNA_COHORT_GERMLINE;
    if (data_controller_.somaticReportSupported()) cohort_type = RNA_COHORT_SOMATIC;

    ExpressionExonWidget* widget = new ExpressionExonWidget(count_file, rna_sys_id, tissue, data_controller_.getSmallVariantsFilterState().getGenes().toStringList().join(", "), variant_target_region, project, rna_ps_id, cohort_type, this);
    auto dlg = GUIHelper::createDialog(widget, "Exon expression of " + db.processedSampleName(rna_ps_id) + " (DNA: " + data_controller_.getAnalysisName() + ")");
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowSplicing_triggered()
{
	NGSD db;
    QString splicing_filepath = data_controller_.getRnaSplicing().filename;

	SplicingWidget* splicing_widget = new SplicingWidget(splicing_filepath, this);

    auto dlg = GUIHelper::createDialog(splicing_widget, "Splicing Alterations of " + data_controller_.getAnalysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowRnaFusions_triggered()
{
    QString fusion_filepath = data_controller_.getRnaFusions().filename;
    QString rna_ps_name = data_controller_.getActiveRnaPsName();

    FusionWidget* fusion_widget = new FusionWidget(fusion_filepath, rna_ps_name, data_controller_.getRnaReportConfig(), this);

    auto dlg = GUIHelper::createDialog(fusion_widget, "Fusions of " + rna_ps_name + " (arriba)");
	dlg->resize(1280, 800);
	addModelessDialog(dlg);
}

void MainWindow::on_actionShowProcessingSystemCoverage_triggered()
{
	//set filter widget
	FilterWidget* variant_filter_widget = nullptr;
    if(data_controller_.isValid()) variant_filter_widget = ui_.filters;

	auto expression_level_widget = new ExpressionOverviewWidget(variant_filter_widget, this);

	auto dlg = GUIHelper::createDialog(expression_level_widget, "Expression of processing systems");
	addModelessDialog(dlg);
}

void MainWindow::on_actionMethylation_triggered()
{
    MethylationWidget* widget = new MethylationWidget(data_controller_.getMethylation().filename, this);
    auto dlg = GUIHelper::createDialog(widget, "Methylation of " + data_controller_.getAnalysisName());

	addModelessDialog(dlg, true);
}

void MainWindow::on_actionRE_triggered()
{
	//show dialog
	RepeatExpansionWidget* widget = new RepeatExpansionWidget(this);
    auto dlg = GUIHelper::createDialog(widget, "Repeat expansions of " + data_controller_.getAnalysisName());

	addModelessDialog(dlg, true);
}

void MainWindow::on_actionPRS_triggered()
{
    PRSWidget* widget = new PRSWidget(data_controller_.getPrsFile().filename);
    auto dlg = GUIHelper::createDialog(widget, "Polygenic Risk Scores of " + data_controller_.getAnalysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionPathogenicWT_triggered()
{
	//check if long-read
    bool is_longread = (LoginManager::active() ? NGSD().isLongRead(data_controller_.getFilename()) : true);

	//show dialog
    PathogenicWtDialog dlg(this, data_controller_.getPathogenicWildtype().filename, is_longread);
	dlg.exec();
}

void MainWindow::on_actionDesignCfDNAPanel_triggered()
{
	// Workaround to manual add panels for non patient-specific processing systems
	DBTable cfdna_processing_systems = NGSD().createTable("processing_system", "SELECT id, name_short FROM processing_system WHERE type='cfDNA (patient-specific)' OR type='cfDNA'");

    QSharedPointer<CfDNAPanelDesignDialog> dialog(new CfDNAPanelDesignDialog(data_controller_.getSmallVariantList(), data_controller_.getSmallVariantsFilterResult(), data_controller_.getSomaticReportConfig(), data_controller_.getMainSampleName(), cfdna_processing_systems, this));
	dialog->setWindowFlags(Qt::Window);

	addModelessDialog(dialog);
}

void MainWindow::on_actionShowCfDNAPanel_triggered()
{
	NGSD db;
    // get cfDNA panels:
    QList<CfdnaPanelInfo> cfdna_panels = db.cfdnaPanelInfo(db.processedSampleId(data_controller_.getMainSampleName()));
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
    auto dlg = GUIHelper::createDialog(widget, "cfDNA panel for tumor " + data_controller_.getAnalysisName());
	addModelessDialog(dlg);
}

void MainWindow::on_actionCfDNADiseaseCourse_triggered()
{
    DiseaseCourseWidget* widget = new DiseaseCourseWidget(data_controller_.getMainSampleName());
	auto dlg = GUIHelper::createDialog(widget, "Personalized cfDNA variants");
	addModelessDialog(dlg);
}

void MainWindow::on_actionCfDNAAddExcludedRegions_triggered()
{
    QSharedPointer<cfDNARemovedRegions> dialog(new cfDNARemovedRegions(data_controller_.getMainSampleName(), this));
	dialog->setWindowFlags(Qt::Window);

	addModelessDialog(dialog);
}

void MainWindow::on_actionGeneOmimInfo_triggered()
{
	GeneOmimInfoWidget* widget = new GeneOmimInfoWidget(this);
	auto dlg = GUIHelper::createDialog(widget, "OMIM information for genes");
	dlg->exec();
}

void MainWindow::on_actionReanalyze_triggered()
{
    AnalysisType type = data_controller_.getAnalysisType();
    SampleHeaderInfo header_info = data_controller_.getSmallVariantList().getSampleHeader();
	QList<AnalysisJobSample> samples;
    if (type==AnalysisType::GERMLINE_SINGLESAMPLE  || type==AnalysisType::CFDNA || type==AnalysisType::SOMATIC_SINGLESAMPLE)
	{
		samples << AnalysisJobSample {header_info[0].name, ""};
	}
    else if (type==AnalysisType::GERMLINE_MULTISAMPLE)
	{
		foreach(const SampleInfo& info, header_info)
		{
			samples << AnalysisJobSample {info.name, info.isAffected() ? "affected" : "control"};
		}
	}
    else if (type==AnalysisType::GERMLINE_TRIO)
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
    else if (type==AnalysisType::SOMATIC_PAIR)
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

            if (! FilterWidgetHelper::setTargetRegionByName(roi_name, ui_.filters->targetRegionBox()))
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
    const Variant& v = data_controller_.getSmallVariantList()[ui_.vars->rowToVariantIndex(row)];
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
    try
	{
        NGSD db;
        QString ps = selectProcessedSample();
        VariantValidation var_val = data_controller_.getSmallVariantValidationEntry(index, ps);

        ValidationDialog dlg(this, var_val);

		if (dlg.exec())
		{
			//update DB
            data_controller_.storeVariantValidation(dlg.getValidation());

			//update variant table
			QByteArray status = dlg.status().toUtf8();
			if (status=="true positive") status = "TP";
			if (status=="false positive") status = "FP";

            data_controller_.changeSmallVariantList(index, "validation", status, true);

			//update details widget and filtering
            ui_.variant_details->updateVariant(data_controller_.getSmallVariantList(), index);
        }
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
	}
}

void MainWindow::editVariantComment(int index)
{
	try
	{
        const Variant& variant = data_controller_.getSmallVariantList()[index];
        NGSD db;
		bool ok = true;
		QByteArray text = QInputDialog::getMultiLineText(this, "Variant comment", "Text: ", db.comment(variant), &ok).toUtf8();

		if (ok)
		{
            data_controller_.setSmallVariantComment(index, text);
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
	QString title = "Copy-number histogram";

	try
	{
		//get region
		Chromosome chr;
		int start, end;
		QString region_text = QInputDialog::getText(this, title, "genomic region");
		if (region_text=="") return;

		NGSHelper::parseRegion(region_text, chr, start, end, true);

        Histogram hist = data_controller_.cnHistogram(data_controller_.germlineReportSample(), chr, start, end);

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
	QString title = "BAF histogram";

	try
	{
		//get region
		Chromosome chr;
		int start, end;
		QString region_text = QInputDialog::getText(this, title, "genomic region");
		if (region_text=="") return;

		NGSHelper::parseRegion(region_text, chr, start, end, true);

        Histogram hist = data_controller_.bafHistogram(data_controller_.germlineReportSample(), chr, start, end);

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
	//show chart
    QChartView* view = GUIHelper::histogramChart(data_controller_.afHistogram(filtered), "Allele frequency");
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
	dlg.gotoPage(page_name, section);
	if (dlg.exec()==QDialog::Accepted)
	{
		dlg.storeSettings();
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
        int errors=0;
        int used=0;
        double percentage = data_controller_.calcMendelianErrorRate(used, errors);
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
	if (Helper::runningInQtCreator())
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
        if (data_controller_.isValid())
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

void MainWindow::loadFile(QString filename)
{
    //store variant list in case it changed
    if (data_controller_.variantListModified())
    {
        int result = QMessageBox::question(this, "Store GSvar file?", "The GSvar file was changed by you.\nDo you want to store the changes to file?", QMessageBox::Yes, QMessageBox::No);
        if (result==QMessageBox::Yes)
        {
            data_controller_.storeSmallVariantList();
        }
    }

    QApplication::setOverrideCursor(Qt::BusyCursor);

    try
    {
        QStringList errors = data_controller_.loadFile(filename);

        //reset GUI and data structures
        setWindowTitle(appName());

		connect(data_controller_.getGermlineReportConfig().data(), SIGNAL(variantsChanged()), this, SLOT(storeReportConfig()));
		connect(data_controller_.getSomaticReportConfig().data(), SIGNAL(variantsChanged()), this, SLOT(storeSomaticReportConfig()));

        ui_.tabs->setCurrentIndex(0);
        ui_.filters->reset(true);

        QMessageBox::warning(this, "Error loading analysis:", errors.join("\n"));

        ui_.filters->setValidFilterEntries(data_controller_.getValidFilterEntries());

        //update GUI
        QString mode_title = data_controller_.isLocal() ? "(local mode)" : "";
        setWindowTitle(appName() + " - " + data_controller_.getAnalysisName() + mode_title);
        ui_.statusBar->showMessage("Loaded variant list with " + QString::number(data_controller_.getSmallVariantList().count()) + " variants.");

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
        ui_.variant_details->setLabelTooltips(data_controller_.getSmallVariantList());
	}
	catch(Exception& e)
	{
		issues << qMakePair(Log::LOG_INFO, e.message());
	}
    issues.append(data_controller_.checkVariantList());
    issues.append(data_controller_.checkProcessedSamplesInNGSD());

	//show issues
    if (showAnalysisIssues(issues)==QDialog::Rejected)
	{
		loadFile();
		return;
	}

	//check mendelian error rate for trios
    AnalysisType type = data_controller_.getAnalysisType();
    if (type==AnalysisType::GERMLINE_TRIO)
	{
		checkMendelianErrorRate();
	}

	//notify for variant validation
	checkPendingVariantValidations();


}

int MainWindow::showAnalysisIssues(QList<QPair<Log::LogLevel, QString>>& issues)
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

    if (!has_error) return QDialog::Accepted;

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
	about_text += "\nQt version: " + QLibraryInfo::version().toString();

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
			about_text += "\n  Qt version: " + server_info.qt_version;
		}
    }
	else
	{
		about_text += "\nMode: stand-alone (no server)";
	}

	QMessageBox::about(this, "About " + appName(), about_text);
}

void MainWindow::chooseGermlineReportSample(QStringList samples)
{
	bool ok = false;

	while (! ok)
	{
		QString selected = QInputDialog::getItem(this, "Report sample", "processed sample used for report:", samples, 0, false, &ok);
		if (ok)
		{
			data_controller_.setGermlineReportSample(selected);
		}
	}
}

void MainWindow::generateEvaluationSheet()
{
    QString base_name = data_controller_.germlineReportSample();

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
    EvaluationSheetData evaluation_sheet_data = data_controller_.getEvaluationSheetData();

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
    GermlineReportGeneratorData generator_data(GSvarHelper::build(), base_name, data_controller_.getSmallVariantList(), data_controller_.getCnvList(), data_controller_.getSvList(), data_controller_.getReList(), prs_table, data_controller_.getGermlineReportSettings(), data_controller_.getSmallVariantsFilterState().getFilterCascade(), GSvarHelper::preferredTranscripts(), GlobalServiceProvider::statistics());
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
        SomaticDataTransferWidget data_transfer(data_controller_.getMainSampleName(), data_controller_.getNormalSampleName(), this);
		data_transfer.exec();
	}
	catch(Exception e)
	{
		GUIHelper::showException(this, e, "Transfer somatic data to MTB");
	}
}

void MainWindow::showReportConfigInfo()
{
    QString ps = data_controller_.germlineReportSupported() ? data_controller_.germlineReportSample() : data_controller_.getMainSampleName();
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
    if(data_controller_.germlineReportSupported())
	{
		int conf_id = db.reportConfigId(processed_sample_id);
		if (conf_id==-1)
		{
			QMessageBox::warning(this, title , "No germline report configuration found in the NGSD!");
			return;
		}


        QSharedPointer<ReportConfiguration> report_config = data_controller_.getGermlineReportConfig();
		QMessageBox::information(this, title, report_config->history() + "\n\n" + report_config->variantSummary());
	}
    else if(data_controller_.somaticReportSupported())
	{
        QString ps_normal_id = db.processedSampleId(data_controller_.getNormalSampleName(), false);
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
        QString ps = data_controller_.germlineReportSample();

		//check sample exists in NGSD
		NGSD db;
		QString processed_sample_id = db.processedSampleId(ps, false);
		if (processed_sample_id=="") INFO(ArgumentException, "Sample was not found in the NGSD!");

		// get report config
        OtherCausalVariant causal_variant = data_controller_.getGermlineReportConfig()->otherCausalVariant();
		QStringList variant_types = db.getEnum("report_configuration_other_causal_variant", "type");
		QStringList inheritance_modes = db.getEnum("report_configuration_other_causal_variant", "inheritance");

		//open edit dialog
		CausalVariantEditDialog dlg(causal_variant, variant_types, inheritance_modes, this);
		dlg.setWindowTitle(title + " of " + ps);

		if (dlg.exec()!=QDialog::Accepted) return;

		//store updated causal variant in NGSD
		if (dlg.causalVariant().isValid())
		{
            data_controller_.getGermlineReportConfig()->setOtherCausalVariant(dlg.causalVariant());
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
        QString ps = data_controller_.germlineReportSample();

		//check sample exists
		NGSD db;
		QString processed_sample_id = db.processedSampleId(ps, false);
		if (processed_sample_id=="") INFO(ArgumentException, "Sample was not found in the NGSD!");

        OtherCausalVariant causal_variant = data_controller_.getGermlineReportConfig()->otherCausalVariant();
		if(!causal_variant.isValid()) return;

		//show dialog to confirm by user
		QString message_text = "Are you sure you want to delete the following causal variant?\n" + causal_variant.type + " at " + causal_variant.coordinates + " (gene: " + causal_variant.gene + ", comment: " + causal_variant.comment.replace("\n", " ") + ")";
		QMessageBox::StandardButton response = QMessageBox::question(this, title + " of " + ps, message_text, QMessageBox::Yes|QMessageBox::No);
		if(response != QMessageBox::Yes) return;

		//replace other causal variant with empty struct
        data_controller_.getGermlineReportConfig()->setOtherCausalVariant(OtherCausalVariant());
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
        NGSD db;
        QString processed_sample_id = db.processedSampleId(data_controller_.germlineReportSample(), false);
        if (processed_sample_id=="") INFO(ArgumentException, "Sample was not found in the NGSD!");


        //check config exists
        int conf_id = db.reportConfigId(processed_sample_id);
        if (conf_id==-1) INFO(ArgumentException, "No report configuration for this sample found in the NGSD!");

        //make sure the user knows what he does
        int button = QMessageBox::question(this, title, "Do you really want to finalize the report configuration?\nIt cannot be modified or deleted when finalized!");
        if (button!=QMessageBox::Yes) return;

        data_controller_.finalizeGermlineReportConfig(LoginManager::userId());
	}
	catch(Exception e)
	{
		GUIHelper::showException(this, e, title);
	}
}

void MainWindow::generateReport()
{
	QString error;

    AnalysisType type = data_controller_.getAnalysisType();
	QString type_str = analysisTypeToString(type);
    if (type==AnalysisType::SOMATIC_PAIR)
	{
        generateReportSomaticRTF();
	}
    else if (type==AnalysisType::SOMATIC_SINGLESAMPLE)
	{
        generateReportTumorOnly();
	}
    else if (type==AnalysisType::GERMLINE_SINGLESAMPLE || type==AnalysisType::GERMLINE_TRIO || type==AnalysisType::GERMLINE_MULTISAMPLE)
	{
		QString error_reason;
        if (data_controller_.germlineReportSupported(true, &error_reason))
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
        TumorOnlyReportWorker::checkAnnotation(data_controller_.getSmallVariantList());
	}
	catch(FileParseException e)
	{
        QMessageBox::warning(this, "Invalid tumor only file" + data_controller_.getFilename(), "Could not find all neccessary annotations in tumor-only variant list. Aborting creation of report. " + e.message());
		return;
	}

    QString ps = data_controller_.getMainSampleName();

	//get report settings
    TumorOnlyReportWorkerConfig config = data_controller_.getTumorOnlyReportWorkerConfig();

    TumorOnlyReportDialog dlg(data_controller_.getSmallVariantList(), config, this);
	if(!dlg.exec()) return;

	//get RTF file name
	QString destination_path = last_report_path_ + "/" + ps + "_DNA_tumor_only_" + QDate::currentDate().toString("yyyyMMdd") + ".rtf";
	QString file_rep = QFileDialog::getSaveFileName(this, "Store report file", destination_path, "RTF files (*.rtf);;All files(*.*)");
	if (file_rep=="") return;

	//Generate RTF
	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
        TumorOnlyReportWorker worker(data_controller_.getSmallVariantList(), config);

		QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
		worker.writeRtf(temp_filename);
		Helper::moveFile(temp_filename, file_rep);

        if(!data_controller_.getSmallVariantsFilterState().getTargetRegionInfo().isValid()) //if no ROI filter was set, use system target information for XML instead
		{
            NGSD db;
            config.roi = GlobalServiceProvider::processingSystemTargetRegionInfo(config.sys.name, db);
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

void MainWindow::generateReportSomaticRTF()
{
    data_controller_.updateSomaticReportSettings();

    SomaticReportDialog dlg(data_controller_, this); //widget for settings

	if(!dlg.exec())
	{
		return;
	}

	dlg.writeBackSettings();

	//store somatic report config in NGSD
	if(!dlg.skipNGSD())
	{
        data_controller_.storeSomaticReportConfig();
	}
    QString ps_tumor = data_controller_.getMainSampleName();
    QString ps_normal = data_controller_.getNormalSampleName();

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
            data_controller_.generateSomaticReport(file_rep);
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
        NGSD db;
        QStringList studies = db.studies(db.processedSampleId(data_controller_.getMainSampleName()));
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
            data_controller_.generateSomaticRnaReport(file_rep, dlg.getRNAid());
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
            data_controller_.generateSomaticCfDnaReport(file_rep);
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
    QString ps_name = data_controller_.germlineReportSample();
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
	ReportDialog dialog(data_controller_, this);
	if (!dialog.exec()) return;

	//get export file name
    QString trio_suffix = (data_controller_.getAnalysisType() == AnalysisType::GERMLINE_TRIO ? "trio_" : "");
	QString type_suffix = dialog.type();
	if (type_suffix!="all") type_suffix = type_suffix.replace(" ", "_") + "s";
    QString roi_name = data_controller_.getSmallVariantsFilterState().getTargetRegionInfo().name;
	if (roi_name!="") //remove date and prefix with '_'
	{
        roi_name.remove(QRegularExpression("_[0-9]{4}_[0-9]{2}_[0-9]{2}"));
		roi_name = "_" + roi_name;
	}
	QString file_rep = QFileDialog::getSaveFileName(this, "Export report file", last_report_path_ + "/" + ps_name + roi_name + "_report_" + trio_suffix + type_suffix + "_" + QDate::currentDate().toString("yyyyMMdd") + ".html", "HTML files (*.html);;All files(*.*)");
	if (file_rep=="") return;
	last_report_path_ = QFileInfo(file_rep).absolutePath();

    data_controller_.generateGermlineReport(file_rep, dialog.type());
}

void MainWindow::openProcessedSampleTabsCurrentAnalysis()
{
    foreach(const QString& name, data_controller_.getSampleNames())
	{
        openProcessedSampleTab(name);
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
    QStringList ps_list = data_controller_.getSampleNames();

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

void MainWindow::on_actionShowDatabaseInfo_triggered()
{
	//get infos from NGSD
	NGSD db;
	DBTable db_table = db.createTable("db_import_info", "SELECT * FROM db_import_info");

	//create table
	DBTableWidget* table = new DBTableWidget(this);
	table->setData(db_table);
	table->setMinimumSize(300, 300);

	//create and show dialog
	QSharedPointer<QDialog> dialog  = GUIHelper::createDialog(table, "Database information", "Version and import date of external data sources imported into NGSD:", true);
	dialog->exec();
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
		"species",
		"cspec_data"
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
		output_stream.setEncoding(QStringConverter::Utf8);

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
		output_stream.setEncoding(QStringConverter::Utf8);		

		QApplication::setOverrideCursor(Qt::BusyCursor);
		timer.start();

		QList<QString> sample_db_data; // store all SQL statements here

		db.exportSampleData(ps_id, sample_db_data);

		foreach (QString single_query, std::as_const(sample_db_data))
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

void MainWindow::on_actionDesignSubpanel_triggered()
{
    FilterWidgetHelper::openSubpanelDesignDialog(GeneSet(), ui_.filters->targetRegionBox(), data_controller_.getSmallVariantsFilterState());
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
    if (!data_controller_.getSmallVariantsFilterState().getTargetRegionInfo().isValid())
	{
        QMessageBox::warning(this, "Gaps by target region filter", "No target region filter set!");
		return;
	}

    showGapsClosingDialog(data_controller_.getSmallVariantsFilterState().getTargetRegionInfo().regions, data_controller_.getSmallVariantsFilterState().getTargetRegionInfo().genes);
}

void MainWindow::calculateGapsByGenes()
{
    QString text = QInputDialog::getMultiLineText(this, "Gaps by gene(s)", "genes (one per line):");
	if (text=="") return;

	QApplication::setOverrideCursor(Qt::BusyCursor);

    GeneSet genes = GeneSet::createFromStringList(text.split("\n"));
    BedFile regions = data_controller_.genesToRegions(genes);

    QApplication::restoreOverrideCursor();

    showGapsClosingDialog(regions, genes);
}

void MainWindow::showGapsClosingDialog(const BedFile& regions, const GeneSet& genes)
{
    //check for BAM file
    NGSD db;
    QString ps = (data_controller_.getAnalysisType()==AnalysisType::SOMATIC_SINGLESAMPLE) ? data_controller_.getMainSampleName() : data_controller_.germlineReportSample();
    QStringList bams = data_controller_.getBamFile(ps);

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
		//store
		QString folder = Settings::path("gsvar_variant_export_folder", true);
        QString file_name = folder + QDir::separator() + QFileInfo(data_controller_.getFilename()).baseName() + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".vcf";

		file_name = QFileDialog::getSaveFileName(this, "Export VCF", file_name, "VCF (*.vcf);;All files (*.*)");

        QApplication::setOverrideCursor(Qt::BusyCursor);

		if (file_name!="")
		{	
            bool as_vcf = true;
            data_controller_.exportGSvar(file_name, as_vcf);
		}

        QApplication::restoreOverrideCursor();
	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();

		QMessageBox::warning(this, "VCF export error", e.message());
	}
}

void MainWindow::exportHerediCareVCF()
{
    QString title = "HerediCare VCF export";
	try
	{
        //get HerediCare ID from user
        QString id = QInputDialog::getText(this, title, "HerediCare ID (used in VCF header):");
        if (id.isEmpty()) return;

        //get export file name from user
        QString file_name = Settings::path("gsvar_variant_export_folder", true) + QDir::separator() + id + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".vcf";
        file_name = QFileDialog::getSaveFileName(this, title, file_name, "VCF (*.vcf);;All files (*.*)");
        if (file_name.isEmpty()) return;

        data_controller_.exportHerediCareVCF(id, file_name);
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

		//store
		QString folder = Settings::path("gsvar_variant_export_folder", true);
        QString file_name = folder + QDir::separator() + QFileInfo(data_controller_.getFilename()).baseName() + "_export_" + QDate::currentDate().toString("yyyyMMdd") + "_" + Helper::userName() + ".GSvar";

		file_name = QFileDialog::getSaveFileName(this, "Export GSvar", file_name, "VCF (*.GSvar);;All files (*.*)");
		if (file_name!="")
		{
            data_controller_.exportGSvar(file_name);
		}

        QApplication::restoreOverrideCursor();
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
    if (! LoginManager::active())
    {
        showInfo("ClinVar Upload", "ClinVar upload is only possible with database access.");
        return;
    }
    else
    {
        NGSD db;
        if (db.userRoleIn(LoginManager::userLogin(), QStringList{"user_restricted"}))
        {
            showInfo("ClinVar Upload", "ClinVar upload is only possible for unrestricted users.");
            return;
        }

        try
        {
			ClinvarUploadDialog dlg(this);
			dlg.setData(data_controller_.getClinvarUploadDataSmallVariant(variant_index1, variant_index2));
            dlg.exec();
        }
        catch(Exception& e)
        {
            GUIHelper::showException(this, e, "ClinVar submission error");
        }
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
    int passing_variants = data_controller_.getSmallVariantsFilterResult().countPassing();
    QString status = QString::number(passing_variants) + " of " + QString::number(data_controller_.getSmallVariantList().count()) + " variants passed filters.";
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

    ui_.vars->update(data_controller_, max_variants);


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

    if(data_controller_.germlineReportSupported())
	{
        a_delete->setEnabled(!data_controller_.getGermlineReportConfig()->isFinalized() && data_controller_.getGermlineReportConfig()->exists(VariantType::SNVS_INDELS, index));
	}
    else if(data_controller_.somaticReportSupported())
	{
         a_delete->setEnabled(data_controller_.getSomaticReportConfig()->exists(VariantType::SNVS_INDELS, index));
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
        if(data_controller_.germlineReportSupported())
        {
            data_controller_.getGermlineReportConfig()->remove(VariantType::SNVS_INDELS, index);
        }
        else if(data_controller_.somaticReportSupported())
        {
            data_controller_.getSomaticReportConfig()->remove(VariantType::SNVS_INDELS, index);
        }

		updateReportConfigHeaderIcon(index);
	}
}

void MainWindow::columnContextMenu(QPoint pos)
{
	int col_index = ui_.vars->indexAt(pos).column();
	if (col_index==-1) return;

	QString col = ui_.vars->horizontalHeaderItem(col_index)->text();
    bool col_is_annotation = data_controller_.getSmallVariantList().annotationIndexByName(col, true, false)!=-1;

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
        if ((! data_controller_.getGermlineReportConfig()->isFinalized() && data_controller_.getGermlineReportConfig()->exists(VariantType::SNVS_INDELS, index)) ||(data_controller_.somaticReportSupported() && data_controller_.getSomaticReportConfig()->exists(VariantType::SNVS_INDELS, index)))
		{
            if(data_controller_.germlineReportSupported())
			{
                data_controller_.getGermlineReportConfig()->remove(VariantType::SNVS_INDELS, index);
			}
            else if(data_controller_.somaticReportSupported())
			{
                data_controller_.getSomaticReportConfig()->remove(VariantType::SNVS_INDELS, index);
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
        editVariantClassification(index);
	}
	else if (action == context_menu_actions_.a_var_interpretation_somatic)
	{
        editSomaticVariantInterpretation(index);
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
            QStringList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(data_controller_.germlineReportSample()).asStringList();
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
	if (name=="" || name=="[default]") name = "windowsvista";
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
    //show widget
    VirusDetectionWidget* widget = new VirusDetectionWidget(data_controller_.getVirusDetectionFile().filename);
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

void MainWindow::editVariantClassification(int index)
{
	try
	{
        const Variant& variant = data_controller_.getSmallVariantList()[index];

		//execute dialog
		ClassificationDialog dlg(this, variant);
		if (dlg.exec()!=QDialog::Accepted) return;

		//update NGSD
		NGSD db;

		ClassificationInfo class_info = dlg.classificationInfo();

        db.setClassification(variant, data_controller_.getSmallVariantList(), class_info);

		//update variant list classification
        QString new_class = class_info.classification.replace("n/a", "");
        data_controller_.changeSmallVariantList(index, "classification", new_class.toUtf8());
        data_controller_.changeSmallVariantList(index, "classification_comment", class_info.comments.toUtf8());

		//check if already uploaded to ClinVar
		QString var_id = db.variantId(variant);
        QString sample_id = db.sampleId(data_controller_.germlineReportSample(), false);
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

		//update details widget and filtering
        ui_.variant_details->updateVariant(data_controller_.getSmallVariantList(), index);
		refreshVariantTable(true, true);
	}
	catch (DatabaseException& e)
	{
		GUIHelper::showMessage("NGSD error", e.message());
		return;
	}
}

void MainWindow::editSomaticVariantInterpretation(int index)
{
    SomaticVariantInterpreterWidget* interpreter = new SomaticVariantInterpreterWidget(this, index, data_controller_.getSmallVariantList());
	auto dlg = GUIHelper::createDialog(interpreter, "Somatic Variant Interpretation");
	connect(interpreter, SIGNAL(stored(int, QString, QString)), this, SLOT(updateSomaticVariantInterpretationAnno(int, QString, QString)) );
	dlg->exec();
}

void MainWindow::updateSomaticVariantInterpretationAnno(int index, QString vicc_interpretation, QString vicc_comment)
{
    data_controller_.changeSmallVariantList(index, "NGSD_som_vicc_interpretation", vicc_interpretation.toUtf8(), false);
    data_controller_.changeSmallVariantList(index, "NGSD_som_vicc_comment", vicc_comment.toUtf8(), false);

	//update details widget and filtering
    ui_.variant_details->updateVariant(data_controller_.getSmallVariantList(), index);
	refreshVariantTable();
}

void MainWindow::on_actionAnnotateSomaticVariantInterpretation_triggered()
{
    //TODO update Button enabled
    if (data_controller_.isValid()) return;

    data_controller_.reannotateSomaticVariantInterpretation();

	//update details widget and filtering
	refreshVariantTable();
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
        ui_.variant_details->updateVariant(data_controller_.getSmallVariantList(), var_current);
	}

	var_last_ = var_current;
}

void MainWindow::editVariantReportConfiguration(int index)
{
    if (! data_controller_.germlineReportSupported() && ! data_controller_.somaticReportSupported())
	{
		QMessageBox::information(this, "Report configuration error", "Report configuration not supported for this type of analysis!");
		return;
	}

	NGSD db;

    if(data_controller_.germlineReportSupported()) //germline report configuration
	{
		//init/get config
		ReportVariantConfiguration var_config;
        if (data_controller_.getGermlineReportConfig()->exists(VariantType::SNVS_INDELS, index))
        {
            var_config = data_controller_.getGermlineReportConfig()->get(VariantType::SNVS_INDELS, index);
		}
		else
		{
			var_config.variant_index = index;
		}

        const Variant& variant = data_controller_.getSmallVariantList()[index];
        //get inheritance mode by gene
        QList<KeyValuePair> inheritance_by_gene = data_controller_.inheritanceByGene(index);

		//exec dialog
        ReportVariantDialog dlg(variant.toString(QChar()), inheritance_by_gene, var_config, this);
        dlg.setEnabled(data_controller_.getGermlineReportConfig()->isFinalized());
		if (dlg.exec()!=QDialog::Accepted) return;


		//update config, GUI and NGSD
        data_controller_.getGermlineReportConfig()->set(var_config);
		updateReportConfigHeaderIcon(index);

		//force classification of causal variants
		if(var_config.causal)
		{
			ClassificationInfo classification_info = db.getClassification(variant);
			if (classification_info.classification=="" || classification_info.classification=="n/a")
			{
				QMessageBox::warning(this, "Variant classification required!", "Causal variants should have a classification!", QMessageBox::Ok, QMessageBox::NoButton);
                editVariantClassification(index);
			}

			//enforce ClinVar upload of class 4/5 variants
			classification_info = db.getClassification(variant);
			if (classification_info.classification=="4" || classification_info.classification=="5")
			{
                QList<int> publication_ids = db.getValuesInt("SELECT id FROM variant_publication WHERE variant_id='" + db.variantId(data_controller_.getSmallVariantList()[index]) + "'");
				if (publication_ids.isEmpty())
				{
					QMessageBox::information(this, "Clinvar upload required!", "Class 4 or 5 variants should be uploaded to ClinVar!", QMessageBox::Ok, QMessageBox::NoButton);
					uploadToClinvar(index);
				}
			}
		}
	}
    else if(data_controller_.somaticReportSupported()) //somatic report variant configuration
	{
		SomaticReportVariantConfiguration var_config;

        if(data_controller_.getSomaticReportConfig()->exists(VariantType::SNVS_INDELS, index))
		{
            var_config = data_controller_.getSomaticReportConfig()->get(VariantType::SNVS_INDELS, index);
		}
		else
		{
			var_config.variant_index = index;
		}

        SomaticReportVariantDialog* dlg = new SomaticReportVariantDialog(data_controller_.getSmallVariantList()[index].toString(), var_config, this);

		if(dlg->exec() != QDialog::Accepted) return;
        data_controller_.getSomaticReportConfig()->addSomaticVariantConfiguration(var_config);

		updateReportConfigHeaderIcon(index);
	}
}

void MainWindow::updateReportConfigHeaderIcon(int index)
{
    //report config-based filter is on => update whole variant list
    const FilterState& state = data_controller_.getSmallVariantsFilterState();
    if (state.getReportConfigFilter()!=ReportConfigFilter::NONE || !state.getTargetRegionInfo().isValid() || state.getFilterCascade().count() > 0)
    {
        refreshVariantTable();
    }
    else //no filter => refresh icon only
    {
        ui_.vars->updateVariantHeaderIcon(data_controller_, index);
    }
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
    QStringList tooltips = notification_label_->toolTip().split("\n", Qt::SkipEmptyParts);
	if (!tooltips.contains(text)) tooltips.prepend(text);
	notification_label_->setToolTip(tooltips.join("<br>"));

	//show popup
	notification_label_->show();
	QPoint pos = ui_.statusBar->mapToGlobal(notification_label_->pos()) + QPoint(8,8);
	QToolTip::showText(pos, text);
}

void MainWindow::variantRanking()
{
	if (!LoginManager::active()) return;

	//determine title
	QString algorithm = sender()->objectName();
	QString title = "Ranking variants with algorithm '" + algorithm + "'";

	//init
	NGSD db;
    QString ps_name = data_controller_.germlineReportSample();

    PhenotypeList phenotypes = data_controller_.getSmallVariantsFilterState().getPhenotypes();
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
		for (const Phenotype& pheno : std::as_const(phenotypes))
		{
			//pheno > genes
			GeneSet genes = db.phenotypeToGenes(db.phenotypeIdByAccession(pheno.accession()), true);

			//genes > roi
			BedFile roi;
			for (const QByteArray& gene : std::as_const(genes))
			{
				roi.add(GlobalServiceProvider::geneToRegions(gene, db));
			}
			roi.merge();

			phenotype_rois[pheno] = roi;
		}

		//score
		VariantScores::Parameters parameters;
        VariantScores::Result result = VariantScores::score(algorithm, data_controller_.getSmallVariantList(), phenotype_rois, parameters);

		//update variant list
        data_controller_.annotateVariantRankingScores(result, true);
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

void MainWindow::showError(QString title, QString text)
{
    QMessageBox::critical(this, title, text);
}

void MainWindow::showWarning(QString title, QString text)
{
    QMessageBox::warning(this, title, text);
}

void MainWindow::showInfo(QString title, QString text)
{
    QMessageBox::information(this, title, text);
}

void MainWindow::updateVariantButtonStatus()
{
    ui_.actionCNV->setEnabled(data_controller_.existCnvs());
    ui_.actionSV->setEnabled(data_controller_.existSvs());
    ui_.actionROH->setEnabled(data_controller_.existRohs() || data_controller_.existUpds());
    ui_.actionRE->setEnabled(data_controller_.existRes());
    ui_.actionPRS->setEnabled(data_controller_.existPrs());
    ui_.actionPathogenicWT->setEnabled(data_controller_.existPathogenicWt());
    ui_.actionCircos->setEnabled(data_controller_.existCircos());
    ui_.actionMethylation->setEnabled(data_controller_.existMethylation());
    ui_.actionVirusDetection->setEnabled(data_controller_.existVirusDetection());
}

void MainWindow::updateRnaMenu()
{
    //activate RNA menu
    ui_.actionExpressionData->setEnabled(false);
    ui_.actionExonExpressionData->setEnabled(false);
    ui_.actionShowSplicing->setEnabled(false);
    ui_.actionShowRnaFusions->setEnabled(false);
    if (LoginManager::active())
    {
        if (data_controller_.isValid() && data_controller_.isRnaSet())
        {
            ui_.actionExpressionData->setEnabled(data_controller_.existRnaExpressionGenes());
            ui_.actionExonExpressionData->setEnabled(data_controller_.existRnaExpressionExons());
            ui_.actionShowSplicing->setEnabled(data_controller_.existRnaSplicing());
            ui_.actionShowRnaFusions->setEnabled(data_controller_.existRnaFusions());
        }
    }
}

void MainWindow::updateCfdnaMenu()
{
    //activate cfDNA menu entries and get all available cfDNA samples
    ui_.actionDesignCfDNAPanel->setVisible(false);
    ui_.actionCfDNADiseaseCourse->setVisible(false);
    ui_.actionDesignCfDNAPanel->setEnabled(false);
    ui_.actionCfDNADiseaseCourse->setEnabled(false);
    cfdna_menu_btn_->setVisible(false);
    cfdna_menu_btn_->setEnabled(false);
    ui_.actionCfDNAAddExcludedRegions->setEnabled(false);

    if (data_controller_.isValid() && (data_controller_.somaticReportSupported() || data_controller_.tumoronlyReportSupported()))
    {
        ui_.actionDesignCfDNAPanel->setVisible(true);
        ui_.actionCfDNADiseaseCourse->setVisible(true);
        cfdna_menu_btn_->setVisible(true);

        if (LoginManager::active())
        {
            ui_.actionDesignCfDNAPanel->setEnabled(true);
            cfdna_menu_btn_->setEnabled(true);
            ui_.actionCfDNAAddExcludedRegions->setEnabled(data_controller_.somaticReportSupported());

            QSet<int> cf_dna_sample_ids = data_controller_.getRelatedCfdnaSampleIds();

            if (cf_dna_sample_ids.size() > 0)
            {
                ui_.actionCfDNADiseaseCourse->setEnabled(data_controller_.somaticReportSupported());
            }
        }
    }
}

void MainWindow::updatePanelButtons()
{
    ui_.actionGeneSelector->setEnabled(false);
    gap_btn_->setEnabled(false);

    AnalysisType type = data_controller_.getAnalysisType();
    NGSD db;
    if (data_controller_.isValid() && LoginManager::active())
    {
        gap_btn_->setEnabled(true);
        foreach(QAction* action, gap_btn_->menu()->actions())
        {

            bool type_ok = (type!=AnalysisType::GERMLINE_SINGLESAMPLE && type!=AnalysisType::GERMLINE_TRIO && type!=AnalysisType::GERMLINE_MULTISAMPLE && type!=AnalysisType::SOMATIC_SINGLESAMPLE);
            QString ps = (type==AnalysisType::SOMATIC_SINGLESAMPLE) ? data_controller_.getMainSampleName() : data_controller_.germlineReportSample();

            bool bam_available = data_controller_.existBam(ps);

            if (action->text().contains("Gaps by target region filter"))
            {
                action->setEnabled(type_ok && bam_available);
            }

            if (action->text().contains("Gaps by gene(s)"))
            {
                action->setEnabled(type_ok && bam_available);
            }
        }

        ui_.actionGeneSelector->setEnabled(type == AnalysisType::GERMLINE_SINGLESAMPLE || type == AnalysisType::GERMLINE_MULTISAMPLE || type == AnalysisType::GERMLINE_TRIO);
    }
    ui_.actionReanalyze->setEnabled(data_controller_.isValid());
}

void MainWindow::updateGsvarButtons()
{
    bool enabled = data_controller_.isValid();
    AnalysisType type = data_controller_.getAnalysisType();

    ui_.ps_details->setEnabled(enabled);
    ui_.vars_copy_btn->setEnabled(enabled);
    ui_.vars_export_btn->setEnabled(enabled);
    ui_.vars_resize_btn->setEnabled(enabled);
    ui_.vars_ranking->setEnabled(enabled);
    ui_.vars_af_hist->setEnabled(enabled && (type==AnalysisType::GERMLINE_SINGLESAMPLE || type==AnalysisType::GERMLINE_TRIO));

    foreach(QAction* action, ui_.vars_af_hist->menu()->actions())
    {
        bool type_ok = (type!=AnalysisType::GERMLINE_SINGLESAMPLE && type!=AnalysisType::GERMLINE_TRIO && type!=AnalysisType::GERMLINE_MULTISAMPLE);
        if (action->text().contains("BAF histogram"))
        {
            action->setEnabled(type_ok && data_controller_.existCnvSegmentation(data_controller_.germlineReportSample()));
        }

        if (action->text().contains("CN histogram"))
        {
            action->setEnabled(type_ok && data_controller_.existBaf(data_controller_.germlineReportSample()));
        }
    }

    foreach(QAction* action, ui_.vars_export_btn->menu()->actions())
    {
        if (action->text().contains("VCF for HerediCare"))
        {
            action->setEnabled(type == AnalysisType::GERMLINE_SINGLESAMPLE);
        }
    }

    ui_.report_btn->setEnabled(enabled && LoginManager::active());

    foreach(QAction* action, ui_.report_btn->menu()->actions())
    {
        bool germlineReport = data_controller_.germlineReportSupported(true);

        if (action->text().contains("Add/edit other causal variant"))
        {
            action->setEnabled(germlineReport);
        }
        if (action->text().contains("Delete other causal variant"))
        {
            action->setEnabled(germlineReport);
        }
        if (action->text().contains("Generate report"))
        {
            action->setEnabled(data_controller_.germlineReportSupported(true) || data_controller_.somaticReportSupported() || data_controller_.tumoronlyReportSupported());
        }
        if (action->text().contains("Generate evaluation sheet"))
        {
            action->setEnabled(germlineReport);
        }
        if (action->text().contains("Show report configuration info"))
        {
            action->setEnabled(germlineReport || data_controller_.somaticReportSupported());
        }
        if (action->text().contains("Finalize report configuration"))
        {
            action->setEnabled(germlineReport);
        }
        if (action->text().contains("Transfer somatic data to MTB"))
        {
            action->setEnabled(type == AnalysisType::SOMATIC_PAIR);
        }
    }
}

