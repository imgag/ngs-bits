#include "IGVSession.h"

IGVSession::IGVSession(QWidget *parent, Ui::MainWindow parent_ui, QString igv_name, QString igv_app, QString igv_host, int igv_port, QString genome)
    : parent_(parent)
    , parent_ui_(parent_ui)
    , igv_name_(igv_name)
    , igv_app_(igv_app)
    , igv_host_(igv_host)
    , igv_port_(igv_port)
    , igv_genome_(genome)
    , is_initialized_(false)
    , is_igv_running_(false)
    , responses_({})
    , error_messages_()
{
    execution_pool_.setMaxThreadCount(1);
}

const QString IGVSession::getName()
{
    return igv_name_;
}

void IGVSession::setName(const QString& name)
{
    igv_name_ = name;
}

const QString IGVSession::getHost()
{
    return igv_host_;
}

void IGVSession::setHost(const QString& host)
{
    igv_host_ = host;
}

int IGVSession::getPort()
{
    return igv_port_;
}

void IGVSession::setPort(const int& port)
{
    igv_port_ = port;
}

const QString IGVSession::getGenome()
{
    return igv_genome_;
}

void IGVSession::setGenome(const QString& genome)
{
    igv_genome_ = genome;
}

void IGVSession::setIGVInitialized(const bool& is_initialized)
{
    is_initialized_ = is_initialized;
}

bool IGVSession::isIGVInitialized()
{
    return is_initialized_;
}

void IGVSession::initIGV()
{
    IGVInitWorker* init_worker = new IGVInitWorker(igv_host_, igv_port_, igv_app_);
    execution_pool_.start(init_worker);
}

void IGVSession::execute(const QStringList& commands)
{
    int execution_dealay_ms = 5000;

    if (commands.count() == 1) execution_dealay_ms = 0;
    IGVCommandWorker* command_worker = new IGVCommandWorker(igv_host_, igv_port_, commands, responses_, execution_dealay_ms);
    connect(command_worker, SIGNAL(commandFailed(QString)), this, SLOT(handleExecptions(QString)));
    connect(command_worker, SIGNAL(commandReported(QString)), this, SLOT(addToHistory(QString)));

    connect(command_worker, SIGNAL(commandQueueStarted()), this, SIGNAL(started()));
    connect(command_worker, SIGNAL(commandQueueFinished()), this, SIGNAL(finished()));

    execution_pool_.start(command_worker);
}

void IGVSession::prepareIfNotAndExecute(const QStringList& commands, bool init_if_not_done)
{
    bool is_igv_running = false;
    try
    {
        is_igv_running = isIgvRunning();
    }
    catch (Exception& e)
    {
        QMessageBox::warning(parent_, "Error while IGV availability check", e.message());
    }
    catch(...)
    {
        QMessageBox::warning(parent_, "Error while IGV availability check", "Unknown error has occurred");
    }

    if (!is_igv_running)
    {
        if (QMessageBox::information(parent_, "IGV not running", "IGV is not running on port " + QString::number(getPort()) + ".\nIt will be started now!", QMessageBox::Ok|QMessageBox::Default, QMessageBox::Cancel|QMessageBox::Escape)!=QMessageBox::Ok) return;
        try
        {
            initIGV();
        }
        catch (Exception& e)
        {
            QMessageBox::warning(parent_, "Error while launching IGV", e.message());
        }
        catch(...)
        {
            QMessageBox::warning(parent_, "Error while launching IGV", "Unknown error has occurred");
        }
        setIGVInitialized(false);
        displayIgvHistoryButton(false);
        init_if_not_done = true;
    }

    if (!isIGVInitialized() && init_if_not_done)
    {
        Log::info("Initialzing IGV for the current sample");

        bool is_igv_ready = false;
        if (getName().toLower().contains("regular")) is_igv_ready = prepareRegularIGV();
        if (getName().toLower().contains("virus")) is_igv_ready = prepareVirusIGV();
        if (!is_igv_ready) return;

        clearHistory();
        setIGVInitialized(true);
        displayIgvHistoryButton(true);
    }

    try
    {       
        execute(commands);
    }
    catch (Exception& e)
    {
        QMessageBox::warning(parent_, "Error while executing an IGV command", e.message());
    }
    catch(...)
    {
        QMessageBox::warning(parent_, "Error while executing an IGV command", "Unknown error has occurred");
    }
}

