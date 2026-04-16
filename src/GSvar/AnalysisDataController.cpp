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
#include "VersatileTextStream.h"
#include "SomaticReportHelper.h"
#include "SomaticRnaReport.h"
#include "SomaticcfDNAReport.h"
#include "PrsTable.h"
#include <QBuffer>
#include "GermlineReportGenerator.h"
#include "Background/ReportWorker.h"

AnalysisDataController& AnalysisDataController::instance()
{
    static AnalysisDataController instance;

    return instance;
}

AnalysisDataController::AnalysisDataController()
    : QObject()
{
    connect(&variants_filter_state_, SIGNAL(filterStateChanged()), this, SLOT(applySmallVariantFilter(bool)));
	connect(&cnvs_filter_state_, SIGNAL(filterStateChanged()), this, SLOT(applyCnvFilter(bool)));
	connect(&svs_filter_state_, SIGNAL(filterStateChanged()), this, SLOT(applySvFilter(bool)));
	connect(&res_filter_state_, SIGNAL(filterStateChanged()), this, SLOT(applyReFilter(bool)));
	connect(&fusions_filter_state_, SIGNAL(filterStateChanged()), this, SLOT(applyFusionFilter(bool)));
}

void AnalysisDataController::clear()
{
    QElapsedTimer timer;
    timer.start();

    GlobalServiceProvider::clearFileLocationProvider();

    IgvSessionManager::get(0).setInitialized(false);

    filename_ = "";
    Settings::setPath("path_variantlists", "");
    active_rna_ps_id_ = -1;

    processed_sample_names_.clear();

	//variants
    variants_.clear();
    variants_changed_.clear();
	var_het_genes_uptodate_ = false;
	var_het_genes_.clear();
	cnvs_.clear();
    svs_.clear();
    repeat_expansions_.clear();
    somatic_control_tissue_variants_.clear();
    fusions_ = ArribaFile();

	//Filters
	variants_filter_state_.clearFilters(true);
	cnvs_filter_state_.clearFilters(true);
	svs_filter_state_.clearFilters(true);
	res_filter_state_.clearFilters(true);
	fusions_filter_state_.clearFilters(true);

	variants_filter_result_.reset();
	cnvs_filter_result_.reset();
	svs_filter_result_.reset();
	res_filter_result_.reset();
	fusions_filter_result_.reset();

	//reports:
    germline_report_settings_ = ReportSettings();
    somatic_report_settings_ = SomaticReportSettings();
    rna_report_config_ = QSharedPointer<RnaReportConfiguration>(new RnaReportConfiguration);

    Log::perf("Clearing variant table took ", timer);

    emit dataCleared();
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

    //load report config
    if (germlineReportSupported())
    {
        loadGermlineReportConfig();
    }
    else if(LoginManager::active() && somaticReportSupported())
    {
        loadSomaticReportConfig();
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


void AnalysisDataController::chooseGermlineReportSample()
{
	if (germline_report_ps_.isEmpty())
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
			THROW(ProgrammingException, "choosegermlineReportSample() cannot be used if there is no affected sample!");
		}
		else if (affected_ps.count()==1) //one affected => auto-select
		{
			germline_report_ps_ = affected_ps[0];
		}
		else //several affected => let user select
		{
			emit chooseGermlineReportSample(affected_ps);
		}
	}
}


void AnalysisDataController::loadGermlineReportConfig()
{
    //check if applicable
    if (! isValid() || ! germlineReportSupported()) return;

    //check if report config exists
    NGSD db;
    QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
    int rc_id = db.reportConfigId(processed_sample_id);
    if (rc_id==-1) return;

    //load
    germline_report_settings_.report_config = db.reportConfig(rc_id, variants_, cnvs_, svs_, repeat_expansions_);

	//route over SIGNAL to save over mainWindow: variantsChanged -> check if user in case of overwrite -> storeGermlineReportConfig
	connect(germline_report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SIGNAL(germlineReportConfigChanged()));

	applySmallVariantFilter();
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
    if (! isValid() || ! somaticReportSupported()) return;

    NGSD db;

    //Determine processed sample ids
    QString ps_tumor = variants_.mainSampleName();
    QString ps_tumor_id = db.processedSampleId(ps_tumor, false);
    if(ps_tumor_id == "") return;
    QString ps_normal = getNormalSampleName();
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
		variants_filter_state_.setTargetRegionInfoByName(somatic_report_settings_.report_config->targetRegionName());
    }

    //Preselect filter from NGSD som. rep. conf.
    if (somatic_report_settings_.report_config->filterName() != "") variants_filter_state_.setFilterName(somatic_report_settings_.report_config->filterName());
    if(somatic_report_settings_.report_config->filters().count() != 0) variants_filter_state_.setFilterCascade(somatic_report_settings_.report_config->filters());


	somatic_report_settings_.target_region_filter = variants_filter_state_.getTargetRegionInfo();

	connect(somatic_report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SIGNAL(somaticReportConfigChanged()));

	applySmallVariantFilter();
}

