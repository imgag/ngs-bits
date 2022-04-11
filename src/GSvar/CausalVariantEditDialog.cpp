#include "CausalVariantEditDialog.h"
#include "ui_CausalVariantEditDialog.h"

CausalVariantEditDialog::CausalVariantEditDialog(const OtherCausalVariant& causal_variant, const QStringList& variant_types, QWidget* parent) :
	QDialog(parent),
	ui_(new Ui::CausalVariantEditDialog),
	causal_variant_(causal_variant)
{
	ui_->setupUi(this);

	// set init values
	ui_->le_coordinates->setText(causal_variant_.coordinates);
	ui_->le_gene->setText(causal_variant_.gene);
	ui_->cb_type->addItems(variant_types);
	if(!causal_variant_.type.isEmpty()) ui_->cb_type->setCurrentText(causal_variant_.type);
	ui_->le_comment->setText(causal_variant_.comment);

	// connect signals and slots
	connect(ui_->buttonBox, SIGNAL(accepted()), this, SLOT(updateCausalVariant()));
	connect(ui_->le_coordinates, SIGNAL(textChanged(QString)), this, SLOT(enableOkButton()));
	connect(ui_->cb_type, SIGNAL(currentIndexChanged(int)), this, SLOT(enableOkButton()));

	// remove question mark from the title bar
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	// init button box
	enableOkButton();
}

CausalVariantEditDialog::~CausalVariantEditDialog()
{
	delete ui_;
}

OtherCausalVariant CausalVariantEditDialog::causalVariant()
{
	return causal_variant_;
}

void CausalVariantEditDialog::updateCausalVariant()
{
	causal_variant_.coordinates = ui_->le_coordinates->text();
	causal_variant_.gene = ui_->le_gene->text();
	causal_variant_.type = ui_->cb_type->currentText();
	causal_variant_.comment = ui_->le_comment->text();

	emit accepted();
}

void CausalVariantEditDialog::enableOkButton()
{
	if((ui_->le_coordinates->text().trimmed().isEmpty()) || (ui_->cb_type->currentText().trimmed().isEmpty()))
	{
		ui_->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}
	else
	{
		ui_->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	}
}