void IGVSession::prepareIfNotAndExecuteSingle(const QString& command, bool init_if_not_done)
{
    prepareIfNotAndExecute(QStringList() << command, init_if_not_done);
}

void IGVSession::gotoInIGV(const QString& region, bool init_if_not_done)
{
    prepareIfNotAndExecuteSingle("goto " + region, init_if_not_done);
}

void IGVSession::loadFileInIGV(QString filename, bool init_if_not_done)
{
    //normalize local files
    filename = Helper::canonicalPath(filename);

    if (ClientHelper::isClientServerMode()) prepareIfNotAndExecuteSingle("SetAccessToken " + LoginManager::userToken() + " *" + Settings::string("server_host") + "*", init_if_not_done);
    prepareIfNotAndExecuteSingle("load \"" + ClientHelper::stripSecureToken(filename) + "\"", init_if_not_done);
}

bool IGVSession::isIgvRunning()
{
    Log::info("IGV availability check");
    QStringList commands = {"echo running"};

    IGVCommandWorker* command_worker = new IGVCommandWorker(igv_host_, igv_port_, commands, responses_, 0);
    execution_pool_.start(command_worker);
    execution_pool_.waitForDone();

    if (responses_.count()>0)
    {
        if (responses_[0].toLower() == "running") return true;
    }

    return false;
}

bool IGVSession::hasRunningCommands()
{
    return (execution_pool_.activeThreadCount()>0);
}

const QStringList IGVSession::getErrorMessages()
{
    return error_messages_;
}

const QStringList IGVSession::getHistory()
{
    if (execution_pool_.activeThreadCount()>0) return QStringList{};
    return command_history_;
}

void IGVSession::clearHistory()
{
    if (execution_pool_.activeThreadCount()>0) return;
    command_history_.clear();
}

QString IGVSession::getCurrentFileName()
{
    foreach(QWidget* widget, QApplication::topLevelWidgets())
    {
        MainWindow* main_window = qobject_cast<MainWindow*>(widget);
        if (main_window!=nullptr)
        {
            return main_window->getCurrentFileName();
        }
    }
    THROW(ProgrammingException, "MainWindow not found!");
}

AnalysisType IGVSession::getCurrentAnalysisType()
{    
    foreach(QWidget* widget, QApplication::topLevelWidgets())
    {
        MainWindow* main_window = qobject_cast<MainWindow*>(widget);
        if (main_window!=nullptr)
        {
            return main_window->getCurrentAnalysisType();
        }
    }
    THROW(ProgrammingException, "MainWindow not found!");
}

void IGVSession::displayIgvHistoryButton(bool visible)
{
    foreach(QWidget* widget, QApplication::topLevelWidgets())
    {
        MainWindow* main_window = qobject_cast<MainWindow*>(widget);
        if (main_window!=nullptr)
        {
            main_window->displayIgvHistoryButton(visible);
            break;
        }
    }
}

QString IGVSession::germlineReportSample()
{
    foreach(QWidget* widget, QApplication::topLevelWidgets())
    {
        MainWindow* main_window = qobject_cast<MainWindow*>(widget);
        if (main_window!=nullptr)
        {
            return main_window->germlineReportSample();
        }
    }
    THROW(ProgrammingException, "MainWindow not found!");
}

