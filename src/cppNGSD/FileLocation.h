#ifndef FILELOCATION_H
#define FILELOCATION_H

#include "cppNGSD_global.h"
#include "Exceptions.h"
#include "Helper.h"
#include <QString>
#include <QFileInfo>

enum class PathType
{
	//folders
	SAMPLE_FOLDER, // folder of a single sample
	FUSIONS_PIC_DIR, //picture of gene fusions determined by arriba from RNA (PNG format)

	//mapping data
	BAM, //BAM file
	VIRAL_BAM, //BAM file for a virus
    CRAM, //compressed version of BAM

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
	MSI, //microsatellite instability files (TSV format)
	BAF, //b-allele frequency file (IGV format)
	ROH, //ROH file (TSV format)
	PRS, //polygenic risk scores (TSV format)
	MANTA_EVIDENCE, //Reads that were used for structural variant calling (BAM format)
	COPY_NUMBER_RAW_DATA, //Copy number estimates based on coverage (SEG format)
	CNV_RAW_DATA_CALL_REGIONS,//Copy number raw data for call regions (SEG format)
	CIRCOS_PLOT, //CIRCOS plot (PNG format)
	REPEAT_EXPANSION_IMAGE, //image of repeat expansions locus (SVG format)
	REPEAT_EXPANSION_HISTOGRAM, //python plot of the lenth distribution of a locus (SVG format)
	FUSIONS, //gene fusions determined from RNA (TSV format)
	FUSIONS_BAM, //gene fusion evidence alignments determined from RNA (BAM format)
	MANTA_FUSIONS, //fusions determined by manta (BEDPE format)
	COUNTS, //gene/transcript counts from RNA (TSV format)
	EXPRESSION, //relative RNA expressions values from RNA (TSV format)
	EXPRESSION_COHORT, //relative RNA expressions values from RNA cohort of this sample (TSV format)
	EXPRESSION_STATS, //statistical summary of ENA expression valuesfrom RNA cohort of tis sample (TSV format),
	EXPRESSION_CORR, //statistical correlation to other samples of the cohort
	EXPRESSION_EXON, //relative exon RNA expressions values from RNA (TSV format)
	SPLICING_BED, //splicing junctions from RNA (BED format)
	SPLICING_ANN, //annotated splicing junctions from RNA (TSV format)
	VIRAL, //viral DNA detected in tumor samples (TSV format)
	VCF_CF_DNA, //cfDNA variants file (VCF format)
	MRD_CF_DNA, // measurable residual disease file of a cfDNA analysis (UmiVar2)
	CFDNA_CANDIDATES, // VCF containing preselected variants for cfDNA panel design
	QC, // variant list QC (qcML) files
	IGV_SCREENSHOT, //screenshot taken from IGV
	HLA_GENOTYPER, // results from hla genotyper (TSV format)
	SIGNATURE_SBS,  // Somatic: resut part from SigProfileExtractor for SNVs (CSV format)
	SIGNATURE_ID,	// Somatic: resut part from SigProfileExtractor for SNVs (CSV format)
	SIGNATURE_DBS,	// Somatic: resut part from SigProfileExtractor for SNVs (CSV format)
	SIGNATURE_CNV,	// Somatic: resut from SigProfileExtractor for CNVs (CSV format)
	METHYLATION, // Methylation calls (TSV format)
	METHYLATION_IMAGE, // image of a given methylation locus (PNG format)
	PARAPHASE_EVIDENCE, //Corrected mapping of pseudo gene regions (BAM format)
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

	//Returns if the file is a HTTP/HTTPS URL.
	bool isHttpUrl() const
	{
		return Helper::isHttpUrl(filename);
	}

