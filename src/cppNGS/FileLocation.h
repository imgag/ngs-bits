#ifndef FILELOCATION_H
#define FILELOCATION_H

#include "cppNGS_global.h"
#include <QString>

enum class PathType
{
        PROJECT_FOLDER, // project root folder
        SAMPLE_FOLDER, // folder with samples
        BAM, // binary alignment map with sequence alignment data
        GSVAR, // GSVar tool sample data

        // VCF
        VCF, // variant call format file storing gene sequence variations
        REPEATS_EXPANSION_HUNTER_VCF, // *_repeats_expansionhunter.vcf

        BAF, // b-allele frequency file

        // BED
        COPY_NUMBER_CALLS, // BED files
        LOWCOV_BED, // *_lowcov.bed
        STAT_LOWCOV_BED, // *_stat_lowcov.bed
        ANY_BED, // *.bed

        // SEG
        CNVS_CLINCNV_SEG, // *_cnvs_clincnv.seg
        CNVS_SEG, // *_cnvs.seg
        COPY_NUMBER_RAW_DATA, // SEG file with copy
        MANTA_EVIDENCE, // also BAM files

        ANALYSIS_LOG, // analysis log files *.log
        CIRCOS_PLOT, // *_circos.png

        // TSV
        CNVS_CLINCNV_TSV, // *_cnvs_clincnv.tsv
        CLINCNV_TSV, // *_clincnv.tsv
        CNVS_TSV, // *_cnvs.tsv
        PRS_TSV, // *_prs.tsv
        ROHS_TSV, // *_rohs.tsv
        VAR_FUSIONS_TSV, // *_var_fusions.tsv

        // GZ
        VCF_GZ, // *_var_annotated.vcf.gz
        FASTQ_GZ, // *.fastq.gz

        OTHER // everything else
};

struct FileLocation
{
        QString id; //sample identifier/name
        PathType type; //file type
        QString filename; //file name
        bool is_found; // indicates if a file exists or not

        bool operator == (const FileLocation& x) const
        {
          return (id == x.id && type == x.type && filename == x.filename);
        }
};

#endif // FILELOCATION_H
