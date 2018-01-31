#ifndef DIAGNOSTICSTATUSOVERVIEWDIALOG_H
#define DIAGNOSTICSTATUSOVERVIEWDIALOG_H

#include <QDialog>
#include "ui_DiagnosticStatusOverviewDialog.h"
#include "NGSD.h"

///Sample overview dialog.
class DiagnosticStatusOverviewDialog
	: public QDialog
{
	Q_OBJECT

public:
	DiagnosticStatusOverviewDialog(QWidget *parent = 0);

	QString processedSampleToOpen();

private slots:
	void updateOverviewTable();
	void copyTableToClipboard();
	void sampleContextMenu(QPoint pos);
	void addItem(int r, int c, QString text, bool text_as_tooltip = false);

private:
	Ui::DiagnosticStatusOverviewDialog ui;
	QString processed_sample_to_open;
};

#endif // DIAGNOSTICSTATUSOVERVIEWDIALOG_H
