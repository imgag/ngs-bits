#ifndef GERMLINEREPORTGENERATOR_H
#define GERMLINEREPORTGENERATOR_H

#include "cppNGSD_global.h"
#include "NGSD.h"
#include "ReportSettings.h"
#include "FilterCascade.h"

//Data for germline report gerneration
struct CPPNGSDSHARED_EXPORT GermlineReportGeneratorData
{
	//constructor
	GermlineReportGeneratorData(QString ps_, const VariantList& variants_, const CnvList& cnvs_, const BedpeFile& svs_, const ReportSettings& report_settings_, const FilterCascade& filters_, const QMap<QByteArray, QByteArrayList>& preferred_transcripts_);

	//sample data
	QString ps;
	const VariantList& variants;
	const CnvList& cnvs;
	const BedpeFile& svs;

	//target region data (set if a target region was used)
	QString roi_file;
	GeneSet roi_genes;

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

	///Returns if the pre-calcualed gaps for the given ROI.
	///If using the pre-calculated gaps file is not possible, @p message contains an error message.
	static BedFile precalculatedGaps(QString bam_file, const BedFile& roi, int min_cov, const BedFile& processing_system_target_region);
private:
	NGSD db_;
	const GermlineReportGeneratorData& data_;
	bool test_mode_;

	QString ps_id_;
	QString ps_bam_;
	BedFile roi_;
	QString sys_roi_file_;
	BedFile sys_roi_;
	QMap<QString, QString> cache_;

	static void writeHtmlHeader(QTextStream& stream, QString sample_name);
	static void writeHtmlFooter(QTextStream& stream);
	QString trans(const QString& text);
	void writeCoverageReport(QTextStream& stream);
	void writeClosedGapsReport(QTextStream& stream, const BedFile& roi);
	void writeCoverageReportCCDS(QTextStream& stream, int extend, bool gap_table=true, bool gene_details=true);
	static QByteArray formatGenotype(const QByteArray& gender, const QByteArray& genotype, const Variant& variant);
	QString formatCodingSplicing(const QList<VariantTranscript>& transcripts);

	GermlineReportGenerator() = delete;
};

#endif // GERMLINEREPORTGENERATOR_H
