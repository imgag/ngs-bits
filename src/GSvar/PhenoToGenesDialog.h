#ifndef PHENOTOGENESDIALOG_H
#define PHENOTOGENESDIALOG_H

#include <QDialog>
#include "NGSD.h"
#include <QListWidgetItem>

namespace Ui {
class PhenoToGenesDialog;
}

class PhenoToGenesDialog
	: public QDialog
{
	Q_OBJECT

public:
        explicit PhenoToGenesDialog(QWidget* parent = 0);
        ~PhenoToGenesDialog();

private slots:
	void copyPhenotype(QString phenotype);
	void deletePhenotype(QListWidgetItem* item);
	void copyGenesToClipboard();
	void storeGenesAsTSV();

	void tabChanged(int);

private:
        Ui::PhenoToGenesDialog *ui;
	NGSD db_;
};

#endif // PHENOTOGENESDIALOG_H
