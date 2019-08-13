#include "ReportVariantDialog.h"

ReportVariantDialog::ReportVariantDialog(ReportVariantConfiguration& config, QWidget* parent)
	: QDialog(parent)
	, ui_()
	, config_(config)
{
	ui_.setupUi(this);
}
