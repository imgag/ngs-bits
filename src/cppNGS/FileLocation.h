#ifndef FILELOCATION_H
#define FILELOCATION_H

#include "cppNGS_global.h"
#include "Exceptions.h"
#include <QString>

enum class PathType
{
	//folders
	SAMPLE_FOLDER, // folder of a single sample

	//mapping data
	BAM, //BAM file

	//variant data
	VCF, //small variants (VCF.GZ format)
	GSVAR, //small variants (GSvar format)
	COPY_NUMBER_CALLS, //copy number calls (TSV format)
	COPY_NUMBER_CALLS_MOSAIC, //mosaic copy number calls (TSV format)
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
	REPEAT_EXPANSION_IMAGE, //image of repeat expansions locus (SVG format)
	FUSIONS, //gene fusions determined from RNA (TSV format)
	COUNTS, //gene/transcript counts from RNA (TSV format)
	VIRAL, //viral DNA detected in tumor samples (TSV format)
	VCF_CF_DNA, //cfDNA variants file (VCF format)
	OTHER // everything else
};

struct FileLocation
{
	QString id; //sample name (for single sample analyses) or analysis name (for multi-sample analyses)
	PathType type = PathType::OTHER; //file type
	QString filename; //file name
	bool exists = false; // if filename actually exists or not

	FileLocation()
		: id()
		, type(PathType::OTHER)
		, filename()
		, exists(false)
	{
	}

	FileLocation(const QString& id_, PathType type_, const QString& filename_, bool exists_)
		: id(id_)
		, type(type_)
		, filename(filename_)
		, exists(exists_)
	{
	}

	QString typeAsString() const
	{
		return typeToString(type);
	}

	QString typeAsHumanReadableString() const
	{
		return typeToHumanReadableString(type);
	}

	static QString typeToString(PathType pathtype)
	{
		switch(pathtype)
		{
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
			case PathType::COPY_NUMBER_CALLS_MOSAIC:
				return "COPY_NUMBER_CALLS_MOSAIC";
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
			case PathType::UPD:
				return "UPD";
			case PathType::CIRCOS_PLOT:
				return "CIRCOS_PLOT";
			case PathType::STRUCTURAL_VARIANTS:
				return "STRUCTURAL_VARIANTS";
			case PathType::REPEAT_EXPANSION_IMAGE:
				return "REPEAT_EXPANSION_IMAGE";
			case PathType::FUSIONS:
				return "FUSIONS";
			case PathType::COUNTS:
				return "COUNTS";
			case PathType::VIRAL:
				return "VIRAL";
			case PathType::VCF_CF_DNA:
				return "VCF_CF_DNA";

		}
		THROW(ProgrammingException, "Unhandled path type '" + QString::number((int)pathtype) + "' in typeToString()!");
	}

	static PathType stringToType(const QString& in)
	{
		QString in_upper = in.toUpper().trimmed();

		if (in_upper == "SAMPLE_FOLDER") return PathType::SAMPLE_FOLDER;
		if (in_upper == "BAM") return PathType::BAM;
		if (in_upper == "GSVAR") return PathType::GSVAR;
		if (in_upper == "VCF") return PathType::VCF;
		if (in_upper == "BAF") return PathType::BAF;
		if (in_upper == "COPY_NUMBER_CALLS") return PathType::COPY_NUMBER_CALLS;
		if (in_upper == "COPY_NUMBER_CALLS_MOSAIC") return PathType::COPY_NUMBER_CALLS_MOSAIC;
		if (in_upper == "COPY_NUMBER_RAW_DATA") return PathType::COPY_NUMBER_RAW_DATA;
		if (in_upper == "MANTA_EVIDENCE") return PathType::MANTA_EVIDENCE;
		if (in_upper == "REPEAT_EXPANSIONS") return PathType::REPEAT_EXPANSIONS;
		if (in_upper == "LOWCOV_BED") return PathType::LOWCOV_BED;
		if (in_upper == "ROH") return PathType::ROH;
		if (in_upper == "PRS") return PathType::PRS;
		if (in_upper == "UPD") return PathType::UPD;
		if (in_upper == "CIRCOS_PLOT") return PathType::CIRCOS_PLOT;
		if (in_upper == "STRUCTURAL_VARIANTS") return PathType::STRUCTURAL_VARIANTS;
		if (in_upper == "REPEAT_EXPANSION_IMAGE") return PathType::REPEAT_EXPANSION_IMAGE;
		if (in_upper == "FUSIONS") return PathType::FUSIONS;
		if (in_upper == "COUNTS") return PathType::COUNTS;
		if (in_upper == "VIRAL") return PathType::VIRAL;
		if (in_upper == "VCF_CF_DNA") return PathType::VCF_CF_DNA;

		THROW(ProgrammingException, "Unhandled path type string '" + in_upper + "' in stringToType()!");
	}

	static QString typeToHumanReadableString(PathType pathtype)
	{
		switch(pathtype)
		{
			case PathType::SAMPLE_FOLDER:
				return "sample/analysis folder";
			case PathType::BAM:
				return "BAM file";
			case PathType::VCF:
				return "small variant calls";
			case PathType::GSVAR:
				return "GSvar file";
			case PathType::BAF:
				return "b-allele frequency file";
			case PathType::COPY_NUMBER_CALLS:
				return "copy-number calls";
			case PathType::COPY_NUMBER_CALLS_MOSAIC:
				return "copy-number calls (mosaic)";
			case PathType::COPY_NUMBER_RAW_DATA:
				return "copy-number raw data";
			case PathType::MANTA_EVIDENCE:
				return "evidence file for Manta structural variants";
			case PathType::REPEAT_EXPANSIONS:
				return "repeat expansions";
			case PathType::LOWCOV_BED:
				return "low coverage regions";
			case PathType::ROH:
				return "runs of homozygosity";
			case PathType::PRS:
				return "polygenic risk scores";
			case PathType::CIRCOS_PLOT:
				return "circos plot";
			case PathType::STRUCTURAL_VARIANTS:
				return "strctural variant calls";
			case PathType::UPD:
				return "uniparental disomy regions";
			case PathType::REPEAT_EXPANSION_IMAGE:
				return "repeat expansion visualization";
			case PathType::FUSIONS:
				return "gene fusions";
			case PathType::COUNTS:
				return "RNA counts";
			case PathType::VIRAL:
				return "viral DNA";
			case PathType::OTHER:
				return "other files";
			case PathType::VCF_CF_DNA:
				return "cfDNA small variant calls";
		}
		THROW(ProgrammingException, "Unhandled path type '" + QString::number((int)pathtype) + "' in typeToHumanReadableString()!");
	}

	bool operator == (const FileLocation& x) const
	{
	  return (id == x.id && type == x.type && filename == x.filename);
	}
};

#endif // FILELOCATION_H
