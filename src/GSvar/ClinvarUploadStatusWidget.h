#ifndef CLINVARUPLOADSTATUSWIDGET_H
#define CLINVARUPLOADSTATUSWIDGET_H

#include <QWidget>

#include "Phenotype.h"
#include "NGSD.h"
#include "HttpHandler.h"

namespace Ui {
class ClinvarUploadStatusWidget;
}

class ClinvarUploadStatusWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ClinvarUploadStatusWidget(QWidget *parent = 0);
	~ClinvarUploadStatusWidget();

private:
	Ui::ClinvarUploadStatusWidget *ui_;

	NGSD db_;
	HttpHandler http_handler_;

	void fillTable();
	QString getSubmissionStatus(const QString& submission_id, HttpHandler& http_handler);
};

#endif // CLINVARUPLOADSTATUSWIDGET_H
