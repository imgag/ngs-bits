#ifndef DIAGNOSTICSTATUSOVERVIEWDIALOG_H
#define DIAGNOSTICSTATUSOVERVIEWDIALOG_H

#include <QDialog>
#include "ui_DiagnosticStatusOverviewDialog.h"
#include "NGSD.h"
#include "DelayedInitializationTimer.h"

///Sample overview dialog.
class DiagnosticStatusOverviewDialog
	: public QDialog
{
	Q_OBJECT

public:
	DiagnosticStatusOverviewDialog(QWidget* parent = 0, QString project="");

signals:
	void openProcessedSampleTab(QString processed_sample_name);

private slots:
	void delayedInitialization();
	void updateOverviewTable();
	void copyTableToClipboard();
	void sampleContextMenu(QPoint pos);
	void addItem(int r, int c, QString text, bool text_as_tooltip = false);

private:
	Ui::DiagnosticStatusOverviewDialog ui;
	DelayedInitializationTimer init_timer_;
};

#endif // DIAGNOSTICSTATUSOVERVIEWDIALOG_H
