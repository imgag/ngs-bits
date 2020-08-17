#include <QDir>
#include <QDebug>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QHttpMultiPart>
#include <QNetworkReply>

#include "SomaticDataTransferWidget.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSD.h"
#include "Settings.h"
#include "GenLabDB.h"

#include "ui_SomaticDataTransferWidget.h"


SomaticDataTransferWidget::SomaticDataTransferWidget(QString t_ps_id, QString n_ps_id, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SomaticDataTransferWidget),
	init_timer_(this, true),
	db_(),
	http_handler_(HttpHandler::ProxyType::NONE, this),
	t_ps_id_(t_ps_id),
    n_ps_id_(n_ps_id)
{
	ui->setupUi(this);
	connect( ui->xml_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->pdf_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadXML()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadPDF()) );

	xml_url_ = Settings::string("mtb_xml_upload_url");
    pdf_url_ = Settings::string("mtb_pdf_upload_url");

	xml_path_ = Settings::string("gsvar_xml_folder") + "/" + t_ps_id + "-" + n_ps_id + ".xml";
    pdf_path_ = Settings::string("genlab_somatic_report_folder") + "/" + t_ps_id + "-" + n_ps_id + ".pdf";
	checkUploadFiles();

	//Check whether there is somatic report configuration
	if( db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) ) == -1)
	{
		THROW(DatabaseException, "Could not find a somatic report configuration for " + t_ps_id_ + "-" +n_ps_id_ +". Please create somatic report configuration before MTB upload.");
	}
}

SomaticDataTransferWidget::~SomaticDataTransferWidget()
{
	delete ui;
}

void SomaticDataTransferWidget::delayedInitialization()
{
	checkApiConnection();

	updateUploadStatus();
}

void SomaticDataTransferWidget::uploadXML()
{
	if(ui->xml_select->isChecked())
	{
		addStatusRow("Uploading XML file");

		QString res = "";
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Content-Type", "application/xml");

			QSharedPointer<QFile> file = Helper::openFileForReading(xml_path_);
			res =  http_handler_.post(xml_url_ + "/mtb_imgag", file->readAll(), add_headers);
			file->close();
		}
		catch(Exception& e)
		{
			res = e.message();
		}

		if(res == "XSD-Validation passed")
		{
			addStatusRow("OK");
			db_.setSomaticMtbXmlUpload( db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) ) );
			updateUploadStatus();
		}
		else
		{
			addStatusRow("Upload of XML failed!","red", true);
			addStatusRow(res, "red");
		}
		addStatusRow("");
	}
}

void SomaticDataTransferWidget::uploadPDF()
{
	if(ui->pdf_select->isChecked())
	{
		addStatusRow("Uploading PDF file");

		QString res = "";
		try
		{
			QHttpMultiPart parts(this);
			parts.setContentType(QHttpMultiPart::FormDataType);

			QHttpPart file_part;
			//filename must contain SAP id as prefix to be recognized @MTB API
			GenLabDB genlab;
			QString file_name_for_api = genlab.sapID(t_ps_id_) + QFileInfo(pdf_path_).fileName();
			file_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data\"; filename=\"" + file_name_for_api + "\""));
			file_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/pdf") );

			QFile file(pdf_path_, this);
			file.open(QIODevice::ReadOnly);

			file_part.setBodyDevice(&file);
			parts.append(file_part);

			HttpHeaders add_headers;
			add_headers.insert("Authorization", "Basic " +QString(Settings::string("mtb_pdf_upload_user") + ":" +Settings::string("mtb_pdf_upload_pass") ).toUtf8().toBase64());

			res = http_handler_.post( pdf_url_ + "/ZPM_rest/mtb_imgag_doc", &parts, add_headers);
		}
		catch(Exception& e)
		{
			res = e.message();
		}

		if(res == "FILE WAS UPLOAD TO ZPM")
		{
			addStatusRow("OK");
			db_.setSomaticMtbPdfUpload( db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) ) );

			updateUploadStatus();

			if(QMessageBox::question(this, "Delete file", "Upload of PDF report successful. Delete PDF report " + pdf_path_ + "?") == QMessageBox::Yes)
			{
				QFile(pdf_path_).remove();//TODO check if deleted => show message if not > AXEL
			}
		}
		else
		{
			addStatusRow("Upload of PDF failed!", "red", true);
			addStatusRow(res, "red");
		}
		addStatusRow("");
	}
}

