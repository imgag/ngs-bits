#include "NGSDReannotationDialog.h"

NGSDReannotationDialog::NGSDReannotationDialog(QString roi_file, QWidget *parent)
	: QDialog(parent)
	, ui_()
	, roi_file_(roi_file)
{
	ui_.setupUi(this);

	ui_.filter_roi->setEnabled(roi_file!="");
	connect(ui_.filter_af, SIGNAL(clicked(bool)), ui_.filter_af_value, SLOT(setEnabled(bool)));
}

double NGSDReannotationDialog::maxAlleleFrequency() const
{
	if (!ui_.filter_af->isChecked()) return 0.0;

	return ui_.filter_af_value->value() / 100.0;
}

QString NGSDReannotationDialog::roiFile() const
{
	if (!ui_.filter_roi->isChecked()) return "";

	return roi_file_;
}
