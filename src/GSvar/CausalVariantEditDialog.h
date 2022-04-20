#ifndef CAUSALVARIANTEDITDIALOG_H
#define CAUSALVARIANTEDITDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include "ReportConfiguration.h"

namespace Ui {
class CausalVariantEditDialog;
}

class CausalVariantEditDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CausalVariantEditDialog(const OtherCausalVariant& causal_variant, const QStringList& variant_types, const QStringList& inheritance_modes, QWidget *parent = 0);
	~CausalVariantEditDialog();
	OtherCausalVariant causalVariant();

private slots:
	void updateCausalVariant();
	void enableOkButton();

private:
	Ui::CausalVariantEditDialog* ui_;
	OtherCausalVariant causal_variant_;

};

#endif // CAUSALVARIANTEDITDIALOG_H
