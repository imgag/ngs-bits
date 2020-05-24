#ifndef CYTOBANDTOREGIONSDIALOG_H
#define CYTOBANDTOREGIONSDIALOG_H

#include <QDialog>
#include "ui_CytobandToRegionsDialog.h"

namespace Ui {
class CytobandToRegionsDialog;
}

class CytobandToRegionsDialog
	: public QDialog
{
	Q_OBJECT

public:
	CytobandToRegionsDialog(QWidget* parent = 0);

protected slots:
	void convert();

private:
	Ui::CytobandToRegionsDialog ui_;
};

#endif // CYTOBANDTOREGIONSDIALOG_H