void AnalysisDataController::storeSomaticReportConfig()
{
    if(filename_ == "") return;
    if(!LoginManager::active()) return;
    if(variants_.type() != AnalysisType::SOMATIC_PAIR) return;

    NGSD db;
    QString ps_tumor_id = db.processedSampleId(variants_.mainSampleName(), false);
    QString ps_normal_id = db.processedSampleId(getNormalSampleName(), false);

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

// void AnalysisDataController::loadRnaReportConfig(QString rna_ps_id)
// {
// 	error
//     if(filename_ == "") return;

//     NGSD db;


//     QStringList messages;
//     // ArribaFile fusions =
//     //rna_report_config_ = db.rnaReportConfig(rna_ps_id, );

// }

// void AnalysisDataController::storeRnaReportConfig()
// {
//     //TODO
// 	error
// }








const FilterResult& AnalysisDataController::applySmallVariantFilter(bool debug_time)
{
	var_het_genes_uptodate_ = false;

    FilterResult& result = variants_filter_result_;
    try
    {
        //apply main filter
        QElapsedTimer timer;
        timer.start();

        result = variants_filter_state_.getFilterCascade().apply(variants_, false, debug_time);
        emit markSmallVariantFilters();

        if (debug_time)
        {
            Log::perf("Applying annotation filters took ", timer);
            timer.start();
        }

        //roi filter
        if (variants_filter_state_.getTargetRegionInfo().isValid())
        {
            FilterRegions::apply(variants_, variants_filter_state_.getTargetRegionInfo().regions, result);

            if (debug_time)
            {
                Log::perf("Applying target region filter took ", timer);
                timer.start();
            }
        }

        //gene filter
        if (!variants_filter_state_.getGenes().isEmpty())
        {
            FilterGenes filter;
            filter.setStringList("genes", variants_filter_state_.getGenes().toStringList());
            filter.apply(variants_, result);

            if (debug_time)
            {
                Log::perf("Applying gene filter took ", timer);
                timer.start();
            }
        }

        //text filter
        if (!variants_filter_state_.getTextFilter().isEmpty())
        {
            FilterAnnotationText filter;
            filter.setString("term", variants_filter_state_.getTextFilter());
            filter.setString("action", "FILTER");
            filter.apply(variants_, result);

            if (debug_time)
            {
                Log::perf("Applying text filter took ", timer);
                timer.start();
            }
        }

        //region filter
        if (variants_filter_state_.getRegionFilter().isValid()) //valid region (chr,start, end or only chr)
        {
            BedFile tmp;
            tmp.append(variants_filter_state_.getRegionFilter());
            FilterRegions::apply(variants_, tmp, result);

            if (debug_time)
            {
                Log::perf("Applying region filter took ", timer);
                timer.start();
            }
        }

        //phenotype filter
        if (! variants_filter_state_.getPhenotypes().isEmpty())
        {
            FilterRegions::apply(variants_, variants_filter_state_.getPhenotypeRoi(), result);

            if (debug_time)
            {
                Log::perf("Applying phenotype filter took ", timer);
                timer.start();
            }
        }

        //report configuration filter (show only variants with report configuration)
        if (germlineReportSupported() && variants_filter_state_.getReportConfigFilter() != ReportConfigFilter::NONE)
        {
            QSet<int> report_variant_indices = Helper::listToSet(germline_report_settings_.report_config->variantIndices(VariantType::SNVS_INDELS, false));
            for(int i=0; i<variants_.count(); ++i)
            {
                if (!result.flags()[i]) continue;

                if (variants_filter_state_.getReportConfigFilter()==ReportConfigFilter::HAS_RC)
                {
                    result.flags()[i] = report_variant_indices.contains(i);
                }
                else if (variants_filter_state_.getReportConfigFilter()==ReportConfigFilter::NO_RC)
                {
                    result.flags()[i] = !report_variant_indices.contains(i);
                }
            }
        }
        else if( somaticReportSupported() && variants_filter_state_.getReportConfigFilter() != ReportConfigFilter::NONE) //somatic report configuration filter (show only variants with report configuration)
        {
            QSet<int> report_variant_indices = Helper::listToSet(somatic_report_settings_.report_config->variantIndices(VariantType::SNVS_INDELS, false));
            for(int i=0; i<variants_.count(); ++i)
            {
                if ( !variants_filter_result_.flags()[i] ) continue;

                if (variants_filter_state_.getReportConfigFilter()==ReportConfigFilter::HAS_RC)
                {
                    variants_filter_result_.flags()[i] = report_variant_indices.contains(i);
                }
                else if (variants_filter_state_.getReportConfigFilter()==ReportConfigFilter::NO_RC)
                {
                    variants_filter_result_.flags()[i] = !report_variant_indices.contains(i);
                }
            }
        }

        //keep somatic variants that are marked with "include" in report settings (overrides possible filtering for that variant)
        if( somaticReportSupported() && variants_filter_state_.getReportConfigFilter() != ReportConfigFilter::NO_RC)
        {
            foreach(int index, somatic_report_settings_.report_config->variantIndices(VariantType::SNVS_INDELS, false))
            {
                variants_filter_result_.flags()[index] = variants_filter_result_.flags()[index] || somatic_report_settings_.report_config->variantConfig(index, VariantType::SNVS_INDELS).showInReport();
            }
        }
    }
    catch(Exception& e)
    {
        emit thrownWarning("Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nPlease re-run the annotation steps for the analysis!");

        result = FilterResult(variants_.count(), false);
    }

	emit smallVariantsFilterResultChanged();
    return result;
}


const FilterResult& AnalysisDataController::applyCnvFilter(bool debug_time)
{
	int rows = cnvs_.count();

	QApplication::setOverrideCursor(Qt::BusyCursor);
	try
	{
		QElapsedTimer timer;
		timer.start();

		//apply main filter
		const FilterCascade& filter_cascade = cnvs_filter_state_.getFilterCascade();
		//set comp-het gene list the first time the filter is applied
		for(int i=0; i<filter_cascade.count(); ++i)
		{
			const FilterCnvCompHet* comphet_filter = dynamic_cast<const FilterCnvCompHet*>(filter_cascade[i].data());
			if (comphet_filter!=nullptr && comphet_filter->hetHitGenes().count()!=getHetHitGenes().count())
			{
				comphet_filter->setHetHitGenes(getHetHitGenes());
			}
		}
		cnvs_filter_result_ = filter_cascade.apply(cnvs_, false, debug_time);
		emit markCnvFilters();

		if (debug_time)
		{
			Log::perf("Applying annotation filters took ", timer);
			timer.start();
		}

		//filter by report config
		ReportConfigFilter rc_filter = cnvs_filter_state_.getReportConfigFilter();
		if (rc_filter!=ReportConfigFilter::NONE)
		{
			for(int r=0; r<rows; ++r)
			{
				if (!cnvs_filter_result_.flags()[r]) continue;

				if (rc_filter==ReportConfigFilter::HAS_RC)
				{
					if(somaticReportSupported())
					{
						cnvs_filter_result_.flags()[r] = getSomaticReportConfig()->exists(VariantType::CNVS, r);
					}
					else
					{
						cnvs_filter_result_.flags()[r] = getGermlineReportConfig()->exists(VariantType::CNVS, r);
					}
				}
				else if (rc_filter==ReportConfigFilter::NO_RC)
				{
					if(somaticReportSupported())
					{
						cnvs_filter_result_.flags()[r] = !getSomaticReportConfig()->exists(VariantType::CNVS, r);
					}
					else
					{
						cnvs_filter_result_.flags()[r] = !getGermlineReportConfig()->exists(VariantType::CNVS, r);
					}
				}
			}
		}

		//filter by genes
		GeneSet genes = cnvs_filter_state_.getGenes();
		if (!genes.isEmpty())
		{
			QByteArray genes_joined = genes.join('|');

			if (genes_joined.contains("*")) //with wildcards
			{
				QRegularExpression reg(genes_joined.replace("-", "\\-").replace("*", "[A-Z0-9-]*"));
				for(int r=0; r<rows; ++r)
				{
					if (!cnvs_filter_result_.flags()[r]) continue;

					bool match_found = false;
					for (const QByteArray& cnv_gene : cnvs_[r].genes())
					{
						if (reg.match(cnv_gene).hasMatch())
						{
							match_found = true;
							break;
						}
					}
					cnvs_filter_result_.flags()[r] = match_found;
				}
			}
			else //without wildcards
			{
				for(int r=0; r<rows; ++r)
				{
					if (!cnvs_filter_result_.flags()[r]) continue;

					cnvs_filter_result_.flags()[r] = cnvs_[r].genes().intersectsWith(genes);
				}
			}
		}

		//filter by ROI
		if (cnvs_filter_state_.getTargetRegionInfo().isValid())
		{
			for(int r=0; r<rows; ++r)
			{
				if (!cnvs_filter_result_.flags()[r]) continue;

				cnvs_filter_result_.flags()[r] = cnvs_filter_state_.getTargetRegionInfo().regions.overlapsWith(cnvs_[r].chr(), cnvs_[r].start(), cnvs_[r].end());
			}
		}

		//filter by region
		BedLine region = cnvs_filter_state_.getRegionFilter();
		if (region.isValid()) //valid region (chr,start, end or only chr)
		{
			for(int r=0; r<rows; ++r)
			{
				if (!cnvs_filter_result_.flags()[r]) continue;

				cnvs_filter_result_.flags()[r] = region.overlapsWith(cnvs_[r].chr(), cnvs_[r].start(), cnvs_[r].end());
			}
		}

		//filter by phenotype (via genes, not genomic regions)
		PhenotypeList phenotypes = cnvs_filter_state_.getPhenotypes();
		if (!phenotypes.isEmpty())
		{
			for(int r=0; r<rows; ++r)
			{
				if (!cnvs_filter_result_.flags()[r]) continue;

				cnvs_filter_result_.flags()[r] = cnvs_filter_state_.getPhenotypeRoi().overlapsWith(cnvs_[r].chr(), cnvs_[r].start(), cnvs_[r].end());
			}
		}

		//filter annotations by text
		QByteArray text = cnvs_filter_state_.getTextFilter().toUtf8().trimmed().toLower();
		if (text!="")
		{
			for(int r=0; r<rows; ++r)
			{
				if (!cnvs_filter_result_.flags()[r]) continue;

				bool match = false;
				foreach(const QByteArray& anno, cnvs_[r].annotations())
				{
					if (anno.toLower().contains(text))
					{
						match = true;
						break;
					}
				}
				cnvs_filter_result_.flags()[r] = match;
			}
		}

	}
	catch(Exception& e)
	{
		QApplication::restoreOverrideCursor();
		thrownWarning("Filtering error", e.message() + "\nA possible reason for this error is an outdated variant list.\nTry re-annotating the NGSD columns.\n If re-annotation does not help, please re-analyze the sample (starting from annotation) in the sample information dialog!");

		cnvs_filter_result_ = FilterResult(cnvs_.count(), false);
	}

	QApplication::restoreOverrideCursor();
	return cnvs_filter_result_;
}

const FilterResult& AnalysisDataController::applySvFilter(bool debug_time)
{

}


const FilterResult& AnalysisDataController::applyReFilter(bool debug_time)
{

}


const FilterResult& AnalysisDataController::applyFusionFilter(bool debug_time)
{

}




bool AnalysisDataController::isValid() const
{
    return filename_ != "";
}

bool AnalysisDataController::isLocal() const
{
    return isValid() && GlobalServiceProvider::fileLocationProvider().isLocal();
}

bool AnalysisDataController::isRnaSet() const
{
    return isValid() && active_rna_ps_id_ != -1;
}

bool AnalysisDataController::existBam(QString ps_name) const
{
    if (ps_name == "")
    {
        ps_name = getMainSampleName();
    }
    return GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(ps_name).asStringList().count() > 0;
}

bool AnalysisDataController::existCnvs() const
{
    return isValid() && cnvs_.isValid();
}

bool AnalysisDataController::existSvs() const
{
    return isValid() && svs_.isValid();
}

bool AnalysisDataController::existRes() const
{
    return isValid() && repeat_expansions_.isValid();
}

bool AnalysisDataController::existFusions() const
{
    return isValid() && fusions_.isValid();
}

bool AnalysisDataController::existMosaicCnvs() const
{
    return isValid() &&GlobalServiceProvider::fileLocationProvider().getAnalysisMosaicCnvFile().exists;
}

bool AnalysisDataController::variantListModified() const
{
    return isValid() && !variants_changed_.isEmpty();
}

bool AnalysisDataController::existCircos() const
{
    return (isValid() && (getAnalysisType()==AnalysisType::GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getCircosPlotFiles(false).isEmpty()));
}

bool AnalysisDataController::existPrs() const
{
    return (isValid() && (getAnalysisType()==AnalysisType::GERMLINE_SINGLESAMPLE && !GlobalServiceProvider::fileLocationProvider().getPrsFiles(false).isEmpty()));
}

bool AnalysisDataController::existRohs() const
{
    AnalysisType type = getAnalysisType();
    if (!isValid() || (type!=AnalysisType::GERMLINE_SINGLESAMPLE && type!=AnalysisType::GERMLINE_MULTISAMPLE)) return false;
    if (GlobalServiceProvider::fileLocationProvider().getRohFiles(false).filterById(germlineReportSample()).asStringList().isEmpty()) return false;

    return true;
}

bool AnalysisDataController::existUpds() const
{
    return (isValid() && (getAnalysisType()==AnalysisType::GERMLINE_TRIO && GlobalServiceProvider::fileLocationProvider().getAnalysisUpdFile().exists));
}

bool AnalysisDataController::existPathogenicWt() const
{
    return (isValid() && (germlineReportSupported(false) && ! GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(germlineReportSample()).isEmpty()));
}

bool AnalysisDataController::existMethylation() const
{
    return (isValid() && (getAnalysisType()==AnalysisType::GERMLINE_SINGLESAMPLE && GlobalServiceProvider::fileLocationProvider().getMethylationFile().exists));
}

bool AnalysisDataController::existVirusDetection() const
{
    if (isValid() && (getAnalysisType()==AnalysisType::SOMATIC_PAIR && NGSD::isAvailable()))
    {
        QString ps_tumor = variants_.mainSampleName();
        NGSD db;
        QString ps_tumor_id = db.processedSampleId(ps_tumor, false);
        if (GlobalServiceProvider::database().processedSamplePath(ps_tumor_id, PathType::VIRAL).exists)
        {
            return true;
        }
    }
    return false;
}

bool AnalysisDataController::existCnvSegmentation(QString sample_id) const
{
    if (! isValid()) return false;

    FileLocationList coverage = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(false);

    if (sample_id != "") coverage = coverage.filterById(sample_id);

    return coverage.count() > 0;
}

bool AnalysisDataController::existBaf(QString sample_id) const
{
	if (! isValid()) return false;

	FileLocationList baf_files = GlobalServiceProvider::fileLocationProvider().getBafFiles(false);

	if (sample_id != "") baf_files = baf_files.filterById(sample_id);

	return baf_files.count() > 0;
}

bool AnalysisDataController::existRnaSplicing(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::SPLICING_ANN).exists;

}

