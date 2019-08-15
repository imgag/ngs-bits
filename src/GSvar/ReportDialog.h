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

protected slots:
	void writeBackSettings();
	void activateOkButtonIfValid();
	void showContextMenu(QPoint pos);
	void updateCoverageSettings(int state);

protected:
	///Transfers report settings to GUI
	void updateGUI();

	Ui::ReportDialog ui_;
	ReportSettings& settings_;
	const VariantList& variants_;
};

#endif // REPORTDIALOG_H
