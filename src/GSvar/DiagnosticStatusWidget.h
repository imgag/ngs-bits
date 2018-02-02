#ifndef DIAGNOSTICSTATUSWIDGET_H
#define DIAGNOSTICSTATUSWIDGET_H

#include <QWidget>
#include "ui_DiagnosticStatusWidget.h"
#include "NGSD.h"

///Widget to view/edit the diagnostic status of a sample
class DiagnosticStatusWidget
 : public QWidget
{
	Q_OBJECT

public:
	DiagnosticStatusWidget(QWidget *parent = 0);

	void setStatus(DiagnosticStatusData data);
	DiagnosticStatusData status() const;

signals:
	void outcomeChanged(QString outcome);

private:
	Ui::DiagnosticStatusWidget ui;
	QString initial_status_text_;
};

#endif // DIAGNOSTICSTATUSWIDGET_H