bool AnalysisDataController::existRnaFusions(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::FUSIONS).exists;

}

bool AnalysisDataController::existRnaExpressionGenes(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::EXPRESSION).exists;
}

bool AnalysisDataController::existRnaExpressionExons(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::EXPRESSION_EXON).exists;
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







void AnalysisDataController::setRnaSampleId(int rna_ps_id)
{
    if (! isValid() || ! getRelatedRnaProcessedSampleIds().contains(rna_ps_id))
    {
        THROW(ProgrammingException, "AnalysisDataController has not yet loaded an analysis or the given RNA processed sample id is not related to the main sample. Given RNA id: " + QString::number(rna_ps_id));
    }

    active_rna_ps_id_ = rna_ps_id;
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

QString AnalysisDataController::getSystemName(bool throw_if_invalid) const
{

	if (!isValid() || !LoginManager::active())
	{
		if (throw_if_invalid) THROW(ProgrammingException, "AnalysisDataController can't determine system of empty data or NGSD is not available");

		return "";
	}

	QString sys_name = "";
	QString ps;
	if (germlineReportSupported(false))
	{
		ps = germlineReportSample();
	}
	else
	{
		ps = getMainSampleName();
	}

	if (LoginManager::active())
	{
		NGSD db;
		QString ps_id = db.processedSampleId(germlineReportSample());
		sys_name = db.getProcessedSampleData(ps_id).processing_system;
	}

	return sys_name;
}

QString AnalysisDataController::getSystemType(bool throw_if_invalid) const
{
	if (!isValid() || !LoginManager::active())
	{
		if (throw_if_invalid) THROW(ProgrammingException, "AnalysisDataController can't determine system of empty data or NGSD is not available");

		return "";
	}

	QString sys_name = getSystemName(throw_if_invalid);

	NGSD db;
	QString sys_type_ = db.getValue("SELECT type FROM processing_system WHERE name_manufacturer LIKE '" + sys_name + "'").toString();

	return sys_type_;
}

QString AnalysisDataController::getMainSampleName() const
{
	if (!isValid()) THROW(ProgrammingException, "AnalysisDataController can't determine main sample if no data is loaded.");

	if (germlineReportSupported(false))
	{
		return germlineReportSample();
	}
	else
	{
		return variants_.mainSampleName();
	}
}
QString AnalysisDataController::getMainProcessedSampleId() const
{
	NGSD db;
	return db.processedSampleId(getMainSampleName());
}

QString AnalysisDataController::getMainSampleId() const
{
	NGSD db;
	return db.sampleId(getMainSampleName());
}

QString AnalysisDataController::getNormalSampleName() const
{
    if (getAnalysisType() != AnalysisType::SOMATIC_PAIR)
    {
        THROW(ArgumentException, "The normal sample name is only available for somatic tumor normal analysis.");
    }

    foreach(const SampleInfo& info, variants_.getSampleHeader())
    {
        if (!info.isTumor()) return info.name;
    }

    THROW(ArgumentException, "The normal sample name was not found in the sample header of the GSvar file.");
}

QStringList AnalysisDataController::getSampleNames() const
{
    QStringList names;
    foreach (const SampleInfo& info, variants_.getSampleHeader())
    {
        names << info.name.trimmed();
    }
    return names;
}

QString AnalysisDataController::germlineReportSample() const
{
    if (!germlineReportSupported(false) && Settings::string("location", true)!="MHH")
    {
        THROW(ProgrammingException, "germlineReportSample() cannot be used if germline report is not supported!");
    }

	if (germline_report_ps_ == "")
	{
		//germline_report_ps_ should be set when loading an analysis that supports the germline report - either automatically or by user input using the signal "germlineReportSample(QStringList samples)"
		THROW(ProgrammingException, "germlineReportSample() was called but germline_report_ps_ not set.");
	}

    return germline_report_ps_;
}

QStringList AnalysisDataController::getBamFile(QString ps_name) const
{
    if (ps_name == "")
    {
        ps_name = getMainSampleName();
    }

    return GlobalServiceProvider::fileLocationProvider().getBamFiles(true).filterById(ps_name).asStringList();
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

QStringList AnalysisDataController::getMosaicCnvs() const
{
    FileLocation mosaic_file = GlobalServiceProvider::fileLocationProvider().getAnalysisMosaicCnvFile();

    return mosaic_file.exists ? Helper::loadTextFile(mosaic_file.filename, false, '#', true) : QStringList();
}

FileLocation AnalysisDataController::getMethylation() const
{
    return GlobalServiceProvider::fileLocationProvider().getMethylationFile();
}

FileLocation AnalysisDataController::getPrsFile() const
{
    return GlobalServiceProvider::fileLocationProvider().getPrsFiles(false)[0];
}

FileLocation AnalysisDataController::getPathogenicWildtype() const
{
    return GlobalServiceProvider::fileLocationProvider().getBamFiles(false).filterById(germlineReportSample())[0];
}

QStringList AnalysisDataController::getUpdFile() const
{
    FileLocation file_loc = GlobalServiceProvider::fileLocationProvider().getAnalysisUpdFile();
    return Helper::loadTextFile(file_loc.filename, false, QChar::Null, true);
}

FileLocation AnalysisDataController::getRohFile() const
{
   return GlobalServiceProvider::fileLocationProvider().getRohFiles(false).filterById(germlineReportSample())[0];
}

FileLocation AnalysisDataController::getCircosFile() const
{
    return GlobalServiceProvider::fileLocationProvider().getCircosPlotFiles(false)[0];
}

FileLocation AnalysisDataController::getVirusDetectionFile() const
{
    NGSD db;
    return GlobalServiceProvider::database().processedSamplePath(db.processedSampleId(getMainSampleName()), PathType::VIRAL);
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

ReportSettings& AnalysisDataController::getGermlineReportSettings()
{
    return germline_report_settings_;
}

VariantList AnalysisDataController::validateGermlineReportSmallVariants()
{
	NGSD db;
	VariantList missing_variants;

	int rc_id = db.reportConfigId(db.processedSampleId(germlineReportSample()));
	QList<int> small_variant_ids = db.getValuesInt("SELECT variant_id FROM report_configuration_variant WHERE report_configuration_id=:0", QString::number(rc_id));
	foreach (int var_id, small_variant_ids)
	{
		//match report variant against variant list
		Variant var = db.variant(QString::number(var_id));
		bool match_found = false;
		for (int i=0; i<getSmallVariantList().count(); ++i)
		{
			if (var==getSmallVariantList()[i])
			{
				match_found = true;
				break;
			}
		}
		if (!match_found) missing_variants.append(var);
	}
	return missing_variants;
}

CnvList AnalysisDataController::validateGermlineReportCnvs()
{
	NGSD db;
	CnvList missing_variants;

	int rc_id = db.reportConfigId(db.processedSampleId(germlineReportSample()));

	QList<int> cnv_ids = db.getValuesInt("SELECT cnv_id FROM report_configuration_cnv WHERE report_configuration_id=:0", QString::number(rc_id));
	foreach (int var_id, cnv_ids)
	{
		//match report variant against variant list
		CopyNumberVariant cnv = db.cnv(var_id);
		bool match_found = false;
		for (int i=0; i<getCnvList().count(); ++i)
		{
			if (getCnvList()[i].hasSamePosition(cnv))
			{
				match_found = true;
				break;
			}
		}
		if (!match_found) missing_variants.append(cnv);
	}
	return missing_variants;
}

BedpeFile AnalysisDataController::validateGermlineReportSvs()
{
	NGSD db;
	BedpeFile missing_variants;

	int rc_id = db.reportConfigId(db.processedSampleId(germlineReportSample()));
	foreach (StructuralVariantType type, QList<StructuralVariantType>() << StructuralVariantType::DEL << StructuralVariantType::DUP << StructuralVariantType::INS << StructuralVariantType::INV << StructuralVariantType::BND)
	{
		QString sv_table = db.svTableName(type);
		QList<int> sv_ids = db.getValuesInt("SELECT " + sv_table + "_id FROM report_configuration_sv WHERE report_configuration_id=:0 AND " + sv_table + "_id IS NOT NULL", QString::number(rc_id));
		foreach (int var_id, sv_ids)
		{
			//match report variant against variant list
			BedpeLine sv = db.structuralVariant(var_id, type, getSvList());
			if (getSvList().findMatch(sv, true, false) == -1) missing_variants.append(sv);
		}
	}
	return missing_variants;
}

RepeatLocusList AnalysisDataController::validateGermlineReportRes()
{
	NGSD db;
	RepeatLocusList missing_variants;

	int rc_id = db.reportConfigId(db.processedSampleId(germlineReportSample()));
	QList<int> re_ids = db.getValuesInt("SELECT cnv_id FROM report_configuration_re WHERE report_configuration_id=:0", QString::number(rc_id));
	foreach (int var_id, re_ids)
	{
		//match report variant against variant list
		RepeatLocus re = db.repeatExpansionGenotype(var_id);
		bool match_found = false;
		for (int i=0; i<getReList().count(); ++i)
		{
			if (getReList()[i].sameRegionAndLocus(re))
			{
				match_found = true;
				break;
			}
		}
		if (!match_found) missing_variants.append(re);
	}
	return missing_variants;
}

SomaticReportSettings& AnalysisDataController::getSomaticReportSettings()
{
    return somatic_report_settings_;
}

void AnalysisDataController::updateSomaticReportSettings()
{
    if (! somaticReportSupported())
    {
        THROW(ArgumentException, "Somatic report settings can only be updated for an analysis of the type " + analysisTypeToString(AnalysisType::SOMATIC_PAIR) + ". Analysis has type: " + analysisTypeToString(getAnalysisType()));
    }

    NGSD db;
    QString ps_tumor = getMainSampleName();
    QString ps_tumor_id = db.processedSampleId(ps_tumor);
    QString ps_normal = getNormalSampleName();
    QString ps_normal_id = db.processedSampleId(ps_normal);


    TargetRegionInfo target_region = variants_filter_state_.getTargetRegionInfo();
    //Set data in somatic report settings
    somatic_report_settings_.report_config->setTargetRegionName(target_region.name);

    somatic_report_settings_.report_config->setFilterName((variants_filter_state_.getFilterName())); //filter name -> goes to NGSD som. rep. conf.
    somatic_report_settings_.report_config->setFilters(variants_filter_state_.getFilterCascade()); //filter cascase -> goes to report helper

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

    somatic_report_settings_.target_region_filter = target_region;
    if(! target_region.isValid()) //use processing system data in case no filter is set
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
        emit thrownWarning("No HRD score found", "Warning:\nNo hrd score values found in the imported QC of tumor sample. HRD score set to 0.");
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

        foreach (const QString& line, cnv_data)
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
}

void AnalysisDataController::generateSomaticReport(QString filepath)
{
    if(!SomaticReportHelper::checkGermlineSNVFile(somatic_control_tissue_variants_))
    {
        thrownWarning("Somatic report", "DNA report cannot be created because germline GSVar-file is invalid. Please check control tissue variant file.");
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
        Helper::moveFile(tmp_xml, Settings::path("gsvar_xml_folder") + "\\" + getMainSampleName() + "-" + getNormalSampleName() + ".xml");

        Log::perf("Generating somatic report XML took ", timer);
    }
    catch(Exception e)
    {
        thrownWarning("creation of XML file failed", e.message());
    }

    //Generate RTF
    timer.start();
    QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
    report.storeRtf(temp_filename);
    Helper::moveFile(temp_filename, filepath);
    Log::perf("Generating somatic report RTF took ", timer);

    //Generate files for QBIC upload
    timer.start();
    QString path = getMainSampleName() + "-" + getNormalSampleName();
    if (GlobalServiceProvider::fileLocationProvider().isLocal()) path = Settings::string("qbic_data_path") + "/" + path;
    report.storeQbicData(path);
    Log::perf("Generating somatic report QBIC data took ", timer);
}

//transforms png data into list of tuples (png data in hex format, width, height)
QList<RtfPicture> AnalysisDataController::pngsFromFiles(QStringList files)
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

void AnalysisDataController::generateSomaticRnaReport(QString filepath, QString rna_ps_name)
{
    QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
    NGSD db;
    QString rna_ps_id = db.processedSampleId(rna_ps_name);

    SomaticRnaReportSettings rna_report_data = somatic_report_settings_;
    rna_report_data.rna_ps_name = rna_ps_name;
    rna_report_data.rna_fusion_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::FUSIONS).filename;
    rna_report_data.rna_expression_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::EXPRESSION).filename;
    rna_report_data.rna_bam_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::BAM).filename;
    rna_report_data.ref_genome_fasta_file = Settings::string("reference_genome");

    try
    {
        QString filename = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::EXPRESSION_CORR).filename;
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
            TSVFileStream cohort_file( GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::EXPRESSION_COHORT).filename);
            rna_report_data.cohort_size = cohort_file.header().count()-1;
        }
        catch(Exception)
        {
        }
    }

    rna_report_data.rna_qcml_data = db.getQCData(rna_ps_id);

    //Add data from fusion pics
    try
    {
        rna_report_data.fusion_pics = pngsFromFiles(GlobalServiceProvider::database().getRnaFusionPics(rna_ps_name));
    }
    catch(Exception) //Nothing to do here
    {
    }
    //Add data from expression plots
    try
    {
        rna_report_data.expression_plots = pngsFromFiles(GlobalServiceProvider::database().getRnaExpressionPlots(rna_ps_name));
    }
    catch(Exception)
    {
    }

    //Look in tumor sample for HPA reference tissue
    QStringList tmp_rna_ref_tissue;
    foreach(const auto& entry, db.getSampleDiseaseInfo(db.sampleId(rna_ps_name), "RNA reference tissue"))
    {
        tmp_rna_ref_tissue.append(entry.disease_info);
    }
    tmp_rna_ref_tissue.removeDuplicates();
    rna_report_data.rna_hpa_ref_tissue = tmp_rna_ref_tissue.join(", ");

    SomaticRnaReport rna_report(variants_, cnvs_, svs_, somatic_control_tissue_variants_,  rna_report_data);

    rna_report.writeRtf(temp_filename);
    Helper::moveFile(temp_filename, filepath);
}

