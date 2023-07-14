#ifndef PUBLISHEDVARIANTSWIDGET_H
#define PUBLISHEDVARIANTSWIDGET_H

#include "ClinvarUploadDialog.h"
#include "HttpHandler.h"

#include <QWidget>




namespace Ui {
class PublishedVariantsWidget;
}

class PublishedVariantsWidget
	: public QWidget
{
	Q_OBJECT

public:
	PublishedVariantsWidget(QWidget* parent = 0);
	~PublishedVariantsWidget();


private slots:
	void updateTable();
	void updateClinvarSubmissionStatus();
	void searchForVariantInLOVD();
	void searchForVariantInClinVar();
	void retryClinvarSubmission();
	void deleteClinvarSubmission();
	void openVariantTab();

private:
	Ui::PublishedVariantsWidget* ui_;
	HttpHandler http_handler_;
	ClinvarUploadData getClinvarUploadData(int var_pub_id);
	QJsonObject createJsonForClinvarDeletion(QString stable_id);

};

#endif // PUBLISHEDVARIANTSWIDGET_H
