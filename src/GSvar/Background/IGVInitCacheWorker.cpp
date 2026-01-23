#include "IGVInitCacheWorker.h"
#include "GlobalServiceProvider.h"
#include "Settings.h"
#include "IgvSessionManager.h"
#include "LoginManager.h"

IGVInitCacheWorker::IGVInitCacheWorker(const AnalysisType analysis_type, const QString current_filename)
    : BackgroundWorkerBase("IGV cache initializer")
    , analysis_type_(analysis_type)
    , current_filename_(current_filename)
{
}

void IGVInitCacheWorker::process()
{
    Log::info("Preloading IGV file information");
    //sample BAM file(s)
    FileLocationList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(true);
    foreach(const FileLocation& file, bams)
    {
        IgvSessionManager::get(0).addLocationToCache(file, true);
    }

    //sample BAF file(s)
    FileLocationList bafs = GlobalServiceProvider::fileLocationProvider().getBafFiles(true);
    foreach(const FileLocation& file, bafs)
    {
        if(analysis_type_ == SOMATIC_PAIR && !file.id.contains("somatic")) continue;
        IgvSessionManager::get(0).addLocationToCache(file, true);
    }

    //analysis VCF
    FileLocation vcf = GlobalServiceProvider::fileLocationProvider().getAnalysisVcf();
    bool igv_default_small = Settings::boolean("igv_default_small", true);
    IgvSessionManager::get(0).addLocationToCache(vcf, igv_default_small);

    //analysis SV file
    FileLocation bedpe = GlobalServiceProvider::fileLocationProvider().getAnalysisSvFile();
    bool igv_default_sv = Settings::boolean("igv_default_sv", true);
    IgvSessionManager::get(0).addLocationToCache(bedpe, igv_default_sv);

    //CNV files
    if (analysis_type_==SOMATIC_SINGLESAMPLE || analysis_type_==SOMATIC_PAIR)
    {
        FileLocation file = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCoverageFile();
        IgvSessionManager::get(0).addLocationToCache(file, true);

        FileLocation file2 = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCallFile();
        IgvSessionManager::get(0).addLocationToCache(file2, true);
    }
    else
    {
        FileLocationList segs = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(true);
        foreach(const FileLocation& file, segs)
        {
            IgvSessionManager::get(0).addLocationToCache(file, true);
        }
    }

    if (analysis_type_ == GERMLINE_SINGLESAMPLE || analysis_type_ == GERMLINE_MULTISAMPLE || analysis_type_ == GERMLINE_TRIO)
    {
        //Manta evidence file(s)
        FileLocationList evidence_files = GlobalServiceProvider::fileLocationProvider().getMantaEvidenceFiles(true);
        foreach(const FileLocation& file, evidence_files)
        {
            IgvSessionManager::get(0).addLocationToCache(file, false);
        }
    }

    //sample low-coverage
    bool igv_default_lowcov = Settings::boolean("igv_default_lowcov", true);
    if (analysis_type_==SOMATIC_SINGLESAMPLE || analysis_type_==SOMATIC_PAIR)
    {
        FileLocationList som_low_cov_files = GlobalServiceProvider::fileLocationProvider().getSomaticLowCoverageFiles(false);
        foreach(const FileLocation& loc, som_low_cov_files)
        {
            if(loc.filename.contains("somatic_custom_panel_stat"))
            {
                IgvSessionManager::get(0).addLocationToCache(FileLocation(loc.id + " (somatic custom panel)", PathType::LOWCOV_BED, loc.filename, loc.modified, loc.exists), igv_default_lowcov);
            }
            else
            {
                IgvSessionManager::get(0).addLocationToCache(loc, igv_default_lowcov);
            }
        }
    }
    else
    {
        FileLocationList low_cov_files = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(true);
        foreach(const FileLocation& file, low_cov_files)
        {
            IgvSessionManager::get(0).addLocationToCache(file, igv_default_lowcov);
        }
    }


    if (LoginManager::active())
    {
        NGSD db;

        QString sample_id = db.sampleId(current_filename_, false);
        if (sample_id!="")
        {
            //related RNA tracks
            foreach (int rna_sample_id, db.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
            {
                // iterate over all processed RNA samples
                foreach (const QString& rna_ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample_id)))
                {
                    //add RNA BAM
                    FileLocation rna_bam_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::BAM);
                    if (rna_bam_file.exists) IgvSessionManager::get(0).addLocationToCache(rna_bam_file, false);

                    //add fusions BAM
                    FileLocation rna_fusions_bam_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::FUSIONS_BAM);
                    if (rna_fusions_bam_file.exists) IgvSessionManager::get(0).addLocationToCache(rna_fusions_bam_file, false);

                    //add splicing BED
                    FileLocation rna_splicing_bed_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::SPLICING_BED);
                    if (rna_splicing_bed_file.exists) IgvSessionManager::get(0).addLocationToCache(rna_splicing_bed_file, false);
                }
            }


            //Paraphase evidence file
            if (db.getProcessedSampleData(db.processedSampleId(current_filename_)).processing_system_type == "lrGS")
            {
                FileLocationList paraphase_files = GlobalServiceProvider::fileLocationProvider().getParaphaseEvidenceFiles(true);
                foreach(const FileLocation& file, paraphase_files)
                {
                    IgvSessionManager::get(0).addLocationToCache(file, false);
                }
            }
        }
    }

    Log::info("Finished preloading IGV file information");
}