	//Returns the base name of the file without the path.
	QString fileName(bool remove_http_get_args=true) const
	{
		QString output = QFileInfo(filename).fileName();

		//remove HTTP GET arguments
		if (remove_http_get_args && isHttpUrl() && output.contains('?'))
		{
			output = output.split('?')[0];
		}

		return output;
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
			case PathType::VIRAL_BAM:
                return "VIRAL_BAM";
            case PathType::CRAM:
                return "CRAM";
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
			case PathType::CNV_RAW_DATA_CALL_REGIONS:
				return "CNV_RAW_DATA_CALL_REGIONS";
			case PathType::MANTA_EVIDENCE:
				return "MANTA_EVIDENCE";
			case PathType::OTHER:
				return "OTHER";
			case PathType::REPEAT_EXPANSIONS:
				return "REPEAT_EXPANSIONS";
			case PathType::LOWCOV_BED:
				return "LOWCOV_BED";
			case PathType::MSI:
				return "MSI";
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
			case PathType::REPEAT_EXPANSION_HISTOGRAM:
				return "REPEAT_EXPANSION_HISTOGRAM";
			case PathType::FUSIONS:
				return "FUSIONS";
			case PathType::FUSIONS_PIC_DIR:
				return "FUSIONS_PIC_DIR";
			case PathType::FUSIONS_BAM:
				return "FUSIONS_BAM";
			case PathType::SPLICING_BED:
				return "SPLICING_BED";
			case PathType::SPLICING_ANN:
				return "SPLICING_ANN";
			case PathType::MANTA_FUSIONS:
				return "MANTA_FUSIONS";
			case PathType::COUNTS:
				return "COUNTS";
			case PathType::VIRAL:
				return "VIRAL";
			case PathType::VCF_CF_DNA:
				return "VCF_CF_DNA";
			case PathType::QC:
				return "QC";
			case PathType::EXPRESSION:
				return "EXPRESSION";
			case PathType::EXPRESSION_COHORT:
				return "EXPRESSION_COHORT";
			case PathType::EXPRESSION_CORR:
				return "EXPRESSION_CORR";
			case PathType::EXPRESSION_STATS:
				return "EXPRESSION_STATS";
			case PathType::EXPRESSION_EXON:
				return "EXPRESSION_EXON";
			case PathType::MRD_CF_DNA:
				return "MRD_CF_DNA";
			case PathType::CFDNA_CANDIDATES:
				return "CFDNA_CANDIDATES";
			case PathType::IGV_SCREENSHOT:
				return "IGV_SCREENSHOT";
			case PathType::HLA_GENOTYPER:
				return "HLA_GENOTYPER";
			case PathType::SIGNATURE_SBS:
				return "SIGNATURE_SBS";
			case PathType::SIGNATURE_ID:
				return "SIGNATURE_ID";
			case PathType::SIGNATURE_DBS:
				return "SIGNATURE_DBS";
			case PathType::SIGNATURE_CNV:
				return "SIGNATURE_CNV";
			case PathType::METHYLATION:
				return "METHYLATION";
			case PathType::METHYLATION_IMAGE:
				return "METHYLATION_IMAGE";
			case PathType::PARAPHASE_EVIDENCE:
				return "PARAPHASE_EVIDENCE";

		}
		THROW(ProgrammingException, "Unhandled path type '" + QString::number((int)pathtype) + "' in typeToString()!");
	}

	static PathType stringToType(const QString& in)
	{
		QString in_upper = in.toUpper().trimmed();

		if (in_upper == "SAMPLE_FOLDER") return PathType::SAMPLE_FOLDER;
		if (in_upper == "BAM") return PathType::BAM;
		if (in_upper == "VIRAL_BAM") return PathType::VIRAL_BAM;
        if (in_upper == "CRAM") return PathType::CRAM;
		if (in_upper == "GSVAR") return PathType::GSVAR;
		if (in_upper == "VCF") return PathType::VCF;
		if (in_upper == "BAF") return PathType::BAF;
		if (in_upper == "COPY_NUMBER_CALLS") return PathType::COPY_NUMBER_CALLS;
		if (in_upper == "COPY_NUMBER_CALLS_MOSAIC") return PathType::COPY_NUMBER_CALLS_MOSAIC;
		if (in_upper == "COPY_NUMBER_RAW_DATA") return PathType::COPY_NUMBER_RAW_DATA;
		if (in_upper == "CNV_RAW_DATA_CALL_REGIONS") return PathType::CNV_RAW_DATA_CALL_REGIONS;
		if (in_upper == "MANTA_EVIDENCE") return PathType::MANTA_EVIDENCE;
		if (in_upper == "REPEAT_EXPANSIONS") return PathType::REPEAT_EXPANSIONS;
		if (in_upper == "LOWCOV_BED") return PathType::LOWCOV_BED;
		if (in_upper == "MSI") return PathType::MSI;
		if (in_upper == "ROH") return PathType::ROH;
		if (in_upper == "PRS") return PathType::PRS;
		if (in_upper == "UPD") return PathType::UPD;
		if (in_upper == "CIRCOS_PLOT") return PathType::CIRCOS_PLOT;
		if (in_upper == "STRUCTURAL_VARIANTS") return PathType::STRUCTURAL_VARIANTS;
		if (in_upper == "REPEAT_EXPANSION_IMAGE") return PathType::REPEAT_EXPANSION_IMAGE;
		if (in_upper == "REPEAT_EXPANSION_HISTOGRAM") return PathType::REPEAT_EXPANSION_HISTOGRAM;
		if (in_upper == "FUSIONS") return PathType::FUSIONS;
		if (in_upper == "FUSIONS_BAM") return PathType::FUSIONS_BAM;
		if (in_upper == "SPLICING_BED") return PathType::SPLICING_BED;
		if (in_upper == "SPLICING_ANN") return PathType::SPLICING_ANN;
		if (in_upper == "MANTA_FUSIONS") return PathType::MANTA_FUSIONS;
		if (in_upper == "FUSIONS_PIC_DIR") return PathType::FUSIONS_PIC_DIR;
		if (in_upper == "COUNTS") return PathType::COUNTS;
		if (in_upper == "VIRAL") return PathType::VIRAL;
		if (in_upper == "VCF_CF_DNA") return PathType::VCF_CF_DNA;
		if (in_upper == "QC") return PathType::QC;
		if (in_upper == "OTHER") return PathType::OTHER;
		if (in_upper == "EXPRESSION") return PathType::EXPRESSION;
		if (in_upper == "EXPRESSION_COHORT") return PathType::EXPRESSION_COHORT;
		if (in_upper == "EXPRESSION_CORR") return PathType::EXPRESSION_CORR;
		if (in_upper == "EXPRESSION_STATS") return PathType::EXPRESSION_STATS;
		if (in_upper == "EXPRESSION_EXON") return PathType::EXPRESSION_EXON;
		if (in_upper == "MRD_CF_DNA") return PathType::MRD_CF_DNA;
		if (in_upper == "CFDNA_CANDIDATES") return PathType::CFDNA_CANDIDATES;
		if (in_upper == "IGV_SCREENSHOT") return PathType::IGV_SCREENSHOT;
		if (in_upper == "HLA_GENOTYPER") return PathType::HLA_GENOTYPER;
		if (in_upper == "SIGNATURE_SBS") return PathType::SIGNATURE_SBS;
		if (in_upper == "SIGNATURE_ID") return PathType::SIGNATURE_ID;
		if (in_upper == "SIGNATURE_DBS") return PathType::SIGNATURE_DBS;
		if (in_upper == "SIGNATURE_CNV") return PathType::SIGNATURE_CNV;
		if (in_upper == "METHYLATION") return PathType::METHYLATION;
		if (in_upper == "METHYLATION_IMAGE") return PathType::METHYLATION_IMAGE;
		if (in_upper == "PARAPHASE_EVIDENCE") return PathType::PARAPHASE_EVIDENCE;
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
            case PathType::CRAM:
                return "compressed version of a BAM file";
			case PathType::VIRAL_BAM:
				return "viral BAM file";
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
			case PathType::CNV_RAW_DATA_CALL_REGIONS:
				return "copy-number raw data for call regions";
			case PathType::MANTA_EVIDENCE:
				return "evidence BAM file for structural variants";
			case PathType::REPEAT_EXPANSIONS:
				return "repeat expansions";
			case PathType::LOWCOV_BED:
				return "low coverage regions";
			case PathType::MSI:
				return "MSI files";
			case PathType::ROH:
				return "runs of homozygosity";
			case PathType::PRS:
				return "polygenic risk scores";
			case PathType::CIRCOS_PLOT:
				return "circos plot";
			case PathType::STRUCTURAL_VARIANTS:
				return "structural variant calls";
			case PathType::UPD:
				return "uniparental disomy regions";
			case PathType::REPEAT_EXPANSION_IMAGE:
				return "repeat expansion visualization";
			case PathType::REPEAT_EXPANSION_HISTOGRAM:
				return "repeat expansion length distribution visualization";
			case PathType::FUSIONS:
				return "gene fusions";
			case PathType::FUSIONS_PIC_DIR:
				return "arriba fusions pictures directory";
			case PathType::FUSIONS_BAM:
				return "gene fusions evidence alignments";
			case PathType::SPLICING_BED:
				return "splicing junctions";
			case PathType::SPLICING_ANN:
				return "annotated splicing junctions";
			case PathType::MANTA_FUSIONS:
				return "gene fusions called by Manta";
			case PathType::COUNTS:
				return "RNA counts";
			case PathType::VIRAL:
				return "viral DNA";
			case PathType::OTHER:
				return "other files";
			case PathType::VCF_CF_DNA:
				return "cfDNA small variant calls";
			case PathType::QC:
				return "variant list QC (qcML) files";
			case PathType::EXPRESSION:
				return "RNA relative expression";
			case PathType::EXPRESSION_COHORT:
				return "RNA relative expression of cohort";
			case PathType::EXPRESSION_STATS:
				return "RNA expression cohort statistics";
			case PathType::EXPRESSION_CORR:
				return "RNA epxression correlation to cohort";
			case PathType::EXPRESSION_EXON:
				return "RNA relative exon expression";
			case PathType::MRD_CF_DNA:
				return "measurable residual disease value (umiVar 2)";
			case PathType::CFDNA_CANDIDATES:
				return "pre-selected variants for cfDNA panel design.";
			case PathType::IGV_SCREENSHOT:
				return "IGV screenshot";
			case PathType::HLA_GENOTYPER:
				return "HLA called by hla genotyper";
			case PathType::SIGNATURE_SBS:
				return "SBS signature";
			case PathType::SIGNATURE_ID:
				return "ID signature";
			case PathType::SIGNATURE_DBS:
				return "DBS signature";
			case PathType::SIGNATURE_CNV:
				return "CNV signature";
			case PathType::METHYLATION:
				return "methylation calls";
			case PathType::METHYLATION_IMAGE:
				return "image of a given methylation locus";
			case PathType::PARAPHASE_EVIDENCE:
				return "Mapping of pseudo gene regions (Paraphase)";
		}
		THROW(ProgrammingException, "Unhandled path type '" + QString::number((int)pathtype) + "' in typeToHumanReadableString()!");
	}

	bool operator == (const FileLocation& x) const
	{
	  return (id == x.id && type == x.type && filename == x.filename);
	}
};

#endif // FILELOCATION_H
