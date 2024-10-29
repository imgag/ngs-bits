#include "IGVInitCachePhaseOneWorker.h"
#include "GlobalServiceProvider.h"

IGVInitCachePhaseOneWorker::IGVInitCachePhaseOneWorker(const AnalysisType analysis_type)
    : analysis_type_(analysis_type)
{
}

void IGVInitCachePhaseOneWorker::run()
{
    Log::info("Preloading IGV file information from the file location provider");
    //sample BAM file(s)
    FileLocationList bams = GlobalServiceProvider::fileLocationProvider().getBamFiles(true);
    foreach(const FileLocation& file, bams)
    {
        IGVInitCache::add(file, true);
    }

    //sample BAF file(s)
    FileLocationList bafs = GlobalServiceProvider::fileLocationProvider().getBafFiles(true);
    foreach(const FileLocation& file, bafs)
    {
        if(analysis_type_ == SOMATIC_PAIR && !file.id.contains("somatic")) continue;
        IGVInitCache::add(file, true);
    }

    //analysis VCF
    FileLocation vcf = GlobalServiceProvider::fileLocationProvider().getAnalysisVcf();
    bool igv_default_small = Settings::boolean("igv_default_small", true);
    IGVInitCache::add(vcf, igv_default_small);

    //analysis SV file
    FileLocation bedpe = GlobalServiceProvider::fileLocationProvider().getAnalysisSvFile();
    bool igv_default_sv = Settings::boolean("igv_default_sv", true);
    IGVInitCache::add(bedpe, igv_default_sv);

    //CNV files
    if (analysis_type_==SOMATIC_SINGLESAMPLE || analysis_type_==SOMATIC_PAIR)
    {
        FileLocation file = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCoverageFile();
        IGVInitCache::add(file, true);

        FileLocation file2 = GlobalServiceProvider::fileLocationProvider().getSomaticCnvCallFile();
        IGVInitCache::add(file2, true);
    }
    else
    {
        FileLocationList segs = GlobalServiceProvider::fileLocationProvider().getCnvCoverageFiles(true);
        foreach(const FileLocation& file, segs)
        {
            IGVInitCache::add(file, true);
        }
    }

    //Manta evidence file(s)
    FileLocationList evidence_files = GlobalServiceProvider::fileLocationProvider().getMantaEvidenceFiles(true);
    foreach(const FileLocation& file, evidence_files)
    {
        IGVInitCache::add(file, false);
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
                IGVInitCache::add(FileLocation{loc.id + " (somatic custom panel)", PathType::LOWCOV_BED, loc.filename, QFile::exists(loc.filename)}, igv_default_lowcov);
            }
            else
            {
                IGVInitCache::add(loc, igv_default_lowcov);
            }
        }
    }
    else
    {
        FileLocationList low_cov_files = GlobalServiceProvider::fileLocationProvider().getLowCoverageFiles(true);
        foreach(const FileLocation& file, low_cov_files)
        {
            IGVInitCache::add(file, igv_default_lowcov);
        }
    }

    Log::info("Finished preloading IGV file information from the file location provider");
}
