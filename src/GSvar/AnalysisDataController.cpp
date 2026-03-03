#include "AnalysisDataController.h"

#include "Log.h"
#include "LoginManager.h"
#include "FilterCascade.h"
#include "GlobalServiceProvider.h"
#include "FileLocationProviderLocal.h"
#include "FileLocationProviderRemote.h"
#include "Exceptions.h"
#include "Settings.h"
#include "IgvSessionManager.h"
#include "HttpHandler.h"
#include "ClientHelper.h"
#include "GenLabDB.h"
#include "GSvarHelper.h"

AnalysisDataController::AnalysisDataController()
    : QObject(),
    db_()
{
}

void AnalysisDataController::clear()
{
    QElapsedTimer timer;
    timer.start();

    GlobalServiceProvider::clearFileLocationProvider();

    IgvSessionManager::get(0).setInitialized(false);

    filename_ = "";
    Settings::setPath("path_variantlists", "");

    processed_sample_names_.clear();

    variants_.clear();
    variants_changed_.clear();
    cnvs_.clear();
    svs_.clear();
    repeat_expansions_.clear();
    somatic_control_tissue_variants_.clear();
    fusions_ = ArribaFile();

    germline_report_settings_ = ReportSettings();
    somatic_report_settings_ = SomaticReportSettings();
    rna_report_config_ = QSharedPointer<RnaReportConfiguration>(new RnaReportConfiguration);

    Log::perf("Clearing variant table took ", timer);
}


QStringList AnalysisDataController::loadFile(QString filename)
{
    clear();

    QStringList errors;

    if (filename=="") return errors;

    QElapsedTimer timer;
    //load variants
    timer.restart();
    try {
        variants_.load(filename);
        analysis_type_ = variants_.type();
    }
    catch (Exception& e)
    {
        errors << "Error loading the file " + filename + ": " + e.message();
        variants_.clear();
        return errors;
    }

    //set only after succesfully loading the gsvar file
    filename_ = filename;
    Log::perf("Loading small variant list took ", timer);

    if (Helper::isHttpUrl(filename))
    {
        GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProviderRemote>(new FileLocationProviderRemote(filename)));
    }
    else
    {
        GlobalServiceProvider::setFileLocationProvider(QSharedPointer<FileLocationProviderLocal>(new FileLocationProviderLocal(filename, variants_.getSampleHeader(), analysis_type_)));
    }

    IgvSessionManager::get(0).removeCache();
    IgvSessionManager::get(0).startCachingForRegularIGV(analysis_type_, filename_);

    //load CNVs
    FileLocation cnv_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisCnvFile();
    if (cnv_loc.exists)
    {
        timer.restart();
        try
        {
            cnvs_.load(cnv_loc.filename);
        }
        catch(Exception& e)
        {
            cnvs_.clear();
            errors << " Error loading CNVs: " + e.message();
        }
        Log::perf("Loading CNV list took ", timer);
    }


    //load SVs
    FileLocation sv_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisSvFile();
    if (sv_loc.exists)
    {
        timer.restart();
        try
        {
            svs_.load(sv_loc.filename);
        }
        catch(Exception& e)
        {
            svs_.clear();
            errors << "Error loading SVs: " + e.message();
        }
        Log::perf("Loading SV list took ", timer);
    }

    //load REs
    FileLocationList re_locs = GlobalServiceProvider::fileLocationProvider().getRepeatExpansionFiles(false);
    if (variants_.type()==AnalysisType::GERMLINE_SINGLESAMPLE && re_locs.count()>0 && re_locs[0].exists)
    {
        timer.restart();
        try
        {
            repeat_expansions_.load(re_locs[0].filename);
        }
        catch(Exception& e)
        {
            repeat_expansions_.clear();
            errors << "Error loading REs: " + e.message();
        }
        Log::perf("Loading RE list took ", timer);
    }

    return errors;
}