bool IGVSession::prepareRegularIGV()
{
    IgvDialog dlg(parent_);
    AnalysisType analysis_type = getCurrentAnalysisType();

    //sample BAM file(s)
    FileLocationList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(true);
    foreach(const FileLocation& file, bams)
    {
        dlg.addFile(file, true);
    }

    //sample BAF file(s)
    FileLocationList bafs = GlobalServiceProvider::fileLocationProvider().getBafFiles(true);
    foreach(const FileLocation& file, bafs)
    {
        if(analysis_type == SOMATIC_PAIR && !file.id.contains("somatic")) continue;
        dlg.addFile(file, true);
    }


    //analysis VCF
    FileLocation vcf = GlobalServiceProvider::fileLocationProvider().getAnalysisVcf();
    dlg.addFile(vcf, parent_ui_.actionIgvSample->isChecked());

    //analysis SV file
    FileLocation bedpe = GlobalServiceProvider::fileLocationProvider().getAnalysisSvFile();
    dlg.addFile(bedpe, parent_ui_.actionSampleBedpe->isChecked());

    //CNV files
    if (analysis_type==SOMATIC_SINGLESAMPLE || analysis_type==SOMATIC_PAIR)
    {
        FileLocation file = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCoverageFile();
        dlg.addFile(file, true);

        FileLocation file2 = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCallFile();
        dlg.addFile(file2, true);
    }
    else
    {
        FileLocationList segs = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(true);
        foreach(const FileLocation& file, segs)
        {
            dlg.addFile(file, true);
        }
    }

    //Manta evidence file(s)
    FileLocationList evidence_files = GlobalServiceProvider::fileLocationProvider().getMantaEvidenceFiles(true);
    foreach(const FileLocation& file, evidence_files)
    {
        dlg.addFile(file, false);
    }

    //target region
    if (parent_ui_.filters->targetRegion().isValid())
    {
        QString roi_file = GSvarHelper::localRoiFolder() + parent_ui_.filters->targetRegion().name + ".bed";
        parent_ui_.filters->targetRegion().regions.store(roi_file);

        dlg.addFile(FileLocation{"target region (selected in GSvar)", PathType::OTHER, roi_file, true}, true);
    }

    //amplicon file (of processing system)
    try
    {
        NGSD db;
        int sys_id = db.processingSystemIdFromProcessedSample(germlineReportSample());
        BedFile ampilicons = GlobalServiceProvider::database().processingSystemAmplicons(sys_id, true);
        if (!ampilicons.isEmpty())
        {
            QString amp_file = GSvarHelper::localRoiFolder() + db.getProcessingSystemData(sys_id).name_short + "_amplicons.bed";
            ampilicons.store(amp_file);

            dlg.addFile(FileLocation{"amplicons track (of processing system)", PathType::OTHER, amp_file, true}, true);
        }
    }
    catch(...) {} //Nothing to do here

    //sample low-coverage
    if (analysis_type==SOMATIC_SINGLESAMPLE || analysis_type==SOMATIC_PAIR)
    {
        FileLocationList som_low_cov_files = GlobalServiceProvider::fileLocationProvider().getSomaticLowCoverageFiles(false);
        foreach(const FileLocation& loc, som_low_cov_files)
        {
            if(loc.filename.contains("somatic_custom_panel_stat"))
            {
                dlg.addFile(FileLocation{loc.id + " (somatic custom panel)", PathType::LOWCOV_BED, loc.filename, QFile::exists(loc.filename)}, parent_ui_.actionIgvLowcov->isChecked());
            }
            else
            {
                dlg.addFile(loc, parent_ui_.actionIgvLowcov->isChecked());
            }
        }
    }
    else
    {
        FileLocationList low_cov_files = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(true);
        foreach(const FileLocation& file, low_cov_files)
        {
            dlg.addFile(file, parent_ui_.actionIgvLowcov->isChecked());
        }
    }

    //custom tracks
    QList<QAction*> igv_actions = parent_ui_.menuTrackDefaults->findChildren<QAction*>();
    foreach(QAction* action, igv_actions)
    {
        QString name = action->text();
        if (!name.startsWith("custom track:")) continue;

        QString filename = action->toolTip().trimmed();
        dlg.addFile(FileLocation{name, PathType::OTHER, filename, QFile::exists(filename)}, action->isChecked());
    }

    //related RNA tracks
    if (LoginManager::active())
    {
        NGSD db;

        QString sample_id = db.sampleId(getCurrentFileName(), false);
        if (sample_id!="")
        {
            foreach (int rna_sample_id, db.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
            {
                // iterate over all processed RNA samples
                foreach (const QString& rna_ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample_id)))
                {
                    //add RNA BAM
                    FileLocation rna_bam_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::BAM);
                    if (rna_bam_file.exists) dlg.addFile(rna_bam_file, false);

                    //add fusions BAM
                    FileLocation rna_fusions_bam_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::FUSIONS_BAM);
                    if (rna_fusions_bam_file.exists) dlg.addFile(rna_fusions_bam_file, false);

                    //add splicing BED
                    FileLocation rna_splicing_bed_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::SPLICING_BED);
                    if (rna_splicing_bed_file.exists) dlg.addFile(rna_splicing_bed_file, false);
                }
            }
        }
    }

    return igvDialogButtonHandler(dlg);
}