void AnalysisDataController::generateSomaticCfDnaReport(QString filepath)
{
    QStringList errors;
    CfdnaDiseaseCourseTable table = GSvarHelper::cfdnaTable(getMainSampleName(), errors, true);
    SomaticcfDNAReportData data(somatic_report_settings_, table);
    SomaticcfDnaReport report(data);

    QByteArray temp_filename = Helper::tempFileName(".rtf").toUtf8();
    report.writeRtf(temp_filename);
    Helper::moveFile(temp_filename, filepath);
}

void AnalysisDataController::generateGermlineReport(QString filepath, QString type)
{
    NGSD db;
    QString ps_name = germlineReportSample();
    QString ps_id = db.processedSampleId(ps_name);

    //set report type
    germline_report_settings_.report_type = type;

    //prepare report generation data
    PrsTable prs_table;
    FileLocationList prs_files = GlobalServiceProvider::fileLocationProvider().getPrsFiles(false).filterById(ps_name);
    if (prs_files.count()==1) prs_table.load(prs_files[0].filename);

    GermlineReportGeneratorData data(GSvarHelper::build(), ps_name, variants_, cnvs_, svs_, repeat_expansions_, prs_table, germline_report_settings_, variants_filter_state_.getFilterCascade(), GSvarHelper::preferredTranscripts(), GlobalServiceProvider::statistics());
    data.processing_system_roi = GlobalServiceProvider::database().processingSystemRegions(db.processingSystemIdFromProcessedSample(ps_name), false);
    data.ps_bam = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::BAM).filename;
    data.ps_lowcov = GlobalServiceProvider::database().processedSamplePath(ps_id, PathType::LOWCOV_BED).filename;
    if (variants_filter_state_.getTargetRegionInfo().isValid())
    {
        data.roi = variants_filter_state_.getTargetRegionInfo();
        data.roi.genes = db.genesToApproved(data.roi.genes, true);
    }

    //start worker in background
    ReportWorker* worker = new ReportWorker(data, filepath);
	GlobalServiceProvider::startJob(worker, true);
}

VariantValidation AnalysisDataController::getSmallVariantValidationEntry(int variant_idx, QString ps_name)
{
	if (ps_name == "") ps_name = getMainSampleName();

    Variant& variant = variants_[variant_idx];

    NGSD db;

    //get variant ID - add if missing
    QString variant_id = db.variantId(variant, false);
    if (variant_id=="")
    {
        variant_id = db.addVariant(variant, variants_);
    }

    //get sample ID
    QString sample_id = db.sampleId(ps_name);

    //get variant validation ID - add if missing
	VariantValidation var_val = db.variantValidationSmallVariant(variant_id.toUtf8(), sample_id.toUtf8(), false);
	if (var_val.val_id == -1)
    {
		//add genotype and status to newly created variant validation
        QByteArray genotype = "het";
        int i_genotype = variants_.getSampleHeader().infoByID(ps_name).column_index;
        if (i_genotype!=-1) //genotype column only available in germline, but not for somatic analysis.
        {
            genotype = variant.annotations()[i_genotype];
        }
		var_val.genotype = genotype;
        var_val.status = "n/a";
	}
	return var_val;
}

VariantValidation AnalysisDataController::getCnvValidationEntry(int variant_idx, QString ps_name)
{
	if (ps_name == "") ps_name = getMainSampleName();

	NGSD db;
	const CopyNumberVariant& cnv = getCnvList()[variant_idx];
	QString ps_id = db.processedSampleId(ps_name, false);

	//get CNV ID
	QString callset_id = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=:0", true, ps_id).toString();
	if (callset_id == "") THROW(DatabaseException, "No CNV callset found for processed sample id " + ps_id + "!");
	QString cnv_id = db.cnvId(cnv, Helper::toInt(callset_id, "callset_id"), false);
	if (cnv_id == "")
	{
		// import CNV into NGSD
		cnv_id = db.addCnv(Helper::toInt(callset_id, "callset_id"), cnv, cnvs_);
	}

	QString sample_id = db.sampleId(db.processedSampleName(ps_id));

	VariantValidation cnv_val = db.variantValidationCnvVariant(cnv_id.toUtf8(), sample_id.toUtf8(), false);

	//add missing fields if newly created
	if (cnv_val.val_id == -1)
	{
		cnv_val.status = "n/a";
		cnv_val.user_id = LoginManager::userIdAsString().toInt();
	}

	return cnv_val;
}

