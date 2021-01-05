#ifndef SOMATICDATATRANSFERWIDGET_H
#define SOMATICDATATRANSFERWIDGET_H

#include <QDialog>
#include "HttpHandler.h"
#include "NGSD.h"
#include "DelayedInitializationTimer.h"

namespace Ui {
class SomaticDataTransferWidget;
}


#include "ui_SomaticDataTransferWidget.h"

class SomaticDataTransferWidget
	: public QDialog
{
	Q_OBJECT

public:
	SomaticDataTransferWidget(QString t_ps_id, QString n_ps_id, QWidget* parent = 0);
	~SomaticDataTransferWidget();

private:
	Ui::SomaticDataTransferWidget *ui;

	DelayedInitializationTimer init_timer_;

	NGSD db_;

	HttpHandler http_handler_;

	//tumor processed sample id
	QString t_ps_id_;
	//normal processed sample id
	QString n_ps_id_;

	//api connection is ok (to be checked during initialization)
	bool api_ok_;

    //URL of MTB XML API
    QString xml_url_;
	//total path to XML file
	QString xml_path_;

	///Adds line to status widget.
	void addStatusRow(QString row, QString color ="", bool bold = false);

public slots:
	///Uploads XML file (from xml_path_) to MTB
	void uploadXML();

private slots:
	///Loads the command line input file.
	void delayedInitialization();

	///Checks all requirements and enables/disables upload button
	void enableUpload();

	///Checks whether MTB server gives service status 200 ok
	void checkApiConnection();

	///Checks if the XML file is present
	void checkUploadFiles();

	///refresh GUI
	void updateUploadStatus();
};

#endif // SOMATICDATATRANSFERWIDGET_H
