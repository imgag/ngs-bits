#ifndef IGVDIALOG_H
#define IGVDIALOG_H

#include <QDialog>

namespace Ui {
class IgvDialog;
}

class IgvDialog : public QDialog
{
	Q_OBJECT

public:
	explicit IgvDialog(QWidget *parent = 0);
	~IgvDialog();
	void addFile(QString label, QString filename, bool checked);
	void addSeparator();
	QStringList filesToLoad();

private:
	Ui::IgvDialog *ui;
};

#endif // IGVDIALOG_H