VariantValidation AnalysisDataController::getSvValidationEntry(int variant_idx, QString ps_name)
{
	if (ps_name == "") ps_name = getMainSampleName();

	NGSD db;
	const BedpeLine& sv = svs_[variant_idx];
	QString ps_id = db.processedSampleId(ps_name, false);

	//get SV ID
	QString callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, ps_id).toString();
	if (callset_id == "") THROW(DatabaseException, "No callset found for processed sample id " + ps_id + "!");
	int sv_id = db.svId(sv, Helper::toInt(callset_id, "callset_id"), svs_);
	if (sv_id == -1)
	{
		sv_id = db.addSv(Helper::toInt(callset_id), sv, svs_);
		THROW(DatabaseException, "SV not found in the NGSD! ");
	}

	//get sample ID
	QString sample_id = db.sampleId(db.processedSampleName(ps_id));

	//get variant validation ID - add if missing
	QVariant val_id = db.getValue("SELECT id FROM variant_validation WHERE "+ db.svTableName(sv.type()) + "_id='" + QString::number(sv_id) + "' AND sample_id='" + sample_id + "'", true);
	VariantValidation sv_val = db.variantValidationSvVariant(QByteArray::number(sv_id), sv.type(), sample_id.toUtf8(), false);

	//add missing fields if newly created
	if (sv_val.val_id == -1)
	{
		sv_val.status = "n/a";
		sv_val.user_id = LoginManager::userIdAsString().toInt();
	}

	return sv_val;
}

void AnalysisDataController::storeVariantValidation(const VariantValidation& var_val)
{
	NGSD db;
	db.storeVariantValidation(var_val, QByteArray::number(LoginManager::userId()));
}

EvaluationSheetData AnalysisDataController::getEvaluationSheetData()
{
    if (! germlineReportSupported())
    {
        THROW(ArgumentException, "Evaluation sheet data can only be generated if the loaded analysis supports the germline report.");
    }

    NGSD db;
    QString base_name = germlineReportSample();
    QString sample_id = db.sampleId(base_name);
    QString ps_id = db.processedSampleId(base_name);

    EvaluationSheetData evaluation_sheet_data = db.evaluationSheetData(ps_id, false);
    evaluation_sheet_data.build = GSvarHelper::build();
    if (evaluation_sheet_data.ps_id == "") //No db entry found > init
    {
        evaluation_sheet_data.ps_id = db.processedSampleId(base_name);
        evaluation_sheet_data.dna_rna = db.getSampleData(sample_id).name_external;
        // make sure reviewer 1 contains name not user id
        evaluation_sheet_data.reviewer1 = db.userName(db.userId(germline_report_settings_.report_config->createdBy()));
        evaluation_sheet_data.review_date1 = germline_report_settings_.report_config->createdAt().date();
        evaluation_sheet_data.reviewer2 = LoginManager::userName();
        evaluation_sheet_data.review_date2 = QDate::currentDate();
    }

    return evaluation_sheet_data;
}

void AnalysisDataController::finalizeGermlineReportConfig(int user_id)
{
    NGSD db;
    QString processed_sample_id = db.processedSampleId(germlineReportSample(), false);
    if (processed_sample_id=="")
    {
        INFO(ArgumentException, "Sample was not found in the NGSD!");
    }

    //check config exists
    int conf_id = db.reportConfigId(processed_sample_id);
    if (conf_id==-1) INFO(ArgumentException, "No report configuration for this sample found in the NGSD!");

    db.finalizeReportConfig(conf_id, user_id);

    //update report settings data structure
    germline_report_settings_.report_config = db.reportConfig(conf_id, variants_, cnvs_, svs_, repeat_expansions_);
    connect(germline_report_settings_.report_config.data(), SIGNAL(variantsChanged()), this, SLOT(storeGermlineReportConfig()));
}

TumorOnlyReportWorkerConfig AnalysisDataController::getTumorOnlyReportWorkerConfig()
{
    NGSD db;
    QString ps = getMainSampleName();
    int sys_id = db.processingSystemIdFromProcessedSample(ps);

    TumorOnlyReportWorkerConfig config;
    config.threads = Settings::integer("threads");
    config.sys = db.getProcessingSystemData(sys_id);
    config.ps_data = db.getProcessedSampleData(db.processedSampleId(ps));
    config.roi = variants_filter_state_.getTargetRegionInfo();
    config.low_coverage_file = GlobalServiceProvider::fileLocationProvider().getSomaticLowCoverageFile().filename;
    config.bam_file = GlobalServiceProvider::fileLocationProvider().getBamFiles(true).at(0).filename;
    config.filter_result = variants_filter_result_;
    config.preferred_transcripts = GSvarHelper::preferredTranscripts();
    config.build = GSvarHelper::build();

    return config;
}

QStringList AnalysisDataController::getValidFilterEntries() const
{
    //determine valid filter entries from filter column (and add new filters low_mappability/mosaic to make outdated GSvar files work as well)
    QStringList valid_filter_entries = variants_.filters().keys();
    if (!valid_filter_entries.contains("low_mappability")) valid_filter_entries << "low_mappability";
    if (!valid_filter_entries.contains("mosaic")) valid_filter_entries << "mosaic";

    return valid_filter_entries;
}

QList<KeyValuePair> AnalysisDataController::inheritanceByGene(int variant_idx)
{
    NGSD db;
    const Variant& variant = variants_[variant_idx];
    QList<KeyValuePair> inheritance_by_gene;
    int i_genes = variants_.annotationIndexByName("gene", true, false);

    if (i_genes!=-1)
    {
        GeneSet genes = GeneSet::createFromText(variant.annotations()[i_genes], ',');
        for (const QByteArray& gene : std::as_const(genes))
        {
            GeneInfo gene_info = db.geneInfo(gene);
            inheritance_by_gene << KeyValuePair{gene, gene_info.inheritance};
        }
    }
    return inheritance_by_gene;
}

ClinvarUploadData AnalysisDataController::getClinvarUploadData(int var_idx1, int var_idx2, VariantType type)
{


	if (type == VariantType::SNVS_INDELS)
	{
		return getClinvarUploadDataSmallVariant(var_idx1, var_idx2);
	}
	else if (type == VariantType::CNVS)
	{
		return getClinvarUploadDataCnv(var_idx1, var_idx2);
	}
	else if (type == VariantType::SVS)
	{
		return getClinvarUploadDataSv(var_idx1, var_idx2);
	}
	else
	{
		THROW(ProgrammingException, "Unsupported VariantType in getClinvarUploadData()! VariantType: " + variantTypeToString(type));
	}
}

ClinvarUploadData AnalysisDataController::getClinvarUploadDataSmallVariant(int var_idx1, int var_idx2)
{
	if(var_idx1 < 0)
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
	if(var_idx2 < 0)
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
	data.snv1 = getSmallVariantList()[var_idx1];
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) data.snv2 = getSmallVariantList()[var_idx2];

	// get report info
	if ( ! getGermlineReportConfig()->exists(VariantType::SNVS_INDELS, var_idx1))
	{
		INFO(InformationMissingException, "The variant 1 has to be in the report configuration to be published!");
	}
	data.report_variant_config1 = getGermlineReportConfig()->get(VariantType::SNVS_INDELS, var_idx1);
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if (getGermlineReportConfig()->exists(VariantType::SNVS_INDELS, var_idx2))
		{
			INFO(InformationMissingException, "The variant 2 has to be in the report configuration to be published!");
		}
		data.report_variant_config2 = getGermlineReportConfig()->get(VariantType::SNVS_INDELS, var_idx2);
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
	int gene_idx = getSmallVariantList().annotationIndexByName("gene");
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

	return data;
}

