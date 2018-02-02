#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include <QDialog>
#include "VariantList.h"
#include "ui_ReportDialog.h"

///Report generation dialog
class ReportDialog
		: public QDialog
{
	Q_OBJECT
	
public:

	///Constructor
	ReportDialog(QString filename, QWidget* parent = 0);
	///Adds a variant to the selection list
	void addVariants(const VariantList& variants, const QBitArray& visible);
	///Updates dialog depending on target region selection state
	void setTargetRegionSelected(bool is_selected);

	///Returns the variants indices that were selected for export (along with the flag for very important variants).
	QList<int> selectedIndices() const;
	///Returns if low-coverage details should be added to the report.
	bool detailsCoverage() const;
	///Returns if low-coverage details should be added to the report for the ROI.
	bool detailsCoverageROI() const;
	///If average coverage should be calculated for the target region (otherwise the processing system average depth is used).
	bool calculateDepth() const;
	///Returns the cutoff for low-coverage statistics
	int minCoverage() const;

	///Returns if tool details should be added to the report.
	bool toolDetails() const;

	///Returns the disgnostic status.
	DiagnosticStatusData diagnosticStatus() const;

private slots:
	void outcomeChanged(QString text);
	void showContextMenu(QPoint pos);
	void updateCoverageSettings(int state);

private:
	Ui::ReportDialog ui_;
	QString filename_;
	QStringList labels_;
};

#endif // REPORTDIALOG_H
