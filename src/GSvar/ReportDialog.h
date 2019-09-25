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
	ReportDialog(ReportSettings& settings, const VariantList& variants, QWidget* parent = 0);
	///Updates dialog depending on target region selection state
	void setTargetRegionSelected(bool is_selected);
	///Returns the report/variant type
	QString type() const
	{
		return ui_.variant_type->currentText();
	}


protected slots:
	void writeBackSettings();
	void activateOkButtonIfValid();
	void updateCoverageSettings(int state);
	void updateGUI();

protected:

	Ui::ReportDialog ui_;
	ReportSettings& settings_;
	const VariantList& variants_;
};

#endif // REPORTDIALOG_H
