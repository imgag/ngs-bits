#ifndef FILELOCATIONPROVIDER_H
#define FILELOCATIONPROVIDER_H
#include "Exceptions.h"

#include "cppNGS_global.h"
#include "VariantList.h"
#include "Helper.h"
#include "FileLocation.h"
#include "FileLocationList.h"

//Analysis file location provider interface for local/client-server mode.
class CPPNGSSHARED_EXPORT FileLocationProvider
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

	//############################## sample-specific files ##############################
	//Returns sample-specific BAM files
	virtual FileLocationList getBamFiles(bool return_if_missing) const = 0;
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

	//############################## somatic-only files ##############################
	//Returns the tumor-normal CNV coverage SEG file (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticCnvCoverageFile() const = 0;
	//Returns the tumor-normal CNV calls SEG file (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticCnvCallFile() const = 0;
	//Returns the somatic low coverage BED file (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticLowCoverageFile() const = 0;
	//Returns the somatic MSI file in TSV format (throws an exception if not SOMATIC_PAIR or SOMATIC_SINGLE)
	virtual FileLocation getSomaticMsiFile() const = 0;

protected:
	//Returns analysis path, i.e. the path of the GSvar file
	virtual QString getAnalysisPath() const = 0;
	//Returns the project path , i.e. the parent directory of the analysis path
	virtual QString getProjectPath() const = 0;
};

#endif // FILELOCATIONPROVIDER_H
