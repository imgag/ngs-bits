#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

#include <QDialog>
#include "ReportSettings.h"
#include "VariantList.h"
#include "ui_ReportDialog.h"


///Report configutation dialog
class ReportDialog
		: public QDialog
{
	Q_OBJECT
	
public:
	///Constructor
	ReportDialog(ReportSettings& settings, const VariantList& variants, const CnvList& cnvs, const BedpeFile& svs, QString target_region, QWidget* parent = 0);
	///Returns the report/variant type
	QString type() const
	{
		return ui_.report_type->currentText();
	}


protected slots:
	void writeBackSettings();
	void activateOkButtonIfValid();
	void initGUI();
	void updateGUI();

protected:
	Ui::ReportDialog ui_;
	ReportSettings& settings_;
	const VariantList& variants_;
	const CnvList& cnvs_;
	const BedpeFile svs_;
	QString roi_file_;
	BedFile roi_;
};

#endif // REPORTDIALOG_H
