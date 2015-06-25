#ifndef VALIDATIONDIALOG_H
#define VALIDATIONDIALOG_H

#include <QDialog>
#include "ui_ValidationDialog.h"
#include "VariantList.h"

///Dialog for variant validation
class ValidationDialog
	: public QDialog
{
	Q_OBJECT

public:
	///Constructor
	ValidationDialog(QWidget* parent, QString filename, const Variant& variant, int quality_annotation_index);

	///Returns the current validation status
	QString status() const;
	///Returns the current validation comment
	QString comment() const;

private slots:
	///Updates the validation comment when the status changes
	void statusChanged();

private:
	Ui::ValidationDialog ui_;
};

#endif // VALIDATIONDIALOG_H
