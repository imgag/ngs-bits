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

	static QString typeToString(PathType pathtype)
	{
		switch(pathtype)
		{
			case PathType::PROJECT_FOLDER:
				return "PROJECT_FOLDER";
			case PathType::SAMPLE_FOLDER:
				return "SAMPLE_FOLDER";
			case PathType::BAM:
				return "BAM";
			case PathType::GSVAR:
				return "GSVAR";
			case PathType::VCF:
				return "VCF";
			case PathType::BAF:
				return "BAF";
			case PathType::COPY_NUMBER_CALLS:
				return "COPY_NUMBER_CALLS";
			case PathType::COPY_NUMBER_RAW_DATA:
				return "COPY_NUMBER_RAW_DATA";
			case PathType::MANTA_EVIDENCE:
				return "MANTA_EVIDENCE";
			case PathType::OTHER:
				return "OTHER";
			default:
			 return "Invalid PathType";
		}
	}

	QString typeToString() const
	{
		return typeToString(type);
	}

	static PathType stringToType(QString in)
	{
		if (in.toUpper() == "PROJECT_FOLDER") return PathType::PROJECT_FOLDER;
		if (in.toUpper() == "SAMPLE_FOLDER") return PathType::SAMPLE_FOLDER;
		if (in.toUpper() == "BAM") return PathType::BAM;
		if (in.toUpper() == "GSVAR") return PathType::GSVAR;
		if (in.toUpper() == "VCF") return PathType::VCF;
		if (in.toUpper() == "BAF") return PathType::BAF;
		if (in.toUpper() == "COPY_NUMBER_CALLS") return PathType::COPY_NUMBER_CALLS;
		if (in.toUpper() == "COPY_NUMBER_RAW_DATA") return PathType::COPY_NUMBER_RAW_DATA;
		if (in.toUpper() == "MANTA_EVIDENCE") return PathType::MANTA_EVIDENCE;

		return PathType::OTHER;
	}

	bool operator == (const FileLocation& x) const
	{
	  return (id == x.id && type == x.type && filename == x.filename);
	}
};

#endif // FILELOCATION_H
