#include <QDir>
#include <QDebug>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>

#include "SomaticDataTransferWidget.h"
#include "Settings.h"
#include "Exceptions.h"
#include "Helper.h"
#include "NGSD.h"
#include "Settings.h"

#include "ui_SomaticDataTransferWidget.h"


SomaticDataTransferWidget::SomaticDataTransferWidget(QString t_ps_id, QString n_ps_id, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SomaticDataTransferWidget),
	db_(),
	t_ps_id_(t_ps_id),
	n_ps_id_(n_ps_id)
{
	url_ = Settings::string("mtb_xml_upload_url");
	xml_path_ = Settings::string("gsvar_somatic_xml_folder") + "/" + t_ps_id + "-" + n_ps_id + ".xml";
	rtf_path_ = Settings::string("genlab_somatic_report_folder") + "/" + t_ps_id + "-" + n_ps_id + ".rtf";


	ui->setupUi(this);
	ui->status_display->setReadOnly(true);

	connect( ui->xml_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->rtf_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadXML()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadRTF()) );
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
	if(ui->xml_select->isChecked())
	{
		addRow("#Uploading XML file " + xml_path_);
		addRow(">POST " + xml_path_ + " " + url_ + "/mtb_imgag");

		QString res = "";
		try
		{
			HttpHeaders add_headers;
			add_headers.insert("Content-Type", "application/xml");

			QSharedPointer<QFile> file = Helper::openFileForReading(xml_path_);
			res =  http_handler_->post(url_ + "/mtb_imgag", file->readAll(), add_headers);
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
	}

	updateGUI();
}

void SomaticDataTransferWidget::uploadRTF()
{
	if( ui->rtf_select->isChecked() )
	{
		;//TODO: implement on both sides > AXEL
	}
}

void SomaticDataTransferWidget::enableUpload()
{
	if(service_condition_ok_ && (ui->xml_select->isChecked() || ui->rtf_select->isChecked()))
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
	//check server is available
	addRow("#Checking server connection to MTB");
	QString service_condition = "";
	try
	{
		addRow(">GET " + url_ + "/condition");
		service_condition = http_handler_->get(url_ + "/condition");
	}
	catch(Exception e ) //connection to server failed
	{
		addRow(e.message(), "red");
		service_condition_ok_ = false;
	}

	if(service_condition == "Service-Condition: OK")
	{
		service_condition_ok_ = true;
		addRow(service_condition);
	}
	else //server replied corrupt answer
	{
		service_condition_ok_ = false;
		addRow(service_condition, "red");
	}
	enableUpload();
}

void SomaticDataTransferWidget::updateGUI()
{
	ui->xml_select->setText(xml_path_);

	if(!QFileInfo(xml_path_).isFile())
	{
		ui->xml_select->setText(ui->xml_select->text() + " (not found)");
		ui->xml_select->setEnabled(false);
	}

	ui->rtf_select->setText(rtf_path_);
	if(!QFileInfo(rtf_path_).isFile())
	{
		ui->rtf_select->setText(ui->rtf_select->text() + " (not found)");
		ui->rtf_select->setEnabled(false);
	}


	int config_id = db_.somaticReportConfigId( db_.processedSampleId(t_ps_id_), db_.processedSampleId(n_ps_id_) );
	SomaticReportConfigurationData conf_data = db_.somaticReportConfigData(config_id);
	if(conf_data.mtb_xml_upload_date != "")
	{
		ui->xml_select->setText(ui->xml_select->text() + " (uploaded: " + conf_data.mtb_xml_upload_date + ")");
	}
	if(conf_data.mtb_rtf_upload_date != "")
	{
		ui->rtf_select->setText(ui->rtf_select->text() + " (uploaded: " + conf_data.mtb_rtf_upload_date + ")");
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

		temp += row.replace("\n", "<br />");

		if(bold) temp += "</b>";
		if(color != "") temp += "</font>";

		temp += "<br />\n";

		ui->status_display->insertHtml(temp);
	}

	ui->status_display->moveCursor(QTextCursor::End);
}
