#include <QDir>
#include <QDebug>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QHttpMultiPart>
#include <QPointer>
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
	db_(),
    xml_service_condition_ok_(false),
    pdf_service_condition_ok_(false),
	t_ps_id_(t_ps_id),
    n_ps_id_(n_ps_id)
{
    xml_url_ = Settings::string("mtb_xml_upload_url");
    pdf_url_ = Settings::string("mtb_pdf_upload_url");
	xml_path_ = Settings::string("gsvar_somatic_xml_folder") + "/" + t_ps_id + "-" + n_ps_id + ".xml";


    pdf_path_ = Settings::string("genlab_somatic_report_folder") + "/" + t_ps_id + "-" + n_ps_id + ".pdf";


	ui->setupUi(this);
	ui->status_display->setReadOnly(true);

	connect( ui->xml_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
    connect( ui->pdf_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadXML()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadPDF()) );
	connect( ui->reconnect_button, SIGNAL(clicked()), this, SLOT(checkConnectionRequired()) );

    http_handler_ = new HttpHandler(HttpHandler::ProxyType::NONE, this);


	//Check whether there is somatic report configuration
	if( db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) ) == -1)
	{
		THROW(DatabaseException, "Could not find a somatic report configuration for " + t_ps_id_ + "-" +n_ps_id_ +". Please create somatic report configuration before MTB upload.");
	}

	//set up GUI
	updateGUI();
	//Check connection (just after termination of constructor)
	QTimer::singleShot( 0,this, SLOT(checkConnectionRequired()) );
}

SomaticDataTransferWidget::~SomaticDataTransferWidget()
{
	delete ui;
}

void SomaticDataTransferWidget::uploadXML()
{
    if( xml_service_condition_ok_ && ui->xml_select->isChecked() )
	{
		addRow("#Uploading XML file " + xml_path_);
        addRow(">POST " + xml_path_ + " " + xml_url_ + "/mtb_imgag");

		QString res = "";
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Content-Type", "application/xml");

			QSharedPointer<QFile> file = Helper::openFileForReading(xml_path_);
            res =  http_handler_->post(xml_url_ + "/mtb_imgag", file->readAll(), add_headers);
			file->close();
		}
		catch(Exception& e)
		{
			res = e.message();
		}

		if(res == "XSD-Validation passed")
		{
			addRow(res);
			addRow("#Upload of XML file successful.","", true);
			db_.setSomaticMtbXmlUpload( db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) ) );
		}
		else
		{
			addRow(res, "red");
			addRow("#Upload of XML failed.","red", true);
		}

		updateGUI();
	}
}

void SomaticDataTransferWidget::uploadPDF()
{
    if( pdf_service_condition_ok_ && ui->pdf_select->isChecked() )
	{
        addRow("#Uploading PDF file " + pdf_path_);
        addRow(">POST " + pdf_path_ + " " + pdf_url_ + "/ZPM_rest/mtb_imgag_doc");

		QString res = "";
		try
		{
			QHttpMultiPart *parts = new QHttpMultiPart(this);
			parts->setContentType(QHttpMultiPart::FormDataType);

			QHttpPart file_part;
			//filename must contain SAP id as prefix to be recognized @MTB API
			GenLabDB genlab;
			QString file_name_for_api = genlab.sapID(t_ps_id_) + QFileInfo(pdf_path_).fileName();
			file_part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data\"; filename=\"" + file_name_for_api + "\""));
			file_part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/pdf") );

			QPointer<QFile> file = new  QFile(pdf_path_, this);
			file->open(QIODevice::ReadOnly);
			file_part.setBodyDevice(file);
			parts->append(file_part);

			HttpHeaders add_headers;
			add_headers.insert("Authorization", "Basic " +QString(Settings::string("mtb_pdf_upload_user") + ":" +Settings::string("mtb_pdf_upload_pass") ).toUtf8().toBase64());

			res = http_handler_->post( pdf_url_ + "/ZPM_rest/mtb_imgag_doc", parts, add_headers);
		}
		catch(Exception& e)
		{
			res = e.message();
		}

		if(res == "FILE WAS UPLOAD TO ZPM")
		{
			addRow(res);
			addRow("#Upload of PDF file successful.","", true);
			db_.setSomaticMtbPdfUpload( db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) ) );
		}
		else
		{
			addRow(res, "red");
			addRow("#Upload of PDF failed.", "red", true);
		}
		updateGUI();
	}
}

void SomaticDataTransferWidget::enableUpload()
{
	//only allow upload if both both APIs are ok
	if( (xml_service_condition_ok_ && pdf_service_condition_ok_)  && ( ui->pdf_select->isChecked() || ui->xml_select->isChecked() ) )
	{
		ui->upload_button->setEnabled(true);
	}
	else
	{
		ui->upload_button->setEnabled(false);
	}
}

void SomaticDataTransferWidget::checkConnectionRequired()
{
    ui->reconnect_button->setEnabled(false);

    addRow("#Checking server connection for MTB PDF upload.");
    QString pdf_service_condition = "";
    try
    {
        addRow("<GET " + pdf_url_ + "/ZPM_rest");
        pdf_service_condition = http_handler_->get(pdf_url_ + "/ZPM_rest");
    }
    catch(Exception e ) //connection to server failed
    {
        addRow(e.message(), "red");
        pdf_service_condition_ok_ = false;
    }
    if(pdf_service_condition == "The rest API from the ZPM is alive")
    {
        pdf_service_condition_ok_ = true;
        addRow(pdf_service_condition);
    }
    else
    {
        pdf_service_condition_ok_ = false;
        addRow(pdf_service_condition, "red");
    }

	//check server is available
    addRow("#Checking server connection to MTB XML upload.");
    QString xml_service_condition = "";
	try
	{
        addRow(">GET " + xml_url_ + "/condition");
        xml_service_condition = http_handler_->get(xml_url_ + "/condition");
	}
	catch(Exception e ) //connection to server failed
	{
		addRow(e.message(), "red");
        xml_service_condition_ok_ = false;
	}

    if(xml_service_condition == "Service-Condition: OK")
	{
        xml_service_condition_ok_ = true;
        addRow(xml_service_condition);
	}
	else //server replied corrupt answer
	{
        xml_service_condition_ok_ = false;
        addRow(xml_service_condition, "red");
	}

	enableUpload();

    ui->reconnect_button->setEnabled(true);
}

void SomaticDataTransferWidget::updateGUI()
{
	ui->xml_select->setText(xml_path_);

	if(!QFileInfo(xml_path_).isFile())
	{
		ui->xml_select->setText(ui->xml_select->text() + " (not found)");
		ui->xml_select->setEnabled(false);
	}

    ui->pdf_select->setText(pdf_path_);
    if(!QFileInfo(pdf_path_).isFile())
	{
        ui->pdf_select->setText(ui->pdf_select->text() + " (not found)");
        ui->pdf_select->setEnabled(false);
	}


	int config_id = db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) );
	SomaticReportConfigurationData conf_data = db_.somaticReportConfigData(config_id);
	if(conf_data.mtb_xml_upload_date != "")
	{
		ui->xml_select->setText(ui->xml_select->text() + " (uploaded: " + conf_data.mtb_xml_upload_date + ")");
	}
    if(conf_data.mtb_pdf_upload_date != "")
	{
        ui->pdf_select->setText(ui->pdf_select->text() + " (uploaded: " + conf_data.mtb_pdf_upload_date + ")");
	}
}


void SomaticDataTransferWidget::addRow(QString row, QString color, bool bold)
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
