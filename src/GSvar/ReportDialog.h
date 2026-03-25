#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include <QDialog>
#include "ReportSettings.h"
#include "VariantList.h"
#include "ui_ReportDialog.h"
#include "NGSD.h"
#include "AnalysisDataController.h"


///Report configutation dialog
class ReportDialog
		: public QDialog
{
	Q_OBJECT
	
public:
	///Constructor
	ReportDialog(AnalysisDataController& data_controller, QWidget* parent = 0);

	///Returns the report/variant type
	QString type() const
	{
		return ui_.report_type->currentText();
	}


protected slots:
	void checkMetaData();
	void writeBackSettings();
	void activateOkButtonIfValid();
	void initGUI();
	void updateVariantTable();
	void updateCoverageCheckboxStatus();
	void validateReportConfig();

	void editDiseaseGroupStatus();
	void editDiseaseDetails();
	void editDiagnosticStatus();

protected:
	Ui::ReportDialog ui_;
	QString ps_;
    AnalysisDataController& data_controller_;
	NGSD db_;

	QTableWidgetItem* addTableItem(int row, int col, QString text);
	QTableWidgetItem* addCheckBox(int row, int col, bool is_checked, bool check_state_editable);
};

#endif // REPORTDIALOG_H