ClinvarUploadData AnalysisDataController::getClinvarUploadDataCnv(int var_idx1, int var_idx2)
{
	if(var_idx1 <0)
	{
		THROW(ArgumentException, "A valid index for the first CNV has to be provided!");
	}
	//abort if API key is missing
	if(Settings::string("clinvar_api_key", true).trimmed().isEmpty())
	{
		THROW(ProgrammingException, "ClinVar API key is needed, but not found in settings.\nPlease inform the bioinformatics team");
	}

	NGSD db;

	//(1) prepare data as far as we can
	ClinvarUploadData data;
	data.processed_sample = getMainSampleName();
	data.variant_type1 = VariantType::CNVS;
	if(var_idx1 < 0)
	{
		//Single variant submission
		data.submission_type = ClinvarSubmissionType::SingleVariant;
		data.variant_type2 = VariantType::INVALID;
	}
	else
	{
		//CompHet variant submission
		data.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
		data.variant_type2 = VariantType::CNVS;
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

	//get copy number variant info
	data.cnv1 = cnvs_[var_idx1];
	data.cn1 = data.cnv1.copyNumber(cnvs_.annotationHeaders());
	data.ref_cn1 = CnvList::determineReferenceCopyNumber(data.cnv1, sample_data.gender, GSvarHelper::build());

	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		data.cnv2 = cnvs_[var_idx1];
		data.cn2 = data.cnv2.copyNumber(cnvs_.annotationHeaders());
		data.ref_cn2 = CnvList::determineReferenceCopyNumber(data.cnv2, sample_data.gender, GSvarHelper::build());
	}

	// get report info
	if (!getGermlineReportConfig()->exists(VariantType::CNVS, var_idx1))
	{
		INFO(InformationMissingException, "The CNV has to be in the report configuration to be published!");
	}
	data.report_variant_config1 = getGermlineReportConfig()->get(VariantType::CNVS, var_idx1);

	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if (!getGermlineReportConfig()->exists(VariantType::CNVS, var_idx2))
		{
			INFO(InformationMissingException, "The CNV 2 has to be in the report configuration to be published!");
		}
		data.report_variant_config2 = getGermlineReportConfig()->get(VariantType::CNVS, var_idx2);
	}

	//check classification
	if (data.report_variant_config1.classification.trimmed().isEmpty() || (data.report_variant_config1.classification.trimmed() == "n/a"))
	{
		INFO(InformationMissingException, "The CNV has to be classified to be published!");
	}
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if (data.report_variant_config2.classification.trimmed().isEmpty() || (data.report_variant_config2.classification.trimmed() == "n/a"))
		{
			INFO(InformationMissingException, "The CNV 2 has to be classified to be published!");
		}
	}

	//genes
	data.genes = data.cnv1.genes();
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) data.genes <<  data.cnv2.genes();

	//determine NGSD ids of variant and report variant for variant 1
	int callset_id = db.getValue("SELECT id FROM cnv_callset WHERE processed_sample_id=" + getMainProcessedSampleId()).toInt();
	QString cnv_id = db.cnvId(data.cnv1, callset_id, false);
	if (cnv_id == "")
	{
		INFO(InformationMissingException, "The CNV has to be in NGSD and part of a report config to be published!");
	}
	data.variant_id1 = Helper::toInt(cnv_id);
	//extract report variant id
	int rc_id = db.reportConfigId(getMainProcessedSampleId());
	if (rc_id == -1 )
	{
		THROW(DatabaseException, "Could not determine report config id for sample " + data.processed_sample + "!");
	}

	data.report_variant_config_id1 = db.getValue("SELECT id FROM report_configuration_cnv WHERE report_configuration_id=" + QString::number(rc_id) + " AND cnv_id="
													 + QString::number(data.variant_id1), false).toInt();

	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		//determine NGSD ids of cnv for variant 2
		cnv_id = db.cnvId(data.cnv2, callset_id, false);
		if (cnv_id == "")
		{
			INFO(InformationMissingException, "The CNV 2 has to be in NGSD and part of a report config to be published!");
		}
		data.variant_id2 = Helper::toInt(cnv_id);

		//extract report variant id
		data.report_variant_config_id2 = db.getValue("SELECT id FROM report_configuration_cnv WHERE report_configuration_id=" + QString::number(rc_id) + " AND cnv_id="
														 + QString::number(data.variant_id2), false).toInt();
	}

	return data;
}

ClinvarUploadData AnalysisDataController::getClinvarUploadDataSv(int var_idx1, int var_idx2)
{
	//abort if 1st index is missing
	if(var_idx1 <0)
	{
		THROW(ArgumentException, "A valid index for the first SV has to be provided!");
	}
	//abort if API key is missing
	if(Settings::string("clinvar_api_key", true).trimmed().isEmpty())
	{
		THROW(ProgrammingException, "ClinVar API key is needed, but not found in settings.\nPlease inform the bioinformatics team");
	}

	NGSD db;
	QString ps_id = getMainProcessedSampleId();

	//(1) prepare data as far as we can
	ClinvarUploadData data;
	data.processed_sample = getMainSampleName();
	data.variant_type1 = VariantType::SVS;
	if(var_idx2 < 0)
	{
		//Single variant submission
		data.submission_type = ClinvarSubmissionType::SingleVariant;
		data.variant_type2 = VariantType::INVALID;
	}
	else
	{
		//CompHet variant submission
		data.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
		data.variant_type2 = VariantType::SVS;
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

	//get sv variant info
	data.sv1 = svs_[var_idx1];
	if(data.sv1.type() == StructuralVariantType::BND)
		WARNING(NotImplementedException, "The upload of translocations is not supported by the ClinVar API. Please use the manual submission through the ClinVar website.");

	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		data.sv2 = svs_[var_idx2];
		if(data.sv2.type() == StructuralVariantType::BND)
			WARNING(NotImplementedException, "The upload of translocations is not supported by the ClinVar API. Please use the manual submission through the ClinVar website.");
	}

	// get report info
	if (!getGermlineReportConfig()->exists(VariantType::SVS, var_idx1))
	{
		INFO(InformationMissingException, "The SV has to be in the report configuration to be published!");
	}
	data.report_variant_config1 = getGermlineReportConfig()->get(VariantType::SVS, var_idx1);
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if (!getGermlineReportConfig()->exists(VariantType::SVS, var_idx2))
		{
			INFO(InformationMissingException, "The SV 2 has to be in the report configuration to be published!");
		}
		data.report_variant_config2 = getGermlineReportConfig()->get(VariantType::SVS, var_idx2);
	}

	//check classification
	if (data.report_variant_config1.classification.trimmed().isEmpty() || (data.report_variant_config1.classification.trimmed() == "n/a"))
	{
		INFO(InformationMissingException, "The SV has to be classified to be published!");
	}
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if (data.report_variant_config2.classification.trimmed().isEmpty() || (data.report_variant_config2.classification.trimmed() == "n/a"))
		{
			INFO(InformationMissingException, "The SV 2 has to be classified to be published!");
		}
	}

	//genes
	data.genes = data.sv1.genes(svs_.annotationHeaders());
	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) data.genes <<  data.sv2.genes(svs_.annotationHeaders());

	//get callset id
	QString callset_id = db.getValue("SELECT id FROM sv_callset WHERE processed_sample_id=:0", true, ps_id).toString();
	if (callset_id == "") THROW(DatabaseException, "No callset found for processed sample id " + ps_id + "!");

	//determine NGSD ids of variant and report variant for variant 1
	int sv_id = db.svId(data.sv1, callset_id.toInt(), svs_, false);
	if (sv_id == -1)
	{
		INFO(InformationMissingException, "The SV has to be in NGSD and part of a report config to be published!");
	}


	data.variant_id1 = Helper::toInt(sv_id);
	//extract report variant id
	int rc_id = db.reportConfigId(ps_id);
	if (rc_id == -1 )
	{
		THROW(DatabaseException, "Could not determine report config id for sample " + data.processed_sample + "!");
	}

	data.report_variant_config_id1 = db.getValue("SELECT id FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(rc_id) + " AND "
													 + db.svTableName(data.sv1.type())+ "_id=" + QString::number(data.variant_id1), false).toInt();

	if(data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		//determine NGSD ids of sv for variant 2
		sv_id = db.svId(data.sv2, callset_id.toInt(), svs_, true);
		if (sv_id == -1)
		{
			INFO(InformationMissingException, "The SV 2 has to be in NGSD and part of a report config to be published!");
		}
		data.variant_id2 = Helper::toInt(sv_id);

		//extract report variant id
		data.report_variant_config_id2 = db.getValue("SELECT id FROM report_configuration_sv WHERE report_configuration_id=" + QString::number(rc_id) + " AND "
														 + db.svTableName(data.sv2.type())+ "_id=" + QString::number(data.variant_id2), false).toInt();
	}

	return data;
}

