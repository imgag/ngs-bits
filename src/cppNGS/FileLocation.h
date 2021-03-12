#ifndef FILELOCATION_H
#define FILELOCATION_H

#include "cppNGS_global.h"
#include <QString>

enum class PathType
{
	//folders
	PROJECT_FOLDER, // project folder (normally the parent folder of analysis folder)
	SAMPLE_FOLDER, // folder of a single sample

	//mapping data
	BAM, //BAM file

	//variant data
	VCF, //small variants (VCF format)
	GSVAR, //small variants (GSvar format)
	COPY_NUMBER_CALLS, //copy number calls (TSV format)
	STRUCTURAL_VARIANTS, //structural variant call file (BEDPE format)
	REPEAT_EXPANSIONS, //repeat expansions (VCF format)
	UPD, //UPD calls (TSV format)

	//other files
	LOWCOV_BED, //Low coverage region files (BED format)
	BAF, //b-allele frequency file (IGV format)
	ROH, //ROH file (TSV format)
	PRS, //polygenic risk scores (TSV format)
	MANTA_EVIDENCE, //Reads that were used for structural variant calling (BAM format)
	COPY_NUMBER_RAW_DATA, //Copy number estimates based on coverage (SEG format)
	CIRCOS_PLOT, //CIRCOS plot (PNG format)
	OTHER // everything else
};

struct FileLocation
{
	QString id; //sample name (for single sample analyses) or analysis name (for multi-sample analyses)
	PathType type; //file type
	QString filename; //file name
	bool exists; // if filename actually exists or not

	QString typeToString() const
	{
		return typeToString(type);
	}

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
			case PathType::REPEAT_EXPANSIONS:
				return "REPEAT_EXPANSIONS";
			case PathType::LOWCOV_BED:
				return "LOWCOV_BED";
			case PathType::ROH:
				return "ROH";
			case PathType::PRS:
				return "PRS";
			case PathType::CIRCOS_PLOT:
				return "CIRCOS_PLOT";

			default:
			 return "Invalid PathType";
		}
	}

	static PathType stringToType(const QString& in)
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
		if (in.toUpper() == "REPEAT_EXPANSIONS") return PathType::REPEAT_EXPANSIONS;
		if (in.toUpper() == "LOWCOV_BED") return PathType::LOWCOV_BED;
		if (in.toUpper() == "ROH") return PathType::ROH;
		if (in.toUpper() == "PRS") return PathType::PRS;
		if (in.toUpper() == "CIRCOS_PLOT") return PathType::CIRCOS_PLOT;

		return PathType::OTHER;
	}

	bool operator == (const FileLocation& x) const
	{
	  return (id == x.id && type == x.type && filename == x.filename);
	}
};

#endif // FILELOCATION_H
