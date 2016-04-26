#ifndef VALIDATIONDIALOG_H
#define VALIDATIONDIALOG_H

#include <QDialog>
#include "ui_ValidationDialog.h"
#include "VariantList.h"
#include "NGSD.h"

///Dialog for variant validation
class ValidationDialog
	: public QDialog
{
	Q_OBJECT

public:
	///Constructor
	ValidationDialog(QWidget* parent, QString filename, const Variant& variant, int quality_annotation_index);

	///Returns the validation information
	ValidationInfo info() const;

private slots:
	///Updates the validation comment when the status changes
	void statusChanged();

private:
	Ui::ValidationDialog ui_;
};

#endif // VALIDATIONDIALOG_H
