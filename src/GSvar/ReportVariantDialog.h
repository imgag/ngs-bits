#ifndef REPORTVARIANTDIALOG_H
#define REPORTVARIANTDIALOG_H

#include <QDialog>
#include "ui_ReportVariantDialog.h"
#include "ReportSettings.h"
#include "KeyValuePair.h"

///Report configutation dialog for variants
class ReportVariantDialog
	: public QDialog
{
	Q_OBJECT

public:
	ReportVariantDialog(QString variant, QList<KeyValuePair> inheritance_by_gene, ReportVariantConfiguration& config, QWidget* parent = 0);

protected slots:
	void writeBackSettings();
	void activateOkButtonIfValid();

protected:
	///Transfers report settings to GUI
	void updateGUI();

	Ui::ReportVariantDialog ui_;
	ReportVariantConfiguration& config_;
};

#endif // REPORTVARIANTDIALOG_H
