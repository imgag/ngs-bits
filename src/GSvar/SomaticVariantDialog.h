#ifndef SOMATICVARIANTDIALOG_H
#define SOMATICVARIANTDIALOG_H

#include <QDialog>

namespace Ui {
class SomaticVariantDialog;
}

class SomaticVariantDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SomaticVariantDialog(QWidget *parent = 0);
	~SomaticVariantDialog();

private:
	Ui::SomaticVariantDialog *ui;
};

#endif // SOMATICVARIANTDIALOG_H
