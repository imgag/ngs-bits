#ifndef REPORTWORKER_H
#define REPORTWORKER_H

#include <QObject>
#include <QString>
#include "VariantList.h"
#include "WorkerBase.h"

///Report generation worker.
class ReportWorker
		: public WorkerBase
{
	Q_OBJECT

public:
	/*
	  @brief Constructor.
	*/
	ReportWorker(QString sample_name, QMap<QString, QString> filters, const VariantList& variants, const QVector< QPair<int, bool> >& variants_selected, QString outcome, QString file_roi, QString file_bam, bool var_details, QStringList log_files, QString file_rep);
	virtual void process();

	///Returns the file to which the HTML report was written.
	QString getReportFile()
	{
		return file_rep_;
	}

private:
	//input variables
	QString sample_name_;
	QString sample_name_external_;
	QMap<QString, QString> filters_;
	const VariantList& variants_;
	QVector< QPair<int, bool> > variants_selected_;
	QString outcome_;
	QString file_roi_;
	QString file_bam_;
	bool var_details_;
	QStringList genes_;
	QStringList log_files_;

	//output variables
	QString file_rep_;

	//temporary variables
	BedFile roi_;
	int var_count_;

	QString filterToGermanText(QString name, QString value);
	QString formatCodingSplicing(QByteArray text);
	int annotationIndexOrException(const QString& name, bool exact_match) const;
	void writeHTML();
	void writeXML();
};

#endif


