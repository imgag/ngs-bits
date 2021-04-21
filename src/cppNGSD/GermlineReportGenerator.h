#ifndef GERMLINEREPORTGENERATOR_H
#define GERMLINEREPORTGENERATOR_H

#include "cppNGSD_global.h"
#include "NGSD.h"
#include "ReportSettings.h"
#include "FilterCascade.h"
#include "PrsTable.h"

//Data for germline report gerneration
struct CPPNGSDSHARED_EXPORT GermlineReportGeneratorData
{
	//constructor
	GermlineReportGeneratorData(QString ps_, const VariantList& variants_, const CnvList& cnvs_, const BedpeFile& svs_, const PrsTable& prs_, const ReportSettings& report_settings_, const FilterCascade& filters_, const QMap<QByteArray, QByteArrayList>& preferred_transcripts_);

	//sample data
	QString ps;
	const VariantList& variants;
	const CnvList& cnvs;
	const BedpeFile& svs;
	PrsTable prs;

	//processing system target region
	BedFile processing_system_roi;

	//target region data (set if a target region was used)
	TargetRegionInfo roi;

	//other data
	const ReportSettings& report_settings;
	const FilterCascade& filters;
	const QMap<QByteArray, QByteArrayList>& preferred_transcripts;
};

//Report generator class
class CPPNGSDSHARED_EXPORT GermlineReportGenerator
{

public:
	///Constructor. If test mode is enabled, the test database is used and stricter error checks are performed.
	GermlineReportGenerator(const GermlineReportGeneratorData& data, bool test_mode=false);
	///Writes the HTML report.
	void writeHTML(QString filename);
	///Writes the XML report, including the HTML report. Call after generating the HTML report - some statistics data is cached between reports.
	void writeXML(QString filename, QString html_document);

	///Overrides BAM file (for testing only)
	void overrideBamFile(QString bam_file);
	///Overrides low-coverage file (for testing only)
	void overrideLowCovFile(QString lowcov_file);
	///Overrides date (for testing only)
	void overrideDate(QDate date);

	///Returns if the pre-calcualed gaps for the given ROI.
	///If using the pre-calculated gaps file is not possible, @p message contains an error message.
	static BedFile precalculatedGaps(QString low_cov_file, const BedFile& roi, int min_cov, const BedFile& processing_system_target_region);

	///Writes a evaluation sheet in HTML format.
	void writeEvaluationSheet(QString filename, const EvaluationSheetData& evaluation_sheet_data);

private:
	NGSD db_;
	const GermlineReportGeneratorData& data_;
	QDate date_;
	bool test_mode_;

	QString ps_id_;
	QString ps_bam_;
	QString ps_lowcov_;
	QMap<QString, QString> cache_;

	static void writeHtmlHeader(QTextStream& stream, QString sample_name);
	static void writeHtmlFooter(QTextStream& stream);
	QString trans(const QString& text);
	void writeCoverageReport(QTextStream& stream);
	void writeClosedGapsReport(QTextStream& stream, const BedFile& roi);
	void writeCoverageReportCCDS(QTextStream& stream, int extend, bool gap_table=true, bool gene_details=true);
	static QByteArray formatGenotype(const QByteArray& gender, const QByteArray& genotype, const Variant& variant);
	QString formatCodingSplicing(const QList<VariantTranscript>& transcripts);

	///Helper functions for writeEvaluationSheet()
	static void printVariantSheetRowHeader(QTextStream& stream, bool causal);
	void printVariantSheetRow(QTextStream& stream, const ReportVariantConfiguration& conf);
	static void printVariantSheetRowHeaderCnv(QTextStream& stream, bool causal);
	void printVariantSheetRowCnv(QTextStream& stream, const ReportVariantConfiguration& conf);
	static void printVariantSheetRowHeaderSv(QTextStream& stream, bool causal);
	void printVariantSheetRowSv(QTextStream& stream, const ReportVariantConfiguration& conf);
	static QString exclusionCriteria(const ReportVariantConfiguration& conf);

	GermlineReportGenerator() = delete;
};

#endif // GERMLINEREPORTGENERATOR_H
