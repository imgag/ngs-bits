#include "NsxSettingsDialog.h"

NsxSettingsDialog::NsxSettingsDialog(QWidget* parent)
	: QDialog(parent)
	, ui_()
{
	ui_.setupUi(this);
}

NsxAnalysisSettings NsxSettingsDialog::getSettings() const
{
	NsxAnalysisSettings output;
	output.adapter_trimming = ui_.adapter_trimming->isChecked();
	output.dragen_analysis = ui_.dragen_analysis->isChecked();

	return output;
}
