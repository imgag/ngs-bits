#include <QDir>
#include <QDebug>
#include <QPushButton>
#include <QTimer>

#include "SomaticDataTransferWidget.h"
#include "Settings.h"
#include "Exceptions.h"

#include "ui_SomaticDataTransferWidget.h"


SomaticDataTransferWidget::SomaticDataTransferWidget(QString url, QString xml_path, QString rtf_path, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SomaticDataTransferWidget),
	url_(url),
	xml_path_(xml_path),
	rtf_path_(rtf_path)
{
	ui->setupUi(this);

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

	connect( ui->xml_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->rtf_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadXML()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadRTF()) );
	connect( ui->reconnect_button, SIGNAL(clicked()), this, SLOT(checkConnectionRequired()) );

	ui->status_display->setReadOnly(true);

	http_handler_ = new HttpHandler(HttpHandler::ProxyType::NONE, this);

	//Check connection (just after termination of constructor)
	QTimer::singleShot( 0,this,SLOT(checkConnectionRequired()) );
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
			res =  http_handler_->post(url_ + "/mtb_imgag", QFile(xml_path_).readAll(), add_headers);
		}
		catch(Exception& e)
		{
			res = e.message();
		}

		if(res == "XSD-Validation passed")
		{
			addRow(res);
			addRow("#Upload of XML file successful.","", true);
		}
		else
		{
			addRow(res, "red");
			addRow("#Upload of XML failed.","red", true);
		}
	}
}

void SomaticDataTransferWidget::uploadRTF()
{
	if( ui->rtf_select->isChecked() )
	{
		;//TODO: implement on both sides
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