bool IGVSession::prepareVirusIGV()
{
    IgvDialog dlg(parent_);

    //sample BAM file(s)
    FileLocationList bams = GlobalServiceProvider::fileLocationProvider().getViralBamFiles(false);
    if (bams.isEmpty())
    {
        QMessageBox::information(parent_, "BAM files not found", "There are no BAM files associated with the virus!");
        return false;
    }

    foreach(const FileLocation& file, bams)
    {
        dlg.addFile(file, true);
    }

    return igvDialogButtonHandler(dlg);
}

void IGVSession::prepareAndRunIGVCommands(QStringList files_to_load)
{
    QStringList init_commands;
    //genome command first, see https://github.com/igvteam/igv/issues/1094
    //choose the correct genome
    init_commands.append("new");
    init_commands.append("genome " + getGenome());
    //	init_commands.append("setSleepInterval 1500");
    //  init_commands.append("currentGenomePath");


    if (ClientHelper::isClientServerMode()) init_commands.append("SetAccessToken " + LoginManager::userToken() + " *" + Settings::string("server_host") + "*");

    //load non-BAM files
    foreach(QString file, files_to_load)
    {
        if (!ClientHelper::isBamFile(file)) init_commands.append("load \"" + Helper::canonicalPath(file) + "\"");
    }

    //collapse tracks
    init_commands.append("setSleepInterval 0");
    init_commands.append("collapse");

    //load BAM files
    foreach(QString file, files_to_load)
    {
        if (ClientHelper::isBamFile(file)) init_commands.append("load \"" + Helper::canonicalPath(file) + "\"");
    }
    init_commands.append("viewaspairs");
    init_commands.append("colorBy UNEXPECTED_PAIR");

    try
    {
        execute(init_commands);
    }
    catch (Exception& e)
    {
        QMessageBox::warning(parent_, "Error while executing an IGV command", e.message());
    }
    catch(...)
    {
        QMessageBox::warning(parent_, "Error while executing an IGV command", "Unknown error has occurred");
    }
}

bool IGVSession::igvDialogButtonHandler(IgvDialog& dlg)
{
    // switch to MainWindow to prevent dialog to appear behind other widgets
    parent_->raise();
    parent_->activateWindow();
    parent_->setFocus();

    //execute dialog
    if (!dlg.exec()) return false;

    QApplication::setOverrideCursor(Qt::BusyCursor);
    try
    {
        if (dlg.initializationAction()==IgvDialog::INIT)
        {
            prepareAndRunIGVCommands(dlg.filesToLoad());
            setIGVInitialized(true);
            displayIgvHistoryButton(true);
        }
        else if (dlg.initializationAction()==IgvDialog::SKIP_SESSION)
        {
            setIGVInitialized(true);
            displayIgvHistoryButton(true);
        }
        else if (dlg.initializationAction()==IgvDialog::SKIP_ONCE)
        {
            //nothing to do there
        }

        QApplication::restoreOverrideCursor();

        return true;
    }
    catch(Exception& e)
    {
        QApplication::restoreOverrideCursor();

        QMessageBox::warning(parent_, "Error while initializing IGV", e.message());

        return false;
    }
}

void IGVSession::addToHistory(QString status)
{
    command_history_.append(status);
    emit historyUpdated(command_history_);
}

void IGVSession::handleExecptions(QString message)
{
    Log::error(message);
    error_messages_.append(message);
}
