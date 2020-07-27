#ifndef SOMATICDATATRANSFERWIDGET_H
#define SOMATICDATATRANSFERWIDGET_H

#include <QDialog>
#include "HttpHandler.h"

namespace Ui {
class SomaticDataTransferWidget;
}


#include "ui_SomaticDataTransferWidget.h"

class SomaticDataTransferWidget : public QDialog
{
	Q_OBJECT

public:
	explicit SomaticDataTransferWidget(QString url, QString xml_path, QString rtf_path, QWidget *parent = 0);
	~SomaticDataTransferWidget();
private:
	Ui::SomaticDataTransferWidget *ui;

	HttpHandler* http_handler_;
	bool service_condition_ok_;

	//URL of MTB API
	QString url_;

	QString xml_path_;
	QString rtf_path_;

	///Adds line to status widget.
	void addRow(QString row, QString color ="", bool bold = false);

public slots:
	///Uploads XML file (from xml_path_) to MTB
	void uploadXML();

	///Uploads RTF file (from rtf_path_) to MTB;
	void uploadRTF();

private slots:
	///Checks all requirements and enables/disables upload button
	void enableUpload();

	///Checks whether MTB server gives service status 200 ok
	void checkConnectionRequired();
};

#endif // SOMATICDATATRANSFERWIDGET_H
