#ifndef HEREDIVARIMPORTDIALOG_H
#define HEREDIVARIMPORTDIALOG_H

#include <QDialog>

#include "ui_HerediVarImportDialog.h"

class HerediVarImportDialog
	: public QDialog
{
	Q_OBJECT

public:
	HerediVarImportDialog(QWidget* parent);

private slots:
	void import();

private:
	Ui::HerediVarImportDialog ui_;
};

#endif // HEREDIVARIMPORTDIALOG_H
