#ifndef VALIDATIONDIALOG_H
#define VALIDATIONDIALOG_H

#include <QDialog>
#include "ui_ValidationDialog.h"
#include "NGSD.h"

///Dialog for variant validation
class ValidationDialog
	: public QDialog
{
	Q_OBJECT

public:
	///Constructor
	ValidationDialog(QWidget* parent, int id);

	///Stores the changed data to the NGSD
	void store();

	///Returns the current status.
	QString status();

private slots:
	///Updates the validation comment when the status changes
	void statusChanged();

private:
	Ui::ValidationDialog ui_;
	NGSD db_;
	QString val_id_;
	QString variant_type_;
	StructuralVariantType sv_type_;
};

#endif // VALIDATIONDIALOG_H
