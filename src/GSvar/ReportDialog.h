#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include <QDialog>
#include "VariantList.h"
#include "ui_ReportDialog.h"

///Datastructure for report settings
struct ReportSettings
{
	DiagnosticStatusData diag_status; //diagnostic status

	QList<int> variants_selected; //variants indices that were selected for export.

	bool show_coverage_details; //slow low-coverage details
		int min_depth; //cutoff for low-coverage statistics
		bool roi_low_cov; //low-coverage details for the ROI are added (not only for CCDS)
		bool recalculate_avg_depth; //average coverage should be calculated for the target region (otherwise the processing system average depth is used)
	bool show_tool_details; //show tool version and parameter table
	bool show_class_details; //show classification information
};

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

	///Returns the report settings
	ReportSettings settings() const;


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
