#include "ReportWorker.h"
#include "Settings.h"
#include "GUIHelper.h"
#include "Helper.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QDesktopServices>

ReportWorker::ReportWorker(GermlineReportGeneratorData data, QString filename)
	: BackgroundWorkerBase("Report generation (germline)")
	, data_(data)
	, filename_(filename)
{
}

void ReportWorker::process()
{
	//generate HTML report
	QString temp_filename = Helper::tempFileName(".html");
	GermlineReportGenerator report_generator(data_);
	report_generator.writeHTML(temp_filename);
	Helper::moveFile(temp_filename, filename_);

	//copy HTML report to archive folder
	QString archive_folder = Settings::path("gsvar_report_archive");
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

	//generate XML report
	QString gsvar_xml_folder = Settings::path("gsvar_xml_folder");
	if (gsvar_xml_folder!="")
	{
		try
		{
			temp_filename = Helper::tempFileName(".xml");
			report_generator.writeXML(temp_filename, filename_);

			QString xml_file = gsvar_xml_folder + "/" + QFileInfo(filename_).fileName().replace(".html", ".xml");
			Helper::moveFile(temp_filename, xml_file);
		}
		catch (Exception& e)
		{
			THROW(Exception, "XML generation failed: " + e.message());
		}
	}
}

void ReportWorker::userInteration()
{
	//show result info box
	if (error_.isEmpty())
	{
		if (QMessageBox::question(GUIHelper::mainWindow(), "Report", "Report generated successfully!\nDo you want to open the report in your browser?")==QMessageBox::Yes)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(filename_));
		}
	}
	else
	{
		QMessageBox::warning(GUIHelper::mainWindow(), "Error", "Report generation failed:\n" + error_);
	}
}


