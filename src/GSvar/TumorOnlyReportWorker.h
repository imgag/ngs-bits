#ifndef TUMORONLYREPORTWORKER_H
#define TUMORONLYREPORTWORKER_H

#include <QObject>
#include "VariantList.h"
#include "RtfDocument.h"
#include "FilterCascade.h"
#include "Transcript.h"

///Input configuration for TumorOnlyReportWorker
struct TumorOnlyReportWorkerConfig
{
	QString ps; //Tumor processed sample name

	QString target_file = "";
	QString low_coverage_file = "";
	QString bam_file = "";

	FilterResult filter_result;

	bool include_coverage_per_gap = false;

	bool include_exon_number_per_gap = false;
};

///Helper class for tumor-only report generation
class TumorOnlyReportWorker
{

public:
	///constructor
	TumorOnlyReportWorker(const VariantList& variants, const TumorOnlyReportWorkerConfig& config);
	///returns CGI cancertype if available from VariantList
	static QByteArray cgiCancerTypeFromVariantList(const VariantList& variants);
	///writes RTF file with report to file_path
	void writeRtf(QByteArray file_path);
	///checks whether all neccessary annotations are available in variants and throws FileParseException if not available
	static void checkAnnotation(const VariantList& variants);

private:
	QString ps_;
	const VariantList& variants_;
	const FilterResult& filter_result_;

	const QMap<QByteArray, QByteArrayList>& preferred_transcripts_;

	RtfDocument doc_;

	//input files
	QString target_file_;
	QString low_cov_file_;
	QString bam_file_;

	bool include_coverage_per_gap_;
	bool include_exons_per_gap_;

	//variant annotation indices
	int i_co_sp_;
	int i_tum_af_;
	int i_cgi_driver_statem_;
	int i_ncg_oncogene_;
	int i_ncg_tsg;
	int i_germl_class_;
	int i_somatic_class_;

	///Returns variant description with information from NCG, CGI and in-house classification
	QByteArray variantDescription(const Variant& var);

	///translates somatic variant classification to German language
	QByteArray trans(QByteArray english);

	///Get exon number according preferred transcript from NGSD and returns parsed string
	QByteArray exonNumber(QByteArray gene, int start, int end);

};

#endif // TUMORONLYREPORTWORKER_H
