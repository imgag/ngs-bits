#ifndef CLINVARUPLOADSTATUSWIDGET_H
#define CLINVARUPLOADSTATUSWIDGET_H

#include <QWidget>

#include "Phenotype.h"
#include "NGSD.h"
#include "HttpHandler.h"
#include "ClinvarUploadDialog.h"

struct SubmissionStatus
{
	QString status;
	QString stable_id;
	QString comment;
};

namespace Ui {
class ClinvarUploadStatusWidget;
}

class ClinvarUploadStatusWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ClinvarUploadStatusWidget(QWidget *parent = 0);
	~ClinvarUploadStatusWidget();

private slots:
	void showContextMenu(QPoint pos);

private:
	Ui::ClinvarUploadStatusWidget *ui_;

	NGSD db_;
	HttpHandler http_handler_;

	void fillTable();
	SubmissionStatus getSubmissionStatus(const QString& submission_id, HttpHandler& http_handler);
	ClinvarUploadData getClinvarUploadData(int var_pub_id);
};

#endif // CLINVARUPLOADSTATUSWIDGET_H
