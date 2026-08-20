#ifndef PHENOTOROIDIALOG_H
#define PHENOTOROIDIALOG_H

#include <QDialog>

namespace Ui {
class PhenoToRoiDialog;
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
	Ui::PhenoToRoiDialog *ui;
};

#endif // PHENOTOROIDIALOG_H
