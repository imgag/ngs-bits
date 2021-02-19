#include "ReportWorker.h"
#include "Settings.h"
#include <QFileInfo>

ReportWorker::ReportWorker(GermlineReportGeneratorData data, QString filename)
	: WorkerBase("Report generation")
	, data_(data)
	, filename_(filename)
{
}

void ReportWorker::process()
{
	qDebug() << __FILE__ << __LINE__;
	//generate HTML report
	QString temp_filename = Helper::tempFileName(".html");
	qDebug() << __FILE__ << __LINE__;
	GermlineReportGenerator report_generator(data_);
	qDebug() << __FILE__ << __LINE__;
	report_generator.writeHTML(temp_filename);
	qDebug() << __FILE__ << __LINE__;
	moveReport(temp_filename, filename_);

	qDebug() << __FILE__ << __LINE__;
	//copy HTML report to archive folder
	QString archive_folder = Settings::string("gsvar_report_archive");
	if (archive_folder!="")
	{
		QString file_rep_copy = archive_folder + "\\" + QFileInfo(filename_).fileName();
		if (QFile::exists(file_rep_copy) && !QFile::remove(file_rep_copy))
		{
			THROW(FileAccessException, "Could not remove previous report in archive folder: " + file_rep_copy);
		}
		if (!QFile::copy(filename_, file_rep_copy))
		{
			THROW(FileAccessException, "Could not copy report to archive folder: " + file_rep_copy);
		}
	}

	qDebug() << __FILE__ << __LINE__;
	//generate XML report
	QString gsvar_xml_folder = Settings::string("gsvar_xml_folder");
	if (gsvar_xml_folder!="")
	{
		temp_filename = Helper::tempFileName(".xml");
		report_generator.writeXML(temp_filename, filename_);

		QString xml_file = gsvar_xml_folder + "/" + QFileInfo(filename_).fileName().replace(".html", ".xml");
		moveReport(temp_filename, xml_file);
	}
	qDebug() << __FILE__ << __LINE__;
}

void ReportWorker::moveReport(QString temp_filename, QString filename)
{
	if (QFile::exists(filename) && !QFile(filename).remove())
	{
		THROW(FileAccessException,"Could not remove previous report: " + filename);
	}
	if (!QFile::rename(temp_filename, filename))
	{
		THROW(FileAccessException,"Could not move report from temporary file " + temp_filename + " to " + filename + " !");
	}
}
