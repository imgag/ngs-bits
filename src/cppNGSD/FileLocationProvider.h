#ifndef FILELOCATIONPROVIDER_H
#define FILELOCATIONPROVIDER_H
#include "Exceptions.h"

#include "cppNGSD_global.h"
#include "VariantList.h"
#include "Helper.h"
#include "FileLocation.h"
#include "FileLocationList.h"

//Analysis file location provider interface for local/client-server mode.
class CPPNGSDSHARED_EXPORT FileLocationProvider
{
public:

	//Returns if the analysis is locally available (not in client-server mode).
	virtual bool isLocal() const = 0;

	//############################## analysis-specific files ##############################
	//Returns the annotated VCF of the current analysis
	virtual FileLocation getAnalysisVcf() const = 0;
	//Returns the structural variant BEDPE file of the current analysis
	virtual FileLocation getAnalysisSvFile() const = 0;
	//Returns the copy-number call TSV file of the current analysis
	virtual FileLocation getAnalysisCnvFile() const = 0;
	//Returns the mosaic copy-number call TSV file of the current analysis
	virtual FileLocation getAnalysisMosaicCnvFile() const = 0;
	//Returns the UPD calls TSV file of the current analysis (works for GERMLINE_TRIO only)
	virtual FileLocation getAnalysisUpdFile() const = 0;
	//Returns the repeat expansion locus image of the current analysis
	virtual FileLocation getRepeatExpansionImage(QString locus) const = 0;
	//Returns the repeat expansion locus image of the current analysis
	virtual FileLocation getRepeatExpansionHistogram(QString locus) const = 0;
	//Returns sample-specific qcML files.
	virtual FileLocationList getQcFiles() const = 0;
	//Returns the methylation TSV file of the current analysis
	virtual FileLocation getMethylationFile() const = 0;
	//Returns the methylation image of a given locus
	virtual FileLocation getMethylationImage(QString locus) const = 0;

	//############################## sample-specific files ##############################
	//Returns sample-specific BAM files
	virtual FileLocationList getBamFiles(bool return_if_missing) const = 0;
	//Returns virus-related BAM files
	virtual FileLocationList getViralBamFiles(bool return_if_missing) const = 0;
	//Returns sample-specific low coverage files in BED format
	virtual FileLocationList getLowCoverageFiles(bool return_if_missing) const = 0;
	//Returns sample-specifi b-allele frequency files (IGV format)
	virtual FileLocationList getBafFiles(bool return_if_missing) const = 0;
	//Returns sample-specifi ROH files (TSV format)
	virtual FileLocationList getRohFiles(bool return_if_missing) const = 0;
	//Returns sample-specific annotated VCF files
	virtual FileLocationList getVcfFiles(bool return_if_missing) const = 0;
	//Returns sample-specific CNV coverage data in SEG format.
	virtual FileLocationList getCnvCoverageFiles(bool return_if_missing) const = 0;
	//Returns sample-specific copy-number call files in TSV format
	virtual FileLocationList getCopyNumberCallFiles(bool return_if_missing) const = 0;
	//Returns sample-specific repeat expansion files (TSV format)
	virtual FileLocationList getRepeatExpansionFiles(bool return_if_missing) const = 0;
	//Returns sample-specific reads supporting structural variant calls (BAM format)
	virtual FileLocationList getMantaEvidenceFiles(bool return_if_missing) const = 0;
	//Returns sample-specific polygenic risk score files (TSV format)
	virtual FileLocationList getPrsFiles(bool return_if_missing) const = 0;
	//Returns sample-specific CIRCOS plot files.
	virtual FileLocationList getCircosPlotFiles(bool return_if_missing) const = 0;
	//Returns sample specific RNA expression files.
	virtual FileLocationList getExpressionFiles(bool return_if_missing) const = 0;
	//Returns sample specific exon RNA expresion files.
	virtual FileLocationList getExonExpressionFiles(bool return_if_missing) const = 0;
	//Returns the somatic low coverage files in BED format
	virtual FileLocationList getSomaticLowCoverageFiles(bool return_if_missing) const = 0;
	//Returns sample-specific reads mapped to the correct pseudo gene (BAM format)
	virtual FileLocationList getParaphaseEvidenceFiles(bool return_if_missing) const = 0;


	//############################## somatic-only files ##############################
	//Returns the tumor-normal CNV coverage SEG file (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticCnvCoverageFile() const = 0;
	//Returns the tumor-normal CNV calls SEG file (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticCnvCallFile() const = 0;
	//Returns the somatic low coverage BED file (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticLowCoverageFile() const = 0;
	//Returns the somatic MSI file in TSV format (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticMsiFile() const = 0;
	//Returns the somatic IGV screenshot in PNG format (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticIgvScreenshotFile() const = 0;
	//Returns the pre-selection of monitoring variants for cfDNA panel design in VCF (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticCfdnaCandidateFile() const = 0;
	//Returns the somatic SBS signature file.
	virtual FileLocation getSignatureSbsFile() const = 0;
	//Returns the somatic ID signature file.
	virtual FileLocation getSignatureIdFile() const = 0;
	//Returns the somatic DBS signature file.
	virtual FileLocation getSignatureDbsFile() const = 0;
	//Returns the somatic CNV signature file.
	virtual FileLocation getSignatureCnvFile() const = 0;
};

#endif // FILELOCATIONPROVIDER_H