QList<QPair<Log::LogLevel, QString>> AnalysisDataController::checkVariantList()
{
    QList<QPair<Log::LogLevel, QString>> issues;
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
    if (type==AnalysisType::GERMLINE_SINGLESAMPLE || type==AnalysisType::GERMLINE_TRIO || type==AnalysisType::GERMLINE_MULTISAMPLE)
    {
        cols << "classification";
        cols << "NGSD_hom";
        cols << "NGSD_het";
        cols << "comment";
        cols << "gene_info";
    }
    if (type==AnalysisType::SOMATIC_SINGLESAMPLE || type==AnalysisType::SOMATIC_PAIR || type==AnalysisType::CFDNA)
    {
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
    if (type==AnalysisType::GERMLINE_SINGLESAMPLE || type==AnalysisType::GERMLINE_TRIO || type==AnalysisType::GERMLINE_MULTISAMPLE)
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
    return issues;
}

QList<QPair<Log::LogLevel, QString>> AnalysisDataController::checkProcessedSamplesInNGSD()
{
    QList<QPair<Log::LogLevel, QString>> issues;
    if (!LoginManager::active()) return issues;


    NGSD db;

    foreach(const SampleInfo& info, variants_.getSampleHeader())
    {
        QString ps = info.name;
        QString ps_id = db.processedSampleId(ps, false);
        if (ps_id=="") continue;

        //check scheduled for resequencing
        QString resequencing = db.getValue("SELECT scheduled_for_resequencing FROM processed_sample WHERE id=" + ps_id).toString();
        if (resequencing=="1")
        {
            issues << qMakePair(Log::LOG_WARNING, "Processed sample '" + ps + "' is scheduled for resequencing!");
        }

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
        if (type==AnalysisType::GERMLINE_SINGLESAMPLE || type==AnalysisType::GERMLINE_TRIO || type==AnalysisType::GERMLINE_MULTISAMPLE)
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

    return issues;
}



void AnalysisDataController::storeSmallVariantList()
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
            emit thrownWarning("Error storing GSvar file", "The GSvar file could not be stored:\n" + e.message());
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
                emit thrownWarning("Could not process the changes to be sent to the server:", e.message());
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
            emit thrownWarning("Could not reach the server:", e.message());
        }
    }

    QApplication::restoreOverrideCursor();
}



void AnalysisDataController::loadGermlineReportConfig()
{
    //check if applicable
    if (!germlineReportSupported()) return;

    //check if report config exists
    NGSD db;
    QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
    int rc_id = db.reportConfigId(processed_sample_id);
    if (rc_id==-1) return;

    //load
    germline_report_settings_.report_config = db.reportConfig(rc_id, variants_, cnvs_, svs_, repeat_expansions_);
    //TODO route over mainWindow: variantsChanged -> 'check if overwrite?' -> storeGermlineReportConfig
    // connect(germline_report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeGermlineReportConfig()));

    //updateGUI
    refreshVariantTable();
}

void AnalysisDataController::storeGermlineReportConfig()
{
    //check if applicable
    if (!germlineReportSupported()) return;

    //check sample
    NGSD db;
    QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
    if (processed_sample_id=="")
    {
        emit thrownWarning("Storing report configuration", "Sample was not found in the NGSD!");
        return;
    }

    //check if config exists and not edited by other user
    int conf_id = db.reportConfigId(processed_sample_id);
    if (conf_id!=-1)
    {
        QSharedPointer<ReportConfiguration> report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, repeat_expansions_);
        if (report_config->lastUpdatedBy()!="" && report_config->lastUpdatedBy()!=LoginManager::userName())
        {
            // TODO
            // if (QMessageBox::question(this, "Storing report configuration", report_config->history() + "\n\nDo you want to override it?")==QMessageBox::No)
            {
                return;
            }
        }
    }

    //store
    try
    {
        germline_report_settings_.report_config.data()->blockSignals(true); //block signals - otherwise the variantsChanged signal is emitted and storeReportConfig is called again, which leads to hanging of the application because of database locks
        db.setReportConfig(processed_sample_id, germline_report_settings_.report_config, variants_, cnvs_, svs_, repeat_expansions_);
        germline_report_settings_.report_config.data()->blockSignals(false);
    }
    catch (Exception& e)
    {
        emit thrownWarning("Storing report configuration", e.message());
    }
}

