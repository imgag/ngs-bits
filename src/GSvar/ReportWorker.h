#ifndef REPORTWORKER_H
#define REPORTWORKER_H

#include <QObject>
#include <QString>
#include <QTextStream>
#include "VariantList.h"
#include "WorkerBase.h"
#include "NGSD.h"

///Report generation worker.
class ReportWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	///Constructor.
	ReportWorker(QString sample_name, QMap<QString, QString> filters, const VariantList& variants, const QList<int>& variants_selected, QMap<QString, QStringList> preferred_transcripts, QString outcome, QString file_roi, QString file_bam, int min_cov, QStringList log_files, QString file_rep, bool calculate_depth);
	virtual void process();

	///Returns the file to which the HTML report was written.
	QString getReportFile()
	{
		return file_rep_;
	}

	///writes a low-coverage report
	static BedFile writeCoverageReport(QTextStream& stream, QString bam_file, QString roi_file, const BedFile& roi, const GeneSet& genes, int min_cov, NGSD& db, bool calculate_depth, QMap<QString, QString>* output=nullptr);
	static void writeCoverageReportCCDS(QTextStream& stream, QString bam_file, const GeneSet& genes, int min_cov, int extend, NGSD& db, QMap<QString, QString>* output=nullptr);

	///Returns if the pre-calcualed gaps for the given ROI.
	///If using the pre-calculated gaps file is not possible, @p message contains an error message.
	static BedFile precalculatedGaps(QString bam_file, const BedFile& roi, int min_cov, NGSD& db, QString& message);

	///Returns if given ROI file is the processing system target file corresponding to the BAM file.
	static bool isProcessingSystemTargetFile(QString bam_file, QString roi_file, NGSD& db);

	static void writeHtmlHeader(QTextStream& stream, QString sample_name);
	static void writeHtmlFooter(QTextStream& stream);
	static void validateAndCopyReport(QString from, QString to, bool put_to_archive, bool is_rtf);

private:
	//input variables
	QString sample_name_;
	QString sample_name_external_;
	QMap<QString, QString> filters_;
	const VariantList& variants_;
	QList<int> variants_selected_;
	QMap<QString, QStringList> preferred_transcripts_;
	QString outcome_;
	QString file_roi_;
	QString file_bam_;
	int min_cov_;
	GeneSet genes_;
	QStringList log_files_;
	bool calculate_depth_;

	//output variables
	QString file_rep_;

	//temporary variables
	BedFile roi_;
	int var_count_;
	QMap<QString, QString> roi_stats_;

	//NGSD access
	NGSD db_;

	QString filterToGermanText(QString name, QString value);
	QString formatCodingSplicing(QByteArray text);
	QString inheritance(QString gene_info, bool color=true);
	int annotationIndexOrException(const QString& name, bool exact_match) const;
	void writeHTML();
	void writeXML(QString outfile_name);
};

#endif


