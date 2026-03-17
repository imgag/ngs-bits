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
    ValidationDialog(QWidget* parent, VariantValidation var_val);

    ///Returns the variant validation object
    VariantValidation getValidation();
	///Returns the current status.
	QString status();

private slots:
	///Updates the validation comment when the status changes
	void statusChanged();
    ///Save changes in the gui to the object
    void changed();

private:
	Ui::ValidationDialog ui_;
	NGSD db_;
    VariantValidation var_val_;
    StructuralVariantType sv_type_;
};

#endif // VALIDATIONDIALOG_H