ClinvarUploadData AnalysisDataController::getClinvarUploadData(int var_pub_id)
{
	ClinvarUploadData data;
	NGSD db;

	data.variant_publication_id = var_pub_id;

	// get publication data
	SqlQuery query = db.getQuery();
	query.prepare("SELECT sample_id, variant_id, variant_table, details, user_id, linked_id FROM variant_publication WHERE id=:0");
	query.bindValue(0, var_pub_id);
	query.exec();

	if (query.size() != 1) THROW(DatabaseException, "Invalid variant publication id!");
	query.next();

	QString sample_id = query.value("sample_id").toString();
	QString variant_table = query.value("variant_table").toString();
	QString variant_table2;
	data.variant_id1 = query.value("variant_id").toInt();
	QStringList details = query.value("details").toString().split(';');
	data.user_id = query.value("user_id").toInt();
	QString linked_id = query.value("linked_id").toString();


	//parse details entry
	data.report_variant_config_id1 = -1;
	data.report_variant_config_id2 = -1;
	bool switch_variants = false;
	foreach (const QString& kv_pair, details)
	{
		//determine submission type
		if (kv_pair.startsWith("submission_type="))
		{
			QString submission_type = kv_pair.split('=').at(1).trimmed();
			if (submission_type=="SingleVariant") data.submission_type = ClinvarSubmissionType::SingleVariant;
			else if(submission_type=="CompoundHeterozygous") data.submission_type = ClinvarSubmissionType::CompoundHeterozygous;
			else THROW(DatabaseException, "Invalid submission type!");
		}

		//get variant order
		if (kv_pair.startsWith("variant_id1="))
		{
			int var_id1 = Helper::toInt(kv_pair.split('=').at(1), "variant_id1");
			switch_variants = (var_id1 != data.variant_id1);
		}
		if (kv_pair.startsWith("variant_id2="))
		{
			data.variant_id2 = Helper::toInt(kv_pair.split('=').at(1), "variant_id2");
		}

		//get variant report config id
		if (kv_pair.startsWith("variant_rc_id1="))
		{
			QString rvc_str = kv_pair.split('=').at(1);
			if (rvc_str.contains(':')) rvc_str = rvc_str.split(':').at(1);
			data.report_variant_config_id1 = Helper::toInt(rvc_str, "variant_rc_id1");
		}
		if (kv_pair.startsWith("variant_rc_id2="))
		{
			QString rvc_str = kv_pair.split('=').at(1);
			if (rvc_str.contains(':'))
			{
				variant_table2 = rvc_str.split(':').at(0);
				rvc_str = rvc_str.split(':').at(1);
			}
			data.report_variant_config_id2 = Helper::toInt(rvc_str, "variant_rc_id2");
		}

		//legacy support: get variant report config id
		if (kv_pair.startsWith("variant_rc_id="))
		{
			QString rvc_str = kv_pair.split('=').at(1);
			if (rvc_str.contains(':')) rvc_str = rvc_str.split(':').at(1);
			data.report_variant_config_id1 = Helper::toInt(rvc_str, "variant_rc_id");
		}

	}
	if (data.report_variant_config_id1 < 0) THROW(DatabaseException, "No report variant config id information found in variant publication!");
	if ((data.submission_type == ClinvarSubmissionType::CompoundHeterozygous) && (data.report_variant_config_id2 < 0))
	{
		THROW(DatabaseException, "No report variant config id for second variant information found in variant publication!");
	}


	//get variant info
	if (variant_table == "variant")
	{
		data.variant_type1 = VariantType::SNVS_INDELS;
		data.snv1 = db.variant(QString::number(data.variant_id1));
	}
	else if(variant_table == "cnv")
	{
		data.variant_type1 = VariantType::CNVS;
		data.cnv1 = db.cnv(data.variant_id1);
	}
	else //SNV
	{
		data.variant_type1 = VariantType::SVS;
		StructuralVariantType sv_type;
		if(variant_table == "sv_deletion") sv_type = StructuralVariantType::DEL;
		else if(variant_table == "sv_duplication") sv_type = StructuralVariantType::DUP;
		else if(variant_table == "sv_insertion") sv_type = StructuralVariantType::INS;
		else if(variant_table == "sv_invertion") sv_type = StructuralVariantType::INV;
		else if(variant_table == "sv_translocation" ) sv_type = StructuralVariantType::BND;
		else THROW(ArgumentException, "Invalid SV table '" + variant_table + "'!");

		data.sv1 = db.structuralVariant(data.variant_id1, sv_type, getSvList());
	}

	if (data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		if (linked_id == "") THROW(DatabaseException, "No linked variant provided for compound heterozygous variant publication!");
		//get publication data of second variant
		query.bindValue(0, var_pub_id);
		query.exec();
		if (query.size() != 1) THROW(DatabaseException, "Invalid variant publication id of second variant!");
		query.next();


		//get variant info for 2nd var from details field
		if (variant_table2 == "variant")
		{
			data.variant_type2 = VariantType::SNVS_INDELS;
			data.snv2 = db.variant(QString::number(data.variant_id2));
		}
		else if(variant_table2 == "cnv")
		{
			data.variant_type2 = VariantType::CNVS;
			data.cnv2 = db.cnv(data.variant_id2);
		}
		else //SNV
		{
			data.variant_type2 = VariantType::SVS;
			StructuralVariantType sv_type;
			if(variant_table2 == "sv_deletion") sv_type = StructuralVariantType::DEL;
			else if(variant_table2 == "sv_duplication") sv_type = StructuralVariantType::DUP;
			else if(variant_table2 == "sv_insertion") sv_type = StructuralVariantType::INS;
			else if(variant_table2 == "sv_invertion") sv_type = StructuralVariantType::INV;
			else if(variant_table2 == "sv_translocation" ) sv_type = StructuralVariantType::BND;
			else THROW(ArgumentException, "Invalid SV table '" + variant_table2 + "'!");

			data.sv2 = db.structuralVariant(data.variant_id2, sv_type, getSvList());
		}
	}

	//switch order of variants
	if (switch_variants)
	{
		int variant_id2 = data.variant_id1;
		int report_config_variant_id2 = data.report_variant_config_id1;
		ReportVariantConfiguration report_variant_config2 = data.report_variant_config1;
		VariantType variant_type2 = data.variant_type1;
		Variant snv2 = data.snv1;
		CopyNumberVariant cnv2 = data.cnv1;
		int cn2 = data.cn1;
		int ref_cn2 = data.ref_cn1;
		BedpeLine sv2 = data.sv1;

		data.variant_id1 = data.variant_id2;
		data.report_variant_config_id1 = data.report_variant_config_id2;
		data.report_variant_config1 = data.report_variant_config2;
		data.variant_type1 = data.variant_type2;
		data.snv1 = data.snv2;
		data.cnv1 = data.cnv2;
		data.cn1 = data.cn2;
		data.ref_cn1 = data.ref_cn2;
		data.sv1 = data.sv2;

		data.variant_id2 = variant_id2;
		data.report_variant_config_id2 = report_config_variant_id2;
		data.report_variant_config2 = report_variant_config2;
		data.variant_type2 = variant_type2;
		data.snv2 = snv2;
		data.cnv2 = cnv2;
		data.cn2 = cn2;
		data.ref_cn2 = ref_cn2;
		data.sv2 = sv2;
	}

	//get sample data
	SampleData sample_data = db.getSampleData(sample_id);

	//get disease info
	data.disease_info = db.getSampleDiseaseInfo(sample_id, "OMIM disease/phenotype identifier");
	data.disease_info.append(db.getSampleDiseaseInfo(sample_id, "Orpha number"));
	if (data.disease_info.length() < 1)
	{
		emit thrownWarning("No disease info", "The sample has to have at least one OMIM or Orphanet disease identifier to publish a variant in ClinVar.");
	}

	// get affected status
	data.affected_status = sample_data.disease_status;

	//get phenotype(s)
	data.phenos = sample_data.phenotypes;

	//get variant report config
	QStringList messages;
	data.report_variant_config1 = db.reportVariantConfiguration(data.report_variant_config_id1, data.variant_type1, messages);
	if (data.submission_type == ClinvarSubmissionType::CompoundHeterozygous)
	{
		data.report_variant_config2 = db.reportVariantConfiguration(data.report_variant_config_id2, data.variant_type2, messages);
	}

	if (messages.size() > 0) emit thrownWarning("Report Variant Configuration", messages.join('\n'));

	//update snv_indel classification
	if(data.variant_type1 == VariantType::SNVS_INDELS)
	{
		data.report_variant_config1.classification = db.getClassification(data.snv1).classification;
		if (data.report_variant_config1.classification.trimmed().isEmpty() || (data.report_variant_config1.classification.trimmed() == "n/a"))
		{
			emit thrownWarning("No Classification", "The variant has to have a classification to be published!");
		}
	}

	// get report config
	int rc_id = -1;
	if (data.variant_type1 == VariantType::SNVS_INDELS)
	{
		rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_variant WHERE id=" + QString::number(data.report_variant_config_id1) + " AND variant_id="
								+ QString::number(data.variant_id1), false).toInt();
	}
	else if (data.variant_type1 == VariantType::CNVS)
	{
		rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_cnv WHERE id=" + QString::number(data.report_variant_config_id1) + " AND variant_id="
								+ QString::number(data.variant_id1), false).toInt();
	}
	else if (data.variant_type1 == VariantType::SVS)
	{
		rc_id = db.getValue("SELECT report_configuration_id FROM report_configuration_sv WHERE id=" + QString::number(data.report_variant_config_id1) + " AND " + db.svTableName(data.sv1.type())
							+ "_id=" + QString::number(data.variant_id1), false).toInt();
	}
	else
	{
		THROW(ArgumentException, "Invalid variant type!");
	}

	//get genes
	data.genes = db.genesOverlapping(data.snv1.chr(), data.snv1.start(), data.snv1.end(), 5000);

	//get processed sample id
	data.processed_sample = db.processedSampleName(db.getValue("SELECT processed_sample_id FROM report_configuration WHERE id=" + QString::number(rc_id)).toString());

	return data;
}



void AnalysisDataController::changeSmallVariantList(int variant_idx, QByteArray column, QByteArray new_text, bool throw_if_not_found)
{
    int idx_column = variants_.annotationIndexByName(column, true, throw_if_not_found);

    if (idx_column!=-1)
    {
        Variant variant = variants_[variant_idx];
        if(variant.annotations()[idx_column] != new_text)
        {
            variant.annotations()[idx_column] = new_text;
            variants_changed_.append(VariantListChange{variant, column, new_text});

            emit smallVariantsChanged();
        }
    }
}

void AnalysisDataController::changeSmallVariantListBatch(QList<int> variant_indicies, QByteArray column, QByteArrayList new_text, bool throw_if_not_found)
{
    if (variant_indicies.count() != new_text.count()) THROW(ProgrammingException, "Number of variants and number of annotation text must be the same!");

    int idx_column = variants_.annotationIndexByName(column, true, throw_if_not_found);

    if (idx_column!=-1)
    {
		for (int i=0; i < variant_indicies.count(); i++)
        {
            Variant variant = variants_[variant_indicies[i]];
            if(variant.annotations()[idx_column] != new_text[i])
            {
                variant.annotations()[idx_column] = new_text[i];
                variants_changed_.append(VariantListChange{variant, column, new_text[i]});
            }
        }
        emit smallVariantsChanged();
    }
}

void AnalysisDataController::setSmallVariantComment(int variant_idx, QByteArray new_comment)
{
    const Variant& variant = variants_[variant_idx];

    //add variant if missing
    NGSD db;
    if (db.variantId(variant, false)=="")
    {
        db.addVariant(variant, variants_);
    }

    //update DB
    db.setComment(variant, new_comment);

    changeSmallVariantList(variant_idx, "comment", new_comment, false);
}

