#ifndef RnaReportFusionDialog_H
#define RnaReportFusionDialog_H

#include <QDialog>
#include "ui_RnaReportFusionDialog.h"
#include "RnaReportConfiguration.h"
class RnaReportFusionDialog : public QDialog
{
	Q_OBJECT

public:
	RnaReportFusionDialog(QString variant, RnaReportFusionConfiguration& conf, QWidget *parent = 0);

	///Disables include text boxes (in case of CNVS)
	void disableIncludeForm();

private:
	Ui::RnaReportFusionDialog ui_;
	RnaReportFusionConfiguration&  var_conf_;

	///sets GUI entries with initial values from var_conf
	void updateGUI();
private slots:
	///Writes settings to reference RnaReportFusionConfiguration
	void writeBackSettings();
};

#endif // RnaReportFusionDialog_H
