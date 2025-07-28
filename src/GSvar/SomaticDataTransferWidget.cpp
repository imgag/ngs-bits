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

#include "ui_SomaticDataTransferWidget.h"


SomaticDataTransferWidget::SomaticDataTransferWidget(QString t_ps_id, QString n_ps_id, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SomaticDataTransferWidget),
	init_timer_(this, true),
	db_(),
	http_handler_(true, this),
	t_ps_id_(t_ps_id),
	n_ps_id_(n_ps_id),
	api_ok_(false)
{
	ui->setupUi(this);
	connect( ui->xml_select, SIGNAL(stateChanged(int)), this, SLOT(enableUpload()) );
	connect( ui->upload_button, SIGNAL(clicked()), this, SLOT(uploadXML()) );

	xml_url_ = Settings::string("mtb_xml_upload_url").trimmed();
	xml_path_ = Settings::path("gsvar_xml_folder") + "/" + t_ps_id + "-" + n_ps_id + ".xml";
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
	try
	{
		checkApiConnection();
	}
	catch(Exception& e)
	{
		QMessageBox::warning(this, "Connection to ZPM API", "Connection to ZPM API failed. Error message: " + e.message());
		close();
	}

	updateUploadStatus();
	enableUpload();
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

			QSharedPointer<VersatileFile> file = Helper::openVersatileFileForReading(xml_path_);
			res =  http_handler_.post(xml_url_ + "/mtb_imgag", file->readAll(), add_headers);
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

void SomaticDataTransferWidget::enableUpload()
{
	//only allow upload if both both APIs are ok
	if(api_ok_ && ui->xml_select->isChecked() )
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

	//check API response for XML files
	QByteArray reply = "";
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
		THROW(Exception, "Connection to the ZPM XML server failed:\nInvalid reply from server:\n" + reply);
	}

	addStatusRow("OK");
	api_ok_ = true;

	addStatusRow("");
}

void SomaticDataTransferWidget::checkUploadFiles()
{
	if(QFile::exists(xml_path_))
	{
		ui->xml_filename->setText(xml_path_);
		ui->xml_select->setChecked(true);
	}
	else
	{
		ui->xml_filename->setText("<font color='red'>not found</font>");
		ui->xml_select->setChecked(false);
		ui->xml_select->setEnabled(false);
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
