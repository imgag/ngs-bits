#ifndef SomaticReportVariantDialog_H
#define SomaticReportVariantDialog_H

#include <QDialog>
#include "ui_SomaticReportVariantDialog.h"
#include "SomaticReportConfiguration.h"
class SomaticReportVariantDialog : public QDialog
{
	Q_OBJECT

public:
	SomaticReportVariantDialog(QString variant, SomaticReportVariantConfiguration& var_conf, QWidget *parent = 0);

private:
	Ui::SomaticReportVariantDialog ui_;
	SomaticReportVariantConfiguration&  var_conf_;

	///sets GUI entries with initial values from var_conf
	void updateGUI();
private slots:
	///Activates "ok" button if entries are valid
	void activateOkButtonIfValid();
	///Writes settings to reference SomaticReportVariantConfiguration
	void writeBackSettings();
};

#endif // SomaticReportVariantDialog_H
