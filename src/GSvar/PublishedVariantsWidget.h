#ifndef PUBLISHEDVARIANTSWIDGET_H
#define PUBLISHEDVARIANTSWIDGET_H

#include "ClinvarUploadDialog.h"
#include "HttpHandler.h"

#include <QWidget>


struct SubmissionStatus
{
	QString status;
	QString stable_id;
	QString comment;
};

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
	void openVariantTab();

private:
	Ui::PublishedVariantsWidget* ui_;
	HttpHandler http_handler_;
	SubmissionStatus getSubmissionStatus(const QString& submission_id);
	ClinvarUploadData getClinvarUploadData(int var_pub_id);

};

#endif // PUBLISHEDVARIANTSWIDGET_H