void AnalysisDataController::reannotateSomaticVariantInterpretation()
{
    AnalysisType type = getAnalysisType();
    if (type!=AnalysisType::SOMATIC_SINGLESAMPLE && type!=AnalysisType::SOMATIC_PAIR) return;

    QList<int> indicies;
    QByteArrayList vicc_scores;
    QByteArrayList vicc_comments;

    NGSD db;
    for(int i=0; i<variants_.count(); ++i)
    {
        //skip variants without VICC infos in NGSD
        SomaticViccData vicc_data = db.getSomaticViccData(variants_[i], false);
        if (vicc_data.created_by.isEmpty()) continue;

        indicies << i;
        vicc_scores << SomaticVariantInterpreter::viccScoreAsString(vicc_data).toUtf8();
        vicc_comments << vicc_data.comment.toUtf8();
    }
    changeSmallVariantListBatch(indicies, "NGSD_som_vicc_interpretation", vicc_scores);
    changeSmallVariantListBatch(indicies, "NGSD_som_vicc_comment", vicc_comments);
}

void AnalysisDataController::annotateVariantRankingScores(VariantScores::Result variant_scores, bool add_explanations)
{
    VariantScores::annotate(variants_, variant_scores, add_explanations);
}

const GeneSet& AnalysisDataController::getHetHitGenes()
{
	if (var_het_genes_uptodate_) return var_het_genes_;

    //create list of genes with heterozygous variant hits
	var_het_genes_.clear();
    int i_genes = variants_.annotationIndexByName("gene", true, false);
    QList<int> i_genotypes = variants_.getSampleHeader().sampleColumns(true);
    i_genotypes.removeAll(-1);

    if (i_genes!=-1 && i_genotypes.count()>0)
    {
        for (int i=0; i<variants_.count(); ++i)
        {
            if (! variants_filter_result_.passing(i)) continue;

            bool all_genos_het = true;
            foreach(int i_genotype, i_genotypes)
            {
                if (variants_[i].annotations()[i_genotype]!="het")
                {
                    all_genos_het = false;
                }
            }
            if (!all_genos_het) continue;
			var_het_genes_.insert(GeneSet::createFromText(variants_[i].annotations()[i_genes], ','));
        }
    }
	var_het_genes_uptodate_ = true;
	return var_het_genes_;
}

PhenotypeList AnalysisDataController::getSamplePhenotypes()
{
	QString ps_name = getMainSampleName();
	NGSD db;
	QString sample_id = db.sampleId(ps_name);
	return db.getSampleData(sample_id).phenotypes;
}

double AnalysisDataController::calcMendelianErrorRate(int& used, int& errors) const
{
    if(getAnalysisType() != AnalysisType::GERMLINE_TRIO)
    {
        THROW(ProgrammingException, "Calculation of mendelian errors is only supported for trio analysis. Can't be called for the analysis type: " + analysisTypeToString(getAnalysisType()));
    }

    SampleHeaderInfo infos = variants_.getSampleHeader();
    int i_c = infos.infoByStatus(true).column_index;
    int i_f = infos.infoByStatus(false, "male").column_index;
    int i_m = infos.infoByStatus(false, "female").column_index;

    int i_qual = variants_.annotationIndexByName("quality");

    used = 0;
    errors = 0;
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

    return percentage;
}

BedFile AnalysisDataController::genesToRegions(GeneSet genes)
{
    NGSD db;
    BedFile regions;
    for(const QByteArray& gene: std::as_const(genes))
    {
        regions.add(GlobalServiceProvider::geneToRegions(gene, db));
    }
    regions.extend(5000);
    regions.merge();

    return regions;
}

Histogram AnalysisDataController::afHistogram(bool filtered) const
{
    //create histogram
    Histogram hist(0.0, 1.0, 0.05);
    int col_quality = variants_.annotationIndexByName("quality");
    for (int i=0; i<variants_.count(); ++i)
    {
        if (filtered && ! variants_filter_result_.passing(i)) continue;

        QByteArrayList parts = variants_[i].annotations()[col_quality].split(';');
        foreach(const QByteArray& part, parts)
        {
            if (part.startsWith("AF="))
            {
                bool ok;
                QString value_str = part.mid(3);
                if (getAnalysisType()==AnalysisType::GERMLINE_TRIO) value_str = value_str.split(',')[0];
                double value = value_str.toDouble(&ok);
                if (ok)
                {
                    hist.inc(value, true);
                }
            }
        }
    }
    return hist;
}

Histogram AnalysisDataController::cnHistogram(QString sample_id, Chromosome chr, int start, int end) const
{
    FileLocation seg_file = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(false).filterById(sample_id)[0];

    //determine CN values
    QVector<double> cn_values;
    VersatileTextStream stream(seg_file.filename);
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

    return hist;
}

Histogram AnalysisDataController::bafHistogram(QString sample_id, Chromosome chr, int start, int end) const
{
    FileLocation baf_file = GlobalServiceProvider::fileLocationProvider().getBafFiles(false).filterById(sample_id)[0];

    //determine CN values
    Histogram hist(0.0, 1.0, 0.025);
    VersatileFile file(baf_file.filename, false);
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

	return hist;
}

void AnalysisDataController::exportGSvar(QString file_name, bool as_vcf)
{
    //create new GSvar file with passing variants
    VariantList output;
    output.copyMetaData(variants_);
    for(int i=0; i<variants_.count(); ++i)
    {
        if (variants_filter_result_.passing(i))
        {
            output.append(variants_[i]);
        }
    }

    if (! as_vcf)
    {
        output.store(file_name);
    }
    else
    {
        //convert to VCF
        QString ref_genome = Settings::string("reference_genome", false);
        VcfFile vcf_file = VcfFile::fromGSvar(output, ref_genome);
        vcf_file.store(file_name);
    }
    emit thrownInfo("GSvar export", QString("Exported GSvar file") + (as_vcf ? " as VCF" : "") + " with " + QString::number(output.count()) + " variants.");
}

void AnalysisDataController::exportHerediCareVCF(QString herediCare_id, QString file_name)
{
    GeneSet genes;
    genes << "ABRAXAS1" << "APC" << "ATM" << "BARD1" << "BRCA1" << "BRCA2" << "BRIP1" << "CDH1" << "CDKN2A" << "CHEK2" << "EPCAM" << "FANCC" << "FANCM" << "HOXB13" << "MEN1" << "MLH1" << "MRE11" << "MSH2" << "MSH6" << "MUTYH" << "NBN" << "NF1" << "NTHL1" << "PALB2" << "PMS2" << "POLD1" << "POLE" << "PTEN" << "RAD50" << "RAD51C" << "RAD51D" << "SMARCA4" << "STK11" << "TP53";
    FastaFileIndex ref_genome(Settings::string("reference_genome", false));


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
    vcf.setSampleNames(QByteArrayList() << herediCare_id.toUtf8());

    //add variants in ROI to VCF
    int i_qual = variants_.annotationIndexByName("quality");
    int i_geno = variants_.getSampleHeader().value(0).column_index;
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
    emit thrownInfo("HerediCare VCF export", "Exported " + QString::number(vcf.count()) + " variants in exons/splice region of the " + QString::number(genes.count()) + " genes:\n" + genes.join(", ") + "\n\n" + QString::number(c_classified) + " of the variants have a classification.");
}


int AnalysisDataController::getValidRna(int rna_ps_id) const
{
    if (rna_ps_id == -1 && active_rna_ps_id_ == -1)
    {
        THROW(ProgrammingException, "No RNA id specified and no RNA id set in AnalysisDataController while checking or getting for RNA file.");
    }

    if (rna_ps_id != -1) return rna_ps_id;

    return active_rna_ps_id_;

}

int AnalysisDataController::getActiveRnaPsId() const
{
    return active_rna_ps_id_;
}
QString AnalysisDataController::getActiveRnaPsName() const
{
    if (! isRnaSet()) return "";

    return NGSD().processedSampleName(QString::number(active_rna_ps_id_));
}

QSet<int> AnalysisDataController::getRelatedRnaSampleIds() const
{
    NGSD db;
    QString sample = variants_.mainSampleName();
    QString sample_id = db.sampleId(sample);

    return db.relatedSamples(sample_id.toInt(), "same sample", "RNA");
}

QSet<int> AnalysisDataController::getRelatedRnaProcessedSampleIds() const
{
    QSet<int> ps_ids;
    QSet<int> sample_ids = getRelatedRnaSampleIds();

    NGSD db;

    foreach (int s_id, sample_ids)
    {
        foreach (int ps_id, db.getValuesInt("SELECT id FROM processed_sample WHERE sample_id = '" + QString::number(s_id) + "'"))
        {
            ps_ids.insert(ps_id);
        }
    }
    return ps_ids;
}

FileLocation AnalysisDataController::getRnaSplicing(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::SPLICING_ANN);
}

FileLocation AnalysisDataController::getRnaFusions(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::FUSIONS);
}

FileLocation AnalysisDataController::getRnaExpressionGenes(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::EXPRESSION);
}

FileLocation AnalysisDataController::getRnaExpressionExons(int rna_ps_id) const
{
    rna_ps_id = getValidRna(rna_ps_id);
    return GlobalServiceProvider::database().processedSamplePath(QString::number(rna_ps_id), PathType::EXPRESSION_EXON);
}

QSet<int> AnalysisDataController::getRelatedCfdnaSampleIds() const
{
    NGSD db;
    int sample_id = db.sampleId(getMainSampleName()).toInt();
    QSet<int> same_sample_ids = db.relatedSamples(sample_id, "same sample");
    same_sample_ids << sample_id; // add current sample id

    // get all related cfDNA
    QSet<int> cf_dna_sample_ids;
    foreach (int cur_sample_id, same_sample_ids)
    {
        cf_dna_sample_ids.unite(db.relatedSamples(cur_sample_id, "tumor-cfDNA"));
    }

    return cf_dna_sample_ids;
}

FilterState& AnalysisDataController::getSmallVariantsFilterState()
{
    return variants_filter_state_;
}

const FilterResult& AnalysisDataController::getSmallVariantsFilterResult() const
{
    return variants_filter_result_;
}

