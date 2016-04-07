#ifndef PHENOTOROIDIALOG_H
#define PHENOTOROIDIALOG_H

#include <QDialog>

namespace Ui {
class PhenotoRoiDialog;
}

class PhenoToRoiDialog
	: public QDialog
{
	Q_OBJECT

public:
	explicit PhenoToRoiDialog(QWidget* parent = 0);
	~PhenoToRoiDialog();

private slots:
	void copyPhenotype(QString phenotype);

private:
	Ui::PhenotoRoiDialog *ui;
};

#endif // PHENOTOROIDIALOG_H