void AnalysisDataController::loadSomaticReportConfig()
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
    catch(Exception& e)
    {
        emit thrownWarning("Could not load germline GSvar file", "Could not load germline GSvar file. No germline variants will be parsed for somatic report generation. Message: " + e.message());
    }



    //Continue loading report (only if existing in NGSD)
    if(db.somaticReportConfigId(ps_tumor_id, ps_normal_id) == -1) return;


    QStringList messages;
    somatic_report_settings_.report_config = db.somaticReportConfig(ps_tumor_id, ps_normal_id, variants_, cnvs_, svs_, somatic_control_tissue_variants_, messages);


    if(!messages.isEmpty())
    {
        emit thrownWarning("Somatic report configuration", "The following problems were encountered while loading the som. report configuration:\n" + messages.join("\n"));
    }

    //Preselect target region bed file in NGSD
    if(somatic_report_settings_.report_config->targetRegionName()!="")
    {
        // ui_.filters->setTargetRegionByDisplayName(somatic_report_settings_.report_config->targetRegionName());
    }

    //Preselect filter from NGSD som. rep. conf.
    if (somatic_report_settings_.report_config->filterName() != "") ui_.filters->setFilter( somatic_report_settings_.report_config->filterName());
    if(somatic_report_settings_.report_config->filters().count() != 0) ui_.filters->setFilterCascade(somatic_report_settings_.report_config->filters());


    somatic_report_settings_.target_region_filter = ui_.filters->targetRegion();

    // refreshVariantTable();
}

void AnalysisDataController::storeSomaticReportConfig()
{
    if(filename_ == "") return;
    if(!LoginManager::active()) return;
    if(variants_.type() != AnalysisType::SOMATIC_PAIR) return;

    NGSD db;
    QString ps_tumor_id = db.processedSampleId(variants_.mainSampleName(), false);
    QString ps_normal_id = db.processedSampleId(normalSampleName(), false);

    if(ps_tumor_id=="" || ps_normal_id == "")
    {
        emit thrownWarning("Storing somatic report configuration", "Samples were not found in the NGSD!");
        return;
    }

    int conf_id = db.somaticReportConfigId(ps_tumor_id, ps_normal_id);

    if (conf_id!=-1)
    {
        SomaticReportConfigurationData conf_creation = db.somaticReportConfigData(conf_id);
        if (conf_creation.last_edit_by!="" && conf_creation.last_edit_by!=LoginManager::userName())
            // if (QMessageBox::question(this, "Storing report configuration", conf_creation.history() + "\n\nDo you want to update/override it?")==QMessageBox::No)
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
        emit thrownWarning("Storing somatic report configuration", "Error: Could not store the somatic report configuration.\nPlease resolve this error or report it to the administrator:\n\n" + e.message());
    }
}

void AnalysisDataController::loadRnaReportConfig(QString rna_ps_id)
{
    if(filename_ == "") return;

    NGSD db;


    QStringList messages;
    // ArribaFile fusions =
    //rna_report_config_ = db.rnaReportConfig(rna_ps_id, );

}

void AnalysisDataController::storeRnaReportConfig()
{
    //TODO
}









QString AnalysisDataController::normalSampleName()
{
    if(variants_.type() != AnalysisType::SOMATIC_PAIR) return "";

    foreach(const SampleInfo& info, variants_.getSampleHeader())
    {
        if (!info.isTumor()) return info.name;
    }

    return "";
}

QString AnalysisDataController::germlineReportSample()
{
    if (!germlineReportSupported(false) && Settings::string("location", true)!="MHH")
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
            // bool ok = false;
            // QString selected = QInputDialog::getItem(this, "Report sample", "processed sample used for report:", affected_ps, 0, false, &ok); TODO
            // if (ok)
            // {
                // germline_report_ps_ = selected;
            // }
        }
    }

    return germline_report_ps_;

}

bool AnalysisDataController::isValid()
{
    return filename_ != "";
}

bool AnalysisDataController::isLocal()
{
    return GlobalServiceProvider::fileLocationProvider().isLocal();
}

bool AnalysisDataController::existCnvs()
{
    return cnvs_.isValid();
}

bool AnalysisDataController::existSvs()
{
    return svs_.isValid();
}

bool AnalysisDataController::existRes()
{
    return repeat_expansions_.isValid();
}

