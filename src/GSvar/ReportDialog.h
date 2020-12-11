#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include <QDialog>
#include "ReportSettings.h"
#include "VariantList.h"
#include "ui_ReportDialog.h"
#include "NGSD.h"


///Report configutation dialog
class ReportDialog
		: public QDialog
{
	Q_OBJECT
	
public:
	///Constructor
	ReportDialog(QString ps, ReportSettings& settings, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, QString target_region, QWidget* parent = 0);

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

	void editDiseaseGroupStatus();
	void editDiseaseDetails();
	void editDiagnosticStatus();

protected:
	Ui::ReportDialog ui_;
	QString ps_;
	ReportSettings& settings_;
	const VariantList& variants_;
	const CnvList& cnvs_;
	const BedpeFile svs_;
	QString roi_file_;
	BedFile roi_;
	NGSD db_;

	QTableWidgetItem* addTableItem(int row, int col, QString text, bool checkable=false, bool checked_and_not_editable=false);
};

#endif // REPORTDIALOG_H
