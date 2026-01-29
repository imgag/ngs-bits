#ifndef TUMORONLYREPORTWORKER_H
#define TUMORONLYREPORTWORKER_H

#include "cppNGSD_global.h"
#include "VariantList.h"
#include "RtfDocument.h"
#include "FilterCascade.h"
#include "NGSHelper.h"
#include "RtfDocument.h"
#include "NGSD.h"

///Input configuration for TumorOnlyReportWorker
struct CPPNGSDSHARED_EXPORT TumorOnlyReportWorkerConfig
{
	TargetRegionInfo roi;

	//processing system
	ProcessingSystemData sys;

	ProcessedSampleData ps_data;

	QString low_coverage_file = "";
	QString bam_file = "";

	FilterResult filter_result;

	bool include_coverage_per_gap = false;
	bool include_exon_number_per_gap = false;

	QMap<QByteArray, QByteArrayList> preferred_transcripts;

	bool use_test_db = false;

	GenomeBuild build;
	int threads = 1;
};

///Helper class for tumor-only report generation
class CPPNGSDSHARED_EXPORT TumorOnlyReportWorker
{

public:
	///constructor
	TumorOnlyReportWorker(const VariantList& variants, const TumorOnlyReportWorkerConfig& config);
	///writes RTF file with report to file_path
	void writeRtf(QByteArray file_path);
	///checks whether all neccessary annotations are available in variants and throws FileParseException if not available
	static void checkAnnotation(const VariantList& variants);

	void writeXML(QString filename, bool test=false);

private:
	const TumorOnlyReportWorkerConfig& config_;
	const VariantList& variants_;
	NGSD db_;

	RtfDocument doc_;

	//variant annotation indices
	int i_co_sp_;
	int i_tum_af_;
	int i_tum_dp_;
	int i_gene_;
	int i_ncg_oncogene_;
	int i_ncg_tsg_;
	int i_germl_class_;
    int i_vicc_class_;

	///Returns variant description with information from NCG and in-house classification
	QByteArray variantDescription(const Variant& var);

	///translates somatic variant classification to German language
	QByteArray trans(QByteArray english);

	///Get exon number according preferred transcript from NGSD and returns parsed string
	QByteArray exonNumber(QByteArray gene, int start, int end);

};

#endif // TUMORONLYREPORTWORKER_H