bool AnalysisDataController::existFusions()
{
    return fusions_.isValid();
}

bool AnalysisDataController::variantListModified()
{
    return ! variants_changed_.isEmpty();
}

bool AnalysisDataController::germlineReportSupported(bool require_ngsd, QString* error) const
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
    if (type!=AnalysisType::GERMLINE_SINGLESAMPLE && type!=AnalysisType::GERMLINE_TRIO && type!=AnalysisType::GERMLINE_MULTISAMPLE)
    {
        if (error!=nullptr) error->operator=("Invalid analysis type!");
        return false;
    }

    //multi-sample only with at least one affected
    if (type==AnalysisType::GERMLINE_MULTISAMPLE && variants_.getSampleHeader().sampleColumns(true).count()<1)
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

bool AnalysisDataController::somaticReportSupported() const
{
    return variants_.type()==AnalysisType::SOMATIC_PAIR;
}

bool AnalysisDataController::tumoronlyReportSupported() const
{
    return variants_.type()==AnalysisType::SOMATIC_SINGLESAMPLE;
}









const QString& AnalysisDataController::getFilename() const
{
    return filename_;
}
AnalysisType AnalysisDataController::getAnalysisType() const
{
    return analysis_type_;
}
QString AnalysisDataController::getAnalysisName() const
{
    return variants_.analysisName();
}


const VariantList& AnalysisDataController::getSmallVariantList() const
{
    return variants_;
}
const CnvList& AnalysisDataController::getCnvList() const
{
    return cnvs_;
}
CnvList AnalysisDataController::getPrefilteredCnvList(int max_number, double& min_ll) const
{
    FilterCnvLoglikelihood filter;
    int cnv_count = cnvs_.count();
    CnvList filtered_list = cnvs_;

    while (cnv_count > max_number)
    {
        min_ll += 1.0;
        FilterResult result(filtered_list.count());
        filter.setDouble("min_ll", min_ll);
        filter.apply(filtered_list, result);
        result.removeFlagged(filtered_list);
    }
    return filtered_list;
}
const BedpeFile& AnalysisDataController::getSvList() const
{
    return svs_;
}
const RepeatLocusList& AnalysisDataController::getReList() const
{
    return repeat_expansions_;
}
const VariantList& AnalysisDataController::getControlTissueSmallVariants() const
{
    return somatic_control_tissue_variants_;
}
const ArribaFile& AnalysisDataController::getFusionList() const
{
    return fusions_;
}

QSharedPointer<ReportConfiguration> AnalysisDataController::getGermlineReportConfig()
{
    return germline_report_settings_.report_config;
}
QSharedPointer<SomaticReportConfiguration> AnalysisDataController::getSomaticReportConfig()
{
    return somatic_report_settings_.report_config;
}
QSharedPointer<RnaReportConfiguration> AnalysisDataController::getRnaReportConfig()
{
    return rna_report_config_;
}

const ReportSettings& AnalysisDataController::getGermlineReportSettings() const
{
    return germline_report_settings_;
}
const SomaticReportSettings& AnalysisDataController::getSomaticReportSettings() const
{
    return somatic_report_settings_;
}

QStringList AnalysisDataController::getValidFilterEntries() const
{
    //determine valid filter entries from filter column (and add new filters low_mappability/mosaic to make outdated GSvar files work as well)
    QStringList valid_filter_entries = variants_.filters().keys();
    if (!valid_filter_entries.contains("low_mappability")) valid_filter_entries << "low_mappability";
    if (!valid_filter_entries.contains("mosaic")) valid_filter_entries << "mosaic";

    return valid_filter_entries;
}

GeneSet AnalysisDataController::getHetHitGenes(const FilterResult& filter_result) const
{
    //create list of genes with heterozygous variant hits
    GeneSet het_hit_genes;
    int i_genes = variants_.annotationIndexByName("gene", true, false);
    QList<int> i_genotypes = variants_.getSampleHeader().sampleColumns(true);
    i_genotypes.removeAll(-1);

    if (i_genes!=-1 && i_genotypes.count()>0)
    {
        for (int i=0; i<variants_.count(); ++i)
        {
            if (!filter_result.passing(i)) continue;

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
    return het_hit_genes;
}
