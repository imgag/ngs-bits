#ifndef GENEINFODIALOG_H
#define GENEINFODIALOG_H

#include <QDialog>
#include "NGSD.h"

namespace Ui {
class GeneInfoDialog;
}

class GeneInfoDialog
	: public QDialog
{
	Q_OBJECT

public:
	explicit GeneInfoDialog(QString symbol, QWidget* parent = 0);
	~GeneInfoDialog();

private slots:
	void enableOkButton();
	void storeGeneInfo();

private:
	Ui::GeneInfoDialog *ui;
	NGSD db_;
};

#endif // GENEINFODIALOG_H