void SomaticDataTransferWidget::enableUpload()
{
	//only allow upload if both both APIs are ok
	if( ui->pdf_select->isChecked() || ui->xml_select->isChecked() )
	{
		ui->upload_button->setEnabled(true);
	}
	else
	{
		ui->upload_button->setEnabled(false);
	}
}

void SomaticDataTransferWidget::checkApiConnection()
{
	addStatusRow("Checking server connection for MTB upload...");

	//PDF
	QString reply = "";
	try
    {
		reply = http_handler_.get(pdf_url_ + "/ZPM_rest");
    }
	catch(Exception& e) //connection to server failed
    {
		THROW(Exception, "Connection to the ZPM PDF server failed:\n" + e.message());
    }
	if(reply!="The rest API from the ZPM is alive")
    {
		THROW(Exception, "Connection to the ZPM PDF server failed:\nInvalid reply from server:\n" + reply);
	}

	//XML
	try
	{
		reply = http_handler_.get(xml_url_ + "/condition");
	}
	catch(Exception& e)  //connection to server failed
	{
		THROW(Exception, "Connection to the ZPM XML server failed:\n" + e.message());
	}
	if(reply!="Service-Condition: OK")
	{
		THROW(Exception, "Connection to the ZPM PDF server failed:\nInvalid reply from server:\n" + reply);
	}

	addStatusRow("OK");
	addStatusRow("");
}

void SomaticDataTransferWidget::checkUploadFiles()
{
	if(QFile::exists(xml_path_))
	{
		ui->xml_filename->setText(xml_path_);
	}
	else
	{
		ui->xml_filename->setText("<font color='red'>not found</font>");
		ui->xml_select->setEnabled(false);
	}

	if(QFile::exists(pdf_path_))
	{
		ui->pdf_filename->setText(pdf_path_);
	}
	else
	{
		ui->pdf_filename->setText("<font color='red'>not found</font>");
		ui->pdf_select->setEnabled(false);
	}

}

void SomaticDataTransferWidget::updateUploadStatus()
{
	try
	{
		QApplication::setOverrideCursor(Qt::BusyCursor);

		int config_id = db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) );
		SomaticReportConfigurationData conf_data = db_.somaticReportConfigData(config_id);

		if(conf_data.mtb_xml_upload_date != "")
		{
			ui->xml_last_upload->setText("last upload: " + conf_data.mtb_xml_upload_date);
		}
		else
		{
			ui->xml_last_upload->clear();
		}

		if(conf_data.mtb_pdf_upload_date != "")
		{
			ui->pdf_last_upload->setText("last upload: " + conf_data.mtb_pdf_upload_date);
		}
		else
		{
			ui->pdf_last_upload->clear();
		}
	}
	catch (...)
	{
		QApplication::restoreOverrideCursor();
		throw;
	}

	QApplication::restoreOverrideCursor();
}


void SomaticDataTransferWidget::addStatusRow(QString row, QString color, bool bold)
{
	if(color == "" && !bold)
	{
		ui->status_display->insertPlainText(row + "\n");
	}
	else
	{
		QString temp = "";
		if(color != "") temp += "<font color='" + color + "'>";
		if(bold) temp += "<b>";

        temp += row.replace("\n", "\n<br />");

		if(bold) temp += "</b>";
		if(color != "") temp += "</font>";

		temp += "<br />\n";

		ui->status_display->insertHtml(temp);
	}

	ui->status_display->moveCursor(QTextCursor::End);
}
