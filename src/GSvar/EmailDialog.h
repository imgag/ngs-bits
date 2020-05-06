#ifndef EMAILDIALOG_H
#define EMAILDIALOG_H

#include <QDialog>
#include "ui_EmailDialog.h"

class EmailDialog
	: public QDialog
{
	Q_OBJECT

public:
	EmailDialog(QWidget* parent, QStringList to, QString subject, QStringList body);

protected slots:
	void sendEmail();

private:
	Ui::EmailDialog ui_;

	EmailDialog() = delete;
};

#endif // EMAILDIALOG_H
