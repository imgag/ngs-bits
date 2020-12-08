#ifndef TUMORONLYREPORTDIALOG_H
#define TUMORONLYREPORTDIALOG_H

#include <QDialog>
#include "TumorOnlyReportWorker.h"

namespace Ui {
class TumorOnlyReportDialog;
}

///creates user setttings for tumor-only report and writes user settings back to config
class TumorOnlyReportDialog : public QDialog
{
	Q_OBJECT

public:
	///constructor
	explicit TumorOnlyReportDialog(const VariantList& variants,TumorOnlyReportWorkerConfig& config, QWidget *parent = 0);
	///desctructor
	~TumorOnlyReportDialog();

private:
	Ui::TumorOnlyReportDialog *ui;

	TumorOnlyReportWorkerConfig& config_;
	const VariantList& variants_;

private slots:
	///provide menu with options to select and deselect more than one row at once
	void rightClickMenu(QPoint p);
public slots:
	///write settings to TumorOnlyReportWorkerConfig "config" which was passed to this Dialog
	void writeBackSettings();
};

#endif // TUMORONLYREPORTDIALOG_H
