#include "IGVInitCachePhaseTwoWorker.h"
#include "GlobalServiceProvider.h"

IGVInitCachePhaseTwoWorker::IGVInitCachePhaseTwoWorker(const QString current_filename)
    : current_filename_(current_filename)
{
}

void IGVInitCachePhaseTwoWorker::run()
{
    Log::info("Preloading IGV file information from the database service");
    //related RNA tracks
    if (LoginManager::active())
    {
        NGSD db;

        QString sample_id = db.sampleId(current_filename_, false);
        if (sample_id!="")
        {
            foreach (int rna_sample_id, db.relatedSamples(sample_id.toInt(), "same sample", "RNA"))
            {
                // iterate over all processed RNA samples
                foreach (const QString& rna_ps_id, db.getValues("SELECT id FROM processed_sample WHERE sample_id=:0", QString::number(rna_sample_id)))
                {
                    //add RNA BAM
                    FileLocation rna_bam_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::BAM);
                    if (rna_bam_file.exists) IGVInitCache::add(rna_bam_file, false);

                    //add fusions BAM
                    FileLocation rna_fusions_bam_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::FUSIONS_BAM);
                    if (rna_fusions_bam_file.exists) IGVInitCache::add(rna_fusions_bam_file, false);

                    //add splicing BED
                    FileLocation rna_splicing_bed_file = GlobalServiceProvider::database().processedSamplePath(rna_ps_id, PathType::SPLICING_BED);
                    if (rna_splicing_bed_file.exists) IGVInitCache::add(rna_splicing_bed_file, false);
                }
            }
        }
    }
    Log::info("Finished preloading IGV file information from the database service");
}
