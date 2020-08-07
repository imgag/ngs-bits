#ifndef SOMATICDATATRANSFERWIDGET_H
#define SOMATICDATATRANSFERWIDGET_H

#include <QDialog>
#include "HttpHandler.h"
#include "NGSD.h"

namespace Ui {
class SomaticDataTransferWidget;
}


#include "ui_SomaticDataTransferWidget.h"

class SomaticDataTransferWidget : public QDialog
{
	Q_OBJECT

public:
	explicit SomaticDataTransferWidget(QString t_ps_id, QString n_ps_id, QWidget *parent = 0);
	~SomaticDataTransferWidget();
private:
	Ui::SomaticDataTransferWidget *ui;

	NGSD db_;

	HttpHandler* http_handler_;

    bool xml_service_condition_ok_;
    bool pdf_service_condition_ok_;

	//tumor processed sample id
	QString t_ps_id_;
	//normal processed sample id
	QString n_ps_id_;

    //URL of MTB XML API
    QString xml_url_;

    //URL of MTB PDF API
    QString pdf_url_;

	//total path to XML file
	QString xml_path_;
	//total path to RTF file
    QString pdf_path_;

	///Adds line to status widget.
	void addRow(QString row, QString color ="", bool bold = false);

public slots:
	///Uploads XML file (from xml_path_) to MTB
	void uploadXML();

    ///Uploads RTF file (from pdf_path_) to MTB;
    void uploadPDF();

private slots:
	///Checks all requirements and enables/disables upload button
	void enableUpload();

	///Checks whether MTB server gives service status 200 ok
	void checkConnectionRequired();

	///refresh GUI
	void updateGUI();
};

#endif // SOMATICDATATRANSFERWIDGET_H
