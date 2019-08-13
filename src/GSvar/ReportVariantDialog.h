#ifndef REPORTVARIANTDIALOG_H
#define REPORTVARIANTDIALOG_H

#include <QDialog>
#include "ui_ReportVariantDialog.h"
#include "ReportSettings.h"

class ReportVariantDialog
	: public QDialog
{
	Q_OBJECT

public:
	ReportVariantDialog(ReportVariantConfiguration& config, QWidget* parent = 0);

private:
	Ui::ReportVariantDialog ui_;
	ReportVariantConfiguration& config_;
};

#endif // REPORTVARIANTDIALOG_H
