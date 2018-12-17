#ifndef GDBODIALOG_H
#define GDBODIALOG_H

#include "cppNGSD_global.h"
#include <QDialog>
#include <QSet>
#include <QLabel>
#include "GDBO.h"

namespace Ui {
class GDBODialog;
}

class CPPNGSDSHARED_EXPORT GDBODialog
		: public QDialog
{
	Q_OBJECT

public:
	explicit GDBODialog(QWidget *parent, GDBO& gdbo, QStringList hidden);
	~GDBODialog();

private:
	Ui::GDBODialog *ui;
	GDBO& gdbo_;
	QSet<QString> hidden_;
	struct ValidationInfo
	{
		bool valid;
		QLabel* label;
	};
	QMap<QString, ValidationInfo> validation;

private slots:
	void init();
	void validate();
	void validate(QWidget* w);
	void updateOkButton();

	bool isValidDate(QString text);
};

#endif // GDBODIALOG_H
